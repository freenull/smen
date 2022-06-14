#include <smen/ser/deserialization.hpp>
#include <smen/ser/serialization.hpp>
#include <smen/variant/type_builder.hpp>
#include <limits>

namespace smen {
    Lexer::Lexer(std::istream & s)
    : _s(s)
    , _eof(false)
    , _cur_row(1)
    , _cur_col(0)
    , _prev_col(0)
    , _peek_token(std::nullopt)
    {
        _move();
    }

    bool Lexer::_is_whitespace(char c) {
        return c == ' ' || c == '\n' || c == '\t';
    }

    char Lexer::_peek() {
        auto val = _s.peek();
        if (val == EOF) return '\0';
        return static_cast<char>(val);
    }

    void Lexer::_move(size_t n) {
        auto val = _s.get();
        _prev_col = _cur_col;
        _cur_col += 1;
        if (val == EOF) {
            _eof = true;
            _cur = '\0';
            return;
        }
        _cur = static_cast<char>(val);
        if (_cur == '\n') {
            _cur_row += 1;
            _cur_col = 0;
        }
    }

    void Lexer::_skip_whitespace() {
        while (_is_whitespace(_cur)) {
            _move();
        }
    }

    FileRegion Lexer::_start_region() {
        return FileRegion(_cur_row, _cur_col);
    }

    FileRegion Lexer::_end_region(const FileRegion & region) {
        return FileRegion(region.start_row, region.start_col, _cur_row, _cur_col);
    }

    FileRegion Lexer::_end_region_before(const FileRegion & region) {
        if (_cur_col == 0) {
            return FileRegion(region.start_row, region.start_col, _cur_row - 1, _prev_col);
        } else {
            return FileRegion(region.start_row, region.start_col, _cur_row, _cur_col - 1);
        }

    }

    std::string Lexer::read_unquoted() {
        auto s = std::ostringstream();
        while (!_is_whitespace(_cur) && !_eof && _cur != '[' && _cur != ']' && _cur != '=' && _cur != '{' && _cur != '}') {
            s << _cur;
            _move();
        }
        return s.str();
    }

    std::string Lexer::read_quoted() {
        bool escape = false;
        auto region = _start_region();

        if (_cur != '"') throw DeserializationException(region, "expected quote");

        auto s = std::ostringstream();

        while(true) {
            _move();

            if (escape) {
                switch(_cur) {
                case 'n':
                    s << '\n';
                    break;
                case 't':
                    s << '\t';
                    break;
                case 'r':
                    s << '\r';
                    break;
                default:
                    s << _cur;
                    break;
                }
                escape = false;
            } else {
                if (_cur == '"') {
                    _move();
                    break;
                }
                else if (_cur == '\\') {
                    escape = true;
                    continue;
                }

                s << _cur;
            }

            if (_eof) {
                throw DeserializationException(_end_region_before(region), "unterminated quoted string (unexpected end of file)");
            }

            if (_cur == '\n') {
                throw DeserializationException(_end_region(region), "unterminated quoted string (line ends unexpectedly, use \\n to insert literal newlines)");
            }
        }

        return s.str();
    }

    std::string Lexer::read_string() {
        if (_cur == '"') return read_quoted();
        return read_unquoted();
    }

    Lexer::Token Lexer::next() {
        if (_peek_token) {
            auto tok = *_peek_token;

            if (_peek2_token) {
                _peek_token = *_peek2_token;
                _peek2_token = std::nullopt;
            } else {
                _peek_token = std::nullopt;
            }

            return tok;
        }

        _skip_whitespace();

        auto region = _start_region();

        switch(_cur) {
        case '[':
            _move();
            return Token(TokenType::HEADER_LIST_BEGIN, "[", region);
        case ']':
            _move();
            return Token(TokenType::HEADER_LIST_END, "]", region);
        case '{':
            _move();
            return Token(TokenType::OBJECT_BEGIN, "{", region);
        case '}':
            _move();
            return Token(TokenType::OBJECT_END, "}", region);
        case '=':
            _move();
            return Token(TokenType::ASSIGN, "=", region);
        case '@': {
            _move();
            _skip_whitespace();
            auto content = read_unquoted();
            return Token(TokenType::REFERENCE, content, _end_region_before(region));
        }
        case '"': {
            auto content = read_quoted();
            return Token(TokenType::STRING, content, _end_region_before(region));
        }
        case '\0':
            return Token(TokenType::END_OF_FILE, "\0", region);
        default: {
            auto content = read_unquoted();
            return Token(TokenType::STRING, content, _end_region_before(region));
        }
        }
    }

