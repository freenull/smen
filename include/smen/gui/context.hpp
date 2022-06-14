#ifndef SMEN_GUI_CONTEXT_HPP
#define SMEN_GUI_CONTEXT_HPP

#include <smen/gui/imgui.hpp>
#include <smen/window.hpp>
#include <smen/renderer.hpp>
#include <functional>

namespace smen {
    class GUIException : public std::runtime_error {
    public:
        inline GUIException(const std::string & msg)
            : std::runtime_error("error in GUI rendering: " + msg) {}

    };

    class GUIContext;

    using GUIDrawFunction = std::function<void (GUIContext &)>;

    struct GUIFrameScopeGuard {
    public:
        GUIContext & ctx;
        GUIFrameScopeGuard(GUIContext & ctx);
        ~GUIFrameScopeGuard();
    };

    class GUIContext {
        friend struct GUIFrameScopeGuard;

    private:
        Renderer & _renderer;
        const Window & _window;
        bool _drawing;

        void _begin_drawing();
        void _end_drawing();

    public:
        GUIContext(Renderer & renderer, const Window & window);
        GUIContext(const GUIContext &) = delete;
        GUIContext & operator =(const GUIContext &) = delete;
        GUIContext(GUIContext &&) = default;
        GUIContext & operator =(GUIContext &&) = delete;

        inline bool drawing() const { return _drawing; }

        void draw(GUIDrawFunction func);
        void process_event(const SDL_Event & ev);

        ~GUIContext();
    };
}

#endif//SMEN_GUI_CONTEXT_HPP
