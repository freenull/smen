#ifndef SMEN_LUA_TYPE_HPP
#define SMEN_LUA_TYPE_HPP

#include <smen/lua/lua.hpp>
#include <iostream>

namespace smen {
    enum class LuaType {
        NONE = LUA_TNONE,
        NIL = LUA_TNIL,
        BOOLEAN = LUA_TBOOLEAN,
        NUMBER = LUA_TNUMBER,
        STRING = LUA_TSTRING,
        FUNCTION = LUA_TFUNCTION,
        USERDATA = LUA_TUSERDATA,
        LIGHT_USERDATA = LUA_TLIGHTUSERDATA,
        THREAD = LUA_TTHREAD,
        TABLE = LUA_TTABLE,
        CDATA = 10
    };

    std::ostream & operator<<(std::ostream & s, LuaType type);
}

#endif//SMEN_LUA_TYPE_HPP
