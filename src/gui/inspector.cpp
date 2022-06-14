#include <smen/gui/inspector.hpp>
#include <smen/gui/imgui.hpp>
#include <smen/ser/serialization.hpp>
#include <smen/ser/deserialization.hpp>
#include <filesystem>
#include <fstream>

namespace smen {
    GUIInspector::GUIInspector(Session * const indirect_session)
    : _indirect_session_ptr(indirect_session)
    {}

    void GUIInspector::draw_variant_section(Variant & variant) {
        using namespace ImGui;

        auto & type = variant.type();
        switch(type.category) {
        case VariantTypeCategory::INT32: {
            int32_t value = variant.int32();
            if (InputScalar("##", ImGuiDataType_S32, &value)) {
                variant.set_int32(value);
            }
            break;
        }
        case VariantTypeCategory::UINT32: {
            uint32_t value = variant.uint32();
                        if (InputScalar("##", ImGuiDataType_U32, &value)) {
                variant.set_uint32(value);
            }
            break;
        }
        case VariantTypeCategory::INT64: {
            auto value = variant.int64();
                        if (InputScalar("##", ImGuiDataType_S64, &value)) {
                variant.set_int64(value);
            }
            break;
        }
        case VariantTypeCategory::UINT64: {
            auto value = variant.uint64();
                        if (InputScalar("##", ImGuiDataType_U64, &value)) {
                variant.set_uint64(value);
            }
            break;
        }
        case VariantTypeCategory::FLOAT32: {
            auto value = variant.float32();
                        if (InputScalar("##", ImGuiDataType_Float, &value)) {
                variant.set_float32(value);
            }
            break;
        }
        case VariantTypeCategory::FLOAT64: {
            auto value = variant.float64();
                        if (InputScalar("##", ImGuiDataType_Double, &value)) {
                variant.set_float64(value);
            }
            break;
        }
        case VariantTypeCategory::BOOLEAN: {
            auto prev_value = variant.boolean();
            auto value = prev_value;
                        Checkbox("##", &value);
            if (value != prev_value) {
                variant.set_boolean(value);
            }

            SameLine();
            if (value) Text("true");
            else Text("false");
            break;
        }
        case VariantTypeCategory::STRING: {
            auto value = variant.string();
                        if (InputText("##", &value)) {
                variant.set_string(value);
            }
            break;
        }
        case VariantTypeCategory::COMPLEX:
        case VariantTypeCategory::COMPONENT: {
            Indent();

            for (auto & field_key : type.ordered_field_keys()) {
                auto & field = type.field(field_key);
                auto & field_type = scene().dir().resolve(field.type_id);
                auto field_variant = variant.get_field(field);

                PushID(field_key.c_str()); 
                if (_variant_selection.selecting && field_variant.is_root_object()) {
                    if (Button("Stash")) {
                        _variant_selection.add(variant);
                    }
                    SameLine();
                }

                if (field_type.is_compound_type() || field_type.category == VariantTypeCategory::REFERENCE || field_type.category == VariantTypeCategory::LIST) {
                    if (CollapsingHeader(field_key.c_str())) {
                        draw_variant_section(field_variant);
                    }
                } else {
                    Text("%s", field_key.c_str());
                    SameLine();
                    SetCursorPosX(GetContentRegionMax().x/2);
                    SetNextItemWidth(GetContentRegionMax().x/2);
                    draw_variant_section(field_variant);
                }
                PopID();
            }

            Unindent();
            break;
        }
        case VariantTypeCategory::REFERENCE: {
            Indent();
            if (variant.reference().null()) {
                Text("NULL");
            } else {
                auto referenced_variant = variant.variant();
                draw_variant_section(referenced_variant);
            }

            Text("Change reference");
            SameLine();
            Variant * v_ptr = nullptr;
            if (draw_variant_reference_picker(v_ptr, type.element_type_id)) {
                if (v_ptr == nullptr) {
                    variant.set_reference(VariantReference::NULL_REFERENCE);
                } else {
                    variant.set_reference(*v_ptr->reference_to());
                }
                return;
            }
            Unindent();
            break;
        }
        case VariantTypeCategory::LIST: {
            Indent();
            uint64_t list_size = variant.list_size();
            Text("Size");
            SameLine();
            SetCursorPosX(GetContentRegionMax().x/2);
            SetNextItemWidth(GetContentRegionMax().x/2);
            BeginDisabled(true);
            InputScalar("##", ImGuiDataType_U64, &list_size);
            EndDisabled();

            for (size_t i = 0; i < list_size; i++) {
                auto header_str = "#" + std::to_string(i);
                auto entry_variant = *variant.list_get(i);

                PushID("list_entry");
                PushID(i);
                if (Button("-")) {
                    variant.list_remove(i);
                    break;
                }
                PopID();
                PopID();

                SameLine();

                if (CollapsingHeader(header_str.c_str())) {
                    draw_variant_section(entry_variant);
                }
            }

            Variant * v_ptr = nullptr;
            if (draw_variant_reference_picker(v_ptr, type.element_type_id, false, false)) {
                variant.list_add(*v_ptr);
            }

            Unindent();
            break;
        }
        }
    }

