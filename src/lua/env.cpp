#include <smen/lua/env.hpp>

#include <cstdlib>

namespace smen {
    void * LuaEnvironment::_default_lua_allocator(
        void * ud, void * ptr, size_t old_size, size_t new_size
    ) {
        (void)old_size;
        (void)ud;

         if (new_size == 0) {
             // ANSI C ensures free(NULL) is a noop
             std::free(ptr);
             return nullptr;
         } else {
             // ANSI C ensures realloc(NULL, size) = malloc(size)
             return std::realloc(ptr, new_size);
         }
    }

    int LuaEnvironment::_default_error_handler(lua_State * L) {
        void * self_opaque = lua_touserdata(L, lua_upvalueindex(1));
        LuaEnvironment & self = *(static_cast<LuaEnvironment *>(self_opaque));

        self.registry.get(self._traceback_reg_idx);
        lua_insert(L, -2);
        lua_pcall(L, 1, 1, 0);
        return 1;
    }

    LuaEnvironment::LuaEnvironment()
    : L(
        lua_newstate(_default_lua_allocator, nullptr)
    )
    , registry(L)
    {
        luaL_openlibs(L);
        _save_traceback_func();
    }

    LuaEnvironment::LuaEnvironment(const LuaEnvironment & env)
    : L(env.L)
    , _traceback_reg_idx(env._traceback_reg_idx)
    , registry(env.registry)
    {}

    void LuaEnvironment::_save_traceback_func() {
        lua_getglobal(L, "debug");
        lua_getfield(L, -1, "traceback");
        _traceback_reg_idx = registry.alloc();
        lua_pop(L, 1);
    }

    LuaStackIndex LuaEnvironment::top() {
        return lua_gettop(luajit_ptr());
    }

    void LuaEnvironment::move_top(LuaStackIndex target_idx) {
        lua_insert(luajit_ptr(), target_idx);
    }

    void LuaEnvironment::push_string(const std::string & str) {
        lua_pushlstring(luajit_ptr(), str.c_str(), str.size());
    }

    void LuaEnvironment::push_number(lua_Number number) {
        lua_pushnumber(luajit_ptr(), number);
    }

    void LuaEnvironment::push_boolean(bool boolean) {
        lua_pushboolean(luajit_ptr(), boolean ? 1 : 0);
    }

    void LuaEnvironment::push_nil() {
        lua_pushnil(luajit_ptr());
    }

    void LuaEnvironment::push_new_table() {
        lua_newtable(luajit_ptr());
    }

    void LuaEnvironment::push_light_userdata(void * ptr) {
        return lua_pushlightuserdata(luajit_ptr(), ptr);
    }

    void * LuaEnvironment::push_new_userdata(size_t size) {
        return lua_newuserdata(luajit_ptr(), size);
    }

    void LuaEnvironment::push_raw_function(lua_CFunction func) {
        lua_pushcclosure(luajit_ptr(), func, 0);
    }

    void LuaEnvironment::push_raw_closure(lua_CFunction func, int upvalue_count) {
        lua_pushcclosure(luajit_ptr(), func, upvalue_count);
    }

    size_t LuaEnvironment::length(LuaStackIndex idx) {
        return lua_objlen(luajit_ptr(), idx);
    }

    void LuaEnvironment::dup(LuaStackIndex idx) {
        lua_pushvalue(luajit_ptr(), idx);
    }

    void LuaEnvironment::pop(int count) {
        lua_pop(luajit_ptr(), count);
    }

    void LuaEnvironment::move(LuaStackIndex target_idx) {
        lua_insert(luajit_ptr(), target_idx);
    }

    int LuaEnvironment::raise_error_and_leave_function() {
        return lua_error(luajit_ptr());
    }

    void LuaEnvironment::get_field(LuaStackIndex idx) {
        lua_gettable(luajit_ptr(), idx);
    }

    void LuaEnvironment::get_string_field(LuaStackIndex idx, const std::string & key) {
        lua_getfield(luajit_ptr(), idx, key.c_str());
    }

    void LuaEnvironment::get_global(const std::string & key) {
        lua_getglobal(luajit_ptr(), key.c_str());
    }

    bool LuaEnvironment::get_metatable(LuaStackIndex idx) {
        return lua_getmetatable(luajit_ptr(), idx) != 0;
    }

