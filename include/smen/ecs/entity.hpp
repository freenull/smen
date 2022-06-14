#ifndef SMEN_ECS_ENTITY_HPP
#define SMEN_ECS_ENTITY_HPP

#include <smen/variant/variant.hpp>
#include <smen/event.hpp>
#include <unordered_map>
#include <unordered_set>
#include <smen/ecs/entity_id.hpp>

namespace smen {
    class Entity {
    private:
        std::unordered_map<VariantTypeID, Variant> _component_by_type_map;
        std::unordered_set<Variant, Variant::HashFunction> _components;
        std::string _name;

    public:
        Entity(const std::string & name);
        Entity(const Entity &) = delete;
        Entity(Entity &&) = default;
        Entity & operator=(const Entity &) = delete;
        Entity & operator=(Entity &&) = default;

        inline const std::string & name() const { return _name; }
        inline void set_name(const std::string & new_name) { _name = new_name; }

        const std::unordered_set<Variant, Variant::HashFunction> & all_components() const;
        void add_component(const Variant & comp);
        bool remove_component(const Variant & comp);
        bool has_component(VariantTypeID type_id) const;
        std::optional<Variant> get_component(VariantTypeID type_id) const;
        std::vector<Variant> get_components(VariantTypeID type_id) const;
    };
}

#endif//SMEN_ECS_ENTITY_HPP
