#include <smen/variant/type_builder.hpp>

namespace smen {
    VariantTypeBuilder::VariantTypeBuilder(VariantTypeDirectory & dir, VariantTypeCategory category, const std::string & name)
    : _type(VariantType(category, name))
    , _cur_offset_bytes(0)
    , dir(dir)
    {}

    VariantTypeBuilder::VariantTypeBuilder(VariantTypeDirectory & dir, const std::string & name)
    : VariantTypeBuilder(dir, VariantTypeCategory::COMPLEX, name) {}

    VariantTypeBuilder & VariantTypeBuilder::field(const std::string & name, const VariantType & type, VariantTypeFieldAttribute attrib) {
        if (type.category == VariantTypeCategory::COMPONENT) {
            throw std::runtime_error("variants cannot have fields with types of the COMPONENT category");
        }
        _type.add_field(VariantTypeField(name, type.id, attrib, _cur_offset_bytes));
        _cur_offset_bytes += type.size(dir);
        return *this;
    }

    VariantTypeBuilder & VariantTypeBuilder::field(const std::string & name, VariantTypeID type_id, VariantTypeFieldAttribute attrib) {
        auto type = dir.resolve(type_id);
        if (type.category == VariantTypeCategory::COMPONENT) {
            throw std::runtime_error("variants cannot have fields with types of the COMPONENT category");
        }
        return field(name, type, attrib);
    }

    VariantTypeBuilder & VariantTypeBuilder::field(const std::string & name, const std::string & type_name, VariantTypeFieldAttribute attrib) {
        return field(name, dir.resolve(type_name), attrib);
    }

    VariantTypeBuilder & VariantTypeBuilder::ctor(const ObjectDatabaseID<VariantConstructor> & ctor) {
        _type.ctor = ctor;
        return *this;
    }

    VariantTypeBuilder & VariantTypeBuilder::dtor(const ObjectDatabaseID<VariantDestructor> & dtor) {
        _type.dtor = dtor;
        return *this;
    }

    VariantType VariantTypeBuilder::build() {
        return _type;
    }

    VariantTypeID VariantTypeBuilder::commit() {
        return dir.add(_type);
    }
}
