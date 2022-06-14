#include <smen/session.hpp>
#include <smen/renderer.hpp>
#include <fstream>
#include <smen/ser/deserialization.hpp>
#include <filesystem>

namespace smen {
    const Logger Session::logger = make_logger("Session");

    Session::Session()
    : root_path()
    , type_dir()
    , scene("Unnamed", type_dir)
    {}

    void Session::load(const std::string & path) {
        root_path = std::filesystem::current_path().string() + "/" + path;

        std::filesystem::current_path(root_path);

        auto types_path = resolve_resource_path("types.smen");
        auto scene_path = resolve_resource_path("scene.smen");

        auto types_f = std::ifstream(types_path);
        if (types_f) {
            logger.debug("loading types from: '" + types_path + "'");
            auto types_lexer = Lexer(types_f);
            auto type_deser = TypeDeserializer(type_dir, types_lexer);
            type_deser.deserialize_all();
        }

        auto scene_f = std::ifstream(scene_path);
        if (scene_f) {
            logger.debug("loading scene from: '" + types_path + "'");
            auto scene_lexer = Lexer(scene_f);
            auto scene_deser = SceneDeserializer(scene, scene_lexer);
            scene_deser.deserialize_all();
        }
    }

    std::string Session::resolve_resource_path(const std::string & path) {
        return root_path + "/" + path;
    }
}
