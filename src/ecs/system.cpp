#include <smen/ecs/system.hpp>
#include <smen/ecs/scene.hpp>

namespace smen {
    System::System(const SystemContainer & container, const std::string & name, const SystemQuery & query)
    : _query(query)
    , _matching_entities()
    , _ev()
    , _container(container)
    , name(name)
    , enabled(true)
    , logger(make_logger("System " + name))
    {}

    void System::initialize_events_in(Scene & scene) {
        _ev.add(scene.entity_added, [&](const Scene & scene, EntityID entity_id) {
            if (scene.system_query_matches(entity_id, _query)) {
                _matching_entities.emplace_back(entity_id);
            }
        });

        _ev.add(scene.component_added, [&](const Scene & scene, EntityID entity_id, Variant & component) {
            if (scene.system_query_matches(entity_id, _query) && std::find(_matching_entities.begin(), _matching_entities.end(), entity_id) == _matching_entities.end()) {
                _matching_entities.emplace_back(entity_id);
            }
        });

        _ev.add(scene.component_removed, [&](const Scene & scene, EntityID entity_id, Variant & component) {
            std::erase(_matching_entities, entity_id);
        });

        _ev.add(scene.entity_removed, [&](const Scene & scene, EntityID entity_id) {
            std::erase(_matching_entities, entity_id);
        });

        _ev.add(scene.on_process, [&](Scene & scene, double delta) {
            if (!enabled) return;
            if (process && _matching_entities.size() > 0) {
                auto entities_copy = _matching_entities;
                for (auto & ent_id : entities_copy) {
                    _container.process_db[process](scene, ent_id, delta);
                }
            }
        });

        _ev.add(scene.on_render, [&](Scene & scene) {
            if (!enabled) return;
            if (render && _matching_entities.size() > 0) {
                auto entities_copy = _matching_entities;
                for (auto & ent_id : entities_copy) {
                    _container.render_db[render](scene, ent_id);
                }
            }
        });

        for (auto & ent_id : scene) {
            if (scene.system_query_matches(ent_id, _query)) {
                _matching_entities.emplace_back(ent_id);
            }
        }
    }

    System & SystemContainer::make_system(const std::string & name, const SystemQuery & query) {
        _systems.emplace(name, System(*this, name, query));
        return _systems.at(name);
    }

    bool SystemContainer::has_system(const std::string & name) {
        return _systems.contains(name);
    }

    System & SystemContainer::get_system(const std::string & name) {
        auto it = _systems.find(name);
        if (it == _systems.end()) {
            throw SystemException("system doesn't exist: '" + name + "'");
        }
        return it->second;
    }

    void SystemContainer::clear() {
        _systems.clear();
    }
}
