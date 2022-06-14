#include <smen/engine.hpp>
#include <smen/texture.hpp>
#include <SDL2/SDL.h>
#include <smen/gui/imgui.hpp>
#include <SDL2/SDL_opengl.h>
#include <smen/gui/context.hpp>
#include <chrono>

namespace smen {
    Window Engine::initialize_sdl_and_main_window(const std::string & name) {
        SDL_Init(SDL_INIT_VIDEO);
        return Window(name, WindowOptions(), Rect(0, 0, 640, 480));
    }

    Engine::Engine(const std::string & name, const std::string & root_path)
    : _running(true)
    , _initialized(false)
    , _window(initialize_sdl_and_main_window(name))
    , _renderer(_window)
    , name(name)
    , default_backdrop_color(Color4(255, 255, 255, 255))
    , session()
    , gui(_renderer, _window)
    , inspector(&session)
    {
        _ev.add(session.scene.smen_library().post_load, [&](LuaVariantLibrary & variant_lib, LuaEngine & engine) {
            variant_lib.set_current_renderer(&_renderer);
            variant_lib.set_current_window(&_window);
            variant_lib.set_current_gui_context(&gui);
        });

        session.load(root_path);
        _window.set_title(session.scene.name());

        run();
    }

    void Engine::run() {
        if (_running) return;
        if (!_initialized) {
            SDL_Init(SDL_INIT_VIDEO);
            IMG_Init(IMG_INIT_JPG | IMG_INIT_PNG | IMG_INIT_WEBP);
        }
        _initialized = true;
        _running = true;
    }
    
    void Engine::shutdown() {
        if (!_running) return;
        _running = false;
    }

    void Engine::execute() {
        auto last_time = std::chrono::steady_clock::now();

        while (true) {
            SDL_Event ev;

            while (SDL_PollEvent(&ev)) {
                gui.process_event(ev);

                _handle_event(ev);
            }

            if (running()) {
                auto now_time = std::chrono::steady_clock::now();
                std::chrono::duration<double> delta = now_time - last_time;
                last_time = now_time;
                if (!inspector.active()) {
                    session.scene.process(delta.count());
                }
                _draw();
            } else break;
        }
    }

    void Engine::_handle_event(const SDL_Event & ev) {
        switch(ev.type) {
        case SDL_QUIT:
            shutdown();
            break;
        case SDL_WINDOWEVENT:
            if (ev.window.type == SDL_WINDOWEVENT_CLOSE) {
                shutdown();
            }
            break;
        case SDL_KEYUP:
            if (ev.key.keysym.sym == SDLK_F1) {
                inspector.toggle();
            }
            break;
        }
    }

    void Engine::_draw() {
        _renderer.set_draw_color(default_backdrop_color);

        _renderer.clear();

        _draw_scene();
        _draw_gui();

        _renderer.execute();
    }

    void Engine::_draw_gui() {
        gui.draw([&](auto & ctx) {
            inspector.draw();
        });
    }

    void Engine::_draw_scene() {
        session.scene.render();
    }

    Engine::~Engine() {
        shutdown();
        if (_initialized) {
            IMG_Quit();
            SDL_Quit();
        }
    }
}
