#include "net_mgr.h"
#include <boost/function.hpp>
#include <boost/bind/bind.hpp>
#include <memory>
#include <iostream>
#include <unistd.h>

#define HEADER_LEN 4
#define RECV_BUFFER_SIZE 0xffff
#define SEND_BUFFER_SIZE 0xffff

std::string boost_lib_version()
{
	return "BOOST VERSION : "
        + std::to_string(BOOST_VERSION / 100000) + "."
        + std::to_string(BOOST_VERSION / 100 % 1000) + "."
        + std::to_string(BOOST_VERSION % 100);
}

class net_mgr::connection
{
    public:
        enum EConnectionStatus
        {
            ECS_UseLess     = 1,
            ECS_Start       = 2,
            ECS_Stop        = 3,
        };
    public:
        connection(net_mgr &_net_mgr)
        : m_cid(_net_mgr._gen_cid())
        , m_net_mgr(_net_mgr)
        , m_status(connection::ECS_UseLess)
        , m_socket(_net_mgr._get_context())
        , m_strand(_net_mgr._get_context())
        , m_recv_buf_ptr(nullptr)
        , m_recv_size(0)
        , m_send_buf_ptr(nullptr)
        , m_send_size(0)
        , m_net_session_ptr(nullptr)
        {
            m_recv_buf_ptr = (char *)malloc(RECV_BUFFER_SIZE);
            m_send_buf_ptr = (char *)malloc(SEND_BUFFER_SIZE);

            m_net_session_ptr = net_session_queue::create();
            m_net_session_ptr->owner_ptr = (void *)(&(*this));
        }
        ~connection()
        {
            free(m_recv_buf_ptr);
            free(m_send_buf_ptr);

            net_session_queue::release(m_net_session_ptr);
        }

        void set_status(int32_t status)
        {
            m_status = status;
        }

        int32_t get_status()
        {
            return m_status;
        }

