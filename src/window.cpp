#include <smen/window.hpp>

namespace smen {
    void Window::set_title(const std::string & title) {
        SDL_SetWindowTitle(_window, title.c_str());
    }

    std::string Window::title() const {
        return std::string(SDL_GetWindowTitle(_window));
    }

    void Window::set_size(Size2 size) {
        auto flags = SDL_GetWindowFlags(_window);
        auto max = (flags & SDL_WINDOW_MAXIMIZED) != 0;
        if (max) {
            SDL_RestoreWindow(_window);
        }
        SDL_SetWindowSize(_window, size.w, size.h);
        if (max) {
            SDL_MaximizeWindow(_window);
        }

    }

    Size2 Window::size() const {
        auto size = Size2(0, 0);
        SDL_GetWindowSize(_window, &size.w, &size.h);
        return size;
    }
}
