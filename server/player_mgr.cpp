#include "./player_mgr.h"

player::player(int32_t uid)
    : m_uid(uid)
    , m_state(EPS_None)
{
    
}

player::~player()
{
    
}

void player::online()
{
    m_state = EPS_Online;
}

void player::offline()
{
    m_state = EPS_Offline;
}

player_mgr::player_mgr()
{

}

player_mgr::~player_mgr()
{

}

void player_mgr::enter(int32_t uid)
{
    auto it = m_players.find(uid);
    if (it == m_players.end())
    {
        auto ptr = std::make_unique<player>(uid);
        ptr->online();
        m_players.insert(std::make_pair(uid, std::move(ptr)));
    }
    else
    {
        it->second->online();
    }
}

void player_mgr::leave(int32_t uid)
{
    auto it = m_players.find(uid);
    if (it != m_players.end())
    {
        it->second->offline();
    }
}

player_mgr g_player_mgr;
