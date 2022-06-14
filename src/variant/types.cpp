#include <smen/variant/types.hpp>
#include <smen/variant/variant.hpp>
#include <smen/ser/serialization.hpp>
#include <sstream>
#include <vector>

namespace smen {
    void VariantType::_recalculate_total_size(const VariantTypeDirectory & dir) const {
        _cached_size = 0;
        for (auto & pair : _field_map) {
            _cached_size += dir.resolve(pair.second.type_id).size(dir);
        }
    }

    const VariantType VariantType::INVALID = VariantType(VariantTypeCategory::INVALID, INVALID_VARIANT_TYPE_ID, "<Invalid>");

    size_t VariantType::initial_size_of_category(VariantTypeCategory category) {
        switch(category) {
        case VariantTypeCategory::INT32:
            return sizeof(int32_t);
        case VariantTypeCategory::UINT32:
            return sizeof(uint32_t);
        case VariantTypeCategory::INT64:
            return sizeof(int64_t);
        case VariantTypeCategory::UINT64:
            return sizeof(uint64_t);
        case VariantTypeCategory::FLOAT32:
            return sizeof(float);
        case VariantTypeCategory::FLOAT64:
            return sizeof(double);
        case VariantTypeCategory::BOOLEAN:
            return sizeof(bool);
        case VariantTypeCategory::STRING:
            return sizeof(std::string);
        case VariantTypeCategory::REFERENCE:
            return sizeof(VariantReference);
        case VariantTypeCategory::LIST:
            return sizeof(std::vector<VariantIndex>);
        case VariantTypeCategory::COMPLEX:
        case VariantTypeCategory::COMPONENT:
        case VariantTypeCategory::INVALID:
        default:
            return 0;
        }
    }

    VariantType::VariantType(VariantTypeCategory cat, VariantTypeID element_type_id, const std::string & name)
    : _cached_size(initial_size_of_category(cat))
    , _offset_bytes_counter(0)
    , category(cat)
    , generic_category(VariantGenericCategory::NON_GENERIC)
    , builtin(false)
    , name(name)
    , id(INVALID_VARIANT_TYPE_ID)
    , element_type_id(element_type_id)
    , source_type_id(INVALID_VARIANT_TYPE_ID)
    {}

    VariantType::VariantType(VariantTypeCategory cat, const std::string & name)
    : VariantType(cat, INVALID_VARIANT_TYPE_ID, name) {}

    size_t VariantType::size(const VariantTypeDirectory & dir) const {
        if (_cached_size != 0) return _cached_size;
        _recalculate_total_size(dir);
        return _cached_size;
    }

    bool VariantType::is_singleton_type() const {
        if (is_compound_type() && _field_key_list.size() == 0) return true;
        return false;
    }

    void VariantType::add_field(const VariantTypeField & field) {
        _field_key_list.emplace_back(field.name);
        _field_map.insert({field.name, field});
        _cached_size = 0;
    }

    const VariantTypeField & VariantType::field(const std::string & name) const {
        auto it = _field_map.find(name);
        if (it == _field_map.end()) return VariantTypeField::INVALID;
        return it->second;
    }

    const VariantTypeField & VariantType::field(VariantTypeFieldIndex idx) const {
        if (idx >= _field_key_list.size()) return VariantTypeField::INVALID;
        return _field_map.at(_field_key_list.at(idx));
    }

    const std::unordered_map<std::string, VariantTypeField> VariantType::fields() const {
        return _field_map;
    }

    const std::vector<std::string> VariantType::ordered_field_keys() const {
        return _field_key_list;
    }

    bool VariantType::is_compound_type() const {
        return category == VariantTypeCategory::COMPLEX || category == VariantTypeCategory::COMPONENT;
    }

    bool VariantType::valid() const {
        return category != VariantTypeCategory::INVALID;
    }

    void VariantType::write_string(std::ostream & s, const VariantTypeDirectory & dir) const {
        s << "VariantType(";
        s << name;
        if (_field_map.size() > 0) {
            s << " {";
            bool first = true;
            for (auto & pair : _field_map) {
                if (!first) s << ", ";
                first = false;

                s << pair.first << " -> " << dir.resolve(pair.second.type_id).name;
            }
            s << "}";
        }
        s << ")";
    }
    
    void VariantType::write_string(std::ostream & s) const {
        s << "VariantType(";
        s << name;
        if (_field_map.size() > 0) {
            s << " {";
            bool first = true;
            for (auto & pair : _field_map) {
                if (!first) s << ", ";
                first = false;

                s << pair.first << " -> " << pair.second.type_id;
            }
            s << "}";
        }
        s << ")";
    }

    std::string VariantType::to_string() const {
        std::ostringstream s;
        write_string(s);
        return s.str();
    }

    std::string VariantType::to_string(const VariantTypeDirectory & dir) const {
        std::ostringstream s;
        write_string(s, dir);
        return s.str();
    }

    const VariantTypeField VariantTypeField::INVALID = VariantTypeField("<invalid>", INVALID_VARIANT_TYPE_ID, VariantTypeFieldAttribute::NONE, SIZE_MAX);