    void Lexer::check(const Token & tok, TokenType type, const std::string & name) {
        if (tok.type != type) {
            throw DeserializationException(tok.region, "expected " + name);
        }
    }

    Lexer::Token Lexer::next_expect(TokenType type, const std::string & name) {
        auto tok = next();
        check(tok, type, name);
        return tok;
    }

    Lexer::Token Lexer::peek() {
        if (_peek_token) return *_peek_token;
        _peek_token = next();
        return *_peek_token;
    }

    Lexer::Token Lexer::peek2() {
        if (_peek2_token) return *_peek2_token;

        if (_peek_token) {
            _peek2_token = next();
        } else {
            _peek_token = next();
            _peek2_token = next();
        }

        return *_peek_token;
    }

    TypeDeserializer::TypeDeserializer(VariantTypeDirectory & dir, Lexer & lexer)
    : _dir(dir)
    , _lexer(lexer)
    {}

    void TypeDeserializer::deserialize_field(VariantTypeBuilder & builder) {
        uint32_t attr = 0;
        auto tok = _lexer.next();

        while (tok.type == Lexer::TokenType::REFERENCE) {
            // attributes or ctor/dtor
            auto special_field_name = tok.content;

            if (special_field_name == "dont_serialize") {
                attr |= static_cast<uint32_t>(VariantTypeFieldAttribute::DONT_SERIALIZE);
            } else if (special_field_name == "always_serialize") {
                attr |= static_cast<uint32_t>(VariantTypeFieldAttribute::ALWAYS_SERIALIZE);
            } else if (special_field_name == "ctor") {
                tok = _lexer.next_expect_assign();
                tok = _lexer.next_expect_string("constructor function ID");

                auto ctor_id = tok.content;

                if (!_dir.ctor_db.has(ctor_id)) {
                    throw DeserializationException(tok.region, "constructor function '" + ctor_id + "' is not available at this point");
                }

                builder.ctor(ctor_id);
            } else if (special_field_name == "dtor") {
                tok = _lexer.next_expect_assign();
                tok = _lexer.next_expect_string("destructor function ID");

                auto dtor_id = tok.content;

                if (!_dir.dtor_db.has(dtor_id)) {
                    throw DeserializationException(tok.region, "destructor function '" + dtor_id + "' is not available at this point");
                }

                builder.dtor(dtor_id);
            } else {
                throw DeserializationException(tok.region, "invalid field attribute or special field: '" + special_field_name + "'");
            }

            tok = _lexer.next();
        } 



        _lexer.check_string(tok, "field name");

        auto field_name = tok.content;

        tok = _lexer.next_expect_assign();
        tok = _lexer.next_expect_string("field type");

        auto field_type_name = tok.content;
        auto & field_type = _dir.resolve(field_type_name);
        if (!field_type.valid()) {
            throw DeserializationException(tok.region, "referenced field type does not exist: '" + field_type_name + "'");
        }

        builder.field(field_name, field_type.id, static_cast<VariantTypeFieldAttribute>(attr));
    }

    VariantType TypeDeserializer::deserialize_next() {
        auto tok = _lexer.next_expect_header_begin();

        tok = _lexer.next_expect_string("type category specifier");

        auto category_specifier = tok.content;

        VariantTypeCategory cat;
        if (category_specifier == "component") {
            cat = VariantTypeCategory::COMPONENT;
        } else if (category_specifier == "type") {
            cat = VariantTypeCategory::COMPLEX;
        } else if (category_specifier == "valuetype") {
            tok = _lexer.next_expect_string("type category");

            auto cat_name = tok.content;

            if (cat_name == "int32") cat = VariantTypeCategory::INT32;
            else if (cat_name == "uint32") cat = VariantTypeCategory::UINT32;
            else if (cat_name == "int64") cat = VariantTypeCategory::INT64;
            else if (cat_name == "uint64") cat = VariantTypeCategory::UINT64;
            else if (cat_name == "float32") cat = VariantTypeCategory::FLOAT32;
            else if (cat_name == "float64") cat = VariantTypeCategory::FLOAT64;
            else if (cat_name == "boolean") cat = VariantTypeCategory::BOOLEAN;
            else if (cat_name == "string") cat = VariantTypeCategory::STRING;
            else if (cat_name == "list") cat = VariantTypeCategory::LIST;
            else if (cat_name == "reference") cat = VariantTypeCategory::REFERENCE;
            else {
                throw DeserializationException(tok.region, "invalid valuetype category: '" + cat_name + "'");
            }
        } else {
            throw DeserializationException(tok.region, "invalid type category specifier: '" + category_specifier + "'");
        }

        bool generic = false;
        std::string name;

        tok = _lexer.next_expect_string("type name (or 'generic' modifier)");

        if (tok.content == "generic") {
            generic = true;

            tok = _lexer.next_expect_string("generic type name");
        } 
        name = tok.content;

        tok = _lexer.next_expect_header_end();

        auto type_builder = VariantTypeBuilder(_dir, cat, name);
        
        auto peek_tok = _lexer.peek();
        while (peek_tok.type != Lexer::TokenType::HEADER_LIST_BEGIN && peek_tok.type != Lexer::TokenType::END_OF_FILE) {
            deserialize_field(type_builder);
            peek_tok = _lexer.peek();
        }

        auto type = type_builder.build();
        if (generic) type.generic_category = VariantGenericCategory::GENERIC_BASE;

        return type;
    }

