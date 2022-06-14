#ifndef SMEN_GUI_INSPECTOR_HPP
#define SMEN_GUI_INSPECTOR_HPP

#include <smen/session.hpp>
#include <smen/gui/context.hpp>

namespace smen {
    class GUIInspector {
    private:

        struct AddComponentWindow {
        private:
            bool _opened = false;

        public:
            EntityID entity_id = 0;
            std::string component_type = "";
            bool input_by_name = false;

            inline bool opened() const { return _opened; }

            inline void open(EntityID new_entity_id) {
                _opened = true;
                ImGui::SetWindowFocus("Add component");
                entity_id = new_entity_id;
                component_type = "";
            }

            inline void close() {
                _opened = false;
            }
        };

        struct CreateVariantWindow {
        private:
            bool _opened = false;

        public:
            std::string variant_type = "";
            std::string element_type = "";
            bool input_by_name = false;

            inline bool opened() const { return _opened; }

            inline void open() {
                _opened = true;
                ImGui::SetWindowFocus("Create variant");
                variant_type = "";
                element_type = "";
            }

            inline void close() {
                _opened = false;
            }
        };

        struct AddScriptWindow {
        private:
            bool _opened = false;

        public:
            std::string id = "";
            std::string path = "";
            bool input_by_path = true;

            inline bool opened() const { return _opened; }

            inline void open() {
                _opened = true;
                ImGui::SetWindowFocus("Add script");
                id = "";
                path = "";
            }

            inline void close() {
                _opened = false;
            }
        };

        struct PickFileWindow {
        private:
            bool _opened = false;
            std::string id = "";
            bool _success = false;

        public:
            std::string path = "";

            inline bool opened() const { return _opened; }

            inline void open(const std::string & id) {
                _opened = true;
                ImGui::SetWindowFocus("Select file");
                this->id = id;
                _success = false;
            }

            inline void close(bool success) {
                _opened = false;
                _success = success;
            }

            inline std::string path_for(const std::string & id) {
                if (!_success) return "";
                if (this->id == id) {
                    return path;
                } 
                return "";
            }

            inline void clear() {
                id = "";
            }
        };

        struct VariantSelection {
            std::vector<Variant> variants;
            std::optional<size_t> current_idx;
            bool selecting = false;

            inline size_t add(const Variant & variant) {
                variants.emplace_back(variant);
                return variants.size() - 1;
            }

            inline void remove(size_t index) {
                if (index >= variants.size()) return;
                variants.erase(variants.begin() + index);
            }

            inline std::string ref_name(size_t index) {
                return variants[index].type().name + " @" + std::to_string(index);
            }

            inline Variant & current() {
                return variants[*current_idx];
            }
        };

        Session * const _indirect_session_ptr;
        bool _active = false;
        EntityID _open_entity = 0;
        PickFileWindow _pick_file_window;
        AddComponentWindow _add_component_window;
        CreateVariantWindow _create_variant_window;
        AddScriptWindow _add_script_window;

        VariantSelection _variant_selection;

    public:
        GUIInspector(Session * const indirect_session);

        inline bool has_session() const { return _indirect_session_ptr != nullptr; }
        inline Session & session() const { return *_indirect_session_ptr; }
        inline Scene & scene() const { return _indirect_session_ptr->scene; }

        inline bool active() { return _active; }
        inline void show() { _active = true; }
        inline void hide() { _active = false; }
        inline void toggle() { _active = !_active; }

        void draw_variant_section(Variant & variant);
        void draw_entity_section(EntityID entity_id);

        bool draw_variant_reference_picker(Variant * & v, VariantTypeID type_id, bool include_null = true, bool require_heap = true);

        void draw_scripts_tab();
        void draw_stash_tab();
        void draw_listing_tab();

        void draw_scene_tab();

        void draw_type_section(const VariantType & type);
        void draw_types_tab();

        void draw_pick_file_window();
        void draw_add_component_window();
        void draw_create_variant_window();
        void draw_add_script_window();

        void draw();
    };
}

#endif//SMEN_GUI_INSPECTOR_HPP
