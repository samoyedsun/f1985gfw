#include "net_mgr.h"

#define HEADER_LEN 4
#define RECV_BUFFER_SIZE 0xffff

std::string boost_lib_version()
{
	return "BOOST VERSION : "
        + std::to_string(BOOST_VERSION / 100000) + "."
        + std::to_string(BOOST_VERSION / 100 % 1000) + "."
        + std::to_string(BOOST_VERSION % 100);
}

class net_mgr::net_msg_queue
{
public:
    net_msg_queue()
    : m_head_ptr(NULL)
    , m_tail_ptr(NULL)
    {}
    ~net_msg_queue()
    {}

    net_msg_t *create_element(uint16_t size)
    {
        return (net_msg_t *)malloc(sizeof(net_msg_t) - 1 + size);
    }
    
    void init_element(net_msg_t *msg_ptr, uint32_t cid, uint16_t id, uint16_t size, const void *body)
    {
        msg_ptr->next = NULL;
        msg_ptr->cid = cid;
        msg_ptr->id = id;
        msg_ptr->size = size;
        if (body != NULL)
        {
            memcpy(msg_ptr->buffer, body, size);
        }
    }

    void release_element(net_msg_t *msg_ptr)
    {
        free(msg_ptr);
    }

    void enqueue(net_msg_t * msg_ptr)
    {
        std::lock_guard<std::mutex> guard(m_mutex);
        
        if (m_tail_ptr)
        {
            m_tail_ptr->next = msg_ptr;
            m_tail_ptr = msg_ptr;
        }
        else
        {
            m_head_ptr = m_tail_ptr = msg_ptr;
        }
    }

    net_msg_t* dequeue()
    {
        if (!m_head_ptr)
        {
            return NULL;
        }
        else
        {
            std::lock_guard<std::mutex> guard(m_mutex);
            net_msg_t* msg_ptr = m_head_ptr;
            m_head_ptr = NULL;
            m_tail_ptr = NULL;
            return msg_ptr;
        }
    }

    void clear()
    {
        net_msg_t* msg_ptr = NULL;
        std::lock_guard<std::mutex> guard(m_mutex);

        while (m_head_ptr)
        {
            msg_ptr = m_head_ptr;
            m_head_ptr = m_head_ptr->next;
            free(msg_ptr);
        }

        m_head_ptr = NULL;
        m_tail_ptr = NULL;
    }

private:
    std::mutex	m_mutex;
    net_msg_t*	m_head_ptr;
    net_msg_t*	m_tail_ptr;
};

class net_mgr::connection
{
    public:
        connection(net_mgr &_net_mgr)
        : m_cid(_net_mgr._gen_cid())
        , m_net_mgr(_net_mgr)
        , m_socket(_net_mgr._get_io_service())
        , m_recv_buf_ptr(NULL)
        , m_recv_size(0)
        {
            m_recv_buf_ptr = (char *)malloc(RECV_BUFFER_SIZE);
        }
        ~connection()
        {
            free(m_recv_buf_ptr);
        }

        void connected()
        {
            boost::system::error_code ec;
            const ip::tcp::endpoint &ep = m_socket.remote_endpoint(ec);
            if (ec)
            {
                std::string content = "remote_endpoint, error: " + ec.message();
                m_net_mgr._post_msg(m_cid, EMIR_Error, content.c_str(), content.size());
            }
            std::string remote_ep_data;
            remote_ep_data += ep.address().to_string(ec);
            if (ec)
            {
                std::string content = "remote_endpoint address, error: " + ec.message();
                m_net_mgr._post_msg(m_cid, EMIR_Error, content.c_str(), content.size());
            }
            remote_ep_data += ":";
            remote_ep_data += std::to_string(ep.port());
            m_net_mgr._post_msg(m_cid, EMIR_Connect, remote_ep_data.c_str(), remote_ep_data.size());

            m_socket.async_read_some(boost::asio::buffer(m_recv_buf_ptr + m_recv_size, RECV_BUFFER_SIZE - m_recv_size),
                [this](const boost::system::error_code& error, std::size_t bytes_size)
                {
                    _read_handler(error, bytes_size);
                }
            );
        }

