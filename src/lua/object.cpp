#include <smen/lua/object.hpp>
#include <smen/lua/result.hpp>
#include <smen/lua/env.hpp>
#include <cassert>
#include <sstream>

namespace smen {
    Logger LuaObject::logger = make_logger("LuaObject");

    void LuaObject::_clear() {
        if (_type == Type::REFERENCE) {
            if (_ref != 0) {
                _env->registry.force_dealloc(_ref);
            }
        } else if (_type == Type::STRING) {
            _str.std::string::~string();
        }
        _type = Type::NIL;
    }

    LuaObject::LuaObject(const LuaObject & obj)
    : _type(obj._type)
    , _env(obj._env)
    {
        switch(_type) {
        case Type::STACK_REFERENCE:
            // copying stack references turns them into regular references
            // the only way to preserve the stack ref is to move it
            logger.debug("stack reference copied to reference");
            _type = Type::REFERENCE;
            _env->dup(obj._stack_ref);
            _ref = _env->registry.alloc(-1);
            _env->pop(1);
            break;
        case Type::REFERENCE:
            _env->registry.get(obj._ref);
            _ref = _env->registry.alloc(-1);
            _env->pop(1);
            break;
        case Type::STRING:
            new(&_str) std::string(obj._str);
            break;
        case Type::NUMBER:
            _num = obj._num;
            break;
        case Type::BOOLEAN:
            _bool = obj._bool;
            break;
        case Type::NIL:
            _bool = false;
            break;
        }
    }

    LuaObject::LuaObject(LuaObject && obj)
    : _type(obj._type)
    , _env(obj._env)
    {
        obj._type = Type::NIL;

        switch(_type) {
        case Type::STACK_REFERENCE:
            _stack_ref = obj._stack_ref;
            // stack refs don't get collected
            break;
        case Type::REFERENCE:
            _ref = obj._ref;
            obj._ref = 0;
            break;
        case Type::STRING:
            new (&_str) std::string(obj._str);
            break;
        case Type::NUMBER:
            _num = obj._num;
            break;
        case Type::BOOLEAN:
            _bool = obj._bool;
            break;
        case Type::NIL:
            _bool = false;
            break;
        }
    }

    LuaObject & LuaObject::operator=(const LuaObject & obj) {
        _clear();

        _type = obj._type;
        _env = obj._env;

        switch(_type) {
        case Type::STACK_REFERENCE:
            // see copy ctor
            _type = Type::REFERENCE;
            _env->dup(obj._stack_ref);
            _ref = _env->registry.alloc(-1);
            _env->pop(1);
            break;
        case Type::REFERENCE:
            _env->registry.get(obj._ref);
            _ref = _env->registry.alloc(-1);
            _env->pop(1);
            break;
        case Type::STRING:
            new(&_str) std::string(obj._str);
            break;
        case Type::NUMBER:
            _num = obj._num;
            break;
        case Type::BOOLEAN:
            _bool = obj._bool;
            break;
        case Type::NIL:
            _bool = false;
            break;
        }

        return *this;
    }

    LuaObject & LuaObject::operator=(LuaObject && obj) {
        _clear();

        _type = obj._type;
        _env = obj._env;

        obj._type = Type::NIL;

        switch(_type) {
        case Type::STACK_REFERENCE:
            _stack_ref = obj._stack_ref;
            break;
        case Type::REFERENCE:
            _ref = obj._ref;
            obj._ref = 0;
            break;
        case Type::STRING:
            new (&_str) std::string(obj._str);
            break;
        case Type::NUMBER:
            _num = obj._num;
            break;
        case Type::BOOLEAN:
            _bool = obj._bool;
            break;
        case Type::NIL:
            _bool = false;
            break;
        }

        return *this;
    }

    LuaObject LuaObject::from_stack_by_type(LuaEnvironment & env, LuaStackIndex idx) {
        LuaType type = env.get_type(idx);
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wswitch-enum"
        switch(type) {
        //case LuaType::STRING: return LuaObject(env, env.read_string(idx));
        // string is stored as a reference for performance
        case LuaType::NUMBER: return LuaObject(env, env.read_number(idx));
        case LuaType::BOOLEAN: return LuaObject(env, env.read_boolean(idx));
        case LuaType::NIL: return LuaObject(env);
        default: {
            env.dup(idx);
            auto ref_idx = env.registry.alloc();
            env.pop(1);
            auto obj = LuaObject(env, LuaRegistryIndex(ref_idx));
            return obj;
        }
        }
#pragma GCC diagnostic pop
    }

    LuaObject LuaObject::consume_from_stack_top_by_type(LuaEnvironment & env) {
        auto o = LuaObject::from_stack_by_type(env, -1);
        env.pop(1);
        return o;
    }

