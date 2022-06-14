#include <smen/variant/variant.hpp>
#include <smen/ser/serialization.hpp>
#include <algorithm>
#include <cassert>
#include <sstream>

namespace smen {
    size_t Variant::HashFunction::operator ()(const Variant & variant) const {
        switch(variant._storage_mode) {
        case VariantStorageMode::MOVED_FROM: return 0;
        case VariantStorageMode::PRIMITIVE_INT32: return std::hash<int32_t>()(variant._i32);
        case VariantStorageMode::PRIMITIVE_UINT32: return std::hash<uint32_t>()(variant._u32);
        case VariantStorageMode::PRIMITIVE_INT64: return std::hash<int64_t>()(variant._i64);
        case VariantStorageMode::PRIMITIVE_UINT64: return std::hash<uint64_t>()(variant._u64);
        case VariantStorageMode::PRIMITIVE_FLOAT32: return std::hash<float>()(variant._f32);
        case VariantStorageMode::PRIMITIVE_FLOAT64: return std::hash<double>()(variant._f64);
        case VariantStorageMode::PRIMITIVE_STRING: return std::hash<std::string>()(variant._str);
        case VariantStorageMode::PRIMITIVE_BOOLEAN: return std::hash<bool>()(variant._bool);
        case VariantStorageMode::HEAP:
            return VariantEntryContent::HashFunction()(variant._content);
        default:
            return 0;
        }
    }
    Variant::TypeMismatchException::TypeMismatchException(const VariantType & expected, const VariantType & actual) {
        msg = "Attempted to set value of Variant to '" + expected.name + "', but the Variant is of the type '" + actual.name + "'";
    }

    Variant Variant::create(VariantContainer & container, const VariantType & type) {
        switch(type.category) {
        case VariantTypeCategory::INT32: return Variant(container, int32_t(0));
        case VariantTypeCategory::UINT32: return Variant(container, uint32_t(0));
        case VariantTypeCategory::INT64: return Variant(container, int64_t(0));
        case VariantTypeCategory::UINT64: return Variant(container, uint64_t(0));
        case VariantTypeCategory::FLOAT32: return Variant(container, float(0));
        case VariantTypeCategory::FLOAT64: return Variant(container, double(0));
        case VariantTypeCategory::STRING: return Variant(container, std::string());
        case VariantTypeCategory::BOOLEAN: return Variant(container, false);
        default:
            return Variant(container, container.alloc(type.id).content(), type.id);
        }
    }

    Variant Variant::create(VariantContainer & container, VariantTypeID type_id) {
        return Variant::create(container, container.dir.resolve(type_id));
    }

    Variant Variant::create(VariantContainer & container, const std::string & type_name) {
        return Variant::create(container, container.dir.resolve(type_name));
    }

    void Variant::_sync_storage(const Variant & other) {
        switch(other._storage_mode) {
        case VariantStorageMode::PRIMITIVE_INT32:
            _i32 = other._i32;
            break;
        case VariantStorageMode::PRIMITIVE_UINT32:
            _u32 = other._u32;
            break;
        case VariantStorageMode::PRIMITIVE_INT64:
            _i64 = other._i64;
            break;
        case VariantStorageMode::PRIMITIVE_UINT64:
            _u64 = other._u64;
            break;
        case VariantStorageMode::PRIMITIVE_FLOAT32:
            _f32 = other._f32;
            break;
        case VariantStorageMode::PRIMITIVE_FLOAT64:
            _f64 = other._f64;
            break;
        case VariantStorageMode::PRIMITIVE_BOOLEAN:
            _bool = other._bool;
            break;
        case VariantStorageMode::PRIMITIVE_STRING:
            _str = other._str;
            break;
        case VariantStorageMode::HEAP:
            std::construct_at(&_content, other._content);
            _container->incref(_content.entry(*_container));
            break;
        case VariantStorageMode::MOVED_FROM:
            throw;
        }
    }

    Variant::Variant(const Variant & other)
    : _storage_mode(other._storage_mode)
    , _container(other._container)
    , _type_id(other._type_id)
    {
        _sync_storage(other);
    }

