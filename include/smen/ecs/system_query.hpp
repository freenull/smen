#ifndef SMEN_ECS_SYSTEM_QUERY_HPP
#define SMEN_ECS_SYSTEM_QUERY_HPP

#include <smen/variant/variant.hpp>
#include <vector>

namespace smen {
    struct SystemQuery {
    public:
        struct HashFunction {
            size_t operator ()(const SystemQuery & query) const;
        };

    private:
        std::vector<VariantTypeID> _required_types;
        VariantTypeDirectory & _dir;

    public:
        SystemQuery(VariantTypeDirectory & dir);

        SystemQuery & require_type(VariantTypeID type_id);
        SystemQuery & require_type(const std::string & name);
        const std::vector<VariantTypeID> & required_type_ids() const;
        void write_string(std::ostream & s) const;
        std::string to_string() const;
    };
}


#endif//SMEN_ECS_SYSTEM_QUERY_HPP