    void TypeDeserializer::deserialize_all() {
        while (_lexer.peek().type != Lexer::TokenType::END_OF_FILE) {
            _dir.add(deserialize_next());
        }
    }

    SceneDeserializer::SceneDeserializer(Scene & scene, Lexer & lexer)
    : _scene(scene)
    , _lexer(lexer)
    , _variant_cache()
    {}


    void SceneDeserializer::deserialize_variant_data(Variant & variant) {
        auto peek = _lexer.peek();
        while (peek.type != Lexer::TokenType::HEADER_LIST_BEGIN && peek.type != Lexer::TokenType::END_OF_FILE) {
            deserialize_variant_field(variant);
            peek = _lexer.peek();
        }
    }

    Variant SceneDeserializer::_get_referenced_variant_from_token(const Lexer::Token & ref) {
        size_t cached_variant_id;
        try {
            cached_variant_id = std::stoull(ref.content);
        } catch (std::invalid_argument &) {
            throw DeserializationException(ref.region, "invalid reference for variant: '" + ref.content + "'");
        }
        auto it = _variant_cache.find(cached_variant_id);
        if (it == _variant_cache.end()) {
            throw DeserializationException(ref.region, "variant reference with ID " + std::to_string(cached_variant_id) + " does not exist");
        }

        return it->second;
    }