        ip::tcp::socket &get_socket()
        {
            return m_socket;
        }

        uint32_t get_cid()
        {
            return m_cid;
        }

        void disconnected()
        {
            _close();
        }

    private:
        void _read_handler(const boost::system::error_code& error, std::size_t bytes_size)
        {
            if (error || 0 == bytes_size)
            {
                _close();
                return;
            }

            m_recv_size += static_cast<uint16_t>(bytes_size);
            if (m_recv_size >= HEADER_LEN)
            {
                uint16_t id = *(uint16_t *)m_recv_buf_ptr;
                uint16_t size = *(uint16_t *)(m_recv_buf_ptr + sizeof(id));
                if (size > RECV_BUFFER_SIZE - HEADER_LEN)
                {
                    std::string content = "recv msg too big.";
                    m_net_mgr._post_msg(m_cid, EMIR_Error, content.c_str(), content.size());
                    _close();
                }
                
                if ((m_recv_size - HEADER_LEN) >= size)
                {
                    m_recv_size -= HEADER_LEN;
                    void * body_ptr = m_recv_buf_ptr + m_recv_size;
                    m_recv_size -= size;
                    m_net_mgr._post_msg(m_cid, id, body_ptr, size);
                }
            }
            
            m_socket.async_read_some(boost::asio::buffer(m_recv_buf_ptr + m_recv_size, RECV_BUFFER_SIZE - m_recv_size),
                [this](const boost::system::error_code& error, std::size_t bytes_size)
                {
                    _read_handler(error, bytes_size);
                }
            );
        }

        void _close()
        {
            if (!m_socket.is_open())
            {
                return;
            }

            boost::system::error_code ec;
            m_socket.shutdown(ip::tcp::socket::shutdown_both, ec);
            if (ec)
            {
                std::string content = "socket shutdown error, " + ec.message();
                m_net_mgr._post_msg(m_cid, EMIR_Error, content.c_str(), content.size());
            }
            m_socket.close(ec);
            if (ec)
            {
                std::string content = "socket close error, " + ec.message();
                m_net_mgr._post_msg(m_cid, EMIR_Error, content.c_str(), content.size());
            }
            m_net_mgr._post_msg(m_cid, EMIR_Disconnect, NULL, 0);
        }
    
    private:
        uint32_t m_cid;
        net_mgr &m_net_mgr;
        ip::tcp::socket m_socket;
        char *m_recv_buf_ptr;
        uint16_t m_recv_size;
};

net_mgr::net_mgr()
    : m_acceptor(m_io_service)
    , m_msg_queue_ptr(NULL)
    , m_cid_seed(0)
{

}
		
net_mgr::~net_mgr()
{

}

const net_mgr::result_t net_mgr::startup(const std::string& ip, uint16_t port, uint8_t concurrent_num)
{
    result_t ret;

    ip::tcp::endpoint endpoint(ip::address_v4::from_string(ip), port);
    boost::system::error_code ec;
    m_acceptor.open(endpoint.protocol(), ec);
    if (ec)
    {
        return ret.make_failure("socket open error, " + ec.message());
    }
    m_acceptor.set_option(ip::tcp::acceptor::reuse_address(true), ec);
    if (ec)
    {
        return ret.make_failure("set_option reuse_address error, " + ec.message());
    }
    m_acceptor.bind(endpoint, ec);
    if (ec)
    {
        return ret.make_failure("socket bind error, " + ec.message());
    }
    m_acceptor.listen(boost::asio::socket_base::max_connections, ec);
    if (ec)
    {
        return ret.make_failure("socket listen error, " + ec.message());
    }

    m_msg_queue_ptr = new net_msg_queue;
    
    for (uint8_t i = 0; i < concurrent_num; ++i)
    {
        _accept_post();
        m_socket_threads.emplace_back(&net_mgr::_work_handler, this);
    }

    return ret;
}

