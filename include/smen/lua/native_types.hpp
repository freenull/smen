#ifndef SMEN_LUA_NATIVE_TYPES_HPP
#define SMEN_LUA_NATIVE_TYPES_HPP

#include <smen/lua/object.hpp>
#include <smen/lua/functions.hpp>
#include <smen/lua/engine.hpp>
#include <span>

namespace smen {
    class LuaUntypedNativeTypeDescriptor {
    protected:
        LuaEngine & _engine;

    private:
        std::unordered_map<std::string, LuaObject> _shared_objects;
        std::function<void (void *)> _empty_ctor;

        static LuaResult _metatype_newindex_func(LuaEngine & engine, const LuaNativeFunctionArgs & args);

    public:
        LuaUntypedNativeTypeDescriptor(LuaEngine & engine, std::function<void (void *)> empty_ctor, size_t size, const std::string & name, const LuaObject & ctor);

        size_t size;
        std::string name;
        LuaObject metatable;
        LuaObject metatype;
        LuaObject metatype_content;
        LuaObject metatype_metatable;

        void add_shared_object(const std::string & shared_object_name, const LuaObject & obj);
        bool has_shared_object(const std::string & shared_object_name) const;

        const LuaObject & get_shared_object(const std::string & shared_object_name) const;
        const std::unordered_map<std::string, LuaObject> shared_objects() const;
        LuaObject create() const;

        virtual ~LuaUntypedNativeTypeDescriptor() = default;
    };

    template <typename T>
    class LuaNativeTypeDescriptor : public LuaUntypedNativeTypeDescriptor {
    public:
        using ToStringFunction = std::function<std::string (const LuaEngine & engine, const T & target)>;
        using GetFunction = std::function<LuaResult (const LuaEngine & engine, const T & target, const LuaObject & key)>;
        using SetFunction = std::function<LuaResult (const LuaEngine & engine, T & target, const LuaObject & key, const LuaObject & value)>;
        using LuaCtorFunction = std::function<LuaResult (const LuaEngine & engine, LuaObject & target_obj, T * target_ptr, const std::span<const LuaObject> & args)>;
        using CtorFunction = std::function<void (const LuaEngine & engine, T * target)>;
        using CopyCtorFunction = std::function<void (const LuaEngine & engine, const T & source, T * target)>;
        using MoveCtorFunction = std::function<void (const LuaEngine & engine, T && source, T * target)>;
        using DtorFunction = void (const LuaEngine & engine, T & target);
        using EqualsFunction = std::function<bool (const LuaEngine & engine, const T & left, const T & right)>;
        using LengthFunction = std::function<size_t (const LuaEngine & engine, const T & object)>;

    private:
        // extra upvalue in all funcs: the LuaNativeTypeDescriptor obj
        
        static LuaResult _ctor_func(LuaEngine & engine, const LuaNativeFunctionArgs & args);
        static LuaResult _index_func(LuaEngine & engine, const LuaNativeFunctionArgs & args);
        static LuaResult _newindex_func(LuaEngine & engine, const LuaNativeFunctionArgs & args);
        static LuaResult _tostring_func(LuaEngine & engine, const LuaNativeFunctionArgs & args);
        static LuaResult _eq_func(LuaEngine & engine, const LuaNativeFunctionArgs & args);
        static LuaResult _len_func(LuaEngine & engine, const LuaNativeFunctionArgs & args);
        static LuaResult _gc_func(LuaEngine & engine, const LuaNativeFunctionArgs & args);

        DtorFunction * _dtor_func;

    public:
        ToStringFunction to_string_func;
        GetFunction get_func;
        SetFunction set_func;
        CopyCtorFunction copy_ctor_func;
        MoveCtorFunction move_ctor_func;
        CtorFunction ctor_func;
        LuaCtorFunction lua_ctor_func;
        EqualsFunction eq_func;
        LengthFunction length_func;

        inline LuaNativeTypeDescriptor<T> & to_string(const ToStringFunction & func) {
            to_string_func = func;
            return *this;
        }

        inline LuaNativeTypeDescriptor<T> & get(const GetFunction & func) {
            get_func = func;
            return *this;
        }

