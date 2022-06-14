#include <smen/lua/registry.hpp>
#include <smen/lua/type.hpp>
#include <sstream>
#include <random>
#include <chrono>
#include <iostream>

namespace smen {
    const std::string LuaRegistry::_generate_random_key() {
        static const std::string charset =
            "0123456789"
            "abcdefghijklmnopqrstuvwxyz"
            "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
        static const size_t charset_length = charset.size();

        std::stringstream s;
        s << "SMEN_";

        auto rand = std::mt19937(
            static_cast<unsigned long>(
                std::chrono::system_clock::now().time_since_epoch().count()
            )
        );

        for (int i = 0; i < KEY_SEED_SIZE; i++) {
            s << charset[rand() % charset_length];
        }

        return s.str();
    }

    LuaRegistry::LuaRegistry(lua_State * const lua_ptr)
    : _key(_generate_random_key())
    , _key_cstr(_key.c_str())
    , L(lua_ptr)
    , _alloc_counter(0)
    , _next_index(1)
    {
        lua_newtable(L);
        lua_setfield(L, REGISTRY_INDEX, _key_cstr);
    }

    void LuaRegistry::get_root() const {
        lua_getfield(L, REGISTRY_INDEX, _key_cstr);
    }

    void LuaRegistry::get_field(const std::string & key) const {
        get_root();
        lua_getfield(L, -1, key.c_str());
        lua_pop(L, 1);
    }

    void LuaRegistry::set_field(const std::string & key, LuaStackIndex idx) {
        get_root();
        lua_pushvalue(L, idx - 1);
        lua_setfield(L, -2, key.c_str());
        lua_pop(L, 2);
    }

    LuaRegistryIndex LuaRegistry::alloc(LuaStackIndex stack_idx) {
        _alloc_counter += 1;

        // try to skip over existing entries
        // (the registry may have holes)
        get_root();
        while (true) { 
            lua_pushnumber(L, _next_index);
            lua_gettable(L, -2);
            _next_index += 1;
            auto is_nil = lua_isnil(L, -1);
            lua_pop(L, 1);
            if (is_nil) break;
        } 

        LuaRegistryIndex reg_idx = _next_index - 1;
        if (_alloc_counter >= ALLOC_COUNT_TO_FIND_HOLES) {
            _alloc_counter = 0;
            _next_index = 1;
        }

        // table is at the top right now
        lua_pushnumber(L, reg_idx);
        // key is pushed
        lua_pushvalue(L, stack_idx - 2);
        // value is copied (there are two values above it in the stack)
        lua_settable(L, -3);

        lua_pop(L, 1);
        // finally, pop root table
        return reg_idx;
    }

    LuaRegistryIndex LuaRegistry::dup(const LuaReference & ref) {
        ref.push_to_stack();
        return alloc(-1);
    }

    LuaReference LuaRegistry::create(LuaStackIndex idx) {
        return LuaReference(this, alloc(idx));
    }

    LuaReference LuaRegistry::create_and_consume(LuaStackIndex idx) {
        auto ref = LuaReference(this, alloc(idx));
        lua_pop(L, -1);
        return ref;
    }

    void LuaRegistry::force_dealloc(LuaRegistryIndex idx) {
        get_root();
        lua_pushnumber(L, idx);
        lua_pushnil(L);
        lua_settable(L, -3);
        lua_pop(L, 1);
    }

    bool LuaRegistry::try_dealloc(LuaRegistryIndex idx) {
        get_root();
        lua_pushnumber(L, idx);
        lua_gettable(L, -2);
        if (lua_isnil(L, -1)) {
            lua_pop(L, 1);
            return false;
        }

        lua_pushnumber(L, idx);
        lua_pushnil(L);
        lua_settable(L, -3);
        lua_pop(L, 1);
        return true;
    }

    void LuaRegistry::get(LuaRegistryIndex idx) {
        get_root();
        lua_pushnumber(L, idx);
        lua_gettable(L, -2);
        lua_insert(L, -2);
        lua_pop(L, 1);
    }

    LuaReference::LuaReference(const LuaReference & ref)
    : _reg_idx(ref._reg_ptr == nullptr ? LuaRegistryIndex(0) : ref._reg_ptr->dup(ref))
    , _reg_ptr(ref._reg_ptr)
    {}

    LuaReference::LuaReference(LuaReference && ref)
    : _reg_idx(ref._reg_idx)
    , _reg_ptr(ref._reg_ptr)
    {
        ref._reg_ptr = nullptr;
        ref._reg_idx = 0;
    }

    LuaReference::LuaReference(LuaRegistry * reg_ptr, LuaRegistryIndex reg_idx)
    : _reg_idx(reg_idx)
    , _reg_ptr(reg_ptr)
    {}

    LuaRegistryIndex LuaReference::registry_idx() const {
        return _reg_idx;
    }

    void LuaReference::push_to_stack() const {
        _reg_ptr->get(_reg_idx);
    }

    LuaReference::~LuaReference() {
        if (_reg_ptr == nullptr) return;
        _reg_ptr->force_dealloc(_reg_idx);
    }
}
