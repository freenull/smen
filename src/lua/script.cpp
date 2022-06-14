#include <smen/lua/script.hpp>
#include <smen/lua/engine.hpp>
#include <smen/lua/native_types.hpp>

namespace smen {
    LuaScript::LuaScript(const LuaEngine & engine)
    : _engine(engine)
    , _path()
    , _result(engine.nil())
    {}

    LuaResult LuaScript::load(const std::string & path) {
        auto result = _engine.load_file(path);
        if (result.fail()) return result;

        auto func_result = result.value().call();
        if (func_result.fail()) return func_result;

        if (func_result.values().size() > 0) {
            _result = func_result.value();
        }
        _path = path;

        return LuaResult(_result);
    }

    LuaResult LuaScript::reload() {
        return load(_path);
    }
}
