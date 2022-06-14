#include <smen/lua/result.hpp>
#include <smen/lua/object.hpp>

namespace smen {
    LuaResult::LuaResult()
    : _status_code(LuaStatus::OK)
    , _error_msg("")
    , _values()
    {}

    LuaResult::LuaResult(std::vector<LuaObject> && values)
    : _status_code(LuaStatus::OK)
    , _error_msg("")
    , _values(std::move(values))
    {}

    LuaResult::LuaResult(const LuaObject & value)
    : _status_code(LuaStatus::OK)
    , _error_msg("")
    , _values(std::vector<LuaObject>{value})
    {}

    LuaResult::LuaResult(LuaStatus result_code, const std::string & error_msg)
    : _status_code(result_code)
    , _error_msg(error_msg)
    , _values(std::vector<LuaObject>())
    {}

    LuaResult LuaResult::error(const std::string & msg) {
        return LuaResult(LuaStatus::SMEN_ERROR, msg);
    }

    bool LuaResult::success() const {
        return _status_code == LuaStatus::OK;
    }

    bool LuaResult::fail() const {
        return _status_code != LuaStatus::OK;
    }

    LuaStatus LuaResult::status() const {
        return _status_code;
    }

    LuaObject LuaResult::value() const {
        if (_status_code != LuaStatus::OK) {
            throw InvalidAccessException("cannot use ::value() on failed LuaResult");
        }
        if (_values.size() < 1) {
            throw InvalidAccessException("cannot use ::value() without values");
        }
        return _values[0];
    }

    const std::vector<LuaObject> & LuaResult::values() const {
        if (_status_code != LuaStatus::OK) {
            throw InvalidAccessException("cannot use ::value() on failed LuaResult");
        }
        return _values;
    }

    std::string LuaResult::error_msg() const {
        if (_status_code == LuaStatus::OK) {
            throw InvalidAccessException("cannot use ::error_msg() on successful LuaResult");
        } else {
            return _error_msg;
        }
    }

    std::ostream & operator<<(std::ostream & s, const LuaStatus & result) {
        switch(result) {
        case LuaStatus::OK: s << "OK"; break;
        case LuaStatus::YIELD: s << "YIELD"; break;
        case LuaStatus::RUNTIME_ERROR: s << "RUNTIME_ERROR"; break;
        case LuaStatus::SYNTAX_ERROR: s << "SYNTAX_ERROR"; break;
        case LuaStatus::MEMORY_ERROR: s << "MEMORY_ERROR"; break;
        case LuaStatus::ERROR_HANDLER_ERROR: s << "ERROR_HANDLER_ERROR"; break;
        case LuaStatus::FILE_LOAD_ERROR: s << "FILE_LOAD_ERROR"; break;
        case LuaStatus::SMEN_ERROR: s << "SMEN_ERROR"; break;
        default: s << "INVALID"; break;
        }
        return s;
    }

    std::ostream & operator<<(std::ostream & s, const LuaResult & result) {
        s << "LuaResult(";
        s << result.status();
        if (result.status() == LuaStatus::OK) {
            s << "; ";
            if (result.values().size() == 0) {
                s << "0 objects";
            } else if (result.values().size() == 1) {
                s << "1 object";
            } else if (result.values().size() > 1) {
                s << result.values().size() << " objects";
            }
            s << ")";
            return s;
        } else {
            s << "), error:\n";
            s << result.error_msg();
            return s;
        }
    }
}
