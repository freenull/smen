#ifndef SMEN_SCENE_HPP
#define SMEN_SCENE_HPP

#include <smen/renderer.hpp>
#include <smen/variant/variant.hpp>
#include <smen/lua/engine.hpp>
#include <smen/lua/script.hpp>
#include <smen/smen_library/variant.hpp>
#include <smen/ecs/entity_container.hpp>
#include <smen/ecs/system_query.hpp>
#include <smen/ecs/system.hpp>
#include <smen/event.hpp>

namespace smen {
    class SceneException : public std::runtime_error {
    public:
        inline SceneException(const std::string & msg)
            : std::runtime_error("error in scene: " + msg) {}
    };

    class SceneIterator {
    private:
        const EntityContainer & _container;
        EntityID _cur;

    public:
        using iterator_category = std::input_iterator_tag;
        using difference_type = EntityID;
        using value_type = EntityID;
        using pointer = value_type *;
        using reference = const value_type &;

        SceneIterator(const EntityContainer & container);
        SceneIterator(const EntityContainer & container, EntityID cur);

        inline reference operator*() const { return _cur; }
        inline pointer operator->() { return &_cur; }

        SceneIterator & operator++();
        SceneIterator & operator++(int);

        inline friend bool operator ==(const SceneIterator & lhs, const SceneIterator & rhs) {
            return &lhs._container == &rhs._container && lhs._cur == rhs._cur;
        }
    };

    class Scene {
        // events must appear before any members that
        // may contain EventHolders of these events,
        // as class members are destroyed in reverse order
        //
        // in our case system_container may contain Systems
        // that have EventHolders in on_process and on_render
        //
        // safest to just put the events first, since they
        // have a no-param initializer anyway so we don't
        // have to worry about them
    public:
        Event<Scene, EntityID> entity_added;
        Event<Scene, EntityID, Variant &> component_added;
        Event<Scene, EntityID, Variant &> component_removed;
        Event<Scene, EntityID> entity_removed;
        Event<Scene, double> on_process;
        Event<Scene> on_render;

    private:
        Logger logger;
        VariantTypeDirectory * _dir;
        VariantContainer _variant_container;
        EntityContainer _entity_container;
        SystemContainer _system_container;
        LuaEngine _lua_engine;
        LuaVariantLibrary _lua_smen_lib;
        EventHolder _ev;
        bool _enabled;
        size_t _total_entity_count;

        static const std::vector<EntityID> _empty_id_vec;


        std::unordered_map<std::string, LuaScript> _script_map;
        std::unordered_map<EntityID, std::vector<EntityID>> _child_map;
        std::unordered_map<EntityID, EntityID> _parent_map;
        
        std::string _name;

        void _debug_hierarchy(std::ostream & s, unsigned int indent, EntityID id);

    public:
        Scene(const std::string & name, VariantTypeDirectory & variant_type_dir);
        
        inline const std::string & name() const { return _name; }
        inline void set_name(const std::string & name) { _name = name; }

        inline bool enabled() const { return _enabled; }
        inline void enable() { _enabled = true; }
        inline void disable() { _enabled = true; }

        inline const VariantContainer & variant_container() const { return _variant_container; }
        inline const EntityContainer & entity_container() const { return _entity_container; }
        inline const SystemContainer & system_container() const { return _system_container; }
        inline const VariantTypeDirectory & dir() const { return *_dir; }
        inline const LuaEngine & lua_engine() const { return _lua_engine; }
        inline LuaVariantLibrary & smen_library() { return _lua_smen_lib; }

        inline size_t total_entity_count() const { return _total_entity_count; }

        inline VariantContainer & variant_container() { return _variant_container; }
        inline EntityContainer & entity_container() { return _entity_container; }
        inline SystemContainer & system_container() { return _system_container; }
        inline VariantTypeDirectory & dir() { return *_dir; }

        EntityID spawn(const std::string & name = "");
        bool is_valid_entity_id(EntityID id);
        Variant add_component(EntityID id, const std::string & name);
        void add_component(EntityID id, Variant & variant);
        bool has_component(EntityID id, VariantTypeID type_id) const;
        bool has_component(EntityID id, const std::string & name) const;

        std::vector<EntityID> entities_with_component(VariantTypeID type_id) const;
        std::vector<EntityID> entities_with_component(const std::string & name) const;
        std::optional<Variant> get_component(EntityID id, VariantTypeID type_id) const;
        std::optional<Variant> get_component(EntityID id, const std::string & name) const;
        std::vector<Variant> get_components(EntityID id, VariantTypeID type_id) const;
        std::vector<Variant> get_components(EntityID id, const std::string & name) const;
        std::vector<Variant> get_components(EntityID id) const;

        const std::vector<EntityID> & children_of(EntityID id) const;
        bool has_children(EntityID id) const;
        std::optional<EntityID> parent_of(EntityID id) const;
        bool is_child_of(EntityID parent_id, EntityID child_id) const;
        bool has_parent(EntityID child_id) const;

        void adopt_child(EntityID parent_id, EntityID child_id);
        EntityID spawn_child(EntityID parent_id, const std::string & name = "");
        void make_orphan(EntityID id);

        bool remove_component(EntityID id, Variant & variant);
        bool remove_component(EntityID id, VariantTypeID type_id);
        bool remove_component(EntityID id, const std::string & name);

        EntityID find_first_by_name(const std::string & name) const;
        std::vector<EntityID> find_by_name(const std::string & name) const;
        const std::string & name_of(EntityID id) const;
        void set_name(EntityID id, const std::string & name);

        bool system_query_matches(EntityID id, const SystemQuery & query) const;
        void kill(EntityID id);

        void new_script(const std::string & id);
        void link_script(const std::string & id, const std::string & new_path);
        void delete_script(const std::string & id);
        bool has_script(const std::string & id) const;
        void reload_script(const std::string & id);
        void reload_all_scripts();
        const std::unordered_map<std::string, LuaScript> & scripts() const;

        SceneIterator begin() const;
        SceneIterator end() const;

        SystemQuery system_query();

        void clear();

        void debug_hierarchy(std::ostream & s, EntityID id);

        void process(double delta);
        void render();
    };
}

#endif//SMEN_SCENE_HPP
