#include <smen/ecs/system_query.hpp>
#include <sstream>

namespace smen {
    size_t SystemQuery::HashFunction::operator ()(const SystemQuery & query) const {
        size_t hash = 0;
        for (auto & type_id : query._required_types) {
            hash ^= std::hash<VariantTypeID>()(type_id);
        }
        return hash;
    }

    SystemQuery::SystemQuery(VariantTypeDirectory & dir)
    : _required_types()
    , _dir(dir)
    {}

    SystemQuery & SystemQuery::require_type(VariantTypeID type_id) {
        _required_types.emplace_back(type_id);
        return *this;
    }

    SystemQuery & SystemQuery::require_type(const std::string & name) {
        _required_types.emplace_back(_dir.resolve(name).id);
        return *this;
    }

    const std::vector<VariantTypeID> & SystemQuery::required_type_ids() const {
        return _required_types;
    }

    void SystemQuery::write_string(std::ostream & s) const {
        s << "SystemQuery(";
        auto first = true;
        for (auto & type_id : _required_types) {
            if (!first) s << ", ";
            first = false;
            auto type = _dir.resolve(type_id);
            s << type.name;
        }
        s << ")";
    }

    std::string SystemQuery::to_string() const {
        std::ostringstream s;
        write_string(s);
        return s.str();
    }
}
