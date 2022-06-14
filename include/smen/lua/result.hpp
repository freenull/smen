#ifndef SMEN_LUA_RESULT_HPP
#define SMEN_LUA_RESULT_HPP

#include <smen/lua/lua.hpp>
#include <iostream>
#include <vector>

namespace smen {
    class LuaObject;

    enum class LuaStatus {
        OK = LUA_OK,
        YIELD = LUA_YIELD,
        RUNTIME_ERROR = LUA_ERRRUN,
        SYNTAX_ERROR = LUA_ERRSYNTAX,
        MEMORY_ERROR = LUA_ERRMEM,
        ERROR_HANDLER_ERROR = LUA_ERRERR,
        FILE_LOAD_ERROR = LUA_ERRFILE,

        SMEN_ERROR // for custom errors
    };

    std::ostream & operator<<(std::ostream & s, const LuaStatus & result);

    struct LuaResult {
    public:
        class InvalidAccessException : public std::runtime_error {
        public:
            InvalidAccessException(const std::string & msg) : std::runtime_error(msg) {}
        };

    private:
        LuaStatus _status_code;
        std::string _error_msg;
        std::vector<LuaObject> _values;

    public:
        explicit LuaResult();
        LuaResult(std::vector<LuaObject> && values);
        LuaResult(const LuaObject & value);
        LuaResult(LuaStatus result_code, const std::string & error_msg);

        static LuaResult error(const std::string & msg);

        bool success() const;
        bool fail() const;
        LuaStatus status() const;
        std::string error_msg() const;
        LuaObject value() const;
        const std::vector<LuaObject> & values() const;
    };

    std::ostream & operator<<(std::ostream & s, const LuaResult & result);
}

#endif//SMEN_LUA_RESULT_HPP