    Variant::Variant(Variant && other)
    : _storage_mode(other._storage_mode)
    , _container(other._container)
    , _type_id(other._type_id)
    {
        _sync_storage(other);
        if (other._storage_mode == VariantStorageMode::HEAP) {
            _container->decref(other._content.entry(*_container));
        }
        other._storage_mode = VariantStorageMode::MOVED_FROM;
    }

    Variant & Variant::operator=(const Variant & other) {
        _storage_mode = other._storage_mode;
        _type_id = other._type_id;
        _container = other._container;
        _sync_storage(other);
        return *this;
    }

    Variant & Variant::operator=(Variant && other) {
        _storage_mode = other._storage_mode;
        _type_id = other._type_id;
        _container = other._container;
        _sync_storage(other);
        if (other._storage_mode == VariantStorageMode::HEAP) {
            _container->decref(other._content.entry(*_container));
        }

        other._storage_mode = VariantStorageMode::MOVED_FROM;
        return *this;
    }

    Variant::Variant(VariantContainer & container, int32_t value)
    : _i32(value)
    , _storage_mode(VariantStorageMode::PRIMITIVE_INT32)
    , _container(&container)
    , _type_id(dir().int32())
    {}

    Variant::Variant(VariantContainer & container, uint32_t value)
    : _u32(value)
    , _storage_mode(VariantStorageMode::PRIMITIVE_UINT32)
    , _container(&container)
    , _type_id(dir().uint32())
    {}

    Variant::Variant(VariantContainer & container, int64_t value)
    : _i64(value)
    , _storage_mode(VariantStorageMode::PRIMITIVE_INT64)
    , _container(&container)
    , _type_id(dir().int64())
    {}

    Variant::Variant(VariantContainer & container, uint64_t value)
    : _u64(value)
    , _storage_mode(VariantStorageMode::PRIMITIVE_UINT64)
    , _container(&container)
    , _type_id(dir().uint64())
    {}

    Variant::Variant(VariantContainer & container, float value)
    : _f32(value)
    , _storage_mode(VariantStorageMode::PRIMITIVE_FLOAT32)
    , _container(&container)
    , _type_id(dir().float32())
    {}

    Variant::Variant(VariantContainer & container, double value)
    : _f64(value)
    , _storage_mode(VariantStorageMode::PRIMITIVE_FLOAT64)
    , _container(&container)
    , _type_id(dir().float64())
    {}


    Variant::Variant(VariantContainer & container, bool value)
    : _bool(value)
    , _storage_mode(VariantStorageMode::PRIMITIVE_BOOLEAN)
    , _container(&container)
    , _type_id(dir().string())
    {}

    Variant::Variant(VariantContainer & container, const std::string & value)
    : _str(value)
    , _storage_mode(VariantStorageMode::PRIMITIVE_STRING)
    , _container(&container)
    , _type_id(dir().string())
    {}

    Variant::Variant(VariantContainer & container, const VariantEntryContent & content, VariantTypeID type_id)
    : _content(content)
    , _storage_mode(VariantStorageMode::HEAP)
    , _container(&container)
    , _type_id(type_id)
    {
        _container->incref(_content.entry(*_container));
    }

    Variant::Variant(VariantContainer & container, const VariantReference & ref)
    : _content(container.entry_at(ref.type_id, ref.index).content())
    , _storage_mode(VariantStorageMode::HEAP)
    , _container(&container)
    , _type_id(ref.type_id)
    {
        _container->incref(_content.entry(*_container));
    }

    Variant Variant::get_field(const std::string & field_name) const {
        assert(type().is_compound_type() && "Variant must be of a COMPLEX or COMPONENT type to use ::get_field");

        auto field = type().field(field_name);
        assert(field && "Field must exist when using ::get_field - use ::try_get_field to avoid asserts");
        return Variant(*_container, _content.of_field(field), field.type_id);
    }

    Variant Variant::get_field(const VariantTypeField & field) const {
        return Variant(*_container, _content.of_field(field), field.type_id);
    }

    std::optional<Variant> Variant::try_get_field(const std::string & field_name) const {
        if(!type().is_compound_type()) return std::nullopt;

        auto field = type().field(field_name);
        if (!field) return std::nullopt;
        return Variant(*_container, _content.of_field(field), field.type_id);
    }

