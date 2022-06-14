#include <smen/lua/type.hpp>

namespace smen {
    std::ostream & operator<<(std::ostream & s, LuaType type) {
        switch (type) {
        case LuaType::NONE: s << "<none>"; break;
        case LuaType::NIL: s << "nil"; break;
        case LuaType::BOOLEAN: s << "boolean"; break;
        case LuaType::NUMBER: s << "number"; break;
        case LuaType::STRING: s << "string"; break;
        case LuaType::FUNCTION: s << "function"; break;
        case LuaType::USERDATA: s << "userdata"; break;
        case LuaType::LIGHT_USERDATA: s << "light_userdata"; break;
        case LuaType::THREAD: s << "thread"; break;
        case LuaType::TABLE: s << "table"; break;
        case LuaType::CDATA: s << "cdata"; break;
        default: s << "<invalid>"; break;
        }
        return s;
    }
}
