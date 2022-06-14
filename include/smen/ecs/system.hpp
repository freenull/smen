#ifndef SMEN_ECS_SYSTEM_HPP
#define SMEN_ECS_SYSTEM_HPP

#include <smen/variant/variant.hpp>
#include <smen/ecs/system_query.hpp>
#include <smen/ecs/entity_id.hpp>
#include <smen/event.hpp>
#include <smen/logger.hpp>
#include <vector>
#include <set>

namespace smen {
    class Scene;

    class SystemException : public std::runtime_error {
    public:
        inline SystemException(const std::string & msg)
            : std::runtime_error(msg) {}
    };

    using SystemProcessFunction = std::function<void (Scene &, EntityID, double)>;
    using SystemRenderFunction = std::function<void (Scene &, EntityID)>;

    class SystemContainer;

    class System {
    private:
        SystemQuery _query;
        std::vector<EntityID> _matching_entities;

        EventHolder _ev;
        const SystemContainer & _container;

    public:
        std::string name;
        bool enabled;
        const Logger logger;

        System(const System &) = delete;
        System & operator =(const System &) = delete;
        System(System &&) = default;
        System & operator =(System &&) = default;

        System(const SystemContainer & container, const std::string & name, const SystemQuery & query);

        ObjectDatabaseID<SystemProcessFunction> process;
        ObjectDatabaseID<SystemRenderFunction> render;

        inline SystemQuery query() const { return _query; }

        void initialize_events_in(Scene & scene);
    };

    class SystemContainer {
    private:
        std::unordered_map<std::string, System> _systems;

    public:
        ObjectDatabase<SystemProcessFunction> process_db;
        ObjectDatabase<SystemRenderFunction> render_db;

        inline const std::unordered_map<std::string, System> & systems() const { return _systems; }

        System & make_system(const std::string & name, const SystemQuery & query);
        bool has_system(const std::string & name);
        System & get_system(const std::string & name);
        void clear();
    };
}

#endif//SMEN_ECS_SYSTEM_HPP
