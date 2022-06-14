#include <smen/ecs/scene.hpp>
#include <smen/lua/script.hpp>
#include <smen/ecs/system.hpp>
#include <smen/lua/native_types.hpp>
#include <algorithm>

namespace smen {
    SceneIterator::SceneIterator(const EntityContainer & container)
    : _container(container)
    , _cur(1)
    {}

    SceneIterator::SceneIterator(const EntityContainer & container, EntityID cur)
    : _container(container)
    , _cur(cur)
    {}

    SceneIterator & SceneIterator::operator++() {
        _cur += 1;

        while (_cur <= _container.max_entity_id() && !_container.has_entity(_cur)) {
            _cur += 1;
        }

        return *this;
    }

    SceneIterator & SceneIterator::operator++(int) {
        operator++();
        return *this;
    }

    const std::vector<EntityID> Scene::_empty_id_vec = std::vector<EntityID>();

    Scene::Scene(const std::string & name, VariantTypeDirectory & variant_type_dir)
    : logger(make_logger("Scene " + name))
    , _dir(&variant_type_dir)
    , _variant_container(variant_type_dir)
    , _entity_container()
    , _system_container()
    , _lua_engine()
    , _lua_smen_lib(_variant_container, _lua_engine)
    , _ev()
    , _enabled(true)
    , _total_entity_count(0)
    , _child_map()
    , _parent_map()
    , _name(name)
    {
        _ev.add(_lua_smen_lib.post_load, [&](LuaVariantLibrary & variant_lib, LuaEngine & engine) {
            variant_lib.set_current_scene(this);
        });

        _lua_smen_lib.init_preloader();
    }

    EntityID Scene::spawn(const std::string & name) {
        auto new_id = _entity_container.add_entity(name);
        entity_added(*this, new_id);
        _total_entity_count += 1;
        return new_id;
    }

    bool Scene::is_valid_entity_id(EntityID id) {
        return _entity_container.is_valid_entity_slot(id) && _entity_container.has_entity(id);
    }

    Variant Scene::add_component(EntityID id, const std::string & name) {
        auto & ent = _entity_container.get_entity(id);
        auto & type = _variant_container.dir.resolve(name);

        if (!type.valid()) {
            throw SceneException("attempted to add component of type '" + name + "' to entity with ID " + std::to_string(id) + ", but the type doesn't exist");
        }

        if (type.category != VariantTypeCategory::COMPONENT) {
            throw std::runtime_error("type '" + name + "' is not a component type");
        }
        auto variant = Variant::create(_variant_container, type.id);
        ent.add_component(variant);
        component_added(*this, id, variant);
        return variant;
    }

    void Scene::add_component(EntityID id, Variant & variant) {
        auto & ent = _entity_container.get_entity(id);
        ent.add_component(variant);
        component_added(*this, id, variant);
    }

    bool Scene::has_component(EntityID id, VariantTypeID type_id) const {
        auto & ent = _entity_container.get_entity(id);
        return ent.has_component(type_id);
    }

    bool Scene::has_component(EntityID id, const std::string & name) const {
        return has_component(id, _dir->resolve(name).id);
    }


    std::vector<EntityID> Scene::entities_with_component(VariantTypeID type_id) const {
        auto vec = std::vector<EntityID>();
        for (auto & ent_id : *this) {
            auto comp = get_component(ent_id, type_id);
            if (comp) {
                vec.emplace_back(ent_id);
            }
        }
        return vec;
    }

    std::vector<EntityID> Scene::entities_with_component(const std::string & name) const {
        auto & type = _dir->resolve(name);
        if (!type.valid()) {
            throw SceneException("attempted to get entities with components of type '" + name + "', but the type doesn't exist");
        }
        return entities_with_component(type.id);
    }

    std::optional<Variant> Scene::get_component(EntityID id, VariantTypeID type_id) const {
        auto & ent = _entity_container.get_entity(id);
        return ent.get_component(type_id);
    }

    std::optional<Variant> Scene::get_component(EntityID id, const std::string & name) const {
        auto & type = _dir->resolve(name);
        if (!type.valid()) {
            throw SceneException("attempted to get component of type '" + name + "' on entity with ID " + std::to_string(id) + ", but the type doesn't exist");
        }
        return get_component(id, type.id);
    }