    void SceneDeserializer::set_variant(Variant & variant, const std::string & field_text, const Lexer::Token & ref) {
        auto & type = variant.type();

        switch(type.category) {
        case VariantTypeCategory::INT32:
        case VariantTypeCategory::UINT32:
        case VariantTypeCategory::INT64:
        case VariantTypeCategory::UINT64:
        case VariantTypeCategory::FLOAT32:
        case VariantTypeCategory::FLOAT64:
        case VariantTypeCategory::BOOLEAN:
        case VariantTypeCategory::STRING:
            _lexer.check_string(ref, "value for '" + type.name + "' " + field_text);
            break;
        case VariantTypeCategory::REFERENCE:
            _lexer.check_reference(ref, "reference for '" + type.name + "' " + field_text);
            break;
        case VariantTypeCategory::LIST:
            _lexer.check_list_begin(ref, "list for '" + type.name + "' " + field_text);
            break;
        case VariantTypeCategory::COMPLEX:
        case VariantTypeCategory::COMPONENT:
            _lexer.check_object_begin(ref, "object for '" + type.name + "' " + field_text);
            break;
        case VariantTypeCategory::INVALID:
            throw std::runtime_error("invalid variant type passed to deserializer");
        default:
            break;
        }

        switch(type.category) {
        case VariantTypeCategory::INT32:
            try {
                variant.set_int32(std::stoi(ref.content));
            } catch (std::invalid_argument &) {
                throw DeserializationException(ref.region, "invalid value for variant of valuetype category int32: '" + ref.content + "'");
            }
            break;
        case VariantTypeCategory::UINT32:
            // for some reason there's no std::stou
            unsigned long long_value;
            try {
                long_value = std::stoul(ref.content);
            } catch (std::invalid_argument &) {
                throw DeserializationException(ref.region, "invalid value for variant of valuetype category uint32: '" + ref.content + "'");
            }
            if (long_value > std::numeric_limits<uint32_t>::max()) {
                throw DeserializationException(ref.region, "invalid value for variant of valuetype category uint32: '" + ref.content + "'");
            }
            variant.set_uint32(static_cast<uint32_t>(long_value));
            break;
        case VariantTypeCategory::INT64:
            try {
                variant.set_int64(std::stol(ref.content));
            } catch (std::invalid_argument &) {
                throw DeserializationException(ref.region, "invalid value for variant of valuetype category int64: '" + ref.content + "'");
            }
            break;
        case VariantTypeCategory::UINT64:
            try {
                variant.set_uint64(std::stoul(ref.content));
            } catch (std::invalid_argument &) {
                throw DeserializationException(ref.region, "invalid value for variant of valuetype category uint64: '" + ref.content + "'");
            }
            break;
        case VariantTypeCategory::FLOAT32:
            try {
                variant.set_float32(std::stof(ref.content));
            } catch (std::invalid_argument &) {
                throw DeserializationException(ref.region, "invalid value for variant of valuetype category float32: '" + ref.content + "'");
            }
            break;
        case VariantTypeCategory::FLOAT64:
            try {
                variant.set_float64(std::stod(ref.content));
            } catch (std::invalid_argument &) {
                throw DeserializationException(ref.region, "invalid value for variant of valuetype category float64: '" + ref.content + "'");
            }
            break;
        case VariantTypeCategory::BOOLEAN:
            if (ref.content != "true" && ref.content != "false") {
                throw DeserializationException(ref.region, "invalid value for variant of valuetype category boolean: '" + ref.content + "'");
            }

            variant.set_boolean(ref.content == "true");
            break;
        case VariantTypeCategory::STRING:
            variant.set_string(ref.content);
            break;
        case VariantTypeCategory::REFERENCE: {
             auto referenced_variant = _get_referenced_variant_from_token(ref);

            if (referenced_variant.storage_mode() != VariantStorageMode::HEAP) {
                throw DeserializationException(ref.region, "variant reference with ID " + ref.content + " is ill-formed, as the variant it points to cannot be referenced");
            }

            variant.set_reference(*referenced_variant.reference_to());
            break;
        }
        case VariantTypeCategory::LIST: {
            auto tok = _lexer.next();
            auto element_type_id = type.element_type_id;
            size_t idx = 0;
            while (tok.type != Lexer::TokenType::HEADER_LIST_END) {
                if (tok.type == Lexer::TokenType::END_OF_FILE) {
                    throw DeserializationException(ref.region, "expected list end marker (']') for '" + type.name + "' " + field_text);
                }

                if (tok.type == Lexer::TokenType::REFERENCE) {
                    auto referenced_variant = _get_referenced_variant_from_token(tok);
                    variant.list_add(referenced_variant);
                } else {
                    auto elem = Variant::create(_scene.variant_container(), element_type_id);
                    set_variant(elem, "element #" + std::to_string(idx), tok);
                    variant.list_add(elem);
                }

                tok = _lexer.next();
            }

            break;
        }
        case VariantTypeCategory::COMPLEX:
        case VariantTypeCategory::COMPONENT: {
            auto peek = _lexer.peek();
            while (peek.type != Lexer::TokenType::OBJECT_END && peek.type != Lexer::TokenType::END_OF_FILE) {
                deserialize_variant_field(variant);
                peek = _lexer.peek();
            }
            _lexer.next_expect_object_end("closing object bracket for '" + type.name + "' " + field_text);
            break;
        }
        case VariantTypeCategory::INVALID:
        default:
            throw std::runtime_error("invalid variant type passed to deserializer");
        }

    }

    void SceneDeserializer::deserialize_variant_field(Variant & variant) {
        auto & type = variant.type();

        auto tok = _lexer.next_expect_string("variant field name");
        auto field_name = tok.content;

        tok = _lexer.next_expect_assign();

        auto & field = type.field(field_name);
        if (!field.valid()) {
            throw DeserializationException(tok.region, "referenced field '" + field_name + "' on type '" + type.name + "' does not exist");
        }

        tok = _lexer.next();
        auto value = tok.content;

        auto field_variant = variant.get_field(field);

        set_variant(field_variant, "field " + field_name, tok);
    }

