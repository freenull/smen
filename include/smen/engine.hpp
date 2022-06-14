#ifndef SMEN_ENGINE_HPP
#define SMEN_ENGINE_HPP

#include <filesystem>
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <smen/window.hpp>
#include <smen/gui/imgui.hpp>
#include <smen/gui/context.hpp>
#include <smen/gui/inspector.hpp>
#include <smen/variant/variant.hpp>
#include <smen/lua/engine.hpp>
#include <smen/ecs/scene.hpp>
#include <smen/object_db.hpp>
#include <smen/lua/script.hpp>
#include <smen/session.hpp>
#include <smen/event.hpp>

namespace smen {
    class Engine {
    private:
        bool _running;
        bool _initialized;

        Window _window;
        Renderer _renderer;
        
        void _handle_event(const SDL_Event & ev);
        void _draw();
        void _draw_gui();
        void _draw_scene();

    public:
        std::string name;
        Color4 default_backdrop_color;

        Session session;
        GUIContext gui;
        GUIInspector inspector;

    private:
        EventHolder _ev;

    public:
        static Window initialize_sdl_and_main_window(const std::string & name);

        Engine(const std::string & name, const std::string & root_path);
        Engine(const Engine &) = delete;
        Engine(Engine &&) = default;
        Engine & operator =(const Engine &) = delete;
        Engine & operator =(Engine &&) = delete;

        inline bool running() { return _running; }
        
        void run();
        void shutdown();
        void execute();

        ~Engine();
    };
}

#endif//SMEN_ENGINE_HPP
