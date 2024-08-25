#include "./engine.h"
#include "./wan_server.h"
#include "./scene_mgr.h"
#include "common.pb.h"
#include "enum_define.pb.h"

#ifdef __cplusplus
extern "C"
{
#endif
#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>
#ifdef __cplusplus
}
#endif

#define LUA_SCRIPT_PATH_DEV "../../../../server/script/?.lua;"
#define LUA_SCRIPT_DEV_LAUNCH "../../../../server/script/main.lua"
#define LUA_SCRIPT_PATH_PUB LUA_SCRIPT_PATH_DEV
#define LUA_SCRIPT_PUB_LAUNCH LUA_SCRIPT_DEV_LAUNCH

static student* global_student_ptr;

static void* l_alloc(void* ud, void* ptr, size_t osize,
    size_t nsize) {
    (void)ud;  (void)osize;  /* not used */
    if (nsize == 0) {
        free(ptr);
        return nullptr;
    }
    else
        return realloc(ptr, nsize);
}

static int lua_exec_sum(lua_State* L) {
    int param1 = luaL_checkinteger(L, 1);
    int param2 = luaL_checkinteger(L, 2);
    
    int result = param1 + param2;
    
    lua_pushinteger(L, result);
    
    return 1;
}

static int lua_create_student(lua_State* lua_vm) {
    size_t size = sizeof(student);
    student* student_ptr = (student*)lua_newuserdata(lua_vm, size);
    return 1;
}

static int lua_student_set_name(lua_State* lua_vm) {
    student* student_ptr = (student*)lua_touserdata(lua_vm, 1);
    size_t length;
    const char* name = luaL_checklstring(lua_vm, 2, &length);
    student_ptr->set_name(name);
    return 0;
}

static int lua_student_get_name(lua_State* lua_vm) {
    student* student_ptr = (student*)lua_touserdata(lua_vm, 1);
    std::string name = student_ptr->get_name();
    lua_pushstring(lua_vm, name.c_str());
    return 1;
}

int lua_student_get_age(lua_State* lua_vm)
{
	student** s = (student**)luaL_checkudata(lua_vm, 1, "StudentUserData");
	luaL_argcheck(lua_vm, s != NULL, 1, "invalid user data");
 
	lua_pushinteger(lua_vm, (*s)->get_age());
 
	return 1;
}
 
int lua_student_set_age(lua_State* lua_vm)
{
	student** s = (student**)luaL_checkudata(lua_vm, 1, "StudentUserData");
	luaL_argcheck(lua_vm, s != NULL, 1, "invalid user data");
 
	luaL_checktype(lua_vm, -1, LUA_TNUMBER);
 
	(*s)->set_age((unsigned)lua_tointeger(lua_vm, -1));
 
	return 0;
}

int lua_get_global_student(lua_State *lua_vm) {
    student** student_ptr = (student**)lua_newuserdata(lua_vm, sizeof(student*));
    *student_ptr = global_student_ptr;
    luaL_getmetatable(lua_vm, "StudentUserData"); 
    lua_setmetatable(lua_vm, -2);
    return 1;
}

student::student()
    : m_name("Empty")
    , m_age(0)
{
    std::cout << "gou zao han shu" << std::endl;
}
student::~student()
{
    std::cout << "xi gou han shu" << std::endl;
}
std::string student::get_name()
{
    return m_name;
}
void student::set_name(std::string name)
{
    m_name = name;
}
uint32_t student::get_age()
{
    return m_age;
}
void student::set_age(uint32_t age)
{
    m_age = age;
}
void student::print()
{
    std::cout << "name:" << m_name << ", age:" << m_age << std::endl;
}

engine::engine()
    : m_net_worker(m_context)
    , m_console_reader(m_context)
    , m_timer(m_context, boost::posix_time::milliseconds(1))
    , m_lua_vm(nullptr)
{
}

engine::~engine()
{
}

void engine::init()
{
    init_script();
    m_net_worker.init(m_context);
    g_wan_server.init(&m_net_worker);
    m_net_worker.open(55890, true);
    m_console_reader.start();
    m_timer.async_wait(boost::bind(&engine::loop, this, boost::asio::placeholders::error));
}

void engine::destory()
{
    g_wan_server.destory();
}

void engine::run()
{
    m_context.run();
    lua_close(m_lua_vm);
}

void engine::loop(const boost::system::error_code& ec)
{
    if (ec)
    {
        std::cout << "loop failed:" << ec.message() << std::endl;
        return;
    }
    auto begin_tick = boost::asio::chrono::steady_clock::now();
    run_once();
    auto end_tick = boost::asio::chrono::steady_clock::now();
    uint32_t spend_tick = static_cast<uint32_t>((end_tick - begin_tick).count() / 1000 / 1000);
    //std::cout << "loop one times. spend_tick:" << spend_tick << std::endl;
    m_delta = 1;
    if (spend_tick < tick_interval)
    {
        m_delta = tick_interval - spend_tick;
    }
    m_timer.expires_from_now(boost::posix_time::milliseconds(m_delta));
    m_timer.async_wait(boost::bind(&engine::loop, this, boost::asio::placeholders::error));
}