    const VariantReference VariantReference::NULL_REFERENCE = VariantReference(INVALID_VARIANT_TYPE_ID, UINT32_MAX);

    size_t VariantReference::HashFunction::operator ()(const VariantReference & ref) const {
        size_t hash = 0;
        hash ^= std::hash<VariantTypeID>()(ref.type_id);
        hash ^= std::hash<uint32_t>()(ref.index);
        return hash;
    }

    void VariantReference::write(void * ptr) const {
        *reinterpret_cast<uint32_t *>(ptr) = index;
    }

    VariantReference VariantReference::read(VariantTypeID type_id, void * ptr) {
        auto index = *reinterpret_cast<uint32_t *>(ptr);
        return VariantReference(type_id, index);
    }
    

    bool operator ==(const VariantReference & lhs, const VariantReference & rhs) {
        return lhs.type_id == rhs.type_id && lhs.index == rhs.index;
    }

    bool operator !=(const VariantReference & lhs, const VariantReference & rhs) {
        return !(lhs == rhs);
    }
    

    VariantTypeField::MissingElementTypeException::MissingElementTypeException(const VariantType & type) {
        _msg = "Missing element type ID for type '" + type.name + "'";
    }

    VariantTypeField::VariantTypeField(const std::string & name, VariantTypeID type_id, VariantTypeFieldAttribute attributes, size_t offset_bytes)
    : name(name)
    , type_id(type_id)
    , attributes(attributes)
    , offset_bytes(offset_bytes)
    {}

    VariantTypeField::VariantTypeField(const std::string & name, VariantTypeID type_id, size_t offset_bytes)
    : VariantTypeField(name, type_id, VariantTypeFieldAttribute::NONE, offset_bytes)
    {}

    bool VariantTypeField::has_attribute(VariantTypeFieldAttribute attr) const {
        return (static_cast<uint32_t>(attributes) & static_cast<uint32_t>(attr)) == static_cast<uint32_t>(attr);
    }

    void VariantTypeField::write_string(std::ostream & s) const {
        s << "VariantTypeField([";
        s << offset_bytes;
        s << "]";
        s << name;
        s << " -> ";
        s << type_id;
        s << ")";
    }

    std::string VariantTypeField::to_string() const {
        std::ostringstream s;
        write_string(s);
        return s.str();
    }

    bool VariantTypeField::valid() const {
        return type_id != INVALID_VARIANT_TYPE_ID;
    }

    VariantTypeDirectory::VariantTypeDirectory()
    : _id_counter(0)
    {
        _int32 = _add_builtin(VariantType(VariantTypeCategory::INT32, next_id(), "Int32"));
        _uint32 = _add_builtin(VariantType(VariantTypeCategory::UINT32, next_id(), "UInt32"));
        _int64 = _add_builtin(VariantType(VariantTypeCategory::INT64, next_id(), "Int64"));
        _uint64 = _add_builtin(VariantType(VariantTypeCategory::UINT64, next_id(), "UInt64"));
        _float32 = _add_builtin(VariantType(VariantTypeCategory::FLOAT32, next_id(), "Float32"));
        _float64 = _add_builtin(VariantType(VariantTypeCategory::FLOAT64, next_id(), "Float64"));
        _boolean = _add_builtin(VariantType(VariantTypeCategory::BOOLEAN, next_id(), "Boolean"));

        auto string_type = VariantType(VariantTypeCategory::STRING, next_id(), "String");

        _string_ctor = ctor_db.reg("builtin/string", [](VariantContainer & alloc, const VariantType & type, const VariantEntryContent & content) {
            (void)alloc;
            (void)type;
            new (content.ptr()) std::string();
        });

        string_type.ctor = _string_ctor.id();

        _string_dtor = dtor_db.reg("builtin/string", [](VariantContainer & alloc, const VariantType & type, const VariantEntryContent & content) {
            (void)alloc;
            (void)type;
            auto * str_ptr = reinterpret_cast<std::string *>(content.ptr());
            std::destroy_at(str_ptr);
        });

        string_type.dtor = _string_dtor.id();

        _string = _add_builtin(std::move(string_type));

        auto reference_type = VariantType(VariantTypeCategory::REFERENCE, next_id(), "Reference");
        _reference_ctor = ctor_db.reg("builtin/reference", [](VariantContainer & alloc, const VariantType & type, const VariantEntryContent & content) {
            (void)alloc;
            (void)type;

            VariantReference::NULL_REFERENCE.write(content.ptr());
        });
        reference_type.ctor = _reference_ctor.id();

        _reference_dtor = dtor_db.reg("builtin/reference", [](VariantContainer & alloc, const VariantType & type, const VariantEntryContent & content) {
            auto ref_value = VariantReference::read(type.element_type_id, content.ptr());
            if (ref_value) {
                auto & type_alloc = alloc.get_container_of(ref_value.type_id);
                type_alloc.decref(type_alloc.entry_at(ref_value.index));
            }
        });
        reference_type.dtor = _reference_dtor.id();

        reference_type.generic_category = VariantGenericCategory::GENERIC_BASE;

        _reference = _add_builtin(std::move(reference_type));

        auto list_type = VariantType(VariantTypeCategory::LIST, next_id(), "List");
        _list_ctor = ctor_db.reg("builtin/list", [](VariantContainer & alloc, const VariantType & type, const VariantEntryContent & content) {
            std::construct_at(reinterpret_cast<std::vector<Variant> *>(content.ptr()));
        });
        list_type.ctor = _list_ctor.id();
        _list_dtor = dtor_db.reg("builtin/list", [](VariantContainer & alloc, const VariantType & type, const VariantEntryContent & content) {
            auto * vec_ptr = reinterpret_cast<std::vector<Variant> *>(content.ptr());
            std::destroy_at(vec_ptr);
            /* for (size_t i = 0; i < vec_ptr->size(); i++) { */
            /*     auto ent = alloc.entry_at(type.element_type_id, vec_ptr->at(i)); */
            /*     alloc.decref(ent); */
            /* } */

            /* std::destroy_at(vec_ptr); */
        });
        list_type.dtor = _list_dtor.id();
        list_type.generic_category = VariantGenericCategory::GENERIC_BASE;
        _list = _add_builtin(std::move(list_type));
    }

