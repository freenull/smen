#ifndef SMEN_LUA_STACK_PUSHABLE_HPP
#define SMEN_LUA_STACK_PUSHABLE_HPP

#include <smen/lua/lua.hpp>

namespace smen {
    class LuaEnvironment;

    using LuaStackIndex = int;
    using LuaNumber = lua_Number;

    class LuaStackPushable {
    public:
        virtual void push_to_stack(LuaEnvironment & env) const = 0;
        virtual ~LuaStackPushable() {}
    };
}

#endif//SMEN_LUA_STACK_PUSHABLE_HPP
