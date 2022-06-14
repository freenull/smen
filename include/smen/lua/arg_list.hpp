#ifndef SMEN_LUA_ARGLIST_HPP
#define SMEN_LUA_ARGLIST_HPP

#include <smen/lua/stack.hpp>
#include <vector>

namespace smen {
    class LuaObject;

    class LuaArgumentList {
    public:
        const std::vector<LuaObject> args;

        void push_all_to_stack() const;

        std::vector<LuaObject>::size_type size();
    };
}

#endif//SMEN_LUA_ARGLIST_HPP

