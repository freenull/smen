#ifndef SMEN_VARIANT_VARIANT_HPP
#define SMEN_VARIANT_VARIANT_HPP

#include <smen/variant/types.hpp>
#include <smen/variant/memory.hpp>
#include <smen/lua/lua.hpp>
#include <iostream>
#include <optional>
#include <vector>

namespace smen {
    enum class VariantStorageMode {
        MOVED_FROM,

        PRIMITIVE_INT32,
        PRIMITIVE_UINT32,
        PRIMITIVE_INT64,
        PRIMITIVE_UINT64,
        PRIMITIVE_FLOAT32,
        PRIMITIVE_FLOAT64,
        PRIMITIVE_BOOLEAN,
        PRIMITIVE_STRING,
        
        HEAP
    };

    class Variant {
        friend class VariantAllocator;

    public:
        struct HashFunction {
            size_t operator ()(const Variant & variant) const;
        };

        class TypeMismatchException : public std::exception {
        private:
            std::string msg;
        public:
            TypeMismatchException(const VariantType & expected, const VariantType & actual);
            inline virtual const char * what() const noexcept(true) override {

                return msg.c_str();
            }
        };
    private:
        union {
            VariantEntryContent _content;
            int32_t _i32;
            uint32_t _u32;
            int64_t _i64;
            uint64_t _u64;
            float _f32;
            double _f64;
            bool _bool;
            std::string _str;
        };

        VariantStorageMode _storage_mode;
        VariantContainer * _container;
        VariantTypeID _type_id;

        void _sync_storage(const Variant & other);
        // returns whether any text has actually been written

    public:
        static Variant create(VariantContainer & allocator, const VariantType & type);
        static Variant create(VariantContainer & allocator, VariantTypeID type_id);
        static Variant create(VariantContainer & allocator, const std::string & type_name);

        inline VariantStorageMode storage_mode() const { return _storage_mode; }
        inline VariantTypeID type_id() const { return _type_id; }
        inline VariantTypeDirectory & dir() const { return _container->dir; }
        inline const VariantType & type() const { return _container->dir.resolve(_type_id); }
        inline VariantContainer & container() { return *_container; }

        inline bool is_root_object() const {
            if (_storage_mode != VariantStorageMode::HEAP) return false;
            auto ent = _content.entry(*_container);
            return ent.type_id == _type_id && ent.content().ptr() == _content.ptr();
        }

        Variant(const Variant & other);
        Variant(Variant && other);
        Variant & operator=(const Variant & other);
        Variant & operator=(Variant && other);

        Variant(VariantContainer & container, int32_t value);
        Variant(VariantContainer & container, uint32_t value);
        Variant(VariantContainer & container, int64_t value);
        Variant(VariantContainer & container, uint64_t value);
        Variant(VariantContainer & container, float value);
        Variant(VariantContainer & container, double value);
        Variant(VariantContainer & container, bool value);
        Variant(VariantContainer & container, const std::string & value);

        Variant(VariantContainer & container, const VariantEntryContent & content, VariantTypeID type_id);

        Variant(VariantContainer & container, const VariantReference & ref);
        
        Variant get_field(const std::string & field_name) const;
        Variant get_field(const VariantTypeField & field) const;
        std::optional<Variant> try_get_field(const std::string & field_name) const;
        void set_field(const std::string & field_name, const Variant & value);
        bool try_set_field(const std::string & field_name, const Variant & value);

        VariantEntryContent content_ptr() const;

        bool is_default_value() const;

        int32_t int32() const;
        void set_int32(int32_t value);
        uint32_t uint32() const;
        void set_uint32(uint32_t value);
        int64_t int64() const;
        void set_int64(int64_t value);
        uint64_t uint64() const;
        void set_uint64(uint64_t value);
        float float32() const;
        void set_float32(float value);
        double float64() const;
        void set_float64(double value);
        bool boolean() const;
        void set_boolean(bool value);
        const std::string & string() const;
        void set_string(const std::string & value);
        Variant variant() const;
        void set_variant(const Variant & value);
        VariantReference reference() const;
        void set_reference(VariantReference ref);

        size_t list_size() const;
        void list_clear();
        void list_add(const Variant & value);
        bool list_remove(size_t index);
        std::optional<Variant> list_get(size_t index) const;
        Variant & list_ref(size_t index) const;
        
        std::optional<VariantReference> reference_to() const;

        void write_contents(std::ostream & s) const;
        void write_string(std::ostream & s) const;
        std::string to_string() const;
        

        friend bool operator ==(const Variant & lhs, const Variant & rhs);
        friend bool operator !=(const Variant & lhs, const Variant & rhs);

        ~Variant();
    };

    inline std::ostream & operator << (std::ostream & s, const Variant & variant) {
        variant.write_string(s);
        return s;
    }
}

#endif//SMEN_VARIANT_VARIANT_HPP
