#ifndef SMEN_VARIANT_TYPE_BUILDER_HPP
#define SMEN_VARIANT_TYPE_BUILDER_HPP

#include <smen/variant/types.hpp>
#include <smen/object_db.hpp>

namespace smen {
    class VariantTypeBuilder {
    private:
        VariantType _type;
        size_t _cur_offset_bytes;

    public:
        VariantTypeDirectory & dir;

        VariantTypeBuilder(VariantTypeDirectory & dir, VariantTypeCategory category, const std::string & name);
        VariantTypeBuilder(VariantTypeDirectory & dir, const std::string & name);
        VariantTypeBuilder & field(const std::string & name, const VariantType & type, VariantTypeFieldAttribute attrib = VariantTypeFieldAttribute::NONE);
        VariantTypeBuilder & field(const std::string & name, VariantTypeID type_id, VariantTypeFieldAttribute attrib = VariantTypeFieldAttribute::NONE);
        VariantTypeBuilder & field(const std::string & name, const std::string & type_name, VariantTypeFieldAttribute attrib = VariantTypeFieldAttribute::NONE);
        VariantTypeBuilder & ctor(const ObjectDatabaseID<VariantConstructor> & ctor);
        VariantTypeBuilder & dtor(const ObjectDatabaseID<VariantDestructor> & dtor);
        VariantType build();
        VariantTypeID commit();
    };
}

#endif//SMEN_VARIANT_TYPE_BUILDER_HPP
