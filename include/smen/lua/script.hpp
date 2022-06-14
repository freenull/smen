#ifndef SMEN_LUA_SCRIPT_HPP
#define SMEN_LUA_SCRIPT_HPP

#include <smen/lua/object.hpp>

namespace smen {
    class LuaEngine;

    class LuaScript {
    private:
        const LuaEngine & _engine;
        std::string _path;
        LuaObject _result;

    public:
        LuaScript(const LuaEngine & engine);

        inline const std::string & path() const { return _path; }
        inline const LuaObject & result() const { return _result; }

        LuaResult load(const std::string & path);
        LuaResult reload();
    };

}

#endif//SMEN_LUA_SCRIPT_HPP
