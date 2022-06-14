#ifndef SMEN_LUA_LIBRARY_HPP
#define SMEN_LUA_LIBRARY_HPP

#include <smen/lua/functions.hpp>
#include <smen/lua/engine.hpp>
#include <smen/event.hpp>

namespace smen {
    class LuaLibrary {
    private:
        static std::unordered_map<std::string, std::unique_ptr<LuaLibrary>> _libs;

    protected:
        LuaEngine * _engine;
        LuaObject _lib_table;

        static LuaResult _require_preloader_func(LuaEngine & engine, const LuaNativeFunctionArgs & args);
    public:
        static LuaLibrary & get();
        static void add(LuaLibrary && lib);

        LuaLibrary(LuaEngine & engine);

        LuaLibrary(const LuaLibrary & lib) = delete;
        LuaLibrary(LuaLibrary && lib) = default;
        LuaLibrary & operator=(const LuaLibrary & lib) = delete;
        LuaLibrary & operator=(LuaLibrary && lib) = default;

        inline LuaObject & table() { return _lib_table; }
        inline const LuaObject & table() const { return _lib_table; }

        virtual void on_load(LuaObject & table) = 0;
        virtual std::string name() = 0;
        virtual LuaObject create_table();

        LuaObject load();
        LuaObject force_reload();
        void init_preloader();

        virtual ~LuaLibrary() = default;
    };
}

#endif//SMEN_LUA_LIBRARY_HPP
