#include <smen/ser/serialization.hpp>
#include <smen/variant/variant.hpp>

namespace smen {
    static std::unordered_set<char> _valid_key_chars = {
        'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm',
        'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z',
        'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M',
        'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z',
        '0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
        '_', '@', '[', ']', '#', '/', '@', '<', '>', '.'
    };

    bool is_unquoted_string_char(char c) {
        return _valid_key_chars.contains(c);
    }

    bool is_unquoted_string(const std::string & s) {
        for (auto & c : s) {
            if (!is_unquoted_string_char(c)) return false;
        }
        return true;
    }

    void write_escaped_string(std::ostream & s, const std::string & str) {
        if (str.size() == 0) {
            s << "\"\"";
            return;
        }

        if (is_unquoted_string(str)) {
            s << str;
        } else {
            s << "\"";
            for (auto & c : str) {
                switch(c) {
                case '"': s << "\\\""; break;
                case '\\': s << "\\\\"; break;
                case '\n': s << "\\n"; break;
                case '\r': s << "\\r"; break;
                case '\t': s << "\\t"; break;
                default: s << c;
                }
            }
            s << "\"";
        }
    }

    TypeSerializer::TypeSerializer(std::ostream & s, VariantTypeDirectory & dir)
    : _dir(dir)
    , _s(s)
    {}

    void TypeSerializer::serialize(const VariantTypeField & field) {
        if (field.attributes != VariantTypeFieldAttribute::NONE) {
            if (field.has_attribute(VariantTypeFieldAttribute::DONT_SERIALIZE)) {
                _s << "@dont_serialize ";
            }

            if (field.has_attribute(VariantTypeFieldAttribute::ALWAYS_SERIALIZE)) {
                _s << "@always_serialize ";
            }
        }

        smen::write_escaped_string(_s, field.name);
        _s << " = ";
        smen::write_escaped_string(_s, _dir.resolve(field.type_id).name);
    }

    void TypeSerializer::serialize(const VariantType & type) {
        if (_serialized_type_ids.contains(type.id)) return;
        if (type.builtin) return;
        if (type.generic_category == VariantGenericCategory::GENERIC_SPECIALIZED) return;

        _serialized_type_ids.emplace(type.id);

        if (type.is_compound_type()) {
            for (auto & pair : type.fields()) {
                auto & field = pair.second;
                auto * field_type = &_dir.resolve(field.type_id);

                if (field_type->generic_category == VariantGenericCategory::GENERIC_SPECIALIZED) {
                    if (!field_type->builtin && !_serialized_type_ids.contains(field_type->element_type_id)) {
                        auto & element_type = _dir.resolve(field_type->element_type_id);
                        serialize(element_type);
                    }

                    field_type = &_dir.resolve(field_type->source_type_id);
                }

                if (!field_type->builtin && !_serialized_type_ids.contains(field_type->id)) {
                    serialize(*field_type);
                }
            }
        }

        _s << "[";
        switch(type.category) {
        case VariantTypeCategory::INT32:
            _s << "valuetype int32";
            break;
        case VariantTypeCategory::UINT32:
            _s << "valuetype uint32";
            break;
        case VariantTypeCategory::INT64:
            _s << "valuetype int64";
            break;
        case VariantTypeCategory::UINT64:
            _s << "valuetype uint64";
            break;
        case VariantTypeCategory::FLOAT32:
            _s << "valuetype float32";
            break;
        case VariantTypeCategory::FLOAT64:
            _s << "valuetype float64";
            break;
        case VariantTypeCategory::BOOLEAN:
            _s << "valuetype boolean";
            break;
        case VariantTypeCategory::STRING:
            _s << "valuetype string";
            break;
        case VariantTypeCategory::LIST:
            _s << "valuetype list";
            break;
        case VariantTypeCategory::REFERENCE:
            _s << "valuetype reference";
            break;
        case VariantTypeCategory::COMPONENT:
            _s << "component";
            break;
        case VariantTypeCategory::COMPLEX:
            _s << "type";
            break;
        case VariantTypeCategory::INVALID:
        default:
            throw std::runtime_error("attempted to serialize invalid variant type category");
        }

        if (type.generic_category == VariantGenericCategory::GENERIC_BASE) {
            _s << " generic";
        }

        _s << " ";
        smen::write_escaped_string(_s, type.name);
        _s << "]";


        if (type.ctor) {
            _s << "\n@ctor = ";
            smen::write_escaped_string(_s, type.ctor);
        }

        if (type.dtor) {
            _s << "\n@dtor = ";
            smen::write_escaped_string(_s, type.dtor);
        }
        
        if (type.category == VariantTypeCategory::COMPLEX || type.category == VariantTypeCategory::COMPONENT) {
            _s << "\n";

            auto first = true;

            for (auto & field_key : type.ordered_field_keys()) {
                if (!first) _s << "\n";
                first = false;
                serialize(type.field(field_key));
            }
        }

        _s << "\n\n";
    }

