#ifndef SMEN_WINDOW_HPP
#define SMEN_WINDOW_HPP

#include <SDL2/SDL.h>
#include <iostream>
#include <sstream>
#include <optional>
#include <smen/math.hpp>
#include <smen/logger.hpp>
#include <smen/renderer.hpp>

namespace smen {
    struct WindowOptions {
        bool fullscreen;

        inline void write_string(std::ostream & s) const {
            s << "WindowOptions(";
            s << "fullscreen = " << (fullscreen ? "true" : "false");
            s << ")";
        }

        inline const std::string to_string() const {
            std::stringstream s;
            write_string(s);
            return s.str();
        }

        inline uint32_t sdl_flags() const {
            uint32_t flags = SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE;
            if (fullscreen) flags |= SDL_WINDOW_FULLSCREEN;
            return flags;
        }
    };

    static inline std::ostream & operator<<(std::ostream & s, const WindowOptions & options) {
        options.write_string(s);
        return s;
    }

    class Window {
    private:
        SDL_Window * const _window;
        Logger _logger;
        std::optional<SDL_GLContext> _gl_context;
        std::optional<Renderer> _renderer;

    public:
        const WindowOptions initial_options;
        const Rect initial_bounds;
        
        Window() = delete;
        Window(const Window &) = delete;
        Window(Window &&) = default;
        Window operator =(const Window &) = delete;
        Window operator =(Window &&) = delete;

        inline Window(std::string title, WindowOptions options, Rect bounds)
        : _window(
            SDL_CreateWindow(
                title.c_str(),
                bounds.x, bounds.y, bounds.w, bounds.h,
                options.sdl_flags()
            )
        )
        , _logger(make_logger("Window '" + title + "'"))
        , initial_options(options)
        , initial_bounds(bounds)
        {
            _logger.debug("Creating window '", title, "' with flags: ", options.sdl_flags());
        }

        inline SDL_Window const * sdl_pointer() const {
            return _window;
        }

        inline SDL_GLContext gl_context() {
            if (_gl_context) return *_gl_context;
            _gl_context.emplace(SDL_GL_CreateContext(_window));
            SDL_GL_MakeCurrent(_window, *_gl_context);
            return *_gl_context;
        }

        void set_title(const std::string & title);
        std::string title() const;
        void set_size(Size2 size);
        Size2 size() const;

        inline ~Window() {
            SDL_DestroyWindow(_window);
        }

        inline void write_string(std::ostream & s) const {
            s << "Window(";
            s << static_cast<void*>(_window);
            s << ")";
        }

        inline std::string to_string() const {
            auto s = std::stringstream();
            write_string(s);
            return s.str();
        }
    };

    inline std::ostream & operator<<(std::ostream & s, const Window & window) {
        window.write_string(s);
        return s;
    }
}

#endif//SMEN_WINDOW_HPP
