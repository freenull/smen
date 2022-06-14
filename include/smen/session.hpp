#ifndef SMEN_SESSION_HPP
#define SMEN_SESSION_HPP

#include <smen/object_db.hpp>
#include <smen/lua/script.hpp>
#include <smen/variant/types.hpp>
#include <smen/ecs/scene.hpp>
#include <smen/renderer.hpp>

namespace smen {
    class SessionException : public std::runtime_error {
    public:
        inline SessionException(const std::string & msg)
            : std::runtime_error("error in session: " + msg) {}
    };

    struct Session {
        static const Logger logger;

        std::string root_path;
        VariantTypeDirectory type_dir;
        Scene scene;

        Session();
        void load(const std::string & root_path);
        std::string resolve_resource_path(const std::string & path);
    };
}

#endif//SMEN_SESSION_HPP
