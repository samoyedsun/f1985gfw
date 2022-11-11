#include <iostream>
#include <map>
#include "net_mgr/net_mgr.h"
#include "net_mgr/net_proto.hpp"
#include "message/hello_define.pb.h"
#include "net_mgr/net_http.hpp"

void on_message(net_mgr::message_back_cb_t back_cb, uint32_t cid, uint16_t id, const char *buffer, uint16_t size)
{
	if (id == net_mgr::EMIR_Connect)
	{
    	std::string remote_ep_data(buffer, size);
		std::cout << "on connect, cid:" << cid << ", remote_ep_data:" << std::string(remote_ep_data) << std::endl;		
	}
	else if (id == net_mgr::EMIR_Disconnect)
	{
		std::cout << "on disconnect, cid:" << cid << std::endl;
	}
	else if (id == net_mgr::EMIR_Error)
	{
		std::string error_data(buffer, size);
		std::cout << "on error, cid:" << cid << ", error:" << error_data << std::endl;
	}
	else
	{
		HelloData data;
		if (false == data.ParsePartialFromArray(buffer, size))
		{
			std::cout << "Parse Fail===========" << std::endl;
		}
		std::cout << "== msg content, id:" << data.id() << ", member:" << data.member(0) << std::endl;

		std::cout << "on message, cid:" << cid << ", id:" << id << ", size:" << size << std::endl;
		back_cb(id, buffer, size);
	}
}

void test_net_http()
{

}

int main()
{
	std::cout << boost_lib_version() << std::endl;

	net_http http;
	net_mgr net;
	net.set_message_cb(on_message);
	http.startup(4, "0.0.0.0", 8080, "./");
	net.startup(4, "0.0.0.0", 19850);
	net.release();
	http.release();

	return 0;
}
