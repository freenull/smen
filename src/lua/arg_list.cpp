#include <smen/lua/arg_list.hpp>
#include <smen/lua/object.hpp>

namespace smen {
    void LuaArgumentList::push_all_to_stack() const {
        for (const LuaObject & arg : args) {
            arg.push_to_stack();
        }
    }

    std::vector<LuaObject>::size_type LuaArgumentList::size() {
        return args.size();
    }
}