    std::vector<Variant> Scene::get_components(EntityID id, VariantTypeID type_id) const {
        auto & ent = _entity_container.get_entity(id);
        return ent.get_components(type_id);
    }

    std::vector<Variant> Scene::get_components(EntityID id, const std::string & name) const {
        auto & type = _dir->resolve(name);
        if (!type.valid()) {
            throw SceneException("attempted to get components of type '" + name + "' on entity with ID " + std::to_string(id) + ", but the type doesn't exist");
        }
        return get_components(id, type.id);
    }
    
    std::vector<Variant> Scene::get_components(EntityID id) const {
        auto vec = std::vector<Variant>();
        for (auto & comp : _entity_container.get_entity(id).all_components()) {
            vec.emplace_back(comp);
        }
        return vec;
    }

    bool Scene::remove_component(EntityID id, Variant & variant) {
        auto & ent = _entity_container.get_entity(id);
        if (ent.remove_component(variant)) {
            component_removed(*this, id, variant);
            return true;
        }
        return false;
    }

    const std::vector<EntityID> & Scene::children_of(EntityID id) const {
        auto it = _child_map.find(id);
        if (it == _child_map.end()) return _empty_id_vec;
        return it->second;
    }

    bool Scene::has_children(EntityID id) const {
        return _child_map.contains(id);
    }

    std::optional<EntityID> Scene::parent_of(EntityID id) const {
        auto it = _parent_map.find(id);
        if (it == _parent_map.end()) return std::nullopt;
        return it->second;
    }

    bool Scene::is_child_of(EntityID parent_id, EntityID child_id) const {
        auto it = _parent_map.find(child_id);
        if (it == _parent_map.end()) return false;
        return it->second == parent_id;
    }

    bool Scene::has_parent(EntityID child_id) const {
        return _parent_map.contains(child_id);
    }

    void Scene::adopt_child(EntityID parent_id, EntityID child_id) {
        auto parent_it = _parent_map.find(child_id);
        if (parent_it != _parent_map.end()) {
            // child already has a parent
            // we need to remove it from its parent's list of children

            auto prev_parent_id = parent_it->second;

            auto prev_parent_children_it = _child_map.find(prev_parent_id);

            if (prev_parent_children_it != _child_map.end()) {
                std::erase(prev_parent_children_it->second, child_id);
            }
        }

        _parent_map[child_id] = parent_id;

        auto child_vec_it = _child_map.find(parent_id);
        if (child_vec_it == _child_map.end()) {
            _child_map.emplace(parent_id, std::vector<EntityID>{ child_id });
            return;
        }

        child_vec_it->second.emplace_back(child_id);
    }

    EntityID Scene::spawn_child(EntityID parent_id, const std::string & name) {
        auto child_id = spawn(name);
        adopt_child(parent_id, child_id);
        return child_id;
    }

    void Scene::make_orphan(EntityID id) {
        auto parent_it = _parent_map.find(id);
        if (parent_it != _parent_map.end()) {
            // see adopt_child
            
            auto prev_parent_id = parent_it->second;

            auto prev_parent_children_it = _child_map.find(prev_parent_id);
            if (prev_parent_children_it != _child_map.end()) {
                std::erase(prev_parent_children_it->second, id);
            }
        }

        _parent_map.erase(id);
    }

    bool Scene::remove_component(EntityID id, VariantTypeID type_id) {
        auto & ent = _entity_container.get_entity(id);
        auto maybe_variant = ent.get_component(type_id);
        if (!maybe_variant) return false;
        if (ent.remove_component(*maybe_variant)) {
            component_removed(*this, id, *maybe_variant);
            return true;
        }
        return false;
    }

    bool Scene::remove_component(EntityID id, const std::string & name) {
        return remove_component(id, _dir->resolve(name).id);
    }

    std::vector<EntityID> Scene::find_by_name(const std::string & name) const {
        auto ents = std::vector<EntityID>();
        for (auto ent_id : *this) {
            if (name_of(ent_id) == name) {
                ents.emplace_back(ent_id);
            }
        }
        return ents;
    }