        void connected()
        {
            boost::system::error_code ec;
            const ip::tcp::endpoint &ep = m_socket.remote_endpoint(ec);
            if (ec)
            {
                std::string content = "remote_endpoint, error: " + ec.message();
                push_msg(net_msg_queue::ENMT_Read, EMIR_Error, content.c_str(), content.size());
            }
            std::string remote_ep_data;
            remote_ep_data += ep.address().to_string(ec);
            if (ec)
            {
                std::string content = "remote_endpoint address, error: " + ec.message();
                push_msg(net_msg_queue::ENMT_Read, EMIR_Error, content.c_str(), content.size());
            }
            remote_ep_data += ":";
            remote_ep_data += std::to_string(ep.port());
            push_msg(net_msg_queue::ENMT_Read, EMIR_Connect, remote_ep_data.c_str(), remote_ep_data.size());

            _read_post();
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

        void write_msg(uint16_t id, const void *data_ptr, uint16_t size)
        {
		    char *offset = m_send_buf_ptr;

            *((uint16_t *)offset) = id;
            offset += sizeof(uint16_t);

            *((uint16_t *)offset) = size;
            offset += sizeof(uint16_t);
            
		    memcpy(offset, data_ptr, size);
            offset += size;

		    size_t write_size = offset - m_send_buf_ptr;
            // m_send_size += write_size;
            
            boost::system::error_code ec;
            m_socket.write_some(boost::asio::buffer(m_send_buf_ptr, write_size), ec);
            if (ec)
            {
                std::string content = "socket write some error, " + ec.message();
                push_msg(net_msg_queue::ENMT_Read, EMIR_Error, content.c_str(), content.size());
                return _close();
            }
        }
        
        void push_msg(uint8_t type, uint16_t id, const void *data_ptr, uint16_t size)
        {
            net_msg_queue::net_msg_t *msg_ptr = net_msg_queue::create(size);
            msg_ptr->next = nullptr;
            msg_ptr->type = type;
            msg_ptr->id = id;
            msg_ptr->size = size;
            if (data_ptr != nullptr)
            {
                memcpy(msg_ptr->buffer, data_ptr, size);
            }
            m_net_session_ptr->msg_queue.enqueue(msg_ptr, [this]()
            {
                if (!m_net_session_ptr->processing)
                {
                    m_net_session_ptr->processing = true;
                    m_net_mgr._session_queue().enqueue(m_net_session_ptr);
                }
            });
        }

    private:
        void _read_post()
        {
            m_socket.async_read_some(boost::asio::buffer(m_recv_buf_ptr + m_recv_size, RECV_BUFFER_SIZE - m_recv_size),
                m_strand.wrap([this](const boost::system::error_code& ec, std::size_t bytes_size)
                {
                    if (ec)
                    {
                        std::string content = "socket async read some error, " + ec.message();
                        push_msg(net_msg_queue::ENMT_Read, EMIR_Error, content.c_str(), content.size());
                        return _close();
                    }
                    if (0 == bytes_size)
                    {
                        std::string content = "socket async read some bytes_size = 0";
                        push_msg(net_msg_queue::ENMT_Read, EMIR_Error, content.c_str(), content.size());
                        return _close();
                    }
                    m_recv_size += static_cast<uint16_t>(bytes_size);
                    _parse_msg();
                    _read_post();
                }));
        }

        void _close()
        {
            if (!m_socket.is_open())
            {
                return;
            }
            boost::system::error_code ec;
            m_socket.close(ec);
            if (ec)
            {
                std::string content = "socket close error, " + ec.message();
                push_msg(net_msg_queue::ENMT_Read, EMIR_Error, content.c_str(), content.size());
            }
            push_msg(net_msg_queue::ENMT_Read, EMIR_Disconnect, NULL, 0);
        }
        
		void _parse_msg()
		{
            while (m_recv_size >= HEADER_LEN)
            {
                uint16_t id = *(uint16_t *)m_recv_buf_ptr;
                uint16_t size = *(uint16_t *)(m_recv_buf_ptr + sizeof(id));
                if (size > RECV_BUFFER_SIZE - HEADER_LEN)
                {
                    std::string content = "recv msg too big.";
                    push_msg(net_msg_queue::ENMT_Read, EMIR_Error, content.c_str(), content.size());
                    _close();
                }
                if ((m_recv_size - HEADER_LEN) < size)
                {
                    break;
                }
                m_recv_size -= HEADER_LEN;
                void * body_ptr = m_recv_buf_ptr + HEADER_LEN;
                m_recv_size -= size;
                push_msg(net_msg_queue::ENMT_Read, id, body_ptr, size);
            }
		}

    private:
        uint32_t m_cid;
        net_mgr &m_net_mgr;
        uint32_t m_status;
        ip::tcp::socket m_socket;
        io_service::strand m_strand;

        char *m_recv_buf_ptr;
        uint16_t m_recv_size;
        char *m_send_buf_ptr;
        uint16_t m_send_size;

        net_session_queue::net_session_t *m_net_session_ptr;
};

net_mgr::net_mgr()
    : m_acceptor(m_context)
    , m_cid_seed(0)
{

}
		
net_mgr::~net_mgr()
{

}

const net_mgr::result_t net_mgr::startup(uint8_t threads, const std::string& ip, uint16_t port)
{
    result_t ret;

    for (int32_t i = 0; i < 1000; ++i)
    {
        m_pconnections.push_back(new connection(*this));
    }

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
    
    for (uint8_t i = 0; i < threads; ++i)
    {
        _accept_post();
        m_socket_threads.emplace_back(&net_mgr::_sock_handler, this);
    }

    for (uint8_t i = 0; i < threads; ++i)
    {
        m_worker_threads.emplace_back(&net_mgr::_work_handler, this);
    }

    return ret;
}

void net_mgr::release()
{
    for (std::thread &t : m_socket_threads)
    {
        t.join();
    }
    for (std::thread &t : m_worker_threads)
    {
        t.join();
    }
    for (auto pconnection : m_pconnections)
    {
        delete pconnection;
    }
    return;
}

void net_mgr::_work_handler()
{
    while(true)
    {
        net_session_queue::net_session_t *net_session_ptr = m_session_queue.dequeue();
        if (net_session_ptr)
        {
            connection *connection_ptr = (connection *)(net_session_ptr->owner_ptr);
            do
            {
                net_msg_queue::net_msg_t *msg_ptr = net_session_ptr->msg_queue.dequeue([connection_ptr, net_session_ptr]()
                {
                    net_session_ptr->processing = false;
                    if (connection_ptr->get_status() == connection::ECS_Stop)
                    {
                        connection_ptr->set_status(connection::ECS_UseLess);
                    }
                });
                if (!msg_ptr)
                {
                    break;
                }
                msg_ptr->next = NULL;
                if (msg_ptr->type == net_msg_queue::ENMT_Read)
                {
                    if (msg_ptr->id == EMIR_Connect)
                    {
                    }
                    else if (msg_ptr->id == EMIR_Disconnect)
                    {
                        connection_ptr->set_status(connection::ECS_Stop);
                    }
                    else if (msg_ptr->id == EMIR_Error)
                    {

                    }
                    m_message_cb([connection_ptr](uint16_t id, const void *data_ptr, uint16_t size){
                        connection_ptr->push_msg(net_msg_queue::ENMT_Write, id, data_ptr, size);
                    }, connection_ptr->get_cid(), msg_ptr->id, msg_ptr->buffer, msg_ptr->size);
                }
                if (msg_ptr->type == net_msg_queue::ENMT_Write)
                {
                    connection_ptr->write_msg(msg_ptr->id, msg_ptr->buffer, msg_ptr->size);
                }
                net_msg_queue::release(msg_ptr);
            } while (true);
        }
        usleep(16);
    }
}

void net_mgr::_sock_handler()
{
    try
    {
        m_context.run();
        std::string content = "io_service thread exit.";
        std::cout << content << std::endl;
    }
    catch (boost::system::system_error& e)
    {
        std::string content = "io_service thread exception, error:" + e.code().message();
        std::cout << content << std::endl;
    }
}

void net_mgr::_accept_post()
{
    pconnection_t pconnnection = _add_pconnection();
    m_acceptor.async_accept(pconnnection->get_socket(), [this, pconnnection](const boost::system::error_code& ec)
    {
        if (!ec)
        {
            pconnnection->connected();
            _accept_post();
        }
        else
        {
            pconnnection->disconnected();
            std::string content = "accept failed. cid:" + std::to_string(pconnnection->get_cid());
            std::cout << content << std::endl;
        }
    });
}

io_context &net_mgr::_get_context()
{
    return m_context;
}

uint32_t net_mgr::_gen_cid()
{
    return ++m_cid_seed;
}

net_mgr::pconnection_t net_mgr::_add_pconnection()
{
    std::lock_guard<std::mutex> lock(m_mutex);
    auto target_iter = std::find_if(m_pconnections.begin(), m_pconnections.end(), [](pconnection_t pconnection)
    {
        return pconnection->get_status() == connection::ECS_UseLess;
    });
    if (target_iter != m_pconnections.end())
    {
        (*target_iter)->set_status(connection::ECS_Start);
        return *target_iter;
    }
    return nullptr;
}

net_session_queue &net_mgr::_session_queue()
{
    return m_session_queue;
}

void net_mgr::_wait()
{
    std::unique_lock<std::mutex> lock{ m_mutex };
    m_condition.wait(lock);
}

void net_mgr::_wakeup()
{
    m_condition.notify_one();
}