        inline LuaNativeTypeDescriptor<T> & set(const SetFunction & func) {
            set_func = func;
            return *this;
        }

        inline LuaNativeTypeDescriptor<T> & copy_ctor(const CopyCtorFunction & func) {
            copy_ctor_func = func;
            return *this;
        }

        inline LuaNativeTypeDescriptor<T> & move_ctor(const MoveCtorFunction & func) {
            move_ctor_func = func;
            return *this;
        }

        inline LuaNativeTypeDescriptor<T> & ctor(const CtorFunction & func) {
            ctor_func = func;
            return *this;
        }

        inline LuaNativeTypeDescriptor<T> & lua_ctor(const LuaCtorFunction & func) {
            lua_ctor_func = func;
            return *this;
        }

        inline LuaNativeTypeDescriptor<T> & eq(const EqualsFunction & func) {
            eq_func = func;
            return *this;
        }

        inline LuaNativeTypeDescriptor<T> & length(const LengthFunction & func) {
            length_func = func;
            return *this;
        }

        inline LuaNativeTypeDescriptor<T> & dtor(DtorFunction * func) {
            set_dtor_func(func);
            return *this;
        }

        LuaNativeTypeDescriptor(LuaEngine & engine, const std::string & name);

        using LuaUntypedNativeTypeDescriptor::add_shared_object;
        using LuaUntypedNativeTypeDescriptor::has_shared_object;
        using LuaUntypedNativeTypeDescriptor::get_shared_object;
        using LuaUntypedNativeTypeDescriptor::create;

        inline DtorFunction * dtor_func() { return _dtor_func; }
        void set_dtor_func(DtorFunction * func);

        LuaObject create(const T & source) const;
        LuaObject create(T && source) const;
    };

    /// TEMPLATE DEFINITIONS ///

    // extra upvalue: the LuaNativeTypeDescriptor obj

    template <typename T>
    LuaResult LuaNativeTypeDescriptor<T>::_ctor_func(LuaEngine & engine, const LuaNativeFunctionArgs & args) {
        auto * type_desc = args[0].userdata<LuaNativeTypeDescriptor>();
        auto args_span = std::span<const LuaObject>(args.begin() + 1, args.end());
        T * target_ptr;
        auto target_obj = engine.new_userdata_with_metatable(target_ptr, type_desc->metatable).value();
        if (type_desc->lua_ctor_func) {
            return type_desc->lua_ctor_func(engine, target_obj, target_ptr, args_span);
        } else return LuaResult::error("the native type " + type_desc->name + " cannot be constructed from Lua");
    }
    
    template <typename T>
    LuaResult LuaNativeTypeDescriptor<T>::_index_func(LuaEngine & engine, const LuaNativeFunctionArgs & args) {
        auto * type_desc = args[0].userdata<LuaNativeTypeDescriptor>();
        auto & obj = *args[1].userdata<T>();
        auto key = args[2];

        if (key.type() == LuaType::STRING) {
            auto shared_object_name = key.string();
            if (type_desc->has_shared_object(shared_object_name)) {
                auto shared_object = type_desc->get_shared_object(key.string());
                return shared_object;
            }
        }

        if (type_desc->get_func) {
            return type_desc->get_func(engine, obj, key);
        } else {
            return engine.nil();
        }
    }

    // extra upvalue: the LuaNativeTypeDescriptor obj
    template <typename T>
    LuaResult LuaNativeTypeDescriptor<T>::_newindex_func(LuaEngine & engine, const LuaNativeFunctionArgs & args) {
        auto * type_desc = args[0].userdata<LuaNativeTypeDescriptor>();
        auto & obj = *args[1].userdata<T>();
        auto key = args[2];
        auto value = args[3];

        if (type_desc->set_func) {
            return type_desc->set_func(engine, obj, key, value);
        } else {
            return LuaResult::error(type_desc->name + " is immutable");
        }
    }

    // extra upvalue: the LuaNativeTypeDescriptor obj
    template <typename T>
    LuaResult LuaNativeTypeDescriptor<T>::_tostring_func(LuaEngine & engine, const LuaNativeFunctionArgs & args) {
        auto * type_desc = args[0].userdata<LuaNativeTypeDescriptor>();
        auto & obj = *args[1].userdata<T>();

        if (type_desc->to_string_func) {
            return engine.string(type_desc->to_string_func(engine, obj));
        } else {
            return engine.string(type_desc->name + "(?)");
        }

    }

