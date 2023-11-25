#pragma once
#include <iostream>
#include <unordered_map>
#include <memory>

class player
{
public:
    enum EPlayerState
    {
        EPS_None    = 0x0,
        EPS_Online  = 0x1,
        EPS_Offline = 0x2,
        EPS_Max
    };

public:
    player(int32_t uid);
    ~player();

    void online();
    void offline();

private:
    int32_t m_uid;
    EPlayerState m_state;
};

class player_mgr
{
public:
    player_mgr();
    ~player_mgr();

    void enter(int32_t uid);
    void leave(int32_t uid);

private:
    std::unordered_map<int32_t, std::unique_ptr<player>> m_players;
};

extern player_mgr g_player_mgr;