void engine::run_once()
{
    g_scene_mgr.update(m_delta);
    {
        console_reader::command cmd;
        if (m_console_reader.pop_front(cmd))
        {
            if (cmd.name == "hello")
            {
                // This number needs to be obtained through an interface that passes in the name
                if (cmd.params.size() <= 0)
                {
                    std::cout << "Insufficient parameters!" << std::endl;
                    return;
                }
                int32_t pointer_id = std::stoi(cmd.params[0]);
                SEND_GUARD(pointer_id, m_net_worker, S2C_Hello);
                reply.set_id(100);
                reply.add_member(3434);
            }
            if (cmd.name == "refresh")
            {
                int32_t ret = luaL_dofile(m_lua_vm, LUA_SCRIPT_PUB_LAUNCH);
                if (ret != 0)
                {
                    const char* errorMsg = lua_tostring(m_lua_vm, -1);
                    std::cout << "error:" << errorMsg << std::endl;
                    lua_pop(m_lua_vm, 1);
                }
            }
            if (cmd.name == "reload")
            {
                init_script();
            }
            if (cmd.name == "testlua1")
            {
                int count = lua_gettop(m_lua_vm);
                int32_t type = lua_getglobal(m_lua_vm, "CheckAddBuffManager");
                count = lua_gettop(m_lua_vm);
                if (type == LUA_TTABLE)
                {
                    lua_pushstring(m_lua_vm, "Check");
                    count = lua_gettop(m_lua_vm);
                    type = lua_gettable(m_lua_vm, -2);
                    count = lua_gettop(m_lua_vm);
                    if (type == LUA_TFUNCTION)
                    {
                        lua_pushvalue(m_lua_vm, -2);
                        count = lua_gettop(m_lua_vm);
                        lua_remove(m_lua_vm, 1);
                        count = lua_gettop(m_lua_vm);
                        lua_pushinteger(m_lua_vm, 111);
                        lua_pushinteger(m_lua_vm, 222);
                        int32_t ret = lua_pcall(m_lua_vm, 3, 0, 0);
                        count = lua_gettop(m_lua_vm);
                        count = lua_gettop(m_lua_vm);
                    }
                    else
                    {
                        lua_pop(m_lua_vm, 2);
                    }
                }
                else {
                    lua_pop(m_lua_vm, 1);
                }
            }
            if (cmd.name == "testlua2")
            {
                int32_t type = lua_getglobal(m_lua_vm, "CheckAddBuffManagerCheck");
                if (type == LUA_TFUNCTION)
                {
                    lua_pushstring(m_lua_vm, "sdfsdf111");
                    lua_pushstring(m_lua_vm, "sdfsdf222");
                    lua_call(m_lua_vm, 2, 0);
                }
                else
                {
                    lua_pop(m_lua_vm, 1);
                }
            }
        }
    }
}

static const luaL_Reg lua_reg_student_member_funcs[] = {
	{ "get_age", lua_student_get_age },
	{ "set_age", lua_student_set_age },
	{ NULL, NULL },
};

void engine::init_script()
{
    if (m_lua_vm)
    {
        lua_close(m_lua_vm);
        m_lua_vm = nullptr;
    }

    m_lua_vm = lua_newstate(l_alloc, nullptr);
    luaL_openlibs(m_lua_vm);

    // setting package.path
    lua_getglobal(m_lua_vm, "package");
    lua_pushstring(m_lua_vm, LUA_SCRIPT_PATH_PUB);
    lua_getfield(m_lua_vm, -2, "path");
    lua_concat(m_lua_vm, 2);
    lua_setfield(m_lua_vm, -2, "path");
    lua_pop(m_lua_vm, 1);

    // register C++ function to lua global env
    lua_pushcfunction(m_lua_vm, lua_exec_sum);
    lua_setglobal(m_lua_vm, "lua_exec_sum");
    lua_pushcfunction(m_lua_vm, lua_create_student);
    lua_setglobal(m_lua_vm, "lua_create_student");
    lua_pushcfunction(m_lua_vm, lua_student_set_name);
    lua_setglobal(m_lua_vm, "lua_student_set_name");
    lua_pushcfunction(m_lua_vm, lua_student_get_name);
    lua_setglobal(m_lua_vm, "lua_student_get_name");
    
    
    // oop access mode, solution params safety problem
    global_student_ptr = new student();
    luaL_newmetatable(m_lua_vm, "StudentUserData");
    //lua_pushcfunction(m_lua_vm, lua_get_global_student);
    //lua_setfield(m_lua_vm, -2, "get_global_student");
    //lua_pushcfunction(m_lua_vm, lua_student_get_age);
    //lua_setfield(m_lua_vm, -2, "get_age"); // let lua_student_get_age add to metatable
    lua_pushvalue(m_lua_vm, -1);// copy metatable to stack top
    lua_setfield(m_lua_vm, -2, "__index");// settiing metatable __index to metatable self
    luaL_setfuncs(m_lua_vm, lua_reg_student_member_funcs, 0);
    lua_pop(m_lua_vm, 1); // clear stack
    lua_pushcfunction(m_lua_vm, lua_get_global_student);
    lua_setglobal(m_lua_vm, "get_global_student");
    int32_t num = lua_gettop(m_lua_vm);
    std::cout << "stack size:" << num << std::endl;
    int32_t ret = luaL_dofile(m_lua_vm, LUA_SCRIPT_PUB_LAUNCH);
    if (ret != 0)
    {
        const char* errorMsg = lua_tostring(m_lua_vm, -1);
        std::cout << "error:" << errorMsg << std::endl;
        lua_pop(m_lua_vm, 1);
    }

}

engine g_engine;
