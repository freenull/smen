#include <smen/lua/library.hpp>

namespace smen {
    std::unordered_map<std::string, std::unique_ptr<LuaLibrary>> LuaLibrary::_libs = {};

    LuaLibrary::LuaLibrary(LuaEngine & engine)
    : _engine(&engine)
    , _lib_table(engine.nil())
    {
        std::cout << "init tab " << _lib_table.type() << " "<< "\n";
    }

    LuaResult LuaLibrary::_require_preloader_func(LuaEngine & engine, const LuaNativeFunctionArgs & args) {
        // closure - first (and only) arg is LuaLibrary
        //
        auto & lib = *args[0].userdata<LuaLibrary>();
        return lib.load();
    }

    LuaObject LuaLibrary::load() {
        std::cout << "load tab " << _lib_table.type() << " " << name() << "\n";
        if (_lib_table.type() == LuaType::NIL) {
            _lib_table = create_table();
            on_load(_lib_table);
        }
        return _lib_table;
    }

    LuaObject LuaLibrary::force_reload() {
        _lib_table = _engine->nil();
        return load();
    }

    void LuaLibrary::init_preloader() {
        auto package_tab = _engine->globals().get("package");
        if (package_tab.type() != LuaType::TABLE) return;
        auto loaders_tab = package_tab.get("preload");
        if (package_tab.type() != LuaType::TABLE) return;

        auto preloader = _engine->closure(_require_preloader_func, { _engine->new_light_userdata(this) });
        loaders_tab.set(name(), preloader);
    }

    LuaObject LuaLibrary::create_table() {
        auto table = _engine->new_table();
        _engine->globals().set(name(), table);
        return table;
    }
}
