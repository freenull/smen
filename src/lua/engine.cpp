#include <smen/lua/engine.hpp>
#include <smen/lua/native_types.hpp>
#include <smen/lua/script.hpp>
#include <sstream>

namespace smen {
    int LuaEngine::_closure_proxy(lua_State * L) {
        auto return_count_tag_and_upvalue_count = static_cast<uint32_t>(lua_tonumber(L, lua_upvalueindex(1)));
        auto * func_ptr = lua_touserdata(L, lua_upvalueindex(2));
        auto * engine_ptr = static_cast<LuaEngine *>(lua_touserdata(L, lua_upvalueindex(3)));
        auto & engine = *engine_ptr;

        auto return_count_tag = static_cast<LuaNativeFunction::ReturnCountTag>(return_count_tag_and_upvalue_count >> 30);
        // 00 -> MANY, 01 -> SINGLE, 10 -> NONE
        auto upvalue_count = return_count_tag_and_upvalue_count & ~(0xC0000000);

        auto func = LuaNativeFunction(return_count_tag, func_ptr);

        std::vector<LuaObject> args;
        for (uint32_t i = 4; i < upvalue_count + 4; i++) {
            auto obj = LuaObject::from_stack_by_type(engine._env, lua_upvalueindex(static_cast<int32_t>(i)));
            args.emplace_back(obj);
        }

        for (LuaStackIndex i = 1; i <= engine._env.top(); i++) {
            args.emplace_back(LuaObject::from_stack_by_type(engine._env, i));
        }

        auto result = func(engine, args);

        if (result.success()) {
            for (auto & obj : result.values()) {
                obj.push_to_stack();
            }

            return static_cast<int>(result.values().size());
        }

        engine._env.push_string(result.error_msg());
        return engine._env.raise_error_and_leave_function();
    }

    int LuaEngine::_function_proxy(lua_State * L) {
        auto return_count_tag = static_cast<LuaNativeFunction::ReturnCountTag>(lua_tonumber(L, lua_upvalueindex(1)));
        auto * func_ptr = lua_touserdata(L, lua_upvalueindex(2));
        auto * engine_ptr = static_cast<LuaEngine *>(lua_touserdata(L, lua_upvalueindex(3)));
        auto & engine = *engine_ptr;

        auto func = LuaNativeFunction(return_count_tag, func_ptr);
        std::vector<LuaObject> args;

        for (LuaStackIndex i = 1; i <= engine._env.top(); i++) {
            args.emplace_back(LuaObject::from_stack_by_type(engine._env, i));
        }

        auto result = func(engine, args);
        if (result.success()) {
            for (auto & obj : result.values()) {
                obj.push_to_stack();
            }

            return static_cast<int>(result.values().size());
        }

        engine._env.push_string(result.error_msg());
        return engine._env.raise_error_and_leave_function();
    }

    LuaEngine::LuaEngine()
    : _env()
    , _nil(LuaObject(_env))
    , _true(LuaObject(_env, true))
    , _false(LuaObject(_env, false))
    , _tostring_func(globals().get("tostring"))
    , _ffi_typeof_func(globals().get("require").call({ string("ffi") }).value().get("typeof"))
    , _int64_ctype(_ffi_typeof_func.call({ int64_cdata(0) }).value())
    , _uint64_ctype(_ffi_typeof_func.call({ uint64_cdata(0) }).value())
    {}

    LuaObject LuaEngine::globals() const {
        _env.dup(LUA_GLOBALS_INDEX);
        return LuaObject::consume_from_stack_top_ref(_env);
    }

    LuaResult LuaEngine::load_string(const std::string & s) const {
        auto status = _env.load_string(s);
        if (status == LuaStatus::OK) {
            return LuaResult(LuaObject::consume_from_stack_top_ref(_env));
        } else {
            auto err_msg = _env.read_string();
            _env.pop(1);
            return LuaResult(status, err_msg);
        }
    }

    LuaResult LuaEngine::load_file(const std::string & path) const {
        auto status = _env.load_file(path);
        if (status == LuaStatus::OK) {
            return LuaResult(LuaObject::consume_from_stack_top_ref(_env));
        } else {
            auto err_msg = _env.read_string();
            _env.pop(1);
            return LuaResult(status, err_msg);
        }
    }

    LuaScript LuaEngine::load_script(const std::string & path) const {
        auto script = LuaScript(*this);
        auto result = script.load(path);
        if (result.fail()) {
            throw LuaEngineException("failed loading script at '" + path + "': " + result.error_msg());
        }

        return script;
    }

    LuaObject LuaEngine::nil() const {
        return _nil;
    }

    LuaObject LuaEngine::new_table() const {
        _env.push_new_table();
        return LuaObject::consume_from_stack_top_ref(_env);
    }

    LuaObject LuaEngine::new_light_userdata(void * ptr) const {
        _env.push_light_userdata(ptr);
        return LuaObject::consume_from_stack_top_ref(_env);
    }

    LuaObject LuaEngine::new_userdata(size_t size, LuaUserdataConstructor<void> ctor) {
        void * ptr = _env.push_new_userdata(size);
        ctor(ptr);
        return LuaObject::consume_from_stack_top_ref(_env);
    }

