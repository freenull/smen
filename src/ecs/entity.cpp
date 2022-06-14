#include <smen/ecs/entity.hpp>
#include <smen/ecs/entity_id.hpp>
#include <smen/ser/serialization.hpp>

namespace smen {
    Entity::Entity(const std::string & name)
    : _component_by_type_map()
    , _components()
    , _name(name)
    {}

    const std::unordered_set<Variant, Variant::HashFunction> & Entity::all_components() const {
        return _components;
    }

    void Entity::add_component(const Variant & comp) {
        _components.emplace(comp);
        _component_by_type_map.emplace(comp.type_id(), comp);
    }

    bool Entity::remove_component(const Variant & comp) {
        auto type_map_it = _component_by_type_map.find(comp.type_id());
        if (type_map_it != _component_by_type_map.end()) {
            _component_by_type_map.erase(type_map_it);
        }

        auto set_it = _components.find(comp);
        if (set_it != _components.end()) {
            _components.erase(set_it);
            return true;
        }

        return false;
    }

    bool Entity::has_component(VariantTypeID type_id) const {
        auto it = _component_by_type_map.find(type_id);
        if (it == _component_by_type_map.end()) return false;
        return true;
    }

    std::optional<Variant> Entity::get_component(VariantTypeID type_id) const {
        auto it = _component_by_type_map.find(type_id);
        if (it == _component_by_type_map.end()) return std::nullopt;
        return it->second;
    }

    std::vector<Variant> Entity::get_components(VariantTypeID type_id) const {
        std::vector<Variant> components;
        for (auto & comp : _components) {
            if (comp.type_id() == type_id) components.emplace_back(comp);
        }
        return components;
    }
}
