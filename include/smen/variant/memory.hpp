#ifndef SMEN_VARIANT_MEMORY_HPP
#define SMEN_VARIANT_MEMORY_HPP

#include <smen/variant/types.hpp>

namespace smen {
    struct VariantEntryContent {
    public:
        struct HashFunction {
            size_t operator ()(const VariantEntryContent & content);
        };

    private:
        void * const _ptr;

    public:
        const VariantTypeID root_type_id;

        inline VariantEntryContent(void * ptr, VariantTypeID root_type_id)
        : _ptr(ptr)
        , root_type_id(root_type_id)
        {}

        inline void * ptr() const { return _ptr; }
        VariantEntry entry(const VariantContainer & alloc) const;
        VariantEntryContent of_field(const VariantTypeField & field) const;
    };

    struct VariantEntry;

    using VariantIndex = uint32_t;
    const VariantIndex INVALID_VARIANT_INDEX = UINT32_MAX;

    class VariantSingleTypeContainer {
        friend class VariantContainer;
    private:
        void * _ptr;
        size_t _alloc_counter;
        VariantIndex _pos;
        VariantIndex _capacity;
        VariantIndex _length;

        bool _enlarge();

    public:
        static const size_t REFCOUNTER_SIZE = sizeof(uint32_t);
        static const VariantIndex INITIAL_CAPACITY = 32;
        static const VariantIndex ENLARGE_FACTOR = 2;
        static const VariantIndex RECLAIM_THRESHOLD_FACTOR = 2;

        VariantTypeDirectory & dir;
        const VariantTypeID type_id;

        const bool requires_ctor;
        const bool requires_dtor;

        VariantSingleTypeContainer(VariantSingleTypeContainer &&) = default;
        VariantSingleTypeContainer(const VariantSingleTypeContainer &) = delete;
        VariantSingleTypeContainer(VariantTypeDirectory & dir, VariantTypeID type_id, bool requires_ctor, bool requires_dtor);

        const VariantType & type() const;
        size_t entry_size() const;
        VariantEntry entry_at(VariantIndex idx) const;
        VariantEntry entry_at_ptr(void * ptr) const;
        VariantIndex index_of(VariantEntry entry) const;
        VariantEntry alloc();
        void incref(VariantEntry ent);
        void decref(VariantEntry ent);
        std::optional<VariantReference> reference_of(VariantEntry ent);
        void reclaim();

        void debug_mem();

        ~VariantSingleTypeContainer();
    };

    struct VariantEntry {
    private:
        uint32_t * const _refcount;

    public:
        static const VariantEntry INVALID_ENTRY;
        const VariantTypeID type_id;

        VariantEntry(void * entry_ptr, VariantTypeID type_id);

        const VariantType & type(VariantTypeDirectory & dir) const;

        inline void * ptr() { return reinterpret_cast<void *>(_refcount); }
        inline uint32_t & refcount_field() { return *_refcount; }
        inline uint32_t refcount() {
            return refcount_field();
        }

        inline VariantEntryContent content() { 
            return VariantEntryContent(reinterpret_cast<void *>(reinterpret_cast<std::byte *>(_refcount) + VariantSingleTypeContainer::REFCOUNTER_SIZE), type_id);
        }
        bool valid() const;
        inline explicit operator bool() const { return valid(); }
    };

    class VariantContainer {
    private:
        mutable std::unordered_map<VariantTypeID, VariantSingleTypeContainer> _alloc_map;

        void _construct(const VariantType & type, VariantEntryContent content);
        void _destroy(const VariantType & type, VariantEntryContent content);
        void _determine_ctor_dtor_behavior(const VariantType & type, bool & requires_ctor, bool & requires_dtor) const;

    public:
        VariantTypeDirectory & dir;

        VariantContainer(const VariantContainer &) = delete;
        VariantContainer(VariantContainer &&) = default;
        explicit VariantContainer(VariantTypeDirectory & dir);
        VariantEntry entry_of_content(const VariantEntryContent & content) const;

        VariantSingleTypeContainer & get_container_of(VariantTypeID type_id) const;
        VariantEntry alloc(VariantTypeID type_id);
        VariantEntry entry_at(VariantTypeID type_id, VariantIndex idx) const;
        VariantIndex index_of(VariantEntry entry) const;
        void incref(VariantEntry ent);
        void decref(VariantEntry ent);
        VariantReference reference_of(VariantEntry ent);
        VariantEntry resolve(VariantReference ref);
    };
}

#endif//SMEN_VARIANT_MEMORY_HPP
