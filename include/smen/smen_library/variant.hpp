#ifndef SMEN_LUA_VARIANT_LIBRARY_HPP
#define SMEN_LUA_VARIANT_LIBRARY_HPP

#include <smen/lua/engine.hpp>
#include <smen/event.hpp>
#include <smen/ecs/system.hpp>
#include <smen/renderer.hpp>
#include <smen/gui/context.hpp>
#include <smen/lua/library.hpp>
#include <smen/object_db.hpp>

namespace smen {
    class Scene;
    
    struct LuaVariantLibrary : public LuaLibrary {
    public:
        Event<LuaVariantLibrary &, LuaEngine &> post_load;

    private:
        VariantContainer * _variant_alloc;
        LuaObject _smen_table;

        Scene * _current_scene_ptr;
        Renderer * _current_renderer_ptr;
        Window * _current_window_ptr;
        GUIContext * _current_gui_context_ptr;

        LuaObject _type_func;

        std::vector<ObjectDatabaseEntry<SystemProcessFunction>> _func_db_process_list;
        std::vector<ObjectDatabaseEntry<SystemRenderFunction>> _func_db_render_list;


        void _load_variant_support(LuaObject & table);

        void _load_vector2_support(LuaObject & table);
        void _load_vector2f_support(LuaObject & table);
        void _load_size2_support(LuaObject & table);
        void _load_size2f_support(LuaObject & table);

        static std::optional<LuaResult> _scene_check(const LuaNativeFunctionArgs & args);

        static LuaResult _scene_spawn_func(LuaEngine & engine, const LuaNativeFunctionArgs & args);
        static LuaResult _scene_all_entities_func(LuaEngine & engine, const LuaNativeFunctionArgs & args);
        static LuaResult _scene_add_component_func(LuaEngine & engine, const LuaNativeFunctionArgs & args);
        static LuaResult _scene_has_component_func(LuaEngine & engine, const LuaNativeFunctionArgs & args);
        static LuaResult _scene_entities_with_component_func(LuaEngine & engine, const LuaNativeFunctionArgs & args);
        static LuaResult _scene_get_component_func(LuaEngine & engine, const LuaNativeFunctionArgs & args);
        static LuaResult _scene_get_components_func(LuaEngine & engine, const LuaNativeFunctionArgs & args);
        static LuaResult _scene_children_of_func(LuaEngine & engine, const LuaNativeFunctionArgs & args);
        static LuaResult _scene_has_children_func(LuaEngine & engine, const LuaNativeFunctionArgs & args);
        static LuaResult _scene_parent_of_func(LuaEngine & engine, const LuaNativeFunctionArgs & args);
        static LuaResult _scene_is_child_of_func(LuaEngine & engine, const LuaNativeFunctionArgs & args);
        static LuaResult _scene_has_parent_func(LuaEngine & engine, const LuaNativeFunctionArgs & args);

        static LuaResult _scene_adopt_child_func(LuaEngine & engine, const LuaNativeFunctionArgs & args);
        static LuaResult _scene_spawn_child_func(LuaEngine & engine, const LuaNativeFunctionArgs & args);
        static LuaResult _scene_make_orphan_func(LuaEngine & engine, const LuaNativeFunctionArgs & args);

        static LuaResult _scene_remove_component_func(LuaEngine & engine, const LuaNativeFunctionArgs & args);
        static LuaResult _scene_find_first_by_name_func(LuaEngine & engine, const LuaNativeFunctionArgs & args);
        static LuaResult _scene_find_by_name_func(LuaEngine & engine, const LuaNativeFunctionArgs & args);
        static LuaResult _scene_name_of_func(LuaEngine & engine, const LuaNativeFunctionArgs & args);
        static LuaResult _scene_set_name_func(LuaEngine & engine, const LuaNativeFunctionArgs & args);
        static LuaResult _scene_kill_func(LuaEngine & engine, const LuaNativeFunctionArgs & args);

        void _load_scene_support(LuaObject & table);

        static LuaResult _renderer_render_func(LuaEngine & engine, const LuaNativeFunctionArgs & args);
        void _load_renderer_support(LuaObject & table);

        static LuaResult _gui_begin_func(LuaEngine & engine, const LuaNativeFunctionArgs & args);
        static LuaResult _gui_text_func(LuaEngine & engine, const LuaNativeFunctionArgs & args);
        static LuaResult _gui_end_func(LuaEngine & engine, const LuaNativeFunctionArgs & args);
        void _load_gui_support(LuaObject & table);

        void _load_window_support(LuaObject & table);

        static LuaResult _texture_load_func(LuaEngine & engine, const LuaNativeFunctionArgs & args);
        void _load_texture_support(LuaObject & table);

        void _load_system_callback_support(LuaObject & table);

        static LuaResult _key_down_func(LuaEngine & engine, const LuaNativeFunctionArgs & args);
        static LuaResult _key_up_func(LuaEngine & engine, const LuaNativeFunctionArgs & args);

        LuaObject _create_key_table();
        void _load_keyboard_support(LuaObject & table);

        void _load_metatype_tools(LuaObject & table);

        static LuaResult _require_preloader_func(LuaEngine & engine, const LuaNativeFunctionArgs & args);

        inline static void _noop_func(LuaEngine & engine, const LuaNativeFunctionArgs & args) {
            (void)engine;
            (void)args;
        }

    public:
        LuaVariantLibrary(VariantContainer & variant_alloc, LuaEngine & engine);

        virtual void on_load(LuaObject & table) override;
        virtual std::string name() override;

        void set_current_scene(Scene * scene);
        void set_current_renderer(Renderer * renderer);
        void set_current_window(Window * window);
        void set_current_gui_context(GUIContext * gui_context);
    };
}

#endif//SMEN_LUA_VARIANT_LIBRARY_HPP