    LuaObject LuaObject::consume_from_stack_top_ref(LuaEnvironment & env) {
        auto ref_idx = env.registry.alloc();
        env.pop(1);
        return LuaObject(env, LuaRegistryIndex(ref_idx));
    }

    LuaResult LuaObject::call(const std::vector<LuaObject> & args) const {
        push_to_stack();
        auto before_top = _env->top();
        for (size_t i = 0; i < args.size(); i++) {
            args[i].push_to_stack();
        }
        /* auto result = _env->call(static_cast<int>(args.size()), LuaEnvironment::UNLIMITED_RETURN_VALUES, 0); */
        auto result = _env->traceback_call(static_cast<int>(args.size()));
        auto after_top = _env->top();

        auto return_count = after_top - before_top + 1;
        if (result == LuaStatus::OK) {
            std::vector<LuaObject> return_values;
            for (int i = 0; i < return_count; i++) {
                return_values.emplace_back(LuaObject::consume_from_stack_top_by_type(*_env));
            }
            return LuaResult(std::move(return_values));
        } else {
            auto msg = _env->read_string();
            _env->pop(1);
            return LuaResult(result, msg);
        }
    }

    LuaObject LuaObject::get(const LuaObject & key) {
        push_to_stack();
        key.push_to_stack();
        _env->get_field(-2);
        auto value = LuaObject::consume_from_stack_top_by_type(*_env);
        _env->pop(1);
        return value;
    }

    void LuaObject::set(const LuaObject & key, const LuaObject & val) {
        push_to_stack();
        key.push_to_stack();
        val.push_to_stack();
        _env->set_field(-3);
        _env->pop(1);
    }

    inline void LuaObject::push_to_stack() const {
        switch(_type) {
        case Type::STRING:
            _env->push_string(_str);
            break;
        case Type::NUMBER:
            _env->push_number(_num);
            break;
        case Type::BOOLEAN:
            _env->push_boolean(_bool);
            break;
        case Type::NIL:
            _env->push_nil();
            break;
        case Type::REFERENCE:
            _env->registry.get(_ref);
            break;
        case Type::STACK_REFERENCE:
            _env->dup(_stack_ref);
            break;
        }
    }

    LuaType LuaObject::type() const {
        switch (_type) {
        case Type::STRING: return LuaType::STRING;
        case Type::NUMBER: return LuaType::NUMBER;
        case Type::BOOLEAN: return LuaType::BOOLEAN;
        case Type::NIL: return LuaType::NIL;
        case Type::REFERENCE: {
            push_to_stack();
            LuaType type = _env->get_type(-1);
            _env->pop(1);
            return type;
        }
        default:
            return LuaType::NONE;
        }
    }

    LuaNumber LuaObject::number() const {
        if (_type == Type::NUMBER) return _num;

        push_to_stack();
        auto num = _env->read_number();
        _env->pop(1);
        return num;
    }
    
    std::string LuaObject::string() const {
        if (_type == Type::STRING) return _str;

        push_to_stack();
        auto str = _env->read_string();
        _env->pop(1);
        return str;
    }

    bool LuaObject::boolean() const {
        switch(_type) {
        case Type::STRING:
        case Type::NUMBER:
            return true;
        case Type::BOOLEAN:
            return _bool;
        case Type::NIL:
            return false;
        case Type::REFERENCE: {
            push_to_stack();
            auto str = _env->read_boolean();
            _env->pop(1);
            return str;
        }
        default:
            return false;
        }
    }

    void * LuaObject::userdata() const {
        if (_type == Type::REFERENCE) {
            push_to_stack();
            auto ud = _env->read_userdata();
            _env->pop(1);
            return ud;
        }
        return nullptr;
    }

    size_t LuaObject::length() const {
        push_to_stack();
        auto len = _env->length(-1);
        _env->pop(1);
        return len;
    }

    LuaObject LuaObject::metatable() const {
        push_to_stack();
        auto mt = LuaObject(*_env); // nil
        if (_env->get_metatable(-1)) {
            mt = LuaObject::consume_from_stack_top_by_type(*_env);
        }
        _env->pop(1);
        return mt;
    }

    void LuaObject::set_metatable(const LuaObject & mt) {
        push_to_stack();
        mt.push_to_stack();
        _env->set_metatable(-2);
    }

    LuaRegistryIndex LuaObject::reference() const {
        if (_type == Type::REFERENCE) return _ref;
        return 0;
    }

    LuaTableIterator LuaObject::begin() const {
        return LuaTableIterator(*_env, *this);
    }

    LuaTableIterator LuaObject::end() const {
        return LuaTableIterator(*_env, *this, {LuaObject(*_env), LuaObject(*_env)});
    }

