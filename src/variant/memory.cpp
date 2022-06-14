#include <smen/variant/memory.hpp>
#include <smen/variant/variant.hpp>
#include <smen/ser/serialization.hpp>
#include <iostream>
#include <iomanip>
#include <vector>

namespace smen {
    const VariantEntry VariantEntry::INVALID_ENTRY = VariantEntry(nullptr, INVALID_VARIANT_TYPE_ID);

    VariantEntry::VariantEntry(void * entry_ptr, VariantTypeID type_id)
    : _refcount(reinterpret_cast<uint32_t *>(entry_ptr))
    , type_id(type_id)
    {}

    bool VariantEntry::valid() const {
        return type_id != INVALID_VARIANT_TYPE_ID;
    }

    size_t VariantEntryContent::HashFunction::operator ()(const VariantEntryContent & content) {
        return std::hash<uintptr_t>()(reinterpret_cast<uintptr_t>(content._ptr));
    }

    VariantEntry VariantEntryContent::entry(const VariantContainer & alloc) const {
        return alloc.entry_of_content(*this);
    }

    VariantEntryContent VariantEntryContent::of_field(const VariantTypeField & field) const {
        return VariantEntryContent(reinterpret_cast<std::byte *>(_ptr) + field.offset_bytes, root_type_id);
    }

    const VariantType & VariantEntry::type(VariantTypeDirectory & dir) const {
        return dir.resolve(type_id);
    }

    void VariantContainer::_determine_ctor_dtor_behavior(const VariantType & type, bool & requires_ctor, bool & requires_dtor) const {
        if (type.ctor && type.dtor) {
            requires_ctor = true;
            requires_dtor = true;
        } else {
            if (type.ctor) requires_ctor = true;
            if (type.dtor) requires_dtor = true;
            for (auto & pair : type.fields()) {
                auto field_type = dir.resolve(pair.second.type_id);
                _determine_ctor_dtor_behavior(field_type, requires_ctor, requires_dtor);
            }
        }
    }

    void VariantContainer::_construct(const VariantType & type, VariantEntryContent content) {
        if (type.is_compound_type()) {
            // default ctor (all fields)
            for (auto & pair : type.fields()) {
                _construct(dir.resolve(pair.second.type_id), content.of_field(pair.second));
            }
        }

        if (type.ctor) {
            dir.ctor_db[type.ctor](*this, type, content);
        }

        return;

        if (type.category == VariantTypeCategory::REFERENCE) {
            VariantReference::NULL_REFERENCE.write(content.ptr());
        } else if (type.category == VariantTypeCategory::LIST) {
            std::construct_at(reinterpret_cast<std::vector<VariantIndex> *>(content.ptr()));
        } else {
            for (auto & pair : type.fields()) {
                auto & field_type = dir.resolve(pair.second.type_id);
                if (field_type.category == VariantTypeCategory::COMPLEX) {
                    _construct(field_type, content.of_field(pair.second));
                }
            }
        }
    }

    void VariantContainer::_destroy(const VariantType & type, VariantEntryContent content) {
        if (type.dtor) {
            dir.dtor_db[type.dtor](*this, type, content);
        }

        if (type.is_compound_type()) {
            // default dtor (all fields)
            for (auto & pair : type.fields()) {
                _destroy(dir.resolve(pair.second.type_id), content.of_field(pair.second));
            }
        }
        return;

        if (type.category == VariantTypeCategory::REFERENCE) {
            auto ref_value = VariantReference::read(type.element_type_id, content.ptr());
            if (ref_value) {
                auto & type_alloc = get_container_of(ref_value.type_id);
                decref(type_alloc.entry_at(ref_value.index));
            }
        } else if (type.category == VariantTypeCategory::LIST) {
            auto * vec_ptr = reinterpret_cast<std::vector<VariantIndex> *>(content.ptr());
            for (size_t i = 0; i < vec_ptr->size(); i++) {
                auto ent = entry_at(type.element_type_id, vec_ptr->at(i));
                decref(ent);
            }

            std::destroy_at(vec_ptr);
        } else if (type.category == VariantTypeCategory::STRING) {
            auto * str_ptr = reinterpret_cast<std::string *>(content.ptr());
            std::destroy_at(str_ptr);
        } else if (type.category == VariantTypeCategory::COMPLEX) {
            for (auto & pair : type.fields()) {
                _destroy(dir.resolve(pair.second.type_id), content.of_field(pair.second));
            }
        }
    }