    LuaObject LuaEngine::new_userdata_with_metatable(size_t size, LuaUserdataConstructor<void> ctor, LuaRegistryIndex mt_reg_index) const {
        void * ptr = _env.push_new_userdata(size);
        ctor(ptr);
        _env.registry.get(mt_reg_index);
        _env.set_metatable(-2);
        return LuaObject::consume_from_stack_top_ref(_env);
    }

    LuaResult LuaEngine::new_userdata_with_metatable(size_t size, LuaUserdataConstructor<void> ctor, const LuaObject & mt_object) const {
        if (mt_object.reference() == 0) return LuaResult(LuaStatus::RUNTIME_ERROR, "attempted to create userdata with object that is not a metatable");
        return LuaResult(new_userdata_with_metatable(size, ctor, mt_object.reference()));
    }

    LuaObject LuaEngine::string(const std::string & val) const {
        return LuaObject(_env, val);
    }

    LuaObject LuaEngine::number(LuaNumber val) const {
        return LuaObject(_env, val);
    }

    LuaObject LuaEngine::boolean(bool val) const {
        return LuaObject(_env, val);
    }

    LuaObject LuaEngine::function(LuaNativeFunction func) const {
        _env.push_number(static_cast<int>(func.return_count_tag()));
        _env.push_light_userdata(func.func_ptr());
        _env.push_light_userdata(this);
        _env.push_raw_closure(_function_proxy, 3);
        return LuaObject::consume_from_stack_top_ref(_env);
    }

    LuaObject LuaEngine::closure(LuaNativeFunction func, const std::vector<LuaObject> & upvalues) const {
        uint32_t return_count_tag_and_upvalue_count;
        return_count_tag_and_upvalue_count = static_cast<uint32_t>(func.return_count_tag());
        return_count_tag_and_upvalue_count <<= 30;
        return_count_tag_and_upvalue_count |= static_cast<uint32_t>(upvalues.size());

        _env.push_number(return_count_tag_and_upvalue_count);
        _env.push_light_userdata(func.func_ptr());
        _env.push_light_userdata(this);
        for (auto & upvalue : upvalues) {
            upvalue.push_to_stack();
        }
        _env.push_raw_closure(_closure_proxy, 3 + upvalues.size());
        return LuaObject::consume_from_stack_top_ref(_env);
    }

    LuaObject LuaEngine::int64_cdata(int64_t val) const {
        std::ostringstream s;
        s << "return ";
        s << val;
        s << "LL";
        return load_string(s.str()).value().call().value();
    }

    LuaObject LuaEngine::uint64_cdata(uint64_t val) const {
        std::ostringstream s;
        s << "return ";
        s << val;
        s << "ULL";
        return load_string(s.str()).value().call().value();
    }

    std::optional<int64_t> LuaEngine::int64_cdata_value(const LuaObject & obj) const {
        if (!is_int64_cdata(obj)) return std::nullopt;
        auto tostring_result = _tostring_func.call({ obj });
        if (tostring_result.fail() || tostring_result.values().size() == 0) return std::nullopt;
        auto tostring_obj = tostring_result.value();
        if (tostring_obj.type() != LuaType::STRING) return std::nullopt;
        auto str = tostring_obj.string();

        return std::stoll(str.substr(0, str.size() - 2)); // remove the LL
    }

    std::optional<uint64_t> LuaEngine::uint64_cdata_value(const LuaObject & obj) const {
        if (!is_uint64_cdata(obj)) return std::nullopt;
        auto tostring_result = _tostring_func.call({ obj });
        if (tostring_result.fail() || tostring_result.values().size() == 0) return std::nullopt;
        auto tostring_obj = tostring_result.value();
        if (tostring_obj.type() != LuaType::STRING) return std::nullopt;
        auto str = tostring_obj.string();
        return std::stoll(str.substr(0, str.size() - 3)); // remove the ULL
    }

    bool LuaEngine::is_int64_cdata(const LuaObject & obj) const {
        auto typeof_result = _ffi_typeof_func.call({ obj });
        if (typeof_result.fail() || typeof_result.values().size() == 0) return false;

        // for some reason == doesn't always work on these ctypes

        auto tostring_result = _tostring_func.call({ typeof_result.value() });
        if (tostring_result.fail() || tostring_result.values().size() == 0) return false;

        if (tostring_result.value().string() != "ctype<int64_t>") return false;

        return true;
    }

    bool LuaEngine::is_uint64_cdata(const LuaObject & obj) const {
        auto typeof_result = _ffi_typeof_func.call({ obj });
        if (typeof_result.fail() || typeof_result.values().size() == 0) return false;

        auto tostring_result = _tostring_func.call({ typeof_result.value() });
        if (tostring_result.fail() || tostring_result.values().size() == 0) return false;

        if (tostring_result.value().string() != "ctype<uint64_t>") return false;

        return true;
    }

    LuaObject LuaEngine::native_type_constructor(const std::string & name) const {
        return native_type(name)->metatype;
    }

    const std::unique_ptr<LuaUntypedNativeTypeDescriptor> & LuaEngine::native_type(const std::string & name) const {
        auto it = _native_types.find(name);
        if (it == _native_types.end()) {
            throw LuaEngineException("native type " + name + " does not exist");
        }
        return it->second;
    }

    bool LuaEngine::is_native_type(const LuaObject & obj, const std::string & name) const {
        return obj.metatable() == native_type(name)->metatable;
    }

    LuaObject LuaEngine::native_type_instance(const std::string & name) const {
        return native_type(name)->create();
    }
}
