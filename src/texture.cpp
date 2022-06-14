#include <smen/texture.hpp>
#include <smen/renderer.hpp>

namespace smen {
    Texture::Texture(Renderer & renderer, const std::string & path, TextureScaleMode scale_mode)
    : _renderer(&renderer)
    , _surface(IMG_Load(path.c_str()))
    , _texture(SDL_CreateTextureFromSurface(
        _renderer->sdl_pointer(),
        _surface
    ))
    {
        auto sdl_scale_mode = SDL_ScaleModeNearest;
        switch(scale_mode) {
        case TextureScaleMode::NEAREST:
            sdl_scale_mode = SDL_ScaleModeNearest;
            break;
        case TextureScaleMode::LINEAR:
            sdl_scale_mode = SDL_ScaleModeLinear;
            break;
        case TextureScaleMode::ANISOTROPIC:
            sdl_scale_mode = SDL_ScaleModeBest;
            break;
        }
        SDL_SetTextureScaleMode(_texture, sdl_scale_mode);
    }

    Texture::Texture(Texture && tex)
    : _renderer(tex._renderer)
    , _surface(tex._surface)
    , _texture(tex._texture)
    {
        tex._surface = nullptr;
        tex._texture = nullptr;
    }

    Texture & Texture::operator=(Texture && tex) {
        _renderer = tex._renderer;
        _surface = tex._surface;
        _texture = tex._texture;

        tex._surface = nullptr;
        tex._texture = nullptr;

        return *this;
    }

    unsigned int Texture::width() const {
        int value;
        SDL_QueryTexture(_texture, nullptr, nullptr, &value, nullptr);
        return static_cast<unsigned int>(value);
    }

    unsigned int Texture::height() const {
        int value;
        SDL_QueryTexture(_texture, nullptr, nullptr, nullptr, &value);
        return static_cast<unsigned int>(value);
    }

    Size2 Texture::size() const {
        auto size = Size2(0, 0);
        SDL_QueryTexture(_texture, nullptr, nullptr, &size.w, &size.h);
        return size;
    }

    Texture::~Texture() {
        if (_texture != nullptr) SDL_DestroyTexture(_texture);
        if (_surface != nullptr) SDL_FreeSurface(_surface);
    }
}
