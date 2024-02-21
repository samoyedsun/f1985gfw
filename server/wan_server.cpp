#include "./wan_server.h"
#include "./player_mgr.h"
#include "../source/common.hpp"
#include "common.pb.h"
#include "enum_define.pb.h"

session::session(int32_t pointer_id, std::string ip, uint32_t port)
    : m_pointer_id(pointer_id)
    , m_ip(ip)
    , m_port(port)
    , m_uid(0)
{
}

void session::set_uid(uint32_t uid)
{
    m_uid = uid;
}

uint32_t session::get_uid()
{
    return m_uid;
}

void wan_server::init(net_worker* worker)
{
    m_net_worker_ptr = worker;
    m_net_worker_ptr->on_connect([this](int32_t pointer_id, std::string ip, uint16_t port)
        {
            add_session(pointer_id, ip, port);
            std::cout << "on_connect, pointer_id:" << pointer_id << ", ip:" << ip << ", port:" << port << std::endl;
        });
    m_net_worker_ptr->on_disconnect([this](int32_t pointer_id)
        {
            del_session(pointer_id);
            std::cout << "on_disconnect, pointer_id:" << pointer_id << std::endl;
        });
    m_net_worker_ptr->on_msg(EnumDefine::EMsgCmd::EMC_C2S_Enter, std::bind(&wan_server::handle_enter, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
}

void wan_server::destory()
{
}

bool wan_server::handle_enter(int32_t pointer_id, void* data_ptr, int32_t size)
{
    RECV_PRASE(data_ptr, C2S_Enter, size);
    int32_t uid = msg.uid();
    std::cout << " msg abot uid:" << uid << ", token:" << msg.token() << std::endl;
    SEND_GUARD(pointer_id, EnumDefine::EMsgCmd::EMC_S2C_Enter, net_worker, *m_net_worker_ptr, S2C_Enter);
    reply.set_result(EnumDefine::EErrorCode::EEC_Success);
    m_sessions[pointer_id]->set_uid(uid);
    g_player_mgr.enter(uid);
    return true;
}

void wan_server::add_session(int32_t pointer_id, std::string ip, uint32_t port)
{
    auto ptr = std::make_unique<session>(pointer_id, ip, port);
    m_sessions.insert(std::make_pair(pointer_id, std::move(ptr)));
}

void wan_server::del_session(int32_t pointer_id)
{
    uint32_t uid = m_sessions[pointer_id]->get_uid();
    m_sessions.erase(pointer_id);
    g_player_mgr.leave(uid);
}

wan_server g_wan_server;