    void SceneDeserializer::deserialize_entity_field(EntityID id) {
        auto tok = _lexer.next();
        if (tok.type == Lexer::TokenType::REFERENCE) {
            auto special_field_name = tok.content;

            tok = _lexer.next_expect_assign();

            if (special_field_name == "parent") {
                tok = _lexer.next_expect_string("ID of parent entity");

                size_t referenced_entity_index;

                try {
                    referenced_entity_index = std::stoull(tok.content);
                } catch (std::invalid_argument &) {
                    throw DeserializationException(tok.region, "invalid numeric entity ID: '" + tok.content + "'");
                }

                auto it = _entity_cache.find(referenced_entity_index);
                if (it == _entity_cache.end()) {
                    throw DeserializationException(tok.region, "referenced entity ID: " + tok.content + " does not exist");
                }

                auto parent_entity_id = it->second;
                _scene.adopt_child(parent_entity_id, id);
            } else if (special_field_name == "name") {
                tok = _lexer.next_expect_string("name of entity");

                _scene.set_name(id, tok.content);
            } else {
                throw DeserializationException(tok.region, "unknown special entity field: '" + special_field_name + "'");
            }

        } else if (tok.type == Lexer::TokenType::STRING) {
            auto component_type_name = tok.content;

            tok = _lexer.next_expect_assign();

            auto & component_type = _scene.dir().resolve(component_type_name);
            if (!component_type.valid()) {
                throw DeserializationException(tok.region, "referenced component type '" + component_type_name + "' does not exist");
            }

            tok = _lexer.next_expect_reference("reference to variant of type " + component_type_name);

            size_t variant_index;

            try {
                variant_index = std::stoull(tok.content);
            } catch (std::invalid_argument &) {
                throw DeserializationException(tok.region, "invalid numeric variant reference ID: '" + tok.content + "'");
            }

            auto it = _variant_cache.find(variant_index);
            if (it == _variant_cache.end()) {
                throw DeserializationException(tok.region, "referenced variant with ID " + tok.content + " does not exist");
            }

            auto component_variant = it->second;
            _scene.add_component(id, component_variant);
        } else {
            throw DeserializationException(tok.region, "expected component type or special field name");
        }
    }

    void SceneDeserializer::deserialize_entity_data(EntityID id) {
        auto peek = _lexer.peek();
        while (peek.type != Lexer::TokenType::HEADER_LIST_BEGIN && peek.type != Lexer::TokenType::END_OF_FILE) {
            deserialize_entity_field(id);
            peek = _lexer.peek();
        }
    }

    SystemQuery SceneDeserializer::deserialize_system_query() {
        auto tok = _lexer.next_expect_list_begin();

        auto query = SystemQuery(_scene.dir());

        auto peek = _lexer.peek();
        while (peek.type != Lexer::TokenType::HEADER_LIST_END) {
            if (peek.type == Lexer::TokenType::END_OF_FILE) {
                throw DeserializationException(peek.region, "unterminated system query list");
            }

            tok = _lexer.next_expect_string("component type name");

            auto & type = _scene.dir().resolve(tok.content);
            if (!type.valid()) {
                throw DeserializationException(peek.region, "type '" + type.name + "' referenced in system query does not exist");
            }

            if (type.category != VariantTypeCategory::COMPONENT) {
                throw DeserializationException(peek.region, "type '" + type.name + "' referenced in system query is not a component type");
            }

            query.require_type(type.id);

            peek = _lexer.peek();
        }

        tok = _lexer.next_expect_list_end();

        return query;
    }

    void SceneDeserializer::deserialize_system_field(System & system) {
        auto tok = _lexer.next_expect_reference("system special field");

        auto field_tok = tok;

        tok = _lexer.next_expect_assign();
        tok = _lexer.next_expect_string("special field value");

        auto value = tok.content;

        if (field_tok.content == "process") {
            if (!_scene.system_container().process_db.has(value)) {
                throw DeserializationException(tok.region, "process function '" + value + "' is not available at this point");
            }

            system.process = value;
        } else if (field_tok.content == "render") {
            if (!_scene.system_container().render_db.has(value)) {
                throw DeserializationException(tok.region, "render function '" + value + "' is not available at this point");
            }

            system.render = value;
        } else {
            throw DeserializationException(tok.region, "invalid special system field: '" + field_tok.content + "'");
        }
    }
    
    void SceneDeserializer::deserialize_system_data(System & system) {
        auto peek = _lexer.peek();
        while (peek.type != Lexer::TokenType::HEADER_LIST_BEGIN && peek.type != Lexer::TokenType::END_OF_FILE) {
            deserialize_system_field(system);

            peek = _lexer.peek();
        }
    }

