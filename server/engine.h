#ifndef _ENGINE_H_
#define _ENGINE_H_

#include "../source/common.hpp"

struct lua_State;
class engine
{
    using timer_umap_t = std::unordered_map<int32_t, boost::asio::deadline_timer*>;

public:
    engine();
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

#endif