    void Variant::set_field(const std::string & field_name, const Variant & value) {
        assert(type().is_compound_type() && "Variant must be of a COMPLEX or COMPONENT type to use ::set_field");

        auto field = type().field(field_name);
        assert(field && "Field must exist when using ::set_field - use ::try_set_field to avoid asserts");

        auto field_variant = Variant(*_container, _content.of_field(field), field.type_id);
        field_variant.set_variant(value);
    }

    bool Variant::try_set_field(const std::string & field_name, const Variant & value) {
        if(type().is_compound_type()) return false;

        auto field = type().field(field_name);
        if (!field) return false;

        auto field_variant = Variant(*_container, _content.of_field(field), field.type_id);
        field_variant.set_variant(value);
        return true;
    }

    VariantEntryContent Variant::content_ptr() const {
        assert(_storage_mode == VariantStorageMode::HEAP);
        return _content;
    }

    bool Variant::is_default_value() const {
    switch(type().category) {
    case VariantTypeCategory::INT32: return int32() == 0;
    case VariantTypeCategory::UINT32: return uint32() == 0;
    case VariantTypeCategory::INT64: return int64() == 0;
    case VariantTypeCategory::UINT64: return uint64() == 0;
    case VariantTypeCategory::FLOAT32: return float32() == 0;
    case VariantTypeCategory::FLOAT64: return float64() == 0;
    case VariantTypeCategory::BOOLEAN: return boolean() == false;
    case VariantTypeCategory::STRING: return string() == "";
    case VariantTypeCategory::REFERENCE: return reference().null();
    case VariantTypeCategory::LIST: return list_size() == 0;
    case VariantTypeCategory::INVALID:
    default:
        return false;
    }
    }

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wswitch"

    int32_t Variant::int32() const {
        if (_storage_mode == VariantStorageMode::HEAP) { 
            switch(type().category) {
            case VariantTypeCategory::INT32:
                return *reinterpret_cast<int32_t*>(_content.ptr());
            }
        } else if (_storage_mode == VariantStorageMode::PRIMITIVE_INT32) {
            return _i32;
        }

        throw TypeMismatchException(dir().int32_type(), type());
    }

    void Variant::set_int32(int32_t value) {
        if (_storage_mode == VariantStorageMode::HEAP) { 
            switch(type().category) {
            case VariantTypeCategory::INT32:
                *reinterpret_cast<int32_t*>(_content.ptr()) = value;
                return;
            case VariantTypeCategory::INT64:
                *reinterpret_cast<int64_t*>(_content.ptr()) = value;
                return;
            }
        } else if (_storage_mode == VariantStorageMode::PRIMITIVE_INT32) {
            _i32 = value;
            return;
        } else if (_storage_mode == VariantStorageMode::PRIMITIVE_INT64) {
            _i64 = value;
            return;
        }

        throw TypeMismatchException(dir().int32_type(), type());
    }

    uint32_t Variant::uint32() const {
        if (_storage_mode == VariantStorageMode::HEAP) { 
            switch(type().category) {
            case VariantTypeCategory::UINT32:
                return *reinterpret_cast<uint32_t*>(_content.ptr());
            }
        } else if (_storage_mode == VariantStorageMode::PRIMITIVE_UINT32) {
            return _u32;
        }

        throw TypeMismatchException(dir().uint32_type(), type());
    }

    void Variant::set_uint32(uint32_t value) {
        if (_storage_mode == VariantStorageMode::HEAP) { 
            switch(type().category) {
            case VariantTypeCategory::UINT32:
                *reinterpret_cast<uint32_t*>(_content.ptr()) = value;
                return;
            case VariantTypeCategory::UINT64:
                *reinterpret_cast<uint64_t*>(_content.ptr()) = value;
                return;
            }
        } else if (_storage_mode == VariantStorageMode::PRIMITIVE_UINT32) {
            _u32 = value;
            return;
        } else if (_storage_mode == VariantStorageMode::PRIMITIVE_UINT64) {
            _u64 = value;
            return;
        }

        throw TypeMismatchException(dir().uint32_type(), type());
    }

