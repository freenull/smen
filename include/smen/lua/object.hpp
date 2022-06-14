#ifndef SMEN_LUA_OBJECT_HPP
#define SMEN_LUA_OBJECT_HPP

#include <smen/lua/type.hpp>
#include <smen/lua/stack.hpp>
#include <smen/lua/registry.hpp>
#include <smen/lua/result.hpp>
#include <smen/logger.hpp>
#include <vector>

namespace smen {
    struct LuaTableIterator;

    class LuaObject {
    private:
        enum class Type {
            STACK_REFERENCE,
            REFERENCE,
            STRING,
            NUMBER,
            BOOLEAN,
            NIL
        };

        static Logger logger;

        Type _type;
        union {
            LuaStackIndex _stack_ref;
            LuaRegistryIndex _ref;
            std::string _str;
            LuaNumber _num;
            bool _bool;
        };
        LuaEnvironment * _env;

        void _clear();

    public:
        LuaObject(const LuaObject &);
        LuaObject(LuaObject &&);
        LuaObject & operator=(const LuaObject &);
        LuaObject & operator=(LuaObject &&);

        inline LuaObject(LuaEnvironment & env, LuaRegistryIndex ref)
        : _type(Type::REFERENCE)
        , _ref(ref)
        , _env(&env)
        {}
        
        inline LuaObject(LuaEnvironment & env, const std::string & str)
        : _type(Type::STRING)
        , _str(str)
        , _env(&env)
        {}

        inline LuaObject(LuaEnvironment & env, LuaNumber number)
        : _type(Type::NUMBER)
        , _num(number)
        , _env(&env)
        {}

        inline LuaObject(LuaEnvironment & env, bool boolean)
        : _type(Type::BOOLEAN)
        , _bool(boolean)
        , _env(&env)
        {}

        inline LuaObject(LuaEnvironment & env)
        : _type(Type::NIL)
        , _bool(false)
        , _env(&env)
        {}


        static LuaObject from_stack_by_type(LuaEnvironment & env, LuaStackIndex idx = -1);
        static LuaObject consume_from_stack_top_by_type(LuaEnvironment & env);
        static LuaObject consume_from_stack_top_ref(LuaEnvironment & env);

        LuaResult call(const std::vector<LuaObject> & args) const;
        inline LuaResult call() const {
            return call(std::vector<LuaObject>());
        }
        LuaObject get(const LuaObject & key);

        inline LuaObject get(LuaNumber key) {
            return get(LuaObject(*_env, key));
        }

        inline LuaObject get(const std::string & key) {
            return get(LuaObject(*_env, key));
        }

        void set(const LuaObject & key, const LuaObject & val);

        inline void set(const std::string & key, const LuaObject & val) {
            set(LuaObject(*_env, key), val);
        }

        inline void set(const std::string & key, const std::string & val) {
            set(LuaObject(*_env, key), LuaObject(*_env, val));
        }

        inline void set(const std::string & key, LuaNumber val) {
            set(LuaObject(*_env, key), LuaObject(*_env, val));
        }

        inline void set(LuaNumber key, const LuaObject & val) {
            set(LuaObject(*_env, key), val);
        }

        inline void set(LuaNumber key, const std::string & val) {
            set(LuaObject(*_env, key), LuaObject(*_env, val));
        }

        inline void set(LuaNumber key, LuaNumber val) {
            set(LuaObject(*_env, key), LuaObject(*_env, val));
        }

        inline void set(const LuaObject & key, const std::string & val) {
            set(key, LuaObject(*_env, val));
        }

        inline void set(const LuaObject & key, LuaNumber val) {
            set(key, LuaObject(*_env, val));
        }

        void push_to_stack() const;
        LuaType type() const;

        LuaNumber number() const;
        std::string string() const;
        bool boolean() const;
        void * userdata() const;

        size_t length() const;

        LuaObject metatable() const;
        void set_metatable(const LuaObject & mt);

        template <typename T>
        T * userdata() const {
            return reinterpret_cast<T *>(userdata());
        }
        
        LuaRegistryIndex reference() const;

        LuaTableIterator begin() const;
        LuaTableIterator end() const;

        void write_string(std::ostream & s) const;
        std::string to_string() const;
        void write_string_lua(std::ostream & s) const;
        std::string to_string_lua() const;

        friend bool operator ==(const LuaObject & left, const LuaObject & right);
        friend bool operator !=(const LuaObject & left, const LuaObject & right);

        ~LuaObject();
    };

    class LuaEngine;

    struct LuaTablePair {
    public:
        LuaObject key;
        LuaObject value;

        LuaTablePair(const LuaObject & key, const LuaObject & value);
    };

    struct LuaTableIterator {
    private:
        LuaEnvironment * _env;
        LuaTablePair _pair;
        const LuaObject & _table;

        void _fill_next();

    public:
        using iterator_category = std::input_iterator_tag;
        using difference_type = uint32_t;
        using value_type = LuaTablePair;
        using pointer = value_type *;
        using reference = value_type &;

        LuaTableIterator(LuaEnvironment & env, const LuaObject & table);
        LuaTableIterator(LuaEnvironment & env, const LuaObject & table, const LuaTablePair & pair);
        reference operator *();
        pointer operator ->();

        LuaTableIterator & operator ++();
        LuaTableIterator operator ++(int amount);

        friend bool operator ==(const LuaTableIterator & left, const LuaTableIterator & right);
        friend bool operator !=(const LuaTableIterator & left, const LuaTableIterator & right); 
    };

    std::ostream & operator<<(std::ostream & s, const LuaObject & obj);
}

#endif//SMEN_LUA_OBJECT_HPP
