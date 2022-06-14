#include <smen/lua/functions.hpp>
#include <smen/lua/env.hpp>
#include <smen/lua/engine.hpp>
#include <sstream>

namespace smen {

    std::string LuaNativeFunctionDiagnostics::full_name() const {
        auto s = std::ostringstream();

        if (self_type.size() != 0) {
            s << self_type;
            s << ":";
        }

        s << name;

        return s.str();
    }

    std::string LuaNativeFunctionDiagnostics::signature() const {
        auto s = std::ostringstream();

        if (self_type.size() != 0) {
            s << self_type;
            s << ":";
        }

        s << name;
        s << "(";

        auto first = true;
        for (auto & arg : args) {
            if (!first) s << ", ";
            first = false;

            s << arg.name;
            if (arg.type.size() > 0) {
                s << " : " << arg.type;
            }
        }

        s << ")";
        return s.str();
    }

    bool LuaNativeFunctionDiagnostics::check_self(LuaResult & result, LuaEngine & engine, const LuaNativeFunctionArgs & args) const {
        auto self_index = upvalues;

        if (self_index >= args.size()) {
            result = LuaResult::error("missing self argument for function " + full_name());
            return false;
        }

        if (!engine.is_native_type(args[self_index], self_type)) {
            result = LuaResult::error("self argument for function " + full_name() + " has invalid type");
            return false;
        }

        return true;
    }

    bool LuaNativeFunctionDiagnostics::check(LuaResult & result, LuaEngine & engine, const LuaNativeFunctionArgs & args, size_t index, std::unordered_set<LuaType> lua_types) const {
        if (index == 0) throw std::runtime_error("invalid index 0");

        auto real_index = index + upvalues;
        if (self_type.size() == 0) {
            real_index -= 1;
        }

        if (real_index >= args.size()) {
            auto s = std::ostringstream();
            s << "missing argument #" << index << " for function " << full_name();
            s << " - expected ";
            auto first = true;
            for (auto type : lua_types) {
                if (!first) s << "|";
                first = false;

                s << type;
            }
            result = LuaResult::error(s.str());

            return false;
        }

        if (lua_types.size() > 0) {
            if (!lua_types.contains(args[real_index].type())) {
                auto s = std::ostringstream();
                s << "argument #" << index << " for function " << full_name();
                s << " has invalid type - expected ";
                auto first = true;
                for (auto type : lua_types) {
                    if (!first) s << "|";
                    first = false;

                    s << type;
                }

                s << ", got " << args[real_index].to_string_lua();
                result = LuaResult::error(s.str());
                return false;
            }
        }

        return true;
    }

    bool LuaNativeFunctionDiagnostics::check(LuaResult & result, LuaEngine & engine, const LuaNativeFunctionArgs & args, size_t index, const std::string & native_type) const {
        if (index == 0) throw std::runtime_error("invalid index 0");

        auto real_index = index + upvalues;
        if (self_type.size() == 0) {
            real_index -= 1;
        }

        if (real_index >= args.size()) {
            auto s = std::ostringstream();
            s << "missing argument #" << index << " for function " << full_name();
            s << " - expected an instance of " << native_type;

            result = LuaResult::error(s.str());
            return false;
        }

        if (native_type.size() > 0) {
            if (!engine.is_native_type(args[real_index], native_type)) {
                auto s = std::ostringstream();
                s << "argument #" << index << " for function " << full_name();
                s << " has invalid type - expected an instance of " << native_type;

                s << ", got " << args[real_index].to_string_lua();
                result = LuaResult::error(s.str());
                return false;
            }
        }

        return true;
    }

    LuaResult LuaNativeFunctionDiagnostics::error_expected(size_t index, const std::string & text) const {
        return LuaResult::error("argument #" + std::to_string(index) + " for function " + full_name() + " " + text);
    }
    
    const LuaObject & LuaNativeFunctionDiagnostics::arg(const LuaNativeFunctionArgs & args, size_t index) const {
        auto real_index = index + upvalues;
        if (self_type.size() == 0) {
            real_index -= 1;
        }

        return args[real_index];
    }

    const LuaObject & LuaNativeFunctionDiagnostics::self(const LuaNativeFunctionArgs & args) const {
        if (self_type.size() == 0) throw std::runtime_error("no self_type defined in function diagnostics");
        auto self_index = upvalues;
        return args[self_index];
    }

    LuaNativeFunction::LuaNativeFunction(LuaNativeFunctionCallbackMany * many_func)
    : _tag(ReturnCountTag::MANY)
    , _many_func(many_func)
    {}

    LuaNativeFunction::LuaNativeFunction(LuaNativeFunctionCallbackSingle * single_func)
    : _tag(ReturnCountTag::SINGLE)
    , _single_func(single_func)
    {}

