#ifndef SMEN_VARIANT_TYPES_HPP
#define SMEN_VARIANT_TYPES_HPP

#include <iostream>
#include <unordered_map>
#include <string>
#include <optional>
#include <functional>
#include <vector>
#include <smen/object_db.hpp>
#include <unordered_set>

namespace smen {
    enum class VariantTypeCategory {
        INVALID, 

        INT32,
        UINT32,
        INT64,
        UINT64,

        FLOAT32,
        FLOAT64,

        BOOLEAN,

        STRING,

        REFERENCE,

        LIST,

        COMPLEX,
        COMPONENT
    };

    using VariantTypeID = uint32_t;
    const VariantTypeID INVALID_VARIANT_TYPE_ID = UINT32_MAX;

    class Variant;

    struct VariantReference {
    public:
        struct HashFunction {
            size_t operator ()(const VariantReference & ref) const;
        };

        static const VariantReference NULL_REFERENCE;

        const VariantTypeID type_id;
        const uint32_t index;

        inline VariantReference(VariantTypeID type_id, uint32_t index)
        : type_id(type_id)
        , index(index)
        {}

        inline bool null() { return index == UINT32_MAX; }
        inline explicit operator bool() { return !null(); }

        void write(void * ptr) const;
        static VariantReference read(VariantTypeID type_id, void * ptr);
        inline static size_t size() { return sizeof(index); }

        friend bool operator ==(const VariantReference & lhs, const VariantReference & rhs);
        friend bool operator !=(const VariantReference & lhs, const VariantReference & rhs);
    };


    class VariantTypeDirectory;
    struct VariantEntry;
    struct VariantEntryContent;
    class VariantContainer;


    class VariantType;

    enum class VariantTypeFieldAttribute : uint32_t {
        NONE = 0,
        DONT_SERIALIZE = 1 << 0,
        ALWAYS_SERIALIZE = 1 << 1
    };

    class VariantTypeField {
    public:
        class MissingElementTypeException : public std::exception {
        private:
            std::string _msg;

        public:
            MissingElementTypeException(const VariantType & type);

            inline virtual const char * what() const noexcept(true) override {
                return _msg.c_str();
            }
        };
    private:

    public:
        static const VariantTypeField INVALID;

        const std::string name;
        const VariantTypeID type_id;
        const VariantTypeFieldAttribute attributes;
        const size_t offset_bytes;


        VariantTypeField(const std::string & name, VariantTypeID type_id, VariantTypeFieldAttribute attributes, size_t offset_bytes);
        VariantTypeField(const std::string & name, VariantTypeID type_id, size_t offset_bytes);

        bool has_attribute(VariantTypeFieldAttribute attr) const;
        void write_string(std::ostream & s) const;
        std::string to_string() const;

        bool valid() const;
        inline explicit operator bool() const { return valid(); }
    };

    inline std::ostream & operator << (std::ostream & s, const VariantTypeField & field) {
        field.write_string(s);
        return s;
    }

    using VariantTypeFieldIndex = size_t;

    using VariantConstructor = std::function<void (VariantContainer & alloc, const VariantType & type, const VariantEntryContent & variant)>;
    using VariantDestructor = std::function<void (VariantContainer & alloc, const VariantType & type, const VariantEntryContent & variant)>;

    enum class VariantGenericCategory {
        NON_GENERIC,
        GENERIC_BASE,
        GENERIC_SPECIALIZED
    };

    class VariantType {
    private:
        std::unordered_map<std::string, VariantTypeField> _field_map;
        std::vector<std::string> _field_key_list;

        void _recalculate_total_size(const VariantTypeDirectory & dir) const;
        mutable size_t _cached_size;
        size_t _offset_bytes_counter;

    public:
        static const VariantType INVALID;
        static size_t initial_size_of_category(VariantTypeCategory category);

        ObjectDatabaseID<VariantConstructor> ctor;
        ObjectDatabaseID<VariantDestructor> dtor;

        VariantTypeCategory category;
        VariantGenericCategory generic_category;

        bool builtin;
        std::string name;
        VariantTypeID id;
        VariantTypeID element_type_id;
        VariantTypeID source_type_id;

        VariantType(VariantTypeCategory cat, VariantTypeID element_type_id, const std::string & name);
        VariantType(VariantTypeCategory cat, const std::string & name);

        size_t size(const VariantTypeDirectory & dir) const;
        bool is_singleton_type() const;