    int64_t Variant::int64() const {
        if (_storage_mode == VariantStorageMode::HEAP) { 
            switch(type().category) {
            case VariantTypeCategory::INT64:
                return *reinterpret_cast<int64_t*>(_content.ptr());
            case VariantTypeCategory::INT32:
                return *reinterpret_cast<int32_t*>(_content.ptr());
            }
        } else if (_storage_mode == VariantStorageMode::PRIMITIVE_INT64) {
            return _i64;
        } else if (_storage_mode == VariantStorageMode::PRIMITIVE_INT32) {
            return _i32;
        }

        throw TypeMismatchException(dir().int64_type(), type());
    }

    void Variant::set_int64(int64_t value) {
        if (_storage_mode == VariantStorageMode::HEAP) { 
            switch(type().category) {
            case VariantTypeCategory::INT64:
                *reinterpret_cast<int64_t*>(_content.ptr()) = value;
                return;
            }
        } else if (_storage_mode == VariantStorageMode::PRIMITIVE_INT64) {
            _i64 = value;
            return;
        }

        throw TypeMismatchException(dir().int64_type(), type());
    }

    uint64_t Variant::uint64() const {
        if (_storage_mode == VariantStorageMode::HEAP) { 
            switch(type().category) {
            case VariantTypeCategory::UINT64:
                return *reinterpret_cast<uint64_t*>(_content.ptr());
            case VariantTypeCategory::UINT32:
                return *reinterpret_cast<uint32_t*>(_content.ptr());
            }
        } else if (_storage_mode == VariantStorageMode::PRIMITIVE_UINT64) {
            return _u64;
        } else if (_storage_mode == VariantStorageMode::PRIMITIVE_UINT32) {
            return _u32;
        }

        throw TypeMismatchException(dir().uint64_type(), type());
    }

    void Variant::set_uint64(uint64_t value) {
        if (_storage_mode == VariantStorageMode::HEAP) { 
            switch(type().category) {
            case VariantTypeCategory::UINT64:
                *reinterpret_cast<uint64_t*>(_content.ptr()) = value;
                return;
            }
        } else if (_storage_mode == VariantStorageMode::PRIMITIVE_UINT64) {
            _u64 = value;
            return;
        }

        throw TypeMismatchException(dir().uint64_type(), type());
    }

    float Variant::float32() const {
        if (_storage_mode == VariantStorageMode::HEAP) { 
            switch(type().category) {
            case VariantTypeCategory::FLOAT32:
                return *reinterpret_cast<float*>(_content.ptr());
            }
        } else if (_storage_mode == VariantStorageMode::PRIMITIVE_FLOAT32) {
            return _f32;
        }

        throw TypeMismatchException(dir().float32_type(), type());
    }

    void Variant::set_float32(float value) {
        if (_storage_mode == VariantStorageMode::HEAP) { 
            switch(type().category) {
            case VariantTypeCategory::FLOAT64:
                *reinterpret_cast<double*>(_content.ptr()) = static_cast<double>(value);
                return;
            case VariantTypeCategory::FLOAT32:
                *reinterpret_cast<float*>(_content.ptr()) = value;
                return;
            }
        } else if (_storage_mode == VariantStorageMode::PRIMITIVE_FLOAT64) {
            _f64 = static_cast<double>(value);
            return;
        } else if (_storage_mode == VariantStorageMode::PRIMITIVE_FLOAT32) {
            _f32 = value;
            return;
        }

        throw TypeMismatchException(dir().float32_type(), type());
    }

    double Variant::float64() const {
        if (_storage_mode == VariantStorageMode::HEAP) { 
            switch(type().category) {
            case VariantTypeCategory::FLOAT64:
                return *reinterpret_cast<double*>(_content.ptr());
            case VariantTypeCategory::FLOAT32:
                return static_cast<double>(*reinterpret_cast<float*>(_content.ptr()));
            }
        } else if (_storage_mode == VariantStorageMode::PRIMITIVE_FLOAT64) {
            return _f64;
        } else if (_storage_mode == VariantStorageMode::PRIMITIVE_FLOAT32) {
            return static_cast<double>(_f32);
        }

        throw TypeMismatchException(dir().float64_type(), type());
    }

