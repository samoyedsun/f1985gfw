#ifndef _NET_MGR_H_
#define _NET_MGR_H_
#include <thread>
#include <condition_variable>
#include <iostream>
#include <map>
#include "boost/asio.hpp"

using namespace boost::asio;

std::string boost_lib_version();

class net_mgr
{

		typedef struct net_msg
		{
			struct net_msg * next;
			uint32_t cid;
			uint16_t id;
			uint16_t size;
			char buffer[1];
		} net_msg_t;
		class net_msg_queue
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

			void enqueue(net_msg_t *msg_ptr)
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
				net_msg_t *msg_ptr = NULL;
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

	public:
		enum EMsgIDReserve
		{
			EMIR_Connect = 1,
			EMIR_Disconnect,
			EMIR_Error,
			EMIR_Max,
		};
		typedef struct result
		{
			bool success;
			std::string msg;

			result() : success(true)
			{}

			const result& make_failure(std::string _msg)
			{
				success = false;
				msg = _msg;
				return *this;
			}
		} result_t;
		
        class connection;
		
		using message_cb_t = std::function<void(uint32_t cid, uint16_t id, char *buffer, uint16_t size)>;

		using pconnection_t = std::shared_ptr<connection>;
		using pconnections_t = std::vector<pconnection_t>;

	public:
		net_mgr();
		~net_mgr();

	public:
		const result_t startup(const std::string& ip, uint16_t port, uint8_t concurrent_num);
		void loop(uint8_t concurrent_num);
		void release();
		inline void set_message_cb(message_cb_t cb) { m_message_cb = cb; };
		
	private:
		void _process_handler();
		void _work_handler();
		void _accept_post();
		net_msg_queue *_get_msg_queue();
		io_context &_get_context();
		uint32_t _gen_cid();
		pconnection_t _add_pconnection();
		void _del_pconnection(uint32_t cid);
		void _post_msg(uint32_t cid, uint16_t id, const void *data_ptr, uint16_t size);

		void _wait();
		void _wakeup();

	private:
		io_context 				m_context;
		ip::tcp::acceptor 		m_acceptor;
		pconnections_t			m_pconnections;
		std::vector<std::thread>m_socket_threads;
		std::vector<std::thread>m_worker_threads;
		net_msg_queue         	m_msg_queue;
		uint32_t 				m_cid_seed;
		
		std::mutex				m_mutex;
		std::condition_variable	m_condition;
		
		message_cb_t			m_message_cb;
};

#endif