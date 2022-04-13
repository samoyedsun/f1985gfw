#include <iostream>
#include <map>
#include "net_mgr/net_mgr.h"
#include "handler.hpp"
#include "message/hello_define.pb.h"

int main()
{
	std::cout << boost_lib_version() << std::endl;
	
	std::map<std::string, logic_handler> handlers;
	handlers.insert(std::pair("on_connect", [](uint32_t cid, std::string &remote_ep_data)
	{
		std::cout << "on connect, cid:" << cid << ", remote_ep_data:" << std::string(remote_ep_data) << std::endl;
	}));
	handlers.insert(std::pair("on_disconnect", [](uint32_t cid)
	{
		std::cout << "on disconnect, cid:" << cid << std::endl;
	}));
	handlers.insert(std::pair("on_error", [](uint32_t cid, std::string &error)
	{
		std::cout << "on error, cid:" << cid << ", error:" << error << std::endl;
	}));
	handlers.insert(std::pair("on_msg", [](uint32_t cid, uint16_t id, char *buffer, uint16_t size)
	{
		std::cout << "on msg, cid:" << cid << ", id:" << id << ", size:" << size << std::endl;
	}));
	
	net_mgr net;
	net.set_handlers(handlers);
	net.startup("0.0.0.0", 19850, 8);
	net.loop(8);
	net.release();
	return 0;
}
