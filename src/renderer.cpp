#include <smen/renderer.hpp>
#include <smen/window.hpp>

namespace smen {
    Renderer::Renderer(const Window & window)
    : _renderer(
        SDL_CreateRenderer(
            const_cast<SDL_Window *>(window.sdl_pointer()),
            -1,
            SDL_RENDERER_ACCELERATED
        )
    )
    {}

    void Renderer::set_draw_color(Color4 color) {
        SDL_SetRenderDrawColor(_renderer, color.r, color.g, color.b, color.a);
    }

    void Renderer::clear() {
        SDL_RenderClear(_renderer);
    }

    void Renderer::render(const Texture & texture, Rect target_rect) {
        auto sdl_target_rect = SDL_Rect { 
            .x = target_rect.x, 
            .y = target_rect.y, 
            .w = target_rect.w, 
            .h = target_rect.h, 
        };

        auto result = SDL_RenderCopy(_renderer, texture.sdl_texture_pointer(), nullptr, &sdl_target_rect);
        if (result != 0) {
            throw RendererException("sdl error: " + std::string(SDL_GetError()));
        }
    }

    void Renderer::render(const Texture & texture, Vector2 position, FloatSize2 scale) {
        auto tex_size = texture.size();
        render(texture, Rect(position.x, position.y, static_cast<int>(static_cast<float>(tex_size.w) * scale.w), static_cast<int>(static_cast<float>(tex_size.h) * scale.h)));
    }

    void Renderer::set_logical_size(Size2 size) {
        SDL_RenderSetLogicalSize(_renderer, size.w, size.h);
        auto sdl_rect = SDL_Rect {
            .x = 0, .y = 0,
            .w = size.w, .h = size.h
        };
        /* SDL_RenderSetViewport(_renderer, &sdl_rect); */
    }

    Size2 Renderer::logical_size() const {
        auto size = Size2(0, 0);
        SDL_RenderGetLogicalSize(_renderer, &size.w, &size.h);
        return size;
    }

    void Renderer::execute() {
        SDL_RenderPresent(_renderer);
    }
}
