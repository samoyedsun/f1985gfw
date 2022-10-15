#include <iostream>
#include <thread>

#include "boost/version.hpp"
#include "boost/asio.hpp"

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

		size_t write_size = 0;
		char *offset = NULL;
		char *buf = (char *)malloc(12);
		offset = buf;

		*((uint16_t *)offset) = 101;
		offset += sizeof(uint16_t);

		*((uint16_t *)offset) = 8;
		offset += sizeof(uint16_t);

		*((uint32_t *)offset) = 9989;
		offset += sizeof(uint32_t);

		*((uint32_t *)offset) = 2312;
		offset += sizeof(uint32_t);

		write_size = offset - buf;

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

		cout << "executor finish" << endl;
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
