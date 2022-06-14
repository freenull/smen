#ifndef SMEN_TEXTURE_HPP
#define SMEN_TEXTURE_HPP

#include <string>
#include <smen/math.hpp>
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>

namespace smen {
    class Renderer;

    enum class TextureScaleMode {
        NEAREST,
        LINEAR,
        ANISOTROPIC
    };

    struct Texture {
    private:
        Renderer * _renderer;
        SDL_Surface * _surface;
        SDL_Texture * _texture;

    public:
        Texture(Renderer & renderer, const std::string & path, TextureScaleMode scale_mode = TextureScaleMode::NEAREST);

        Texture(const Texture & tex) = delete;
        Texture(Texture && tex);
        Texture & operator=(const Texture & tex) = delete;
        Texture & operator=(Texture && tex);

        inline SDL_Surface * sdl_surface_pointer() const { return _surface; }
        inline SDL_Texture * sdl_texture_pointer() const { return _texture; }
        inline bool valid() const { return _texture != nullptr; }
        unsigned int width() const;
        unsigned int height() const;
        Size2 size() const;
        ~Texture();
    };
}

#endif//SMEN_TEXTURE_HPP