    LuaNativeFunction::LuaNativeFunction(LuaNativeFunctionCallbackNone * none_func)
    : _tag(ReturnCountTag::NONE)
    , _none_func(none_func)
    {}

    LuaNativeFunction::LuaNativeFunction(LuaNativeFunctionCallbackErrorable * errorable_func)
    : _tag(ReturnCountTag::ERRORABLE)
    , _errorable_func(errorable_func)
    {}

    LuaNativeFunction::LuaNativeFunction(ReturnCountTag tag, void * unknown_func)
    : _tag(tag) {
        switch(_tag) {
        case ReturnCountTag::MANY:
            _many_func = reinterpret_cast<LuaNativeFunctionCallbackMany *>(unknown_func);
            break;
        case ReturnCountTag::SINGLE:
            _single_func = reinterpret_cast<LuaNativeFunctionCallbackSingle *>(unknown_func);
            break;
        case ReturnCountTag::NONE:
            _none_func = reinterpret_cast<LuaNativeFunctionCallbackNone *>(unknown_func);
            break;
        case ReturnCountTag::ERRORABLE:
            _errorable_func = reinterpret_cast<LuaNativeFunctionCallbackErrorable *>(unknown_func);
            break;
        default: throw;
        }
    }

    LuaResult LuaNativeFunction::operator()(LuaEngine & engine, const LuaNativeFunctionArgs & args) noexcept {
        LuaResult result;
        try {
            switch(_tag) {
            case ReturnCountTag::MANY:
                result = LuaResult(_many_func(engine, args));
                break;
            case ReturnCountTag::SINGLE:
                result = LuaResult({ _single_func(engine, args) });
                break;
            case ReturnCountTag::NONE:
                _none_func(engine, args);
                result = LuaResult();
                break;
            case ReturnCountTag::ERRORABLE:
                result = _errorable_func(engine, args);
                break;
            default:
                result = LuaResult::error("invalid return count tag");
                break;
            }
        } catch (const std::exception & e) {
            result = LuaResult::error(e.what());
        } catch (const std::string & e) {
            result = LuaResult::error(e);
        } catch (const char * e) {
            result = LuaResult::error(e);
        } catch (...) {
            result = LuaResult::error("unknown error");
        }   
        return result;
    }

    /* LuaNativeFunctionResult::LuaNativeFunctionResult() {} */

    /* void LuaNativeFunctionResult::add_return(const LuaObject & object) { */
    /*     returns.emplace_back(object); */
    /* } */

    /* LuaLowLevelFunctionResult::LuaLowLevelFunctionResult() {} */

    /* void LuaLowLevelFunctionResult::add_returns(int count) { */
    /*     return_count += count; */
    /* } */

    /* void LuaLowLevelFunctionResult::add_return() { */
    /*     return_count += 1; */
    /* } */

    /* LuaClosureContext LuaClosureContext::get_from_upvalues(LuaEnvironment & env, uint32_t start_idx) { */
    /*     if constexpr (sizeof(intptr_t) == sizeof(uint64_t)) { */
    /*         uint64_t packed_ctx = reinterpret_cast<uint64_t>(env.read_userdata(lua_upvalueindex(start_idx))); */
    /*         uint32_t count = static_cast<uint32_t>(packed_ctx); */
    /*         uint32_t offs = static_cast<uint32_t>(packed_ctx >> 32); */
    /*         return LuaClosureContext(count, offs + 1); */
            
    /*     } else { */
    /*         uint64_t count = reinterpret_cast<uint64_t>(env.read_userdata(lua_upvalueindex(start_idx))); */
    /*         uint64_t offs = reinterpret_cast<uint64_t>(env.read_userdata(lua_upvalueindex(start_idx + 1))); */
    /*         return LuaClosureContext(count, offs + 2); */
    /*     } */
    /* } */

    /* LuaClosureContext::LuaClosureContext(uint32_t upvalue_count, uint32_t upvalue_offset) */
    /* : upvalue_count(upvalue_count) */
    /* , upvalue_offset(upvalue_offset) */
    /* {} */

    /* void LuaClosureContext::push(LuaEnvironment & env) const { */
    /*     if constexpr (sizeof(intptr_t) == sizeof(uint64_t)) { */
    /*         lua_pushlightuserdata(env.luajit_ptr(), reinterpret_cast<void *>((static_cast<uint64_t>(upvalue_offset) << 32) | upvalue_count)); */
            
    /*     } else { */
    /*         lua_pushlightuserdata(env.luajit_ptr(), reinterpret_cast<void *>(upvalue_count)); */
    /*         lua_pushlightuserdata(env.luajit_ptr(), reinterpret_cast<void *>(upvalue_offset)); */
    /*     } */
    /* } */

