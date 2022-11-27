#include <iostream>
#include <map>
#include "net_mgr/net_mgr.h"
#include "net_mgr/net_proto.hpp"
#include "message/hello_define.pb.h"
#include "net_mgr/net_http.hpp"
#include "nlohmann/json.hpp"

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

int main()
{
	std::cout << boost_lib_version() << std::endl;

	std::map<std::string, std::string> accounts;

	net_http http;
	http.register_post("/user/register-user", [&accounts](std::string_view body) ->std::string
	{
		struct
		{
			std::string nickname;
			std::string password;
			void from_json(const nlohmann::json& j)
			{
				j.at("nickname").get_to(this->nickname);
				j.at("password").get_to(this->password);
			}
		} req_data;
		nlohmann::json res_body;
		try
		{
			nlohmann::json req_body = nlohmann::json::parse(body.data());
			req_data.from_json(req_body);
		}
		catch(std::exception& e)
		{
			res_body["result"] = "fail";
			res_body["msg"] = e.what();
			return res_body.dump();
		}

		if (accounts.find(req_data.nickname) != accounts.end())
		{
			res_body["result"] = "fail";
			res_body["msg"] = "it is already registered.";
			return res_body.dump();
		}
		accounts.insert({req_data.nickname, req_data.password});

		res_body["result"] = "succ";
		res_body["msg"] = "register success.";
		nlohmann::json res_body_data;
		res_body_data["id"] = 18;
		res_body_data["nickname"] = req_data.nickname;
		res_body_data["password"] = req_data.password;
		res_body_data["kills"] = 0;
		res_body["data"] = res_body_data;
		return res_body.dump();
	});
	http.register_post("/user/login-user", [&accounts](std::string_view body) ->std::string
	{
		struct
		{
			std::string nickname;
			std::string password;
			void from_json(const nlohmann::json& j)
			{
				j.at("nickname").get_to(this->nickname);
				j.at("password").get_to(this->password);
			}
		} req_data;
		nlohmann::json res_body;
		try
		{
			nlohmann::json req_body = nlohmann::json::parse(body.data());
			req_data.from_json(req_body);
		}
		catch(std::exception& e)
		{
			res_body["result"] = "fail";
			res_body["error"] = e.what();
			return res_body.dump();
		}

		if (accounts.find(req_data.nickname) == accounts.end())
		{
			res_body["result"] = "fail";
			res_body["msg"] = "account not exists.";
			return res_body.dump();
		}
		if (accounts[req_data.nickname] != req_data.password)
		{
			res_body["result"] = "fail";
			res_body["msg"] = "password error.";
			return res_body.dump();
		}
		res_body["result"] = "succ";
		res_body["msg"] = "login success.";
		nlohmann::json res_body_data;
		res_body_data["id"] = 18;
		res_body_data["nickname"] = req_data.nickname;
		res_body_data["password"] = req_data.password;
		res_body_data["kills"] = 0;
		res_body["data"] = res_body_data;
		return res_body.dump();
	});

	net_mgr net;
	net.set_message_cb(on_message);
	http.startup(4, "0.0.0.0", 8080, "./");
	net.startup(4, "0.0.0.0", 19850);
	net.release();
	http.release();

	return 0;
}
