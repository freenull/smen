#include <smen/gui/context.hpp>
#include <smen/math.hpp>

namespace smen {
    GUIFrameScopeGuard::GUIFrameScopeGuard(GUIContext & ctx)
    : ctx(ctx) {}

    GUIFrameScopeGuard::~GUIFrameScopeGuard() {
        ctx._end_drawing();
    }

    GUIContext::GUIContext(Renderer & renderer, const Window & window)
    : _renderer(renderer)
    , _window(window)
    , _drawing(false)
    {
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGui_ImplSDL2_InitForSDLRenderer(const_cast<SDL_Window *>(_window.sdl_pointer()), const_cast<SDL_Renderer *>(_renderer.sdl_pointer()));
        ImGui_ImplSDLRenderer_Init(const_cast<SDL_Renderer *>(_renderer.sdl_pointer()));
    }

    void GUIContext::_begin_drawing() {
        _drawing = true;

        ImGui_ImplSDLRenderer_NewFrame();
        ImGui_ImplSDL2_NewFrame();
        ImGui::NewFrame();
    }

    void GUIContext::_end_drawing() {
        _drawing = false;

        ImGui::Render();
        Size2 prev_size = _renderer.logical_size();
        _renderer.set_logical_size(Size(0, 0));
        ImGui_ImplSDLRenderer_RenderDrawData(ImGui::GetDrawData());
        _renderer.set_logical_size(prev_size);
    }


    static Vector2 mouse_pos(0, 0);

    void GUIContext::draw(GUIDrawFunction func) {
        _begin_drawing();
        GUIFrameScopeGuard scope(*this);

        func(*this);
    }

    void GUIContext::process_event(const SDL_Event & ev) {
        // mouse events from logical size-bound window must be translated
        // to window coordinates
        auto window_size = _window.size();
        auto logical_size = _renderer.logical_size();

        double scale_amount = 1;
        if (static_cast<double>(window_size.w) / logical_size.w > static_cast<double>(window_size.h) / logical_size.h) {
            scale_amount = static_cast<double>(window_size.h) / logical_size.h;
        } else if (static_cast<double>(window_size.h) / logical_size.h > static_cast<double>(window_size.w) / logical_size.w) {
            scale_amount = static_cast<double>(window_size.w) / logical_size.w;
        }

        auto real_view_size = Size2(logical_size.w * scale_amount, logical_size.h * scale_amount);
        auto view_offset_from_win = Vector2(static_cast<double>(window_size.w - real_view_size.w) / 2, static_cast<double>(window_size.h - real_view_size.h) / 2);

        auto scale_w = static_cast<double>(real_view_size.w) / logical_size.w;
        auto scale_h = static_cast<double>(real_view_size.h) / logical_size.h;

        SDL_Event new_ev = ev;
        if (new_ev.type == SDL_MOUSEBUTTONDOWN || new_ev.type == SDL_MOUSEBUTTONUP) {
            
            // imgui sdl2 backend doesn't use x/y on this ev
        } else if (new_ev.type == SDL_MOUSEMOTION) {
            // only x/y are used here
            new_ev.motion.x = static_cast<double>(new_ev.motion.x) * scale_w + view_offset_from_win.x;
            new_ev.motion.y = static_cast<double>(new_ev.motion.y) * scale_h + view_offset_from_win.y;
            mouse_pos = Vector2(new_ev.motion.x, new_ev.motion.y);
        }

        ImGui_ImplSDL2_ProcessEvent(&new_ev);
    }

    GUIContext::~GUIContext() {
        ImGui_ImplSDLRenderer_Shutdown();
        ImGui_ImplSDL2_Shutdown();
        ImGui::DestroyContext();
    }
}
