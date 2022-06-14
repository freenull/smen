#ifndef SMEN_RENDERER_HPP
#define SMEN_RENDERER_HPP

#include <SDL2/SDL.h>
#include <iostream>
#include <sstream>
#include <smen/math.hpp>
#include <smen/texture.hpp>
#include <smen/color.hpp>

namespace smen {
    class RendererException : public std::runtime_error {
    public:
        inline RendererException(const std::string & msg)
            : std::runtime_error("error in renderer: " + msg) {}
    };

    class Window;

    class Renderer {
    private:
        SDL_Renderer * const _renderer;

    public:
        explicit Renderer(const Window & window);
        Renderer(const Renderer &) = delete;
        Renderer() = delete;
        Renderer(Renderer &&) = default;
        Renderer && operator =(Renderer &&) = delete;

        inline const SDL_Renderer * sdl_pointer() const {
            return _renderer;
        }

        inline SDL_Renderer * sdl_pointer() {
            return _renderer;
        }

        void set_draw_color(Color4 color);
        void clear();
        void render(const Texture & texture, Rect target_rect);
        void render(const Texture & texture, Vector2 position, FloatSize2 scale);

        void set_logical_size(Size2 size);
        Size2 logical_size() const;

        void execute();

        inline ~Renderer() {
            SDL_DestroyRenderer(_renderer);
        }

        inline void write_string(std::ostream & s) const {
            s << "Renderer(";
            s << static_cast<void*>(_renderer);
            s << ")";
        }

        inline std::string to_string() const {
            auto s = std::stringstream();
            write_string(s);
            return s.str();
        }
    };


    inline std::ostream & operator<<(std::ostream & s, const Renderer & renderer) {
        renderer.write_string(s);
        return s;
    }
}

#endif//SMEN_RENDERER_HPP
