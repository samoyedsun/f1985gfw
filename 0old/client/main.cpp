#include <iostream>
#include <thread>

#include "boost/version.hpp"
#include "boost/asio.hpp"
#include "message/hello_define.pb.h"

using namespace std;
using namespace boost::asio;

void version()
{
	cout << "BOOST VERSION : "
		<< BOOST_VERSION / 100000 << "."
		<< BOOST_VERSION / 100 % 1000 << "."
		<< BOOST_VERSION % 100 << endl;
	return;
}

#define HEADER_SIZE 4
#define MAX_SIZE 4096

void process(int i)
{
	cout << "thread i:" << i << endl;
	try
	{
		io_service io_srv;
		ip::tcp::socket socket(io_srv);

		ip::tcp::endpoint endpoint(ip::address::from_string("127.0.0.1"), 19850);
		socket.connect(endpoint);
		
		boost::system::error_code ec;
		
		char *buf = (char *)malloc(MAX_SIZE);

		HelloData data;
		data.set_id(101);
		data.add_member(111);
		data.add_member(222);
		data.add_member(333);
		data.add_member(444);
		
		const ::google::protobuf::MessageLite &source_data = data;
		if (!source_data.SerializePartialToArray(buf + HEADER_SIZE, MAX_SIZE))
		{
			return;
		}
		int32_t size = source_data.GetCachedSize();
		std::cout << "size:" << size << std::endl;
		
		char *offset = buf;

		*((uint16_t *)offset) = 141;
		offset += sizeof(uint16_t);

		*((uint16_t *)offset) = size;
		offset += sizeof(uint16_t);

		size_t write_size = HEADER_SIZE + size;

		socket.write_some(buffer(buf, write_size), ec);
		if (ec)
		{
			throw boost::system::system_error(ec);
		}
		std::cout << "send finish" << std::endl;
		socket.read_some(boost::asio::buffer(buf, 12), ec);
		if (ec)
		{
			throw boost::system::system_error(ec);
		}
		std::cout << "recv finish" << std::endl;

		socket.shutdown(ip::tcp::socket::shutdown_both, ec);
		if (ec)
		{
			throw boost::system::system_error(ec);
		}
		socket.close(ec);
		if (ec)
		{
			throw boost::system::system_error(ec);
		}

		io_srv.run();

		std::cout << "executor finish" << std::endl;
	}
	catch (std::exception& e)
	{
		cout << "异常:" << e.what() << endl;
	}
}

int main()
{
	version();
	
	std::vector<std::thread> m_socket_threads;
    for (uint8_t i = 0; i < 10; ++i)
    {
        m_socket_threads.emplace_back([i](){
			process(i);
		});
    }

    for (uint8_t i = 0; i < 10; ++i)
    {
        m_socket_threads[i].join();
    }

	return 0;
}