    void TypeSerializer::serialize_all() {
        for (auto & pair : _dir.types()) {
            serialize(pair.second);
        }
    }

    VariantSerializer::VariantSerializer(std::ostream & s, VariantContainer & container)
    : _ref_map()
    , _ref_counter(0)
    , _s(s)
    , _container(container)
    {}

    void VariantSerializer::_serialize_references(const Variant & variant, const VariantType & type) {
        if (type.is_compound_type()) {
            for (auto & pair : type.fields()) {
                auto & field = pair.second;
                auto & field_type = _container.dir.resolve(field.type_id);

                auto field_variant = variant.get_field(field);
                _serialize_references(field_variant, field_type);
            }
        }

        if (type.category == VariantTypeCategory::LIST && type.generic_category == VariantGenericCategory::GENERIC_SPECIALIZED) {
            for (size_t i = 0; i < variant.list_size(); i++) {
                auto entry_variant = variant.list_get(i);
                auto maybe_ref = entry_variant->reference_to();
                if (maybe_ref) {
                    serialize_reference(*maybe_ref);
                }
            }
        } else if (type.category == VariantTypeCategory::REFERENCE && type.generic_category == VariantGenericCategory::GENERIC_SPECIALIZED) {
            auto ref = variant.reference();
            if (!ref.null()) {
                serialize_reference(ref);
            }
        }

        auto maybe_ref = variant.reference_to();
        if (maybe_ref) {
            serialize_reference(*maybe_ref);
        }
    }

    void VariantSerializer::serialize_reference(const VariantReference & ref) {
        auto singleton_type = _container.dir.resolve(ref.type_id).is_singleton_type();

        if (!singleton_type) {
            // singleton types are treated as unique for the purpose
            // of serialization
            if (_ref_map.contains(ref)) return;
        }

        auto ref_variant = Variant(_container, ref);

        _ref_map[ref] = _ref_counter;
        _ref_counter += 1;
        auto ref_id = _ref_counter - 1;
        if (!singleton_type) {
            _serialize_references(ref_variant, ref_variant.type());
        }

        _s << "[variant ";
        write_escaped_string(_s, ref_variant.type().name);
        _s << " ";
        _s << ref_id;
        _s << "]\n";
        if (singleton_type) {
            _s << "\n";
        } else {
            _serialize("", ref_variant, ref_variant.type());
            _s << "\n\n";
        }
    }

    void VariantSerializer::_serialize(const std::string & field_key, const Variant & variant, const VariantType & type) {
        _serialize_references(variant, type);

        switch(type.category) {
        case VariantTypeCategory::INT32:
            if (field_key.size() > 0) _s << field_key << " = ";
            _s << variant.int32();
            break;
        case VariantTypeCategory::UINT32:
            if (field_key.size() > 0) _s << field_key << " = ";
            _s << variant.uint32();
            break;
        case VariantTypeCategory::INT64:
            if (field_key.size() > 0) _s << field_key << " = ";
            _s << variant.int64();
            break;
        case VariantTypeCategory::UINT64:
            if (field_key.size() > 0) _s << field_key << " = ";
            _s << variant.uint64();
            break;
        case VariantTypeCategory::FLOAT32:
            if (field_key.size() > 0) _s << field_key << " = ";
            _s << variant.float32();
            break;
        case VariantTypeCategory::FLOAT64:
            if (field_key.size() > 0) _s << field_key << " = ";
            _s << variant.float64();
            break;
        case VariantTypeCategory::STRING:
            _s << field_key << " = ";
            smen::write_escaped_string(_s, variant.string());
            break;
        case VariantTypeCategory::BOOLEAN:
            _s << field_key << " = " << (variant.boolean() ? "true" : "false");
            break;
        case VariantTypeCategory::REFERENCE: {
            if (field_key.size() > 0) _s << field_key << " = ";
            _s << "@";
            auto ref = variant.reference();
            if (ref.null()) {
                _s << "null";
            } else {
                _s << _ref_map[ref];
            }
            break;
        }
        case VariantTypeCategory::LIST: {
            auto list_element_type = _container.dir.resolve(type.element_type_id);
            if (field_key.size() > 0) _s << field_key << " = ";
            _s << "[";
            for (size_t i = 0; i < variant.list_size(); i++) {
                if (i > 0) _s << " ";
                auto new_path = field_key + "";
                auto elem = variant.list_get(i);
                auto maybe_ref = elem->reference_to();
                if (maybe_ref) {
                    _s << "@";
                    _s << _ref_map[*maybe_ref];
                } else {
                    elem->write_contents(_s);
                    //elem->_serialize(_s, "", list_element_type);
                }
            }
            _s << "]";
            break;
        }
        case VariantTypeCategory::COMPONENT:
        case VariantTypeCategory::COMPLEX: {
            if (field_key.size() > 0) {
                _s << field_key << " = ";
                _s << "{";
            }
            auto & inner_field_keys = type.ordered_field_keys();
            auto first = true;
            for (size_t i = 0; i < inner_field_keys.size(); i++) {
                auto & inner_field_key = inner_field_keys[i];
                auto & field = type.field(inner_field_key);

                auto field_variant = variant.get_field(field);

                if (!field.has_attribute(VariantTypeFieldAttribute::ALWAYS_SERIALIZE)) {
                    if (field.has_attribute(VariantTypeFieldAttribute::DONT_SERIALIZE)) continue;

                    if (field_variant.is_default_value()) continue;
                }

                if (!first) {
                    if (field_key.size() > 0) _s << " ";
                    else _s << "\n";
                }
                first = false;

                _serialize(field.name, field_variant, field_variant.type());
            }
            if (field_key.size() > 0) {
                _s << "}";
            }
            break;
        }
        case VariantTypeCategory::INVALID:
        default:
           throw SerializationException("attempt to serialize variant of invalid type category");
        }
    }

