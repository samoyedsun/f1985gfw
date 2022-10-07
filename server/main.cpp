#include <iostream>
#include <map>
#include "net_mgr/net_mgr.h"
#include "net_mgr/net_msg.hpp"
#include "message/hello_define.pb.h"

void connect_cb(uint32_t cid, std::string &remote_ep_data)
{
	std::cout << "on connect, cid:" << cid << ", remote_ep_data:" << std::string(remote_ep_data) << std::endl;
}
void disconnect_cb(uint32_t cid)
{
	std::cout << "on disconnect, cid:" << cid << std::endl;
}
void error_cb(uint32_t cid, std::string &error)
{
	std::cout << "on error, cid:" << cid << ", error:" << error << std::endl;
}
void message_cb(uint32_t cid, uint16_t id, char *buffer, uint16_t size)
{
	std::cout << "on message, cid:" << cid << ", id:" << id << ", size:" << size << std::endl;
}

int main()
{
	std::cout << boost_lib_version() << std::endl;

	net_mgr net;
	net.set_connect_cb(connect_cb);
	net.set_disconnect_cb(disconnect_cb);
	net.set_error_cb(error_cb);
	net.set_message_cb(message_cb);
	net.startup("0.0.0.0", 19850, 8);
	net.loop(1);
	net.release();
	return 0;
}
