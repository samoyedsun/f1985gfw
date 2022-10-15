#ifndef _NET_MGR_H_
#define _NET_MGR_H_
#include <thread>
#include <condition_variable>
#include <iostream>
#include <map>
#include "boost/asio.hpp"
#include "net_queue.hpp"

using namespace boost::asio;

std::string boost_lib_version();

class net_mgr
{
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
		
		using message_back_cb_t = std::function<void(uint16_t id, const void *buffer, uint16_t size)>;
		using message_cb_t = std::function<void(message_back_cb_t, uint8_t type, uint16_t id, const char *buffer, uint16_t size)>;

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
		net_session_queue &_session_queue();
		void _wait();
		void _wakeup();

	private:
		io_context 				m_context;
		ip::tcp::acceptor 		m_acceptor;
		pconnections_t			m_pconnections;
		std::vector<std::thread>m_socket_threads;
		std::vector<std::thread>m_worker_threads;
		net_session_queue		m_session_queue;
		
		uint32_t 				m_cid_seed;
		
		std::mutex				m_mutex;
		std::condition_variable	m_condition;
		
		message_cb_t			m_message_cb;
};

#endif