void net_mgr::loop(uint8_t concurrent_num)
{
    for (uint8_t i = 0; i < concurrent_num; ++i)
    {
        m_worker_threads.emplace_back(&net_mgr::_process_handler, this);
    }
}

void net_mgr::release()
{
    for (size_t i = 0; i < m_socket_threads.size(); ++i)
    {
        m_socket_threads[i].join();
    }
    // post close socket
    delete m_msg_queue_ptr;
    return;
}

void net_mgr::_process_handler()
{
    while(true)
    {
        net_msg_t *msg_ptr = m_msg_queue_ptr->dequeue();
        while (msg_ptr)
        {
            net_msg_t *p = msg_ptr;
            msg_ptr = msg_ptr->next;
            p->next = NULL;
            if (p->id == EMIR_Connect)
            {
                std::string remote_ep_data(p->buffer, p->size);
                m_handlers["on_connect"](p->cid, remote_ep_data);
            }
            else if (p->id == EMIR_Disconnect)
            {
                m_handlers["on_disconnect"](p->cid);
                _del_connection(p->cid);
            }
            else if (p->id == EMIR_Error)
            {
                std::string error_data(p->buffer, p->size);
                m_handlers["on_error"](p->cid, error_data);
            }
            else
            {
                m_handlers["on_msg"](p->cid, p->id, p->buffer, p->size);
            }

            m_msg_queue_ptr->release_element(p);
        }

        std::unique_lock<std::mutex> lock{ m_mutex };
        m_condition.wait(lock);
    }

}

void net_mgr::_work_handler()
{
    try
    {
        m_io_service.run();
        std::string content = "io_service thread exit.";
        _post_msg(0, EMIR_Error, content.c_str(), content.size());
    }
    catch (boost::system::system_error& e)
    {
        std::string content = "io_service thread exception, error:" + e.code().message();
        _post_msg(0, EMIR_Error, content.c_str(), content.size());
    }
}

void net_mgr::_accept_post()
{
    connection *new_conn_ptr = _add_connection();
    m_acceptor.async_accept(new_conn_ptr->get_socket(),
        [this, new_conn_ptr](const boost::system::error_code& ec)
        {
            _accept_handler(ec, new_conn_ptr);
        }
    );
}

void net_mgr::_accept_handler(const boost::system::error_code& ec, connection *curr_conn_ptr)
{
    if (!ec)
    {
        curr_conn_ptr->connected();
        _accept_post();
    }
    else
    {
        curr_conn_ptr->disconnected();
        std::string content = "accept failed. cid:" + std::to_string(curr_conn_ptr->get_cid());
        _post_msg(0, EMIR_Error, content.c_str(), content.size());
    }
}

net_mgr::net_msg_queue *net_mgr::_get_msg_queue()
{
    return m_msg_queue_ptr;
}

io_service &net_mgr::_get_io_service()
{
    return m_io_service;
}

uint32_t net_mgr::_gen_cid()
{
    return ++m_cid_seed;
}

net_mgr::connection *net_mgr::_add_connection()
{
    std::lock_guard<std::mutex> lock(m_mutex);
    connection *new_conn_ptr = new connection(*this);
    m_connections.push_back(new_conn_ptr);
    return new_conn_ptr;
}

void net_mgr::_del_connection(uint32_t cid)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    auto target_iter = std::find_if(m_connections.begin(), m_connections.end(), [cid](connection *conn_ptr)
        {
            return conn_ptr->get_cid() == cid;
        }
    );
    if (target_iter != m_connections.end())
    {
        delete (*target_iter);
        
        
        m_connections.erase(target_iter);

        iter_swap(target_iter, m_connections.end() - 1);
        m_connections.pop_back();
    }
}

void net_mgr::_post_msg(uint32_t cid, uint16_t id, const void *data_ptr, uint16_t size)
{
    net_msg_t * msg_ptr = _get_msg_queue()->create_element(size);
    _get_msg_queue()->init_element(msg_ptr, cid, id, size, data_ptr);
    _get_msg_queue()->enqueue(msg_ptr);

    _wakeup();
}

void net_mgr::_wakeup()
{
    m_condition.notify_one();
}