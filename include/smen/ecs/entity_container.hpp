#ifndef SMEN_ECS_ENTITY_CONTAINER_HPP
#define SMEN_ECS_ENTITY_CONTAINER_HPP

#include <vector>
#include <optional>
#include <smen/ecs/entity.hpp>
#include <smen/ecs/entity_id.hpp>
#include <smen/event.hpp>

namespace smen {
    class EntityContainer {
    private:
        static const size_t ALLOC_COUNT_TO_RECLAIM = 64;
        size_t _alloc_count;
        size_t _pos;
        std::vector<std::optional<Entity>> _entities;
        void _move_to_first_empty_spot();

    public:
        EntityContainer();

        EntityID add_entity(const std::string & name);
        const Entity & get_entity(EntityID id) const;
        Entity & get_entity(EntityID id);
        bool remove_entity(EntityID id);
        bool has_entity(EntityID id) const;
        bool is_valid_entity_slot(EntityID id) const;
        EntityID max_entity_id() const;
        void clear();
    };
}

#endif//SMEN_ECS_ENTITY_CONTAINER_HPP