    VariantSingleTypeContainer::VariantSingleTypeContainer(VariantTypeDirectory & dir, VariantTypeID type_id, bool requires_ctor, bool requires_dtor)
    : _ptr(nullptr)
    , _alloc_counter(0)
    , _pos(0)
    , _capacity(0)
    , _length(0)
    , dir(dir)
    , type_id(type_id)
    , requires_ctor(requires_ctor)
    , requires_dtor(requires_dtor)
    {}

    bool VariantSingleTypeContainer::_enlarge() {
        size_t new_capacity;

        if (_capacity == 0) new_capacity = INITIAL_CAPACITY;
        else new_capacity = _capacity * ENLARGE_FACTOR;

        void * new_ptr = realloc(_ptr, new_capacity * entry_size());
        if (new_ptr == nullptr) return false;
        _ptr = new_ptr;
        _capacity = new_capacity;
        return true;
    }

    const VariantType & VariantSingleTypeContainer::type() const {
        return dir.resolve(type_id);
    }

    size_t VariantSingleTypeContainer::entry_size() const {
        return REFCOUNTER_SIZE + type().size(dir);
    }

    VariantEntry VariantSingleTypeContainer::entry_at(VariantIndex idx) const {
        if (idx >= _length) return VariantEntry::INVALID_ENTRY;
        void * entry_ptr = reinterpret_cast<std::byte *>(_ptr) + (entry_size() * idx);
        return VariantEntry(entry_ptr, type_id);
    }

    VariantEntry VariantSingleTypeContainer::entry_at_ptr(void * ptr) const {
        if (ptr < _ptr || ptr >= (reinterpret_cast<std::byte *>(_ptr) + _length * entry_size())) {
            return VariantEntry::INVALID_ENTRY;
        }

        ptrdiff_t relative = reinterpret_cast<std::byte *>(ptr) - reinterpret_cast<std::byte *>(_ptr);
        VariantIndex entry_idx = static_cast<VariantIndex>(static_cast<size_t>(relative) / entry_size());
        return entry_at(entry_idx);
    }

    VariantIndex VariantSingleTypeContainer::index_of(VariantEntry entry) const {
        if (entry.ptr() < _ptr || entry.ptr() >= (reinterpret_cast<std::byte *>(_ptr) + _length * entry_size())) {
            return INVALID_VARIANT_INDEX;
        }

        ptrdiff_t relative = reinterpret_cast<std::byte *>(entry.ptr()) - reinterpret_cast<std::byte *>(_ptr);
        VariantIndex entry_idx = static_cast<VariantIndex>(static_cast<size_t>(relative) / entry_size());
        return entry_idx;
    }

    VariantEntry VariantSingleTypeContainer::alloc() {
        if (_alloc_counter == _capacity / RECLAIM_THRESHOLD_FACTOR) {
            _alloc_counter = 0;
            reclaim();
        } else _alloc_counter += 1;

        while (_pos < _length && entry_at(_pos).refcount() > 0) {
            _pos += 1;
        }

        if (_pos == _capacity) {
            _enlarge();
        }

        if (_pos == _length) {
            _length += 1;
        }

        auto entry = entry_at(_pos);
        std::byte * entry_ptr = reinterpret_cast<std::byte *>(entry.ptr());


        std::fill(entry_ptr, entry_ptr + entry_size(), static_cast<std::byte>(0));

        return entry;
    }

    void VariantSingleTypeContainer::incref(VariantEntry ent) {
        ent.refcount_field() += 1;
    }

    void VariantSingleTypeContainer::decref(VariantEntry ent) {
        if (ent.refcount() > 0) {
            ent.refcount_field() -= 1;
        }
    }

    std::optional<VariantReference> VariantSingleTypeContainer::reference_of(VariantEntry ent) {
        auto * ptr = ent.ptr();
        if (ptr < _ptr || ptr >= (reinterpret_cast<std::byte *>(_ptr) + _length * entry_size())) {
            return std::nullopt;
        }

        ptrdiff_t relative = reinterpret_cast<std::byte *>(ptr) - reinterpret_cast<std::byte *>(_ptr);
        size_t entry_idx = static_cast<size_t>(relative) / entry_size();
        return VariantReference(ent.type_id, entry_idx);
    }

    void VariantSingleTypeContainer::reclaim() {
        _pos = 0;
    }