    /* LuaStackIndex LuaClosureContext::upvalue_index(uint32_t n) const { */
    /*     return static_cast<LuaStackIndex>(lua_upvalueindex(upvalue_offset + n)); */
    /* } */

    /* int LuaNativeFunctionManager::_closure_proxy(lua_State * L) { */
    /*     auto * actual_func = reinterpret_cast<LuaNativeClosure *>(lua_touserdata(L, lua_upvalueindex(1))); */
    /*     auto * func_manager = static_cast<LuaNativeFunctionManager *>(lua_touserdata(L, lua_upvalueindex(2))); */

    /*     LuaClosureContext ctx = func_manager->read_context_from_upvalues(3); */

    /*     try { */
    /*         auto result = actual_func(func_manager->env, ctx); */

    /*         for (auto & obj : result.returns) { */
    /*             obj.push_to_stack(); */
    /*         } */

    /*         return static_cast<int>(result.returns.size()); */
    /*     } catch (const std::exception & e) { */
    /*         func_manager->env.push_string(e.what()); */
    /*     } catch (const std::string & e) { */
    /*         func_manager->env.push_string(e); */
    /*     } catch (const char * e) { */
    /*         func_manager->env.push_string(e); */
    /*     } catch (...) { */
    /*         func_manager->env.push_string("Unknown error"); */
    /*     } */   

    /*     return lua_error(func_manager->env.luajit_ptr()); */
    /* } */

    /* int LuaNativeFunctionManager::_function_proxy(lua_State * L) { */
    /*     LuaNativeFunction * actual_func = reinterpret_cast<LuaNativeFunction *>(lua_touserdata(L, lua_upvalueindex(1))); */
    /*     LuaNativeFunctionManager * func_manager = static_cast<LuaNativeFunctionManager *>(lua_touserdata(L, lua_upvalueindex(2))); */


    /*     try { */
    /*         auto result = actual_func(func_manager->env); */

    /*         for (auto & obj : result.returns) { */
    /*             obj.push_to_stack(); */
    /*         } */

    /*         return static_cast<int>(result.returns.size()); */
    /*     } catch (const std::exception & e) { */
    /*         func_manager->env.push_string(e.what()); */
    /*     } catch (const std::string & e) { */
    /*         func_manager->env.push_string(e); */
    /*     } catch (const char * e) { */
    /*         func_manager->env.push_string(e); */
    /*     } catch (...) { */
    /*         func_manager->env.push_string("Unknown error"); */
    /*     } */   

    /*     return func_manager->env.raise_error_and_leave_function(); */
    /* } */

    /* LuaNativeFunctionManager::LuaNativeFunctionManager(LuaEnvironment & env) */
    /* : env(env) */
    /* {} */

    /* void LuaNativeFunctionManager::push_function(LuaNativeFunction * func) { */
    /*     env.push_light_userdata(func); */
    /*     env.push_light_userdata(this); */
    /*     env.push_raw_closure(_function_proxy, 2); */
    /* } */

    /* void LuaNativeFunctionManager::push_closure(LuaNativeClosure * func, int upvalue_count) { */
    /*     env.push_light_userdata(func); */
    /*     env.push_light_userdata(this); */

    /*     auto ctx = LuaClosureContext(upvalue_count, 2); */
    /*     ctx.push(env); */

    /*     env.move(-upvalue_count - 3); */
    /*     env.move(-upvalue_count - 3); */
    /*     for (uint32_t i = 0; i < LuaClosureContext::STACK_SLOTS; i++) { */
    /*         env.move(-upvalue_count - 3); */
    /*     } */

    /*     env.push_raw_closure(_closure_proxy, upvalue_count + LuaClosureContext::STACK_SLOTS + 2); */
    /* } */

    /* LuaClosureContext LuaNativeFunctionManager::read_context_from_upvalues(uint32_t start_idx) { */
    /*     if constexpr (sizeof(intptr_t) == sizeof(uint64_t)) { */
    /*         uint64_t packed_ctx = reinterpret_cast<uint64_t>(env.read_userdata(lua_upvalueindex(start_idx))); */
    /*         uint32_t count = static_cast<uint32_t>(packed_ctx); */
    /*         uint32_t offs = static_cast<uint32_t>(packed_ctx >> 32); */
    /*         return LuaClosureContext(count, offs + 1); */
            
    /*     } else { */
    /*         uint64_t count = reinterpret_cast<uint64_t>(env.read_userdata(lua_upvalueindex(start_idx))); */
    /*         uint64_t offs = reinterpret_cast<uint64_t>(env.read_userdata(lua_upvalueindex(start_idx + 1))); */
    /*         return LuaClosureContext(count, offs + 2); */
    /*     } */
    /* } */
}
