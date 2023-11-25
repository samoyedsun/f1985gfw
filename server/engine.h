#pragma once
#include "../source/common.hpp"

struct lua_State;
class engine
{
public:
    engine();
    ~engine();
    void init();
    void destory();
    void run();

private:
    void loop(const boost::system::error_code& ec);
    void run_once();
    void init_script();

    static const int32_t tick_interval = 16;

private:
    boost::asio::io_context m_context;
    ws_worker m_net_worker;
    console_reader m_console_reader;
    boost::asio::deadline_timer m_timer;
    lua_State* m_lua_vm;
};

extern engine g_engine;
