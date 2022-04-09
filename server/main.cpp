#include <iostream>
#include "net_mgr.h"

int main()
{
	std::cout << boost_lib_version() << std::endl;
	
	auto on_connect = [](uint32_t cid, std::string &remote_ep_data)
	{
		std::cout << "on connect, cid:" << cid << ", remote_ep_data:" << std::string(remote_ep_data) << std::endl;
	};
	auto on_disconnect = [](uint32_t cid)
	{
		std::cout << "on disconnect, cid:" << cid << std::endl;
	};
	auto on_error = [](uint32_t cid, std::string &error)
	{
		std::cout << "on error, cid:" << cid << ", error:" << error << std::endl;
	};
	auto on_msg = [](uint32_t cid, uint16_t id, char *buffer, uint16_t size)
	{
		std::cout << "on msg, cid:" << cid << ", id:" << id << ", size:" << size << std::endl;
	};

	net_mgr net;
	net.set_connect_cb(on_connect);
	net.set_disconnect_cb(on_disconnect);
	net.set_error_cb(on_error);
	net.set_msg_cb(on_msg);
	net.startup("0.0.0.0", 19850, 8);
	net.loop(8);
	net.release();

	return 0;
}