    void GUIInspector::draw_entity_section(EntityID entity_id) {
        using namespace ImGui;
        std::string new_name = scene().name_of(entity_id);
        Text("Name: ");
        SameLine();
        if (InputText("##", &new_name)) {
            scene().set_name(entity_id, new_name);
        }

        auto parent_id = scene().parent_of(entity_id);
        if (parent_id) {
            if (scene().name_of(*parent_id) != "") {
                Text("Child of %u (\"%s\")", *parent_id, scene().name_of(*parent_id).c_str());
            } else {
                Text("Child of %u", *parent_id);
            }
        }

        auto comps = scene().get_components(entity_id);
        for (auto & comp : comps) {
            PushID("component");
            PushID(comp.type().name.c_str());
            Indent();
            auto quit_loop = false;
            if (CollapsingHeader(comp.type().name.c_str())) {
                if (_variant_selection.selecting && comp.is_root_object() && Button("Stash")) {
                    _variant_selection.add(comp);
                }

                draw_variant_section(comp);
                Indent();

                auto entry = comp.content_ptr().entry(scene().variant_container());
                if (entry.ptr() == nullptr) {
                    Text("This object is not reference counted");
                } else {
                    Text("Alive references: %u", entry.refcount());
                }
                if (Button("Delete")) {
                    scene().remove_component(entity_id, comp);
                    quit_loop = true;
                }
                Unindent();
            }
            Unindent();
            PopID();
            PopID();
            if (quit_loop) break;
        }

        if (scene().has_children(entity_id)) {
            if (CollapsingHeader("Children")) {
                auto & children = scene().children_of(entity_id);

                for (auto ent_id : children) {
                    if (CollapsingHeader(("Entity " + std::to_string(ent_id)).c_str())) {
                        PushID("child_entity");
                        PushID(ent_id);
                        draw_entity_section(ent_id);
                        PopID();
                        PopID();
                    }
                }
            }
        }

        Indent();
        if (Button("Add component")) {
            _add_component_window.open(entity_id);
        }
        Unindent();
    }

    bool GUIInspector::draw_variant_reference_picker(Variant * & v, VariantTypeID type_id, bool include_null, bool require_heap) {
        using namespace ImGui;
        std::optional<Variant *> selected_variant_ptr = std::nullopt;

        if (BeginCombo("##ref_list", "Select object")) {
            if (include_null) {
                if (Selectable("NULL", false)) {
                    selected_variant_ptr = nullptr;
                }
            }

            for (size_t i = 0; i < _variant_selection.variants.size(); i++) {
                auto & v = _variant_selection.variants[i];
                if (v.type_id() != type_id) continue;
                if (require_heap && v.storage_mode() != VariantStorageMode::HEAP) continue;

                auto ref_name = _variant_selection.ref_name(i);
                if (Selectable(ref_name.c_str(), false)) {
                    selected_variant_ptr = &_variant_selection.variants[i];
                }
            }
            EndCombo();
        }

        if (selected_variant_ptr) {
            v = *selected_variant_ptr;
            return true;
        }

        return false;
    }