    void SceneDeserializer::deserialize_script_field(const std::string & id) {
        auto tok = _lexer.next_expect_string("script entry field");
        auto name_tok = tok;
        tok = _lexer.next_expect_assign();

        tok = _lexer.next_expect_string("script entry value");

        if (name_tok.content == "path") {
            _scene.link_script(id, tok.content);
        } else {
            throw DeserializationException(name_tok.region, "invalid script entry field: '" + name_tok.content + "'");
        }
    }

    void SceneDeserializer::deserialize_script_data(const std::string & id) {
        auto peek = _lexer.peek();
        while (peek.type != Lexer::TokenType::HEADER_LIST_BEGIN && peek.type != Lexer::TokenType::END_OF_FILE) {
            deserialize_script_field(id);

            peek = _lexer.peek();
        }
    }

    void SceneDeserializer::deserialize_scene_field() {
        auto tok = _lexer.next_expect_string("scene entry field");
        auto name_tok = tok;
        tok = _lexer.next_expect_assign();

        tok = _lexer.next_expect_string("scene entry field value");

        if (name_tok.content == "name") {
            _scene.set_name(tok.content);
        } else {
            throw DeserializationException(name_tok.region, "invalid scene entry field: '" + name_tok.content + "'");
        }
    }

    void SceneDeserializer::deserialize_scene_data() {
        auto peek = _lexer.peek();
        while (peek.type != Lexer::TokenType::HEADER_LIST_BEGIN && peek.type != Lexer::TokenType::END_OF_FILE) {
            deserialize_scene_field();

            peek = _lexer.peek();
        }
    }

    void SceneDeserializer::deserialize_next() {
        auto tok = _lexer.next_expect_header_begin();
        tok = _lexer.next_expect_string("entry type");

        std::string entry_type = tok.content;

        if (entry_type == "variant") {
            tok = _lexer.next_expect_string("variant type name");

            auto variant_type = tok.content;

            auto & type = _scene.dir().resolve(variant_type);
            if (!type.valid()) {
                throw DeserializationException(tok.region, "referenced type does not exist: '" + variant_type + "'");
            }

            auto variant = Variant::create(_scene.variant_container(), type.id);

            tok = _lexer.next_expect_string("variant ID");

            size_t variant_index;

            try {
                variant_index = std::stoull(tok.content);
            } catch (std::invalid_argument &) {
                throw DeserializationException(tok.region, "invalid numeric variant ID: '" + tok.content + "'");
            }

            _variant_cache.emplace(variant_index, variant);

            tok = _lexer.next_expect_header_end();

            deserialize_variant_data(variant);
        } else if (entry_type == "entity") {
            tok = _lexer.next();

            auto ent_id = _scene.spawn();

            if (tok.type == Lexer::TokenType::STRING) {
                size_t serialized_entity_id;

                try {
                    serialized_entity_id = std::stoull(tok.content);
                } catch (std::invalid_argument &) {
                    throw DeserializationException(tok.region, "invalid numeric entity ID: '" + tok.content + "'");
                }

                _entity_cache[serialized_entity_id] = ent_id;

                tok = _lexer.next();
            }

            _lexer.check_header_end(tok, "entry header marker (']')");

            deserialize_entity_data(ent_id);

        } else if (entry_type == "system") {
            tok = _lexer.next_expect_string("system name");
            auto system_name = tok.content;

            auto query = deserialize_system_query();

            tok = _lexer.next_expect_header_end();

            auto & system = _scene.system_container().make_system(system_name, query);
            deserialize_system_data(system);
            system.initialize_events_in(_scene);

        } else if (entry_type == "script") {
            tok = _lexer.next_expect_string("script ID");
            auto script_id = tok.content;

            _scene.new_script(script_id);

            tok = _lexer.next_expect_header_end();

            deserialize_script_data(script_id);
        } else if (entry_type == "scene") {
            tok = _lexer.next_expect_header_end();

            deserialize_scene_data();
        } else {
            throw DeserializationException(tok.region, "invalid entry type: '" + entry_type + "'");
        }
    }

    void SceneDeserializer::deserialize_all() {
        auto peek = _lexer.peek();
        while (peek.type != Lexer::TokenType::END_OF_FILE) {
            deserialize_next();
            peek = _lexer.peek();
        }
    }
}