    EntityID Scene::find_first_by_name(const std::string & name) const {
        for (auto ent_id : *this) {
            if (name_of(ent_id) == name) return ent_id;
        }

        return 0;
    }

    const std::string & Scene::name_of(EntityID id) const {
        auto & ent = _entity_container.get_entity(id);
        return ent.name();
    }

    void Scene::set_name(EntityID id, const std::string & name) {
        auto & ent = _entity_container.get_entity(id);
        ent.set_name(name);
    }

    bool Scene::system_query_matches(EntityID id, const SystemQuery & query) const {
        for (auto & type_id : query.required_type_ids()) {
            if (!has_component(id, type_id)) return false;
        }
        return true;
    }

    void Scene::kill(EntityID id) {
        if (_entity_container.remove_entity(id)) {
            _total_entity_count -= 1;
            entity_removed(*this, id);
        }
    }

    void Scene::new_script(const std::string & id) {
        if (_script_map.contains(id)) throw SceneException("script with '" + id + "' already exists, use relink_script to change its path");
        _script_map.emplace(id, LuaScript(_lua_engine));
        logger.debug("created new unlinked script '", id, "'");
    }

    void Scene::link_script(const std::string & id, const std::string & new_path) {
        if (!_script_map.contains(id)) throw SceneException("script with '" + id + "' doesn't exist, use new_script to create a new script");

        auto & script = _script_map.at(id);
        auto old_path = script.path();

        auto result = script.load(new_path);
        if (result.fail()) {
            throw SceneException("failed loading script at '" + new_path + "': " + result.error_msg());
        }

        if (old_path.size() == 0) {
            logger.debug("linked script '", id, "' to '", new_path, "'");
        } else {
            logger.debug("relinked script '", id, "' from '", old_path, "' to '", new_path, "'");
        }
    }

    void Scene::delete_script(const std::string & id) {
        if (!_script_map.contains(id)) throw SceneException("script with '" + id + "' doesn't exist");
        _script_map.erase(id);
    }

    bool Scene::has_script(const std::string & id) const {
        return _script_map.contains(id);
    }

    void Scene::reload_script(const std::string & id) {
        if (!_script_map.contains(id)) throw SceneException("script with '" + id + "' doesn't exist, use new_script to create a new script");
        _script_map.at(id).reload();
        logger.debug("reloaded script '", id, "' from '", _script_map.at(id).path());
    }

    void Scene::reload_all_scripts() {
        logger.debug("reloading all scripts");
        for (auto & pair : _script_map){
            pair.second.reload();
        }
    }

    const std::unordered_map<std::string, LuaScript> & Scene::scripts() const {
        return _script_map;
    }

    SceneIterator Scene::begin() const {
        return SceneIterator(_entity_container, 1);
    }

    SceneIterator Scene::end() const {
        return SceneIterator(_entity_container, _entity_container.max_entity_id() + 1);
    }

    SystemQuery Scene::system_query() {
        return SystemQuery(*_dir);
    }

    void Scene::_debug_hierarchy(std::ostream & s, unsigned int indent, EntityID id) {
        if (indent >= 1) {
            for (unsigned int i = 0; i < indent - 1; i++) {
                s << "   ";
            }
            s << "|- ";
        }
        auto parent_id = parent_of(id);
        s << name_of(id) << " (" << (parent_id ? name_of(*parent_id) : "root") << ")" << "\n";;
        auto & children = children_of(id);
        for (auto & child : children) {
            _debug_hierarchy(s, indent + 1, child);
        }
    }

    void Scene::clear() {
        _system_container.clear();
        _entity_container.clear();
        _total_entity_count = 0;
        _script_map.clear();
    }

    void Scene::debug_hierarchy(std::ostream & s, EntityID id) {
        _debug_hierarchy(s, 0, id);
    }

    void Scene::process(double delta) {
        if (!_enabled) return;
        on_process(*this, delta);
    }

    void Scene::render() {
        if (!_enabled) return;
        on_render(*this);
    }
}