    void GUIInspector::draw_stash_tab() {
        using namespace ImGui;

        PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.0f);
        BeginGroup();
        if (_variant_selection.current_idx && *_variant_selection.current_idx >= _variant_selection.variants.size()) {
            _variant_selection.current_idx = std::nullopt;
        }

        SetNextItemWidth(4);
        if (Button(_variant_selection.selecting ? "OFF" : "ON")) {
            _variant_selection.selecting = !_variant_selection.selecting;
        }

        SameLine();
        Text("Variant stash");

        SameLine();
        SetNextItemWidth(4);
        if (Button("Create")) {
            _create_variant_window.open();
        }


        if (BeginCombo("##list", !_variant_selection.current_idx ? "Stashed objects..." : _variant_selection.ref_name(*_variant_selection.current_idx).c_str())) {
            if (Selectable("Deselect", false)) {
                _variant_selection.current_idx = std::nullopt;
            }

            for (size_t i = 0; i < _variant_selection.variants.size(); i++) {
                auto ref_name = _variant_selection.ref_name(i);
                if (Selectable(ref_name.c_str(), i == *_variant_selection.current_idx)) {
                    _variant_selection.current_idx = i;
                }
            }
            EndCombo();
        }

        Separator();

        if (_variant_selection.current_idx) {
            draw_variant_section(_variant_selection.current());
        }


