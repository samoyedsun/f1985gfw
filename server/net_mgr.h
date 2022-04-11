#ifndef _NET_MGR_H_
#define _NET_MGR_H_
#include <thread>
#include <condition_variable>
#include <iostream>
#include <map>
#include "boost/asio.hpp"
#include "handler.hpp"

using namespace boost::asio;

std::string boost_lib_version();

class net_mgr
{
	enum EMsgIDReserve
	{
		EMIR_Connect = 1,
		EMIR_Disconnect,
		EMIR_Error,
		EMIR_Max,
	};

	public:
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

		typedef struct net_msg
		{
			struct net_msg * next;
			uint32_t cid;
			uint16_t id;
			uint16_t size;
			char buffer[1];
		} net_msg_t;
		
        class net_msg_queue;
        class connection;

		using handlers_t = std::map<std::string, logic_handler>;
		typedef std::function<void(uint32_t cid, std::string &remote_ep_data)> connect_cb_t;
		typedef std::function<void(uint32_t cid)> disconnect_cb_t;
		typedef std::function<void(uint32_t cid, std::string &error)> error_cb_t;
		typedef std::function<void(uint32_t cid, uint16_t id, char *buffer, uint16_t size)> msg_cb_t;

	public:
		net_mgr();
		~net_mgr();

	public:
		const result_t startup(const std::string& ip, uint16_t port, uint8_t concurrent_num);
		void loop(uint8_t concurrent_num);
		void release();
		
		inline void set_handlers(handlers_t &handlers) { m_handlers = handlers; }
		
	private:
		void _process_handler();
		void _work_handler();

		void _accept_post();
		void _accept_handler(const boost::system::error_code& ec, connection *curr_conn_ptr);

		net_msg_queue *_get_msg_queue();

		io_service &_get_io_service();

		uint32_t _gen_cid();

		connection *_add_connection();
		void _del_connection(uint32_t cid);

		void _post_msg(uint32_t cid, uint16_t id, const void *data_ptr, uint16_t size);

		void _wakeup();

	private:
		io_service 				m_io_service;
		ip::tcp::acceptor 		m_acceptor;
		std::vector<connection*>m_connections;
		std::vector<std::thread>m_socket_threads;
		std::vector<std::thread>m_worker_threads;
		net_msg_queue *         m_msg_queue_ptr;
		uint32_t 				m_cid_seed;
		
		std::mutex				m_mutex;
		std::condition_variable	m_condition;
		
		handlers_t				m_handlers;
};

#endif