    template <typename T>
    LuaResult LuaNativeTypeDescriptor<T>::_eq_func(LuaEngine & engine, const LuaNativeFunctionArgs & args) {
        auto * type_desc = args[0].userdata<LuaNativeTypeDescriptor>();

        // __eq is only called if both objects have the same __eq
        // as per https://www.lua.org/manual/5.1/manual.html#2.8
        // therefore there's no need to test whether rhs is actually T userdata
        auto & lhs = *args[1].userdata<T>();
        auto & rhs = *args[2].userdata<T>();

        if (type_desc->eq_func) {
            return engine.boolean(type_desc->eq_func(engine, lhs, rhs));
        } else {
            return engine.boolean(false);
        }
    }

    template <typename T>
    LuaResult LuaNativeTypeDescriptor<T>::_len_func(LuaEngine & engine, const LuaNativeFunctionArgs & args) {
        auto * type_desc = args[0].userdata<LuaNativeTypeDescriptor>();

        auto & obj = *args[1].userdata<T>();

        if (type_desc->length_func) {
            return engine.number(static_cast<LuaNumber>(type_desc->length_func(engine, obj)));
        } else {
            return LuaResult::error("attempt to get length of a " + type_desc->name + " value");
        }
    }

    template <typename T>
    LuaResult LuaNativeTypeDescriptor<T>::_gc_func(LuaEngine & engine, const LuaNativeFunctionArgs & args) {
        auto * dtor_func = args[0].userdata<LuaNativeTypeDescriptor::DtorFunction>();
        auto & obj = *args[1].userdata<T>();

        if (dtor_func != nullptr) {
            dtor_func(engine, obj);
        }

        return LuaResult();
    }

    template <typename T>
    LuaNativeTypeDescriptor<T>::LuaNativeTypeDescriptor(LuaEngine & engine, const std::string & name)
    : LuaUntypedNativeTypeDescriptor(
        engine,
        [&](void * target_ptr) {
            if (!this->ctor_func) throw std::runtime_error("LuaNativeTypeDescriptor has no ctor");
            this->ctor_func(engine, reinterpret_cast<T *>(target_ptr));
        },
        sizeof(T),
        name, 
        engine.closure(_ctor_func, { engine.new_light_userdata(this) })
    )
    , _dtor_func(nullptr)
    {
        metatable.set("__index", engine.closure(_index_func, { engine.new_light_userdata(this) }));
        metatable.set("__newindex", engine.closure(_newindex_func, { engine.new_light_userdata(this) }));
        metatable.set("__tostring", engine.closure(_tostring_func, { engine.new_light_userdata(this) }));
        metatable.set("__eq", engine.closure(_eq_func, { engine.new_light_userdata(this) }));
        metatable.set("__len", engine.closure(_len_func, { engine.new_light_userdata(this) }));
    }

    template <typename T>
    void LuaNativeTypeDescriptor<T>::set_dtor_func(DtorFunction func) {
        // the destructor is much, much more limited than any other native
        // type functions, due to the fact that it may be ran after
        // the native type descriptor is already freed - this is also
        // why instead of pushing this, we push a pointer to the functino

        _dtor_func = func;
        metatable.set("__gc", _engine.closure(_gc_func, { _engine.new_light_userdata(_dtor_func) }));
    }

    template <typename T>
    LuaObject LuaNativeTypeDescriptor<T>::create(const T & source) const {
        return _engine.new_userdata_with_metatable<T>([&](T * target_ptr) {
            copy_ctor_func(_engine, source, target_ptr);
        }, metatable).value();
    }

    template <typename T>
    LuaObject LuaNativeTypeDescriptor<T>::create(T && source) const {
        return _engine.new_userdata_with_metatable<T>([&](T * target_ptr) {
            if (!move_ctor_func) {
                copy_ctor_func(_engine, source, target_ptr);
            } else {
                move_ctor_func(_engine, std::move(source), target_ptr);
            }
        }, metatable).value();
    }
}

#endif//SMEN_LUA_NATIVE_TYPES_HPP
