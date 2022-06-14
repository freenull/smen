#ifndef SMEN_LUA_REGISTRY_HPP
#define SMEN_LUA_REGISTRY_HPP

#include <string>
#include <smen/lua/stack.hpp>

namespace smen {
    /* struct LuaRegistryIndex { */
    /* private: */
    /*     int32_t _value; */

    /* public: */
    /*     inline LuaRegistryIndex() { _value = 0; } */
    /*     inline LuaRegistryIndex(int32_t value) : _value(value) {} */
    /*     inline operator LuaNumber() const { return _value; } */
    /*     inline LuaNumber operator =(int32_t x) { return LuaRegistryIndex(_value + x); } */
    /*     inline LuaNumber operator +=(int32_t x) { */
    /*         _value += x; */
    /*         return *this; */
    /*     } */
    /*     inline LuaNumber operator -=(int32_t x) { */
    /*         _value -= x; */
    /*         return *this; */
    /*     } */
    /* }; */

    using LuaRegistryIndex = int32_t;

    class LuaRegistry;
    struct LuaReference {
    private:
        LuaRegistryIndex _reg_idx;
        LuaRegistry * _reg_ptr;

    public:
        LuaReference(const LuaReference & ref);
        LuaReference(LuaReference && ref);

        LuaReference(LuaRegistry * reg_ptr, LuaRegistryIndex reg_idx);
        LuaRegistryIndex registry_idx() const;
        void push_to_stack() const;
        ~LuaReference();
    };

    class LuaRegistry {
        const LuaStackIndex REGISTRY_INDEX = LUA_REGISTRYINDEX;

    private:
        static const int ALLOC_COUNT_TO_FIND_HOLES = 32;
        static const int KEY_SEED_SIZE = 16;
        static const std::string _generate_random_key();

        const std::string _key;
        const char * _key_cstr;
        lua_State * const L;
        int _alloc_counter;

        LuaRegistryIndex _next_index;

    public:
        LuaRegistry(lua_State * const lua_ptr);

        void get_root() const;
        void get_field(const std::string & key) const;
        void set_field(const std::string & key, LuaStackIndex idx = -1);

        LuaRegistryIndex alloc(LuaStackIndex idx = -1);
        LuaRegistryIndex dup(const LuaReference & ref);

        LuaReference create(LuaStackIndex idx = -1);
        LuaReference create_and_consume(LuaStackIndex idx = -1);

        void force_dealloc(LuaRegistryIndex idx);
        bool try_dealloc(LuaRegistryIndex idx);
        void get(LuaRegistryIndex idx);
    };
}

#endif//SMEN_LUA_REGISTRY_HPP