    void Variant::set_float64(double value) {
        if (_storage_mode == VariantStorageMode::HEAP) { 
            switch(type().category) {
            case VariantTypeCategory::FLOAT64:
                *reinterpret_cast<double*>(_content.ptr()) = value;
                return;
            }
        } else if (_storage_mode == VariantStorageMode::PRIMITIVE_FLOAT64) {
            _f64 = value;
            return;
        }

        throw TypeMismatchException(dir().float64_type(), type());
    }

    bool Variant::boolean() const {
        if (_storage_mode == VariantStorageMode::HEAP) { 
            switch(type().category) {
            case VariantTypeCategory::BOOLEAN:
                return *reinterpret_cast<bool*>(_content.ptr());
            }
        } else if (_storage_mode == VariantStorageMode::PRIMITIVE_BOOLEAN) {
            return _bool;
        }

        throw TypeMismatchException(dir().boolean_type(), type());
    }

    void Variant::set_boolean(bool value) {
        if (_storage_mode == VariantStorageMode::HEAP) { 
            switch(type().category) {
            case VariantTypeCategory::BOOLEAN:
                *reinterpret_cast<bool*>(_content.ptr()) = value;
                return;
            }
        } else if (_storage_mode == VariantStorageMode::PRIMITIVE_BOOLEAN) {
            _bool = value;
            return;
        }

        throw TypeMismatchException(dir().boolean_type(), type());
    }

    const std::string & Variant::string() const {
        if (_storage_mode == VariantStorageMode::HEAP) { 
            switch(type().category) {
            case VariantTypeCategory::STRING:
                return *reinterpret_cast<std::string *>(_content.ptr());
            }
        } else if (_storage_mode == VariantStorageMode::PRIMITIVE_STRING) {
            return _str;
        }

        throw TypeMismatchException(dir().string_type(), type());
    }

    void Variant::set_string(const std::string & value) {
        if (_storage_mode == VariantStorageMode::HEAP) { 
            switch(type().category) {
            case VariantTypeCategory::STRING:
                *reinterpret_cast<std::string *>(_content.ptr()) = value;
                return;
            }
        } else if (_storage_mode == VariantStorageMode::PRIMITIVE_STRING) {
            _str = value;
            return;
        }

        throw TypeMismatchException(dir().string_type(), type());
    }
#pragma GCC diagnostic pop

    Variant Variant::variant() const {
        if (type().category == VariantTypeCategory::REFERENCE) {
            auto resolved = _container->resolve(reference());
            return Variant(*_container, resolved.content(), resolved.type_id);
        }
        return *this;
    }

    void Variant::set_variant(const Variant & value) {
        auto & value_type = value.type();
        auto & self_type = type();

        // @TODO better compat detection?
        if (self_type.name != value_type.name) {
            throw TypeMismatchException(value_type, self_type);
        }

        if (!value_type.is_compound_type()) {
            switch(value_type.category) {
            case VariantTypeCategory::INT32:
                set_int32(value.int32());
                break;
            case VariantTypeCategory::UINT32:
                set_uint32(value.uint32());
                break;
            case VariantTypeCategory::INT64:
                set_int64(value.int64());
                break;
            case VariantTypeCategory::UINT64:
                set_uint64(value.uint64());
                break;
            case VariantTypeCategory::FLOAT32:
                set_float32(value.float32());
                break;
            case VariantTypeCategory::FLOAT64:
                set_float64(value.float64());
                break;
            case VariantTypeCategory::BOOLEAN:
                set_boolean(value.boolean());
                break;
            case VariantTypeCategory::STRING:
                set_string(value.string());
                break;
            case VariantTypeCategory::REFERENCE:
                set_reference(value.reference());
                break;
            case VariantTypeCategory::LIST:
                list_clear();
                for (size_t i = 0; i < value.list_size(); i++) {
                    list_add(value.list_get(i).value());
                }
                break;
            case VariantTypeCategory::INVALID:
            case VariantTypeCategory::COMPLEX:
            case VariantTypeCategory::COMPONENT:
                break;
            }

            return;
        }        

        for (auto & pair : self_type.fields()) {
            auto field_variant = Variant(*_container, _content.of_field(pair.second), pair.second.type_id);
            set_field(pair.first, field_variant);
        }
    }

