#include <smen/ecs/entity_container.hpp>

namespace smen {
    EntityContainer::EntityContainer()
    : _alloc_count(0)
    , _pos(0)
    , _entities()
    {}

    void EntityContainer::_move_to_first_empty_spot() {
        while (_pos < _entities.size() && _entities.at(_pos) != std::nullopt) {
            _pos += 1;
        }
    }
    
    EntityID EntityContainer::add_entity(const std::string & name) {
        if (_alloc_count == ALLOC_COUNT_TO_RECLAIM) {
            _alloc_count = 0;
            _pos = 0;
            _move_to_first_empty_spot();
        }

        if (_pos == _entities.size()) {
            _entities.emplace_back(std::nullopt);
        }

        auto new_id = EntityID(static_cast<uint32_t>(_pos));

        _entities[_pos] = Entity(name);
        _pos += 1;

        _alloc_count += 1;

        return new_id + 1; // 0 is an invalid ID
    }

    const Entity & EntityContainer::get_entity(EntityID id) const {
        if (id == 0) throw std::runtime_error("received entity ID 0 (get_entity const)");
        id -= 1;

        if (id < _entities.size()) {
            auto & ent = _entities[id];
            if (ent) {
                return *ent;
            }
        }
        throw std::runtime_error("no entity with ID " + std::to_string(id));
    }

    Entity & EntityContainer::get_entity(EntityID id) {
        if (id == 0) throw std::runtime_error("received entity ID 0 (get_entity non-const)");
        id -= 1;

        if (id < _entities.size()) {
            auto & ent = _entities[id];
            if (ent) {
                return *ent;
            }
        }
        throw std::runtime_error("no entity with ID " + std::to_string(id));
    }

    bool EntityContainer::remove_entity(EntityID ent_id) {
        if (ent_id == 0) throw std::runtime_error("received entity ID 0 (remove_entity)");
        ent_id -= 1;

        if (ent_id >= _entities.size()) return false;
        auto & ent_optional = _entities.at(ent_id);
        if (!ent_optional) return false;
        _entities[ent_id] = std::nullopt;
        return true;
    }

    bool EntityContainer::has_entity(EntityID ent_id) const {
        if (ent_id == 0) throw std::runtime_error("received entity ID 0 (has_entity)");
        ent_id -= 1;

        if (ent_id >= _entities.size()) return false;
        if (!_entities.at(ent_id)) return false;
        return true;
    }

    bool EntityContainer::is_valid_entity_slot(EntityID ent_id) const {
        if (ent_id == 0) return false;
        if (ent_id > _entities.size()) return false;
        return true;
    }

    EntityID EntityContainer::max_entity_id() const {
        return static_cast<EntityID>(_entities.size());
    }

    void EntityContainer::clear() {
        _entities.clear();
        _pos = 0;
    }
}