        void add_field(const VariantTypeField & field);
        const VariantTypeField & field(const std::string & name) const;
        const VariantTypeField & field(VariantTypeFieldIndex idx) const;
        const std::unordered_map<std::string, VariantTypeField> fields() const;
        const std::vector<std::string> ordered_field_keys() const;
        bool is_compound_type() const;
        bool valid() const;
        void write_string(std::ostream & s) const;
        void write_string(std::ostream & s, const VariantTypeDirectory & dir) const;
        std::string to_string() const;
        std::string to_string(const VariantTypeDirectory & dir) const;
        inline explicit operator bool() const { return valid(); }
    };

    inline std::ostream & operator << (std::ostream & s, const VariantType & type) {
        type.write_string(s);
        return s;
    }

    struct VariantTypeSpecialization {
    public:
        const VariantTypeID type_id;
        const VariantTypeID element_type_id;

        inline VariantTypeSpecialization(VariantTypeID type_id, VariantTypeID element_type_id)
        : type_id(type_id)
        , element_type_id(element_type_id)
        {}

        struct HashFunction {
            std::size_t operator() (const VariantTypeSpecialization & spec) const {
                return std::hash<VariantTypeID>()(spec.type_id) ^ std::hash<VariantTypeID>()(spec.element_type_id);
            }
        };

        inline bool operator == (const VariantTypeSpecialization & other) const {
            return type_id == other.type_id && element_type_id == other.element_type_id;
        }

        inline bool operator != (const VariantTypeSpecialization & other) const {
            return type_id != other.type_id || element_type_id != other.element_type_id;
        }
    };

    class VariantTypeDirectory {

    private:
        VariantTypeID _id_counter;
        std::unordered_map<VariantTypeID, VariantType> _type_map;
        std::unordered_map<std::string, VariantTypeID> _type_name_map;
        std::unordered_map<VariantTypeSpecialization, VariantType, VariantTypeSpecialization::HashFunction> _generic_type_map;

        ObjectDatabaseEntry<VariantDestructor> _string_ctor;
        ObjectDatabaseEntry<VariantDestructor> _string_dtor;
        ObjectDatabaseEntry<VariantConstructor> _reference_ctor;
        ObjectDatabaseEntry<VariantDestructor> _reference_dtor;
        ObjectDatabaseEntry<VariantConstructor> _list_ctor;
        ObjectDatabaseEntry<VariantConstructor> _list_dtor;

        VariantTypeID _int32;
        VariantTypeID _uint32;
        VariantTypeID _int64;
        VariantTypeID _uint64;
        VariantTypeID _float32;
        VariantTypeID _float64;
        VariantTypeID _boolean;
        VariantTypeID _string;
        VariantTypeID _reference;
        VariantTypeID _list;

        VariantTypeID _add_builtin(VariantType && type);

    public:
        ObjectDatabase<VariantConstructor> ctor_db;
        ObjectDatabase<VariantDestructor> dtor_db;

        std::string script_path;

        VariantTypeDirectory();

        VariantTypeID next_id() const;
        VariantTypeID add(const VariantType & type);
        const VariantType & make_generic(VariantTypeID id, VariantTypeID element_type_id);
        const VariantType & resolve(VariantTypeID id) const;
        const VariantType & resolve(const std::string & name);

        inline std::unordered_map<VariantTypeID, VariantType> types() const { return _type_map; }

        inline VariantTypeID int32() const { return _int32; }
        inline VariantTypeID uint32() const { return _uint32; }
        inline VariantTypeID int64() const { return _int64; }
        inline VariantTypeID uint64() const { return _uint64; }
        inline VariantTypeID float32() const { return _float32; }
        inline VariantTypeID float64() const { return _float64; }
        inline VariantTypeID boolean() const { return _boolean; }
        inline VariantTypeID string() const { return _string; }
        inline VariantTypeID reference() const { return _reference; }
        inline VariantTypeID list() const { return _list; }

        inline const VariantType & int32_type() const { return resolve(_int32); }
        inline const VariantType & uint32_type() const { return resolve(_uint32); }
        inline const VariantType & int64_type() const { return resolve(_int64); }
        inline const VariantType & uint64_type() const { return resolve(_uint64); }
        inline const VariantType & float32_type() const { return resolve(_float32); }
        inline const VariantType & float64_type() const { return resolve(_float64); }
        inline const VariantType & boolean_type() const { return resolve(_boolean); }
        inline const VariantType & string_type() const { return resolve(_string); }
        inline const VariantType & reference_type() { return resolve(_reference); }
        inline const VariantType & list_type() { return resolve(_list); }

    };
    
    std::ostream & operator <<(std::ostream & s, VariantTypeCategory cat);
}

#endif//SMEN_VARIANT_TYPES_HPP