    VariantReference Variant::reference() const {
        if (_storage_mode == VariantStorageMode::HEAP && type().category == VariantTypeCategory::REFERENCE) {
            return VariantReference::read(type().element_type_id, _content.ptr());
        }
        throw TypeMismatchException(dir().reference_type(), type());
    }

    void Variant::set_reference(VariantReference ref) {
        if (_storage_mode == VariantStorageMode::HEAP && type().category == VariantTypeCategory::REFERENCE) {
            auto cur_ref = reference();
            if (cur_ref) _container->decref(_container->resolve(cur_ref));
            ref.write(_content.ptr());
            if (!ref.null()) {
                _container->incref(_container->resolve(ref));
            }
            return;
        }
        throw TypeMismatchException(dir().reference_type(), type());
    }

    size_t Variant::list_size() const {
        if (_storage_mode == VariantStorageMode::HEAP && type().category == VariantTypeCategory::LIST) {
            auto * vec_ptr = reinterpret_cast<std::vector<Variant> *>(_content.ptr());
            return vec_ptr->size();
        }

        throw TypeMismatchException(dir().list_type(), type());
    }

    void Variant::list_clear() {
        if (_storage_mode == VariantStorageMode::HEAP && type().category == VariantTypeCategory::LIST) {
            auto * vec_ptr = reinterpret_cast<std::vector<Variant> *>(_content.ptr());
            vec_ptr->clear();
            return;
        }

        throw TypeMismatchException(dir().list_type(), type());
    }

    void Variant::list_add(const Variant & value) {
        if (_storage_mode == VariantStorageMode::HEAP && type().category == VariantTypeCategory::LIST) {
            auto * vec_ptr = reinterpret_cast<std::vector<Variant> *>(_content.ptr());
            vec_ptr->push_back(value);
            //_container->incref(value._content.entry(_container));
            return;
        }

        throw TypeMismatchException(dir().list_type(), type());
    }

    bool Variant::list_remove(size_t index) {
        if (_storage_mode == VariantStorageMode::HEAP && type().category == VariantTypeCategory::LIST) {
            auto * vec_ptr = reinterpret_cast<std::vector<Variant> *>(_content.ptr());
            if (index >= vec_ptr->size()) return false;
            /* auto ent = _container->entry_at(type().element_type_id, vec_ptr->at(index)); */
            /* if (!ent) { */
            /*     return false; */
            /* } */

            /* _container->decref(ent); */
            vec_ptr->erase(vec_ptr->begin() + static_cast<long>(index));
            return true;
        }

        throw TypeMismatchException(dir().list_type(), type());
    }

    std::optional<Variant> Variant::list_get(size_t index) const {
        if (_storage_mode == VariantStorageMode::HEAP && type().category == VariantTypeCategory::LIST) {
            auto * vec_ptr = reinterpret_cast<std::vector<Variant> *>(_content.ptr());
            if (index >= vec_ptr->size()) return std::nullopt;
            /* auto ent = _container->entry_at(type().element_type_id, vec_ptr->at(index)); */
            /* if (!ent) { */
            /*     return std::nullopt; */
            /* } */
            /* auto v = Variant(*_container, ent.content(), type().element_type_id); */
            /* return v; */
            return vec_ptr->at(index);
        }

        throw TypeMismatchException(dir().list_type(), type());
    }

    Variant & Variant::list_ref(size_t index) const {
        if (_storage_mode == VariantStorageMode::HEAP && type().category == VariantTypeCategory::LIST) {
            auto * vec_ptr = reinterpret_cast<std::vector<Variant> *>(_content.ptr());
            if (index >= vec_ptr->size()) throw std::runtime_error("index out of bounds in list_ref: " + std::to_string(index));
            return vec_ptr->at(index);
        }

        throw TypeMismatchException(dir().list_type(), type());
    }

    std::optional<VariantReference> Variant::reference_to() const {
        if (_storage_mode == VariantStorageMode::HEAP) {
            return _container->reference_of(_content.entry(*_container));
        } else {
            return std::nullopt;
        }
    }

