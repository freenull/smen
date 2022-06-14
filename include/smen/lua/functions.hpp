#ifndef SMEN_LUA_FUNCTIONS
#define SMEN_LUA_FUNCTIONS

#include <smen/lua/object.hpp>
#include <smen/lua/result.hpp>
#include <optional>
#include <span>
#include <sstream>
#include <unordered_map>
#include <unordered_set>

namespace smen {
    class LuaEnvironment;
    class LuaEngine;

    using LuaNativeFunctionArgs = std::span<const LuaObject>;

    struct LuaNativeFunctionDiagnostics {
    public:
        struct Argument {
            std::string name;
            std::string type;
        };

        std::string name;
        std::string self_type;
        std::string return_type;
        std::vector<Argument> args;
        size_t upvalues;

        std::string full_name() const;
        std::string signature() const;
        bool check_self(LuaResult & result, LuaEngine & engine, const LuaNativeFunctionArgs & args) const;
        bool check(LuaResult & result, LuaEngine & engine, const LuaNativeFunctionArgs & args, size_t index, std::unordered_set<LuaType> lua_types) const;
        bool check(LuaResult & result, LuaEngine & engine, const LuaNativeFunctionArgs & args, size_t index, const std::string & native_type = "") const;
        LuaResult error_expected(size_t index, const std::string & text) const;
        const LuaObject & arg(const LuaNativeFunctionArgs & args, size_t index) const;
        const LuaObject & self(const LuaNativeFunctionArgs & args) const;
    };

    using LuaNativeFunctionCallbackMany = std::vector<LuaObject> (LuaEngine & engine, const LuaNativeFunctionArgs & args);
    using LuaNativeFunctionCallbackSingle = LuaObject (LuaEngine & engine, const LuaNativeFunctionArgs & args);
    using LuaNativeFunctionCallbackNone = void (LuaEngine & engine, const LuaNativeFunctionArgs & args);
    using LuaNativeFunctionCallbackErrorable = LuaResult (LuaEngine & engine, const LuaNativeFunctionArgs & args);

    struct LuaNativeFunction {
    public:
        enum class ReturnCountTag {
            MANY,
            SINGLE,
            NONE,
            ERRORABLE
        };

    private:
        const ReturnCountTag _tag;
        union {
            LuaNativeFunctionCallbackMany * _many_func;
            LuaNativeFunctionCallbackSingle * _single_func;
            LuaNativeFunctionCallbackNone * _none_func;
            LuaNativeFunctionCallbackErrorable * _errorable_func;
        };
    public:
        LuaNativeFunction(LuaNativeFunctionCallbackMany * many_func);
        LuaNativeFunction(LuaNativeFunctionCallbackSingle * single_func);
        LuaNativeFunction(LuaNativeFunctionCallbackNone * none_func);
        LuaNativeFunction(LuaNativeFunctionCallbackErrorable * errorable_func);
        LuaNativeFunction(ReturnCountTag tag, void * unknown_func);

        LuaResult operator()(LuaEngine & engine, const LuaNativeFunctionArgs & args) noexcept;
        inline ReturnCountTag return_count_tag() { return _tag; }
        inline void * func_ptr() {
            switch(_tag) {
            case ReturnCountTag::MANY:
                return reinterpret_cast<void *>(_many_func);
            case ReturnCountTag::SINGLE:
                return reinterpret_cast<void *>(_single_func);
            case ReturnCountTag::NONE:
                return reinterpret_cast<void *>(_none_func);
            case ReturnCountTag::ERRORABLE:
                return reinterpret_cast<void *>(_errorable_func);
            default: throw;
            }
        }
    };

    /* struct LuaLowLevelFunctionResult { */
    /* public: */
    /*     int return_count; */

    /*     LuaLowLevelFunctionResult(); */
    /*     void add_returns(int count); */
    /*     void add_return(); */
    /* }; */
    

    /* class LuaClosureContext { */
    /* public: */
    /*     static constexpr uint32_t STACK_SLOTS = (sizeof(intptr_t) == sizeof(uint64_t)) ? 1 : 2; */

    /*     const uint32_t upvalue_count; */
    /*     const uint32_t upvalue_offset; */

    /*     static LuaClosureContext get_from_upvalues(LuaEnvironment & env, uint32_t start); */

    /*     LuaClosureContext(uint32_t upvalue_count, uint32_t upvalue_offset); */

    /*     void reset(); */
    /*     void push(LuaEnvironment & env) const; */
    /*     LuaStackIndex upvalue_index(uint32_t n) const; */
    /* }; */

    /* using LuaLowLevelFunction = LuaLowLevelFunctionResult (*)(LuaEnvironment & env); */
    /* using LuaLowLevelClosure = LuaLowLevelFunctionResult (*)(LuaEnvironment & env, const LuaClosureContext & ctx); */

    /* using LuaNativeFunction = LuaNativeFunctionResult (LuaEnvironment & env); */
    /* using LuaNativeClosure = LuaNativeFunctionResult (LuaEnvironment & env, const LuaClosureContext & ctx); */

    /* class LuaNativeFunctionManager { */
    /* private: */
    /*     static int _closure_proxy(lua_State * L); */
    /*     static int _function_proxy(lua_State * L); */


    /* public: */
    /*     LuaEnvironment & env; */

    /*     LuaNativeFunctionManager(LuaEnvironment & env); */

    /*     LuaClosureContext read_context_from_upvalues(uint32_t start_idx); */
    /*     void push_function(LuaNativeFunction * func); */
    /*     void push_closure(LuaNativeClosure * func, int upvalue_count); */
    /* }; */
}

#endif//SMEN_LUA_FUNCTIONS