    void VariantSerializer::serialize_references(const Variant & variant) {
        _serialize_references(variant, variant.type());
    }

    void VariantSerializer::serialize(const Variant & variant) {
        auto maybe_ref = variant.reference_to();
        if (maybe_ref) {
            auto ref = *maybe_ref;
            auto it = _ref_map.find(ref);
            if (it != _ref_map.end()) {
                _s << "@" << it->second;
                return;
            }
        }

        _serialize("", variant, variant.type());
    }

    SceneSerializer::SceneSerializer(std::ostream & s, Scene & scene) 
    : _scene(scene)
    , _s(s)
    , _variant_ser(s, scene.variant_container())
    , _parent_id_counter(1)
    , _parent_id_map()
    {}

    void SceneSerializer::serialize_system_query(const SystemQuery & query) {
        _s << "[";
        auto first = true;
        for (auto & type_id : query.required_type_ids()) {
            if (!first) _s << " ";
            first = false;

            auto & type = _scene.dir().resolve(type_id);
            _s << type.name;
        }
        _s << "]";
    }

    void SceneSerializer::serialize_system(const System & system) {
        _s << "[system ";
        _s << system.name;
        _s << " ";
        serialize_system_query(system.query());
        _s << "]";

        if (system.process) {
            _s << "\n";
            _s << "@process = " << system.process;
        }

        if (system.render) {
            _s << "\n";
            _s << "@render = " << system.render;
        }
    }

    void SceneSerializer::serialize_entity(EntityID ent_id) {
        for (auto & comp : _scene.get_components(ent_id)) {
            _variant_ser.serialize_references(comp);
        }

        _s << "[entity";
        if (_scene.has_children(ent_id)) {
            _s << " " << _parent_id_counter;
            _parent_id_map[ent_id] = _parent_id_counter;
            _parent_id_counter += 1;
        }
        _s << "]";

        auto parent = _scene.parent_of(ent_id);
        if (parent) {
            _s << "\n";
            _s << "@parent = " << _parent_id_map[*parent];
        }

        auto & name = _scene.name_of(ent_id);
        if (name.size() > 0) {
            _s << "\n";
            _s << "@name = ";
            smen::write_escaped_string(_s, name);
        }

        for (auto & comp : _scene.get_components(ent_id)) {
            _s << "\n";
            _s << comp.type().name << " = ";
            _variant_ser.serialize(comp);
        }
    }

    void SceneSerializer::serialize_script(const std::string & id, const LuaScript & script) {
        _s << "[script ";
        write_escaped_string(_s, id);
        _s << "]\n";
        _s << "path = ";
        write_escaped_string(_s, script.path());
    }

    void SceneSerializer::serialize_entities() {
        auto first = true;
        for (auto ent_id : _scene) {
            if (!first) _s << "\n\n";
            first = false;

            serialize_entity(ent_id);
        }
    }

    void SceneSerializer::serialize_systems() {
        auto first = true;
        for (auto & pair : _scene.system_container().systems()) {
            if (!first) _s << "\n\n";
            first = false;

            serialize_system(pair.second);
        }
    }

    void SceneSerializer::serialize_scripts() {
        auto first = true;
        for (auto & pair : _scene.scripts()) {
            if (!first) _s << "\n\n";
            first = false;

            serialize_script(pair.first, pair.second);
        }
    }

    void SceneSerializer::serialize_all() {
        _s << "[scene]\n";
        _s << "name = ";
        write_escaped_string(_s, _scene.name());
        _s << "\n\n";

        if (_scene.entity_container().max_entity_id() > 0) {
            serialize_entities();
            _s << "\n\n";
        }

        if (_scene.scripts().size() > 0) {
            serialize_scripts();
            _s << "\n\n";
        }

        serialize_systems();
    }
}
