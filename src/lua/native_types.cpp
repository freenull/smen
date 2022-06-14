#include <smen/lua/native_types.hpp>
#include <smen/ecs/scene.hpp>

namespace smen {
    LuaResult LuaUntypedNativeTypeDescriptor::_metatype_newindex_func(LuaEngine & engine, const LuaNativeFunctionArgs & args) {
        return LuaResult::error("cannot modify meta-type");
    }

    LuaUntypedNativeTypeDescriptor::LuaUntypedNativeTypeDescriptor(LuaEngine & engine, std::function<void (void *)> empty_ctor, size_t size, const std::string & name, const LuaObject & ctor)
    : _engine(engine)
    , _empty_ctor(empty_ctor)
    , size(size)
    , name(name)
    , metatable(engine.new_table())
    , metatype(engine.new_table())
    , metatype_content(engine.new_table())
    , metatype_metatable(engine.new_table())
    {
        metatype.set_metatable(metatype_metatable);
        metatype_metatable.set("__call", ctor);
        metatype_metatable.set("__index", metatype_content);
        metatype_metatable.set("__newindex", engine.function(_metatype_newindex_func));
        metatable.set("__typename", name);
    }

    void LuaUntypedNativeTypeDescriptor::add_shared_object(const std::string & shared_object_name, const LuaObject & obj) {
        _shared_objects.emplace(shared_object_name, obj);
    }

    bool LuaUntypedNativeTypeDescriptor::has_shared_object(const std::string & shared_object_name) const {
        return _shared_objects.find(shared_object_name) != _shared_objects.end();
    }

    const LuaObject & LuaUntypedNativeTypeDescriptor::get_shared_object(const std::string & shared_object_name) const {
        return _shared_objects.at(shared_object_name);
    }

    const std::unordered_map<std::string, LuaObject> LuaUntypedNativeTypeDescriptor::shared_objects() const {
        return _shared_objects;
    }

    LuaObject LuaUntypedNativeTypeDescriptor::create() const {
        return _engine.new_userdata_with_metatable(size, _empty_ctor, metatable).value();
    }
}
