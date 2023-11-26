#pragma once
#include <iostream>
#include <unordered_map>
#include <memory>

class scene
{
public:
    scene();
    ~scene();
};

class scene_mgr
{
public:
    scene_mgr();
    ~scene_mgr();

public:
    void update(int32_t delta);
    
private:
    std::unordered_map<int32_t, std::unique_ptr<scene>> m_scenes;
};

extern scene_mgr g_scene_mgr;
