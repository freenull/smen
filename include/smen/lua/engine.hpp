#ifndef SMEN_LUA_ENGINE_HPP
#define SMEN_LUA_ENGINE_HPP

#include <vector>
#include <string>
#include <variant>
#include <smen/lua/env.hpp>
#include <smen/lua/script.hpp>
#include <smen/lua/functions.hpp>
#include <smen/variant/variant.hpp>
#include <memory>

namespace smen {
    class LuaEngineException : public std::runtime_error {
    public:
        inline LuaEngineException(const std::string & msg)
            : std::runtime_error("exception thrown by Lua engine: " + msg) {}
    };

    template <typename T>
    using LuaUserdataConstructor = std::function<void (T * ptr)>;

    const LuaStackIndex LUA_GLOBALS_INDEX = LUA_GLOBALSINDEX;

    class LuaEngine;

    template <typename T>
    class LuaNativeTypeDescriptor;
    class LuaUntypedNativeTypeDescriptor;

    class LuaEngine {
    private:
        static int _closure_proxy(lua_State * L);
        static int _function_proxy(lua_State * L);

        // _env must be collected after all other members
        // (that means it must appear before them)
        mutable LuaEnvironment _env;

        std::unordered_map<std::string, std::unique_ptr<LuaUntypedNativeTypeDescriptor>> _native_types;

        LuaObject _nil;
        LuaObject _true;
        LuaObject _false;

        LuaObject _tostring_func;
        LuaObject _ffi_typeof_func;
        LuaObject _int64_ctype;
        LuaObject _uint64_ctype;

    public:
        LuaEngine();

        LuaObject globals() const;

        LuaResult load_string(const std::string & s) const;
        LuaResult load_file(const std::string & path) const;
        LuaScript load_script(const std::string & path) const;

        LuaObject nil() const;

        LuaObject new_table() const;
        LuaObject new_light_userdata(void * ptr) const;
        LuaObject new_userdata(size_t size, LuaUserdataConstructor<void> ctor);
        LuaObject new_userdata_with_metatable(size_t size, LuaUserdataConstructor<void> ctor, LuaRegistryIndex mt_reg_index) const;
        LuaResult new_userdata_with_metatable(size_t size, LuaUserdataConstructor<void> ctor, const LuaObject & mt_object) const;

        LuaObject string(const std::string & val) const;
        LuaObject number(LuaNumber val) const;
        LuaObject boolean(bool val) const;
        LuaObject function(LuaNativeFunction func) const;
        LuaObject closure(LuaNativeFunction func, const std::vector<LuaObject> & upvalues) const;

        LuaObject int64_cdata(int64_t val) const;
        LuaObject uint64_cdata(uint64_t val) const;

        std::optional<int64_t> int64_cdata_value(const LuaObject & obj) const;
        std::optional<uint64_t> uint64_cdata_value(const LuaObject & obj) const;

        bool is_int64_cdata(const LuaObject & obj) const;
        bool is_uint64_cdata(const LuaObject & obj) const;

        template <typename T>
        LuaObject new_light_userdata(T * ptr) const {
            return new_light_userdata(reinterpret_cast<void *>(ptr));
        }

        template <typename T>
        LuaObject new_userdata(LuaUserdataConstructor<T> ctor) const {
            void * ptr = _env.push_new_userdata(sizeof(T));
            ctor(reinterpret_cast<T *>(ptr));
            return LuaObject::consume_from_stack_top_ref(_env);
        }

        template <typename T>
        LuaObject new_userdata_with_metatable(LuaUserdataConstructor<T> ctor, LuaRegistryIndex mt_reg_index) const {
            void * ptr = _env.push_new_userdata(sizeof(T));
            ctor(reinterpret_cast<T *>(ptr));
            _env.registry.get(mt_reg_index);
            _env.set_metatable(-2);
            return LuaObject::consume_from_stack_top_ref(_env);
        }

        template <typename T>
        LuaObject new_userdata_with_metatable(T * & target, LuaRegistryIndex mt_reg_index) const {
            void * ptr = _env.push_new_userdata(sizeof(T));
            target = reinterpret_cast<T *>(ptr);
            _env.registry.get(mt_reg_index);
            _env.set_metatable(-2);
            return LuaObject::consume_from_stack_top_ref(_env);
        }

        template <typename T>
        LuaResult new_userdata_with_metatable(LuaUserdataConstructor<T> ctor, const LuaObject & mt_object) const {
            if (mt_object.reference() == 0) return LuaResult(LuaStatus::RUNTIME_ERROR, "attempted to create userdata with object that is not a metatable");
            return LuaResult(new_userdata_with_metatable<T>(ctor, mt_object.reference()));
        }

        template <typename T>
        LuaResult new_userdata_with_metatable(T * & target, const LuaObject & mt_object) const {
            if (mt_object.reference() == 0) return LuaResult(LuaStatus::RUNTIME_ERROR, "attempted to create userdata with object that is not a metatable");
            return LuaResult(new_userdata_with_metatable<T>(target, mt_object.reference()));
        }

        template <typename T>
        LuaNativeTypeDescriptor<T> & create_native_type(const std::string & name);

        template <typename T>
        LuaNativeTypeDescriptor<T> & get_native_type(const std::string & name);

        LuaObject native_type_constructor(const std::string & name) const;
        const std::unique_ptr<LuaUntypedNativeTypeDescriptor> & native_type(const std::string & name) const;
        bool is_native_type(const LuaObject & obj, const std::string & name) const;
        LuaObject native_type_instance(const std::string & name) const;

        template <typename T>
        LuaObject native_type_copy(const std::string & name, const T & source) const;

        template <typename T>
        LuaObject native_type_move(const std::string & name, T && source) const;
    };

}

#include <smen/lua/native_types.hpp>

namespace smen {
    /// TEMPLATE DEFINITIONS ///

    template <typename T>
    LuaNativeTypeDescriptor<T> & LuaEngine::create_native_type(const std::string & name) {
        auto type_desc_ptr = std::make_unique<LuaNativeTypeDescriptor<T>>(*this, name);
        _native_types.emplace(name, std::move(type_desc_ptr));
        return static_cast<LuaNativeTypeDescriptor<T> &>(*_native_types.at(name));
    }

    template <typename T>
    LuaNativeTypeDescriptor<T> & LuaEngine::get_native_type(const std::string & name) {
        return static_cast<LuaNativeTypeDescriptor<T> &>(*_native_types.at(name));
    }

    template <typename T>
    LuaObject LuaEngine::native_type_copy(const std::string & name, const T & source) const {
        return static_cast<LuaNativeTypeDescriptor<T> &>(*native_type(name)).create(source);
    }

    template <typename T>
    LuaObject LuaEngine::native_type_move(const std::string & name, T && source) const {
        return static_cast<LuaNativeTypeDescriptor<T> &>(*native_type(name)).create(std::move(source));
    }
}


#endif//SMEN_LUA_ENGINE_HPP