    VariantTypeID VariantTypeDirectory::next_id() const {
        return _id_counter;
    }

    VariantTypeID VariantTypeDirectory::_add_builtin(VariantType && type) {
        type.builtin = true;
        auto id = add(type);
        return id;
    }

    VariantTypeID VariantTypeDirectory::add(const VariantType & type) {
        _type_map.insert({_id_counter, type});
        _type_name_map.insert({type.name, _id_counter});

        auto & type_ref = _type_map.at(_id_counter);

        type_ref.id = _id_counter;

        if (type_ref.source_type_id == INVALID_VARIANT_TYPE_ID) {
            type_ref.source_type_id = type_ref.id;
        }

        _id_counter += 1;
        return _id_counter - 1;
    }

    const VariantType & VariantTypeDirectory::resolve(VariantTypeID id) const {
        if (id == INVALID_VARIANT_TYPE_ID) return VariantType::INVALID;
        auto it = _type_map.find(id);
        if (it == _type_map.end()) return VariantType::INVALID;
        return it->second;
    }

    const VariantType & VariantTypeDirectory::resolve(const std::string & name) {
        auto angle_bracket_pos = name.find('<');
        if (angle_bracket_pos != std::string::npos) {
            if (name[name.size() - 1] != '>') return VariantType::INVALID;

            auto base_name = name.substr(0, angle_bracket_pos);
            auto element_name = name.substr(angle_bracket_pos + 1, name.size() - angle_bracket_pos - 2);

            auto base_type = resolve(base_name);
            auto element_type = resolve(element_name);

            return make_generic(base_type.id, element_type.id);
        }

        auto id_it = _type_name_map.find(name);
        if (id_it == _type_name_map.end()) return VariantType::INVALID;

        auto it = _type_map.find(id_it->second);
        if (it == _type_map.end()) return VariantType::INVALID;

        return it->second;
    }

    const VariantType & VariantTypeDirectory::make_generic(VariantTypeID id, VariantTypeID element_type_id) {
        if (id == INVALID_VARIANT_TYPE_ID) return VariantType::INVALID;
        auto it = _generic_type_map.find({id, element_type_id});
        if (it == _generic_type_map.end()) {
            auto & generic_type = resolve(id);
            if (generic_type.generic_category != VariantGenericCategory::GENERIC_BASE) return VariantType::INVALID;

            auto & element_type = resolve(element_type_id);
            auto new_type = VariantType(generic_type.category, element_type_id, generic_type.name + "<" + element_type.name + ">");
            new_type.generic_category = VariantGenericCategory::GENERIC_SPECIALIZED;
            new_type.ctor = generic_type.ctor;
            new_type.dtor = generic_type.dtor;
            new_type.source_type_id = id;
            auto new_type_id = add(new_type);

            _generic_type_map.insert({VariantTypeSpecialization(id, element_type_id), new_type});

            return resolve(new_type_id);
        }
        return it->second;
    }

    std::ostream & operator <<(std::ostream & s, VariantTypeCategory cat) {
        switch(cat) {
        case VariantTypeCategory::INT32: s << "int32"; break;
        case VariantTypeCategory::UINT32: s << "uint32"; break;
        case VariantTypeCategory::INT64: s << "int64"; break;
        case VariantTypeCategory::UINT64: s << "uint64"; break;
        case VariantTypeCategory::FLOAT32: s << "float32"; break;
        case VariantTypeCategory::FLOAT64: s << "float64"; break;
        case VariantTypeCategory::STRING: s << "string"; break;
        case VariantTypeCategory::BOOLEAN: s << "boolean"; break;
        case VariantTypeCategory::REFERENCE: s << "reference"; break;
        case VariantTypeCategory::LIST: s << "list"; break;
        case VariantTypeCategory::COMPLEX: s << "complex"; break;
        case VariantTypeCategory::COMPONENT: s << "component"; break;
        case VariantTypeCategory::INVALID:
        default:
            s << "invalid";
            break;
        }
    }
}
