#ifndef SMEN_LUA_ENVIRONMENT_HPP
#define SMEN_LUA_ENVIRONMENT_HPP

#include <string>
#include <smen/lua/lua.hpp>
#include <smen/lua/type.hpp>
#include <smen/lua/result.hpp>
#include <smen/lua/stack.hpp>
#include <smen/lua/registry.hpp>
#include <smen/lua/functions.hpp>

namespace smen {
    class LuaEnvironment {
    public:
        static const int UNLIMITED_RETURN_VALUES = -1;

    private:
        lua_State * const L;
        LuaRegistryIndex _traceback_reg_idx;

        static void * _default_lua_allocator(
            void * ud, void * ptr, size_t old_size, size_t new_size
        );

        static int _default_error_handler(lua_State * L);
        void _save_traceback_func();

    public:
        LuaRegistry registry;

        LuaEnvironment();
        LuaEnvironment(const LuaEnvironment & env);

        LuaStackIndex top();
        void move_top(LuaStackIndex target_idx);

        void push_string(const std::string & str);
        void push_number(lua_Number number);
        void push_boolean(bool boolean);
        void push_nil();
        void push_new_table();
        void push_light_userdata(void * ptr);
        template <typename T>
        void push_light_userdata(T * ptr) {
            push_light_userdata(reinterpret_cast<void *>(const_cast<typename std::remove_const<T>::type *>(ptr)));
        }
        void * push_new_userdata(size_t size);
        void push_raw_function(lua_CFunction func);
        void push_raw_closure(lua_CFunction func, int upvalue_count);
        size_t length(LuaStackIndex idx = -1);

        void dup(LuaStackIndex idx);
        void pop(int count);
        void move(LuaStackIndex target_idx);

        int raise_error_and_leave_function();
        // long name indicates that this method performs a longjmp,
        // so any RAII will not happen after this point

        void get_field(LuaStackIndex idx);
        void get_string_field(LuaStackIndex idx, const std::string & key);
        void get_global(const std::string & key);
        bool get_metatable(LuaStackIndex idx);

        bool table_next(LuaStackIndex table_idx);
        bool equal(LuaStackIndex left_idx, LuaStackIndex right_idx);

        void set_field(LuaStackIndex idx);
        void set_string_field(LuaStackIndex idx, const std::string & key);
        void set_global(const std::string & key);
        void set_metatable(LuaStackIndex idx);

        std::string read_string(LuaStackIndex idx = -1);
        lua_Number read_number(LuaStackIndex idx = -1);
        bool read_boolean(LuaStackIndex idx = -1);
        void * read_userdata(LuaStackIndex idx = -1);

        LuaType get_type(LuaStackIndex idx = -1);

        LuaStatus load_file(const std::string & path);

        LuaStatus load_string(const std::string & script);
        LuaStatus do_string(const std::string & script);

        LuaStatus call(int arg_count, int result_count, LuaStackIndex err_func);
        LuaStatus traceback_call(int arg_count, int result_count = UNLIMITED_RETURN_VALUES);

        void debug_stack();

        lua_State * luajit_ptr() {
            return L;
        }

        ~LuaEnvironment();
    };
}

#endif//SMEN_LUA_ENVIRONMENT_HPP