    void VariantSingleTypeContainer::debug_mem() {
        std::cout << "CAPACITY: " << _capacity << "\n";
        std::cout << "CAPACITY (bytes): " << (_capacity * entry_size()) << "\n";
        std::cout << "LENGTH: " << _length << "\n";
        std::cout << "LENGTH (bytes): " << (_length * entry_size()) << "\n";

        std::byte * ptr = reinterpret_cast<std::byte *>(_ptr);
        for (size_t i = 0; i < _length * entry_size(); i++) {
            if (i % entry_size() == 0) std::cout << "[";

            std::cout << std::hex << std::setfill('0') << std::setw(2) << static_cast<int>(ptr[i]);
            if (i % entry_size() == sizeof(int32_t) - 1) std::cout << "]";
            std::cout << " ";
            if (i % 4 == 3) std::cout << "   ";
        }
        std::cout << "\n";
    }

    VariantSingleTypeContainer::~VariantSingleTypeContainer() {
        if (_ptr != nullptr) free(_ptr);
    }

    VariantContainer::VariantContainer(VariantTypeDirectory & dir)
    : dir(dir)
    {}

    VariantEntry VariantContainer::entry_of_content(const VariantEntryContent & content) const {
        if (dir.resolve(content.root_type_id).is_singleton_type()) return VariantEntry(nullptr, content.root_type_id);

        auto & alloc = get_container_of(content.root_type_id);
        return alloc.entry_at_ptr(content.ptr());
    }

    VariantSingleTypeContainer & VariantContainer::get_container_of(VariantTypeID type_id) const {
        auto it = _alloc_map.find(type_id);
        if (it != _alloc_map.end()) {
            return it->second;
        }

        auto & type = dir.resolve(type_id);
        if (type.is_singleton_type()) throw std::runtime_error("get_container_of on singleton type");
        bool requires_ctor = false;
        bool requires_dtor = false;
        _determine_ctor_dtor_behavior(type, requires_ctor, requires_dtor);


        auto alloc = VariantSingleTypeContainer(dir, type_id, requires_ctor, requires_dtor);
        _alloc_map.emplace(type_id, std::move(alloc));
        return _alloc_map.at(type_id);
    }

    VariantEntry VariantContainer::alloc(VariantTypeID type_id) {
        auto & type = dir.resolve(type_id);
        if (type.is_singleton_type()) return VariantEntry(nullptr, type_id);

        auto & alloc = get_container_of(type_id);
        auto ent = alloc.alloc();

        if (alloc.requires_ctor) {
            _construct(dir.resolve(type_id), ent.content());
        }

        return ent;
    }

    VariantEntry VariantContainer::entry_at(VariantTypeID type_id, VariantIndex idx) const {
        // 0 size types have a single static entry
        if (dir.resolve(type_id).is_singleton_type()) return VariantEntry(nullptr, type_id);

        return get_container_of(type_id).entry_at(idx);
    }

    VariantIndex VariantContainer::index_of(VariantEntry entry) const {
        if (entry.ptr() == nullptr) return 0;

        return get_container_of(entry.type_id).index_of(entry);
    }
}
#include <stdio.h>
#include <execinfo.h>
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>

namespace smen {
    std::unordered_map<void *, size_t> counter;
    size_t count = 0;

    void VariantContainer::incref(VariantEntry ent) {
        if (ent.ptr() == nullptr) return;
        get_container_of(ent.type_id).incref(ent);
    }

    void VariantContainer::decref(VariantEntry ent) {
        if (ent.ptr() == nullptr) return;

        auto & alloc = get_container_of(ent.type_id);

        if (ent.refcount() == 1 && alloc.requires_dtor) {
            _destroy(dir.resolve(ent.type_id), ent.content());
        }

        alloc.decref(ent);
    }

    VariantReference VariantContainer::reference_of(VariantEntry ent) {
        if (ent.ptr() == nullptr) {
            // non-unique reference
            auto ref = VariantReference(ent.type_id, 0);
            return ref;
        }

        auto & alloc = get_container_of(ent.type_id);
        return *alloc.reference_of(ent);
    }

    VariantEntry VariantContainer::resolve(VariantReference ref) {
        if (dir.resolve(ref.type_id).is_singleton_type()) return VariantEntry(nullptr, ref.type_id);
        return get_container_of(ref.type_id).entry_at(ref.index);
    }
}