    void LuaObject::write_string(std::ostream & s) const {
        s << "LuaObject(";
        switch(type()) {
        case LuaType::NONE:
            s << "none";
            break;
        case LuaType::NIL:
            s << "nil";
            break;
        case LuaType::BOOLEAN: 
            s << "boolean; ";
            s << (boolean() ? "true" : "false");
            break;
        case LuaType::NUMBER:
            s << "number; " << number();
            break;
        case LuaType::STRING:
            s << "string; \"";
            s << string();
            s << "\"";
            break;
        case LuaType::FUNCTION:
            s << "function";
            break;
        case LuaType::USERDATA:
            s << "userdata";
            break;
        case LuaType::LIGHT_USERDATA:
            s << "light_userdata";
            break;
        case LuaType::THREAD:
            s << "coroutine";
            break;
        case LuaType::TABLE:
            s << "table";
            break;
        case LuaType::CDATA:
            s << "cdata";
            break;
        default:
            s << "invalid";
            break;
        }

        if (reference() != 0) {
            s << "; ref " << reference();
        }

        s << ")";
    }

    std::string LuaObject::to_string() const {
        std::ostringstream s;
        write_string(s);
        return s.str();
    }

    void LuaObject::write_string_lua(std::ostream & s) const {
        switch(type()) {
        case LuaType::STRING: {
            s << "\"";
            push_to_stack();
            auto str = string();
            for (size_t i = 0; i < str.size(); i++) {
                auto c = str[i];
                if (c == '"') {
                    s << "\\\"";
                } else if (c == '\\') {
                    s << "\\\\";
                } else {
                    s << c;
                }
            }
            s << "\"";
            break;
        }
        case LuaType::NUMBER:
            s << number();
            break;
        case LuaType::BOOLEAN:
            s << boolean();
            break;
        default: {
            auto mt = metatable();
            if (mt.type() != LuaType::NIL) {
                auto to_string = mt.get("__tostring");
                if (to_string.type() != LuaType::NIL) {
                    auto tostr_result = to_string.call({ *this });
                    if (tostr_result.success()) {
                        if (tostr_result.values().size() == 0) {
                            s << "<__tostring returned nothing>";
                            break;
                        } else {
                            s << tostr_result.value().string();
                            break;
                        }
                    } else {
                        s << "<failed calling __tostring> ";
                        s << tostr_result.error_msg();
                        break;
                    }
                }
            }
            s << type();
            break;
        }
        }
    }

    std::string LuaObject::to_string_lua() const {
        std::ostringstream s;
        write_string_lua(s);
        return s.str();
    }

    bool operator ==(const LuaObject & left, const LuaObject & right) {
        assert(left._env == right._env);
        left.push_to_stack();
        right.push_to_stack();
        auto eq = left._env->equal(-1, -2);
        left._env->pop(2);
        return eq;
    }

    bool operator !=(const LuaObject & left, const LuaObject & right) {
        return !(left == right);
    }

    LuaObject::~LuaObject() {
        _clear();
    }

    LuaTablePair::LuaTablePair(const LuaObject & key, const LuaObject & value)
    : key(key)
    , value(value)
    {}

    void LuaTableIterator::_fill_next() {
        _table.push_to_stack();
        _pair.key.push_to_stack();
        if (_env->table_next(-2)) {
            _pair = {
                LuaObject::from_stack_by_type(*_env, -2),
                LuaObject::from_stack_by_type(*_env, -1)
            };
            _env->pop(2);
        } else {
            _pair = {LuaObject(*_env), LuaObject(*_env)};
        }
        _env->pop(1);
    }

    LuaTableIterator::LuaTableIterator(LuaEnvironment & env, const LuaObject & table)
    : _env(&env)
    , _pair({LuaObject(env), LuaObject(env)})
    , _table(table)
    {
        _fill_next();
    }

    LuaTableIterator::LuaTableIterator(LuaEnvironment & env, const LuaObject & table, const LuaTablePair & pair)
    : _env(&env)
    , _pair(pair)
    , _table(table)
    {}

    LuaTableIterator::reference LuaTableIterator::operator *() {
        return _pair;
    }

    LuaTableIterator::pointer LuaTableIterator::operator ->() {
        return &_pair;
    }

    LuaTableIterator & LuaTableIterator::operator ++() {
        _fill_next();
        return *this;
    };

    LuaTableIterator LuaTableIterator::operator ++(int amount) {
        for (int i = 0; i < amount; i++) {
            if (_pair.key.type() == LuaType::NIL) break;
            _fill_next();
        }
        return *this;
    }

    bool operator ==(const LuaTableIterator & left, const LuaTableIterator & right) {
        return left._table == right._table && left._pair.key == right._pair.key && left._pair.value == right._pair.value;
    }

    bool operator !=(const LuaTableIterator & left, const LuaTableIterator & right) {
        return !(left == right);
    }

    std::ostream & operator<<(std::ostream & s, const LuaObject & obj) {
        obj.write_string(s);
        return s;
    }
}