    void Variant::write_contents(std::ostream & s) const {
        switch(type().category) {
        case VariantTypeCategory::INT32:
            s << int32();
            break;
        case VariantTypeCategory::UINT32:
            s << uint32();
            break;
        case VariantTypeCategory::INT64:
            s << int64();
            s << "LL";
            break;
        case VariantTypeCategory::UINT64:
            s << uint64();
            s << "ULL";
            break;
        case VariantTypeCategory::FLOAT32:
            s << float32();
            s << "f";
            break;
        case VariantTypeCategory::FLOAT64:
            s << float64();
            s << "d";
            break;
        case VariantTypeCategory::BOOLEAN:
            s << (boolean() ? "true" : "false");
            break;
        case VariantTypeCategory::STRING:
            s << "\"" << string() << "\"";
            break;
        case VariantTypeCategory::REFERENCE:
            s << type().name << " [";
            if (reference()) {
                s << "@";
                s << reference().index;
                s << ": ";
                variant().write_contents(s);
            } else {
                s << "NULL";
            }
            s << "]";
            break;
        case VariantTypeCategory::LIST:
            s << type().name << " [";
            for (size_t i = 0; i < list_size(); i++) {
                if (i != 0) s << ", ";

                list_get(i)->write_contents(s);
            }
            s << "]";
            break;
        case VariantTypeCategory::COMPONENT:
            s << "component ";
            [[fallthrough]];
        case VariantTypeCategory::COMPLEX: {
            s << type().name << " ";
            s << "{";
            bool first = true;
            for (auto & key : type().ordered_field_keys()) {
                auto field = type().field(key);
                if (!first) s << ", ";
                first = false;
                s << key;
                s << " = ";
                get_field(field).write_contents(s);
            }
            s << "}";
            break;
        }
        case VariantTypeCategory::INVALID:
        default:
            s << "<INVALID>";
            break;
        }
    }

    void Variant::write_string(std::ostream & s) const {
        s << "Variant(";
        write_contents(s);
        s << ")";
    }

    std::string Variant::to_string() const {
        std::ostringstream s;
        write_string(s);
        return s.str();
    }

    bool operator ==(const Variant & lhs, const Variant & rhs) {
        auto lhs_type = lhs.type();
        auto rhs_type = rhs.type();
        if (lhs_type.category != rhs_type.category) return false;

        switch (lhs_type.category) {
        case VariantTypeCategory::INT32:
            return lhs.int32() == rhs.int32();
        case VariantTypeCategory::UINT32:
            return lhs.uint32() == rhs.uint32();
        case VariantTypeCategory::INT64:
            return lhs.int64() == rhs.int64();
        case VariantTypeCategory::UINT64:
            return lhs.uint64() == rhs.uint64();
        case VariantTypeCategory::FLOAT32:
            return lhs.float32() == rhs.float32();
        case VariantTypeCategory::FLOAT64:
            return lhs.float64() == rhs.float64();
        case VariantTypeCategory::BOOLEAN:
            return lhs.boolean() == rhs.boolean();
        case VariantTypeCategory::STRING:
            return lhs.string() == rhs.string();
        case VariantTypeCategory::REFERENCE:
            return lhs.reference() == rhs.reference();
        case VariantTypeCategory::LIST:
            if (lhs.list_size() != rhs.list_size()) return false;
            for (size_t i = 0; i < lhs.list_size(); i++) {
                if (lhs.list_get(i) != rhs.list_get(i)) return false;
            }
            return true;
        case VariantTypeCategory::COMPONENT:
        case VariantTypeCategory::COMPLEX:
            return lhs.reference_to() == rhs.reference_to();
        case VariantTypeCategory::INVALID:
            return false;
        default:
            return false;
        }
    }

    bool operator !=(const Variant & lhs, const Variant & rhs) {
        return !(lhs == rhs);
    }

    Variant::~Variant() {
        if (_storage_mode == VariantStorageMode::MOVED_FROM) return;

        if (_storage_mode == VariantStorageMode::HEAP) {
            _container->decref(_content.entry(*_container));
        } else if (_storage_mode == VariantStorageMode::PRIMITIVE_STRING) {
            std::destroy_at(&_str);
        }
    }
}
