#ifndef SMEN_LUA_ERROR_HANDLER_HPP
#define SMEN_LUA_ERROR_HANDLER_HPP

#include <smen/lua/stack.hpp>

namespace smen {
    class LuaErrorHandler : public LuaStackPushable {
    public:
        virtual void push_to_stack(LuaEnvironment & env) const override;
    };
}

#endif//SMEN_LUA_ERROR_HANDLER_HPP