    bool LuaEnvironment::table_next(LuaStackIndex table_idx) {
        return lua_next(luajit_ptr(), table_idx) != 0;
    }

    bool LuaEnvironment::equal(LuaStackIndex left_idx, LuaStackIndex right_idx) {
        return lua_equal(luajit_ptr(), left_idx, right_idx) != 0;
    }

    void LuaEnvironment::set_field(LuaStackIndex idx) {
        lua_settable(luajit_ptr(), idx);
    }

    void LuaEnvironment::set_string_field(LuaStackIndex idx, const std::string & key) {
        lua_setfield(luajit_ptr(), idx, key.c_str());
    }

    void LuaEnvironment::set_global(const std::string & key) {
        lua_setglobal(luajit_ptr(), key.c_str());
    }

    void LuaEnvironment::set_metatable(LuaStackIndex idx) {
        lua_setmetatable(luajit_ptr(), idx);
    }

    std::string LuaEnvironment::read_string(LuaStackIndex idx) {
        size_t cstr_len;
        const char * cstr_ptr = lua_tolstring(
            luajit_ptr(), idx, &cstr_len
        );
        return std::string(cstr_ptr, cstr_len);
    }

    lua_Number LuaEnvironment::read_number(LuaStackIndex idx) {
        return lua_tonumber(luajit_ptr(), idx);
    }

    bool LuaEnvironment::read_boolean(LuaStackIndex idx) {
        return lua_toboolean(luajit_ptr(), idx);
    }

    void * LuaEnvironment::read_userdata(LuaStackIndex idx) {
        return lua_touserdata(luajit_ptr(), idx);
    }

    LuaType LuaEnvironment::get_type(LuaStackIndex idx) {
        return static_cast<LuaType>(lua_type(luajit_ptr(), idx));
    }

    LuaStatus LuaEnvironment::load_file(const std::string & path) {
        return static_cast<LuaStatus>(luaL_loadfile(luajit_ptr(), path.c_str()));
    }

    LuaStatus LuaEnvironment::load_string(const std::string & script) {
        return static_cast<LuaStatus>(luaL_loadstring(luajit_ptr(), script.c_str()));
    }

    LuaStatus LuaEnvironment::do_string(const std::string & script) {
        auto load_result = load_string(script);
        if (load_result != LuaStatus::OK) return load_result;

        //@TODO err handling
        return call(0, 0, 0);
    }

    LuaStatus LuaEnvironment::call(int arg_count, int result_count, LuaStackIndex err_func) {
        return static_cast<LuaStatus>(lua_pcall(
            luajit_ptr(),
            arg_count,
            result_count,
            err_func
        ));
    }

    LuaStatus LuaEnvironment::traceback_call(int arg_count, int result_count) {
        lua_pushlightuserdata(L, this);
        lua_pushcclosure(L, _default_error_handler, 1);
        lua_insert(L, -arg_count - 2); // move error handler below function
        auto result = static_cast<LuaStatus>(lua_pcall(
            luajit_ptr(),
            arg_count,
            result_count,
            -arg_count - 2
        ));
        lua_insert(L, -2);
        lua_pop(L, 1);

        return result;
    }

    void LuaEnvironment::debug_stack() {
        auto top_idx = top();
        for (auto i = top_idx; i > 0; i--) {
            std::cout << "[" << i << "] " << get_type(i) << " ";
            auto type = get_type(i);

            switch(type) {
            case LuaType::STRING:
                std::cout << "\"" << read_string(i) << "\"";
                break;
            case LuaType::NUMBER:
                std::cout << read_number(i);
                break;
            case LuaType::BOOLEAN:
                std::cout << (read_boolean(i) ? "true" : "false");
                break;
            case LuaType::LIGHT_USERDATA:
            case LuaType::USERDATA:
                std::cout << lua_touserdata(luajit_ptr(), i);
                break;
            case LuaType::FUNCTION:
                std::cout << lua_topointer(luajit_ptr(), i);
                break;
            default:
                std::cout << read_string(i);
                break;
            }

            std::cout << "\n";
        }
    }

    LuaEnvironment::~LuaEnvironment() {
        lua_close(L);
    }
}