        BeginDisabled(_variant_selection.current_idx == std::nullopt);
        if (Button("Remove from stash")) {
            _variant_selection.remove(*_variant_selection.current_idx);
        }
        EndDisabled();
        EndGroup();
        PopStyleVar();
    }

    void GUIInspector::draw_listing_tab() {
        using namespace ImGui;

        if (Button("Spawn")) {
            scene().spawn();
        }

        if (!scene().is_valid_entity_id(_open_entity)) {
            _open_entity = 0;
        }

        for (auto ent_id : scene()) {

            auto quit_loop = false;
            SetNextItemOpen(_open_entity == ent_id);

            auto header_title = "Entity " + std::to_string(ent_id);
            if (scene().name_of(ent_id) != "") {
                header_title += " (" + scene().name_of(ent_id) + ")";
            }

            if (CollapsingHeader(header_title.c_str())) {
                _open_entity = ent_id;
                PushID("entity");
                PushID(ent_id);
                draw_entity_section(ent_id);
                SameLine();

                if (Button("Kill")) {
                    scene().kill(ent_id);
                    quit_loop = true;
                }

                PopID();
                PopID();
            }

            if (quit_loop) break;
        }
    }

    void GUIInspector::draw_pick_file_window() {
        using namespace ImGui;
        if (!_pick_file_window.opened()) return;

        Begin("Select file");
        Text("Path");
        InputText("##", &_pick_file_window.path);

        if (Button("OK")) {
            _pick_file_window.close(true);
        }
        SameLine();
        if (Button("Cancel")) {
            _pick_file_window.path = "";
            _pick_file_window.close(false);
        }
        End();
    }

    void GUIInspector::draw_add_component_window() {
        using namespace ImGui;
        if (!_add_component_window.opened()) return;

        Begin("Add component");
        Text(
            "Adding component to entity %u (\"%s\")", 
            _add_component_window.entity_id,
            scene().name_of(_add_component_window.entity_id).c_str()
        );

        auto & cur_type = scene().dir().resolve(_add_component_window.component_type);

        std::string error_info = "";
        if (_add_component_window.component_type == "") {
            error_info = "No type selected";
        } else if (!cur_type.valid()) {
            error_info = "Type doesn't exist";
        } else if (cur_type.category != VariantTypeCategory::COMPONENT) {
            error_info = "Not a component type";
        }

        if (error_info == "") {
            TextColored(ImVec4(0.2f, 1.0f, 0.2f, 1.0f), "OK");
        } else {
            TextColored(ImVec4(1.0f, 0.2f, 0.2f, 1.0f), "%s", error_info.c_str());
        }

        if (_add_component_window.input_by_name) {
            if (InputText("##by_name", &_add_component_window.component_type)) {
            }
        } else if (BeginCombo("##list", _add_component_window.component_type == "" ? "Select type..." : _add_component_window.component_type.c_str())) {
            for (auto & pair : scene().dir().types()) {
                auto & type = pair.second;
                if (type.generic_category == VariantGenericCategory::GENERIC_SPECIALIZED) continue;
                if (type.category != VariantTypeCategory::COMPONENT) continue;

                if (Selectable(type.name.c_str(), _add_component_window.component_type == type.name)) {
                    _add_component_window.component_type = type.name;
                }
            }
            EndCombo();
        }

        SameLine();
        if (Button("...")) _add_component_window.input_by_name = !_add_component_window.input_by_name;

        BeginDisabled(error_info != "");
        if (Button("Add")) {
            scene().add_component(_add_component_window.entity_id, _add_component_window.component_type);
            _add_component_window.close();
        }
        EndDisabled();
        SameLine();
        if (Button("Cancel")) {
            _add_component_window.close();
        }
        End();
    }

    void GUIInspector::draw_create_variant_window() {
        using namespace ImGui;
        if (!_create_variant_window.opened()) return;

        Begin("Create variant");

        auto & cur_type = scene().dir().resolve(_create_variant_window.variant_type);

        std::string error_info = "";
        if (_create_variant_window.variant_type == "") {
            error_info = "No type selected";
        } else if (!cur_type.valid()) {
            error_info = "Type doesn't exist";
        } else if (scene().dir().resolve(_create_variant_window.variant_type).generic_category == VariantGenericCategory::GENERIC_BASE) {
            if (_create_variant_window.element_type == "") {
                error_info = "No element type selected";
            } else if (!cur_type.valid()) {
                error_info = "Element type doesn't exist";
            }
        }

        if (error_info == "") {
            TextColored(ImVec4(0.2f, 1.0f, 0.2f, 1.0f), "OK");
        } else {
            TextColored(ImVec4(1.0f, 0.2f, 0.2f, 1.0f), "%s", error_info.c_str());
        }

        if (_create_variant_window.input_by_name) {
            if (InputText("##by_name", &_create_variant_window.variant_type)) {
            }
        } else if (BeginCombo("##list", _create_variant_window.variant_type == "" ? "Select type..." : _create_variant_window.variant_type.c_str())) {
            for (auto & pair : scene().dir().types()) {
                auto & type = pair.second;
                if (type.generic_category == VariantGenericCategory::GENERIC_SPECIALIZED) continue;

                if (Selectable(type.name.c_str(), _create_variant_window.variant_type == type.name)) {
                    _create_variant_window.variant_type = type.name;
                }
            }
            EndCombo();
        }

        if (scene().dir().resolve(_create_variant_window.variant_type).generic_category == VariantGenericCategory::GENERIC_BASE) {
            if (_create_variant_window.input_by_name) {
                if (InputText("##element_by_name", &_create_variant_window.element_type)) {
                }
            } else if (BeginCombo("##element_list", _create_variant_window.element_type == "" ? "Select type..." : _create_variant_window.element_type.c_str())) {
                for (auto & pair : scene().dir().types()) {
                    auto & type = pair.second;
                    if (type.generic_category != VariantGenericCategory::NON_GENERIC) continue;

                    if (Selectable(type.name.c_str(), _create_variant_window.element_type == type.name)) {
                        _create_variant_window.element_type = type.name;
                    }
                }
                EndCombo();
            }
        }

        SameLine();
        if (Button("...")) _create_variant_window.input_by_name = !_create_variant_window.input_by_name;

        BeginDisabled(error_info != "");
        if (Button("Create")) {
            auto full_type_name = _create_variant_window.variant_type;
            if (scene().dir().resolve(_create_variant_window.variant_type).generic_category == VariantGenericCategory::GENERIC_BASE) {
                full_type_name += "<" + _create_variant_window.element_type + ">";
            }


            _variant_selection.add(Variant::create(scene().variant_container(), full_type_name));
            _create_variant_window.close();
        }
        EndDisabled();
        SameLine();
        if (Button("Cancel")) {
            _create_variant_window.close();
        }
        End();
    }

    void GUIInspector::draw_add_script_window() {
        using namespace ImGui;
        if (!_add_script_window.opened()) return;

        Begin("Add script");

        std::string error_info = "";
        if (_add_script_window.id == "") {
            error_info = "Invalid ID";
        } else if (scene().has_script(_add_script_window.id)) {
            error_info = "Script with this ID already exists";
        } else if (_add_script_window.path == "") {
            error_info = "Invalid path";
        } else if (!std::filesystem::exists(_add_script_window.path)) {
            error_info = "File doesn't exist";
        }

        if (error_info == "") {
            TextColored(ImVec4(0.2f, 1.0f, 0.2f, 1.0f), "OK");
        } else {
            TextColored(ImVec4(1.0f, 0.2f, 0.2f, 1.0f), "%s", error_info.c_str());
        }


        Text("ID");
        SameLine();
        SetCursorPosX(GetContentRegionMax().x/3);
        SetNextItemWidth(GetContentRegionMax().x - GetContentRegionMax().x/3);
        InputText("##id", &_add_script_window.id);

        if (_add_script_window.input_by_path) {
            Text("Path");
            SameLine();
            SetCursorPosX(GetContentRegionMax().x/3);
            SetNextItemWidth(GetContentRegionMax().x - GetContentRegionMax().x/3);
            InputText("##by_path", &_add_script_window.path);
        }

        BeginDisabled(error_info != "");
        if (Button("Add")) {
            scene().new_script(_add_script_window.id);
            scene().link_script(_add_script_window.id, _add_script_window.path);
            _add_script_window.close();
        }
        EndDisabled();
        SameLine();
        if (Button("Cancel")) {
            _add_script_window.close();
        }
        End();
    }

    void GUIInspector::draw_scripts_tab() {
        using namespace ImGui;


        if (Button("Add...")) {
            _add_script_window.open();
        }

        for (auto & pair : scene().scripts()) {
            auto quit_loop = false;

            PushID("script");
            PushID(pair.first.c_str());
            auto header_str = pair.first + " (" + pair.second.path() + ")";
            if (CollapsingHeader(header_str.c_str())) {
                Text("path");
                SameLine();
                SetCursorPosX(GetContentRegionMax().x/2);
                SetNextItemWidth(GetContentRegionMax().x/2);
                auto path = pair.second.path();
                BeginDisabled(true);
                InputText("##", &path);
                EndDisabled();

                if (Button("Reload")) {
                    scene().reload_script(pair.first);
                    quit_loop = true;
                }
                SameLine();
                
                if (Button("Delete")) {
                    scene().delete_script(pair.first);
                    quit_loop = true;
                }
            }

            PopID();
            PopID();

            if (quit_loop) break;
        }
    }

    void GUIInspector::draw_scene_tab() {
        using namespace ImGui;

        if (Button("Save")) {
            _pick_file_window.open("save_scene");
        }

        SameLine();

        if (Button("Load")) {
            _pick_file_window.open("load_scene");
        }

        SameLine();

        if (Button("Clear")) {
            scene().clear();
        }

        auto saved_path = _pick_file_window.path_for("save_scene");
        if (saved_path != "") {
            auto f = std::ofstream(saved_path);
            auto ser = SceneSerializer(f, scene());
            ser.serialize_all();
            f.flush();
            _pick_file_window.clear();
        }

        auto loaded_path = _pick_file_window.path_for("load_scene");
        if (loaded_path != "") {
            auto f = std::ifstream(loaded_path);
            auto lexer = Lexer(f);
            scene().clear();
            auto ser = SceneDeserializer(scene(), lexer);
            ser.deserialize_all();
            _pick_file_window.clear();
        }

        Text("%s", session().scene.name().c_str());
        Text("%u entities", static_cast<uint32_t>(session().scene.total_entity_count()));

        BeginTabBar("inspector.scene.entities");
        if (BeginTabItem("Listing")) {
            draw_listing_tab();
            EndTabItem();
        }

        if (BeginTabItem("Stash")) {
            draw_stash_tab();
            EndTabItem();
        }

        if (BeginTabItem("Scripts")) {
            draw_scripts_tab();
            EndTabItem();
        }

        ImGui::EndTabBar();

    }

    void GUIInspector::draw_type_section(const VariantType & type) {
        using namespace ImGui;

        BeginDisabled(true);

        Text("Category");
        SameLine();
        SetCursorPosX(GetContentRegionMax().x/2);
        SetNextItemWidth(GetContentRegionMax().x/2);
        std::ostringstream cat_name_s;
        cat_name_s << type.category;
        auto cat_name = cat_name_s.str();
        InputText("##", &cat_name);

        if (type.ctor) {
            Text("@ctor");
            SameLine();
            SetCursorPosX(GetContentRegionMax().x/2);
            SetNextItemWidth(GetContentRegionMax().x/2);
            auto ctor_id = std::string(type.ctor);
            InputText("##", &ctor_id);
        }

        if (type.dtor) {
            Text("@dtor");
            SameLine();
            SetCursorPosX(GetContentRegionMax().x/2);
            SetNextItemWidth(GetContentRegionMax().x/2);
            auto dtor_id = std::string(type.dtor);
            InputText("##", &dtor_id);
        }

        for (auto & field_key : type.ordered_field_keys()) {
            auto & field = type.field(field_key);
            auto & field_type = scene().dir().resolve(field.type_id);

            Text("%s", field_key.c_str());
            SameLine();
            SetCursorPosX(GetContentRegionMax().x/2);
            SetNextItemWidth(GetContentRegionMax().x/2);
            auto field_type_name = field_type.name;
            InputText("##", &field_type_name);
        }

        EndDisabled();
    }

    void GUIInspector::draw_types_tab() {
        using namespace ImGui;

        for (auto & pair : scene().dir().types()) {
            auto & type = pair.second;
            if (type.generic_category == VariantGenericCategory::GENERIC_SPECIALIZED) continue;

            PushID("type");
            PushID(type.id);
            auto cat_s = std::ostringstream();
            cat_s << type.category;

            auto header = type.name + " (" + cat_s.str() + ")";
            if (CollapsingHeader(header.c_str())) {
                draw_type_section(type);
            }
            PopID();
            PopID();
        }
    }

    void GUIInspector::draw() {
        using namespace ImGui;

        if (!_active) return;

        draw_pick_file_window();
        draw_add_component_window();
        draw_create_variant_window();
        draw_add_script_window();

        Begin("SMEN Inspector");

        if (!has_session()) {
            Text("No scene loaded");
            return;
        }

        BeginTabBar("inspector.tabs");
        if (BeginTabItem("Scene")) {
            draw_scene_tab();
            EndTabItem();
        }

        if (BeginTabItem("Types")) {
            draw_types_tab();
            EndTabItem();
        }
        ImGui::EndTabBar();

        End();
    }
}
