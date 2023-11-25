#pragma once
#include <iostream>
#include <unordered_map>
#include <memory>

class session
{
public:
    session(int32_t pointer_id, std::string ip, uint32_t port);
    void set_uid(uint32_t uid);
    uint32_t get_uid();

private:
    int32_t m_pointer_id;
    std::string m_ip;
    uint32_t m_port;
    uint32_t m_uid;
};

class ws_worker;
class wan_server
{
public:
    void init(ws_worker* worker);
    void destory();

public:
    bool handle_enter(int32_t pointer_id, void* data_ptr, int32_t size);

private:
    void add_session(int32_t pointer_id, std::string ip, uint32_t port);
    void del_session(int32_t pointer_id);
    std::unique_ptr<session> get_session(int32_t pointer_id);

private:
    std::unordered_map<int32_t, std::unique_ptr<session>> m_sessions;
    ws_worker* m_net_worker_ptr;
};

extern wan_server g_wan_server;
