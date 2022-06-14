#include <smen/lua/library.hpp>
#include <smen/smen_library/variant.hpp>
#include <smen/ecs/scene.hpp>

namespace smen {
    LuaVariantLibrary::LuaVariantLibrary(VariantContainer & variant_alloc, LuaEngine & engine)
    : LuaLibrary(engine)
    , _variant_alloc(&variant_alloc)
    , _smen_table(engine.nil())
    , _current_scene_ptr(nullptr)
    , _current_renderer_ptr(nullptr)
    , _current_window_ptr(nullptr)
    , _current_gui_context_ptr(nullptr)
    , _type_func(engine.nil())
    {}

    void LuaVariantLibrary::on_load(LuaObject & table) {
        _load_variant_support(table);

        _load_vector2_support(table);
        _load_vector2f_support(table);
        _load_size2_support(table);
        _load_size2f_support(table);

        _load_keyboard_support(table);

        _load_scene_support(table);
        _load_system_callback_support(table);

        _load_window_support(table);
        _load_texture_support(table);
        _load_renderer_support(table);

        _load_gui_support(table);

        _load_metatype_tools(table);

        post_load(*this, *_engine);
    }

    std::string LuaVariantLibrary::name() {
        return "smen";
    }

    void LuaVariantLibrary::_load_variant_support(LuaObject & table) {
        auto & variant_typedesc = _engine->create_native_type<Variant>("Variant");
        variant_typedesc.copy_ctor_func = [](const smen::LuaEngine & engine, const Variant & source, Variant * target) {
            new (target) Variant(source);
        };

        variant_typedesc.lua_ctor_func = [&](const smen::LuaEngine & engine, LuaObject & target_obj, Variant * target_ptr, const std::span<const LuaObject> & args) -> LuaResult {
            if (args.size() == 1) return LuaResult::error("invalid variant constructor call - missing type name");
            auto type_name_obj = args[1];
            if (type_name_obj.type() != LuaType::STRING) {
                return LuaResult::error("invalid variant constructor call - type name must be a string, got invalid argument: " + type_name_obj.to_string_lua());
            }

            auto type_name = type_name_obj.string();
            auto & type = _variant_alloc->dir.resolve(type_name);
            
            if (!type.valid()) {
                return LuaResult::error("no such Variant type '" + type_name + "'");
            }

            new (target_ptr) Variant(Variant::create(*_variant_alloc, type.id));

            return LuaResult(target_obj);
        };

        variant_typedesc.set_dtor_func([](const smen::LuaEngine & engine, Variant & target) {
            target.~Variant();
        });

        variant_typedesc.to_string_func = [](const smen::LuaEngine & engine, const Variant & target) -> std::string {
            return target.to_string();
        };

        variant_typedesc.get_func = [](const smen::LuaEngine & engine, const Variant & target, const LuaObject & key) -> LuaResult {
            if (target.type().category == VariantTypeCategory::LIST) {
                if (key.type() != LuaType::NUMBER) return engine.nil();
                auto index = static_cast<size_t>(key.number());

                if (index == 0) return engine.nil();
                if (index > target.list_size()) return engine.nil();
                index -= 1;

                return LuaResult(engine.native_type_copy("Variant", target.list_get(index)));
            }

            if (key.type() != LuaType::STRING) return engine.nil();

            auto maybe_field_variant = target.try_get_field(key.string());
            if (!maybe_field_variant) {
                return LuaResult::error("field '" + key.string() + "' doesn't exist on Variant of type " + target.type().name);
            }

            auto & field_variant = *maybe_field_variant;
            if (field_variant.type().category == VariantTypeCategory::REFERENCE) {
                if (field_variant.reference().null()) return engine.nil();
            }

            switch (field_variant.type().category) {
            case VariantTypeCategory::BOOLEAN:
                return engine.boolean(field_variant.boolean());
            case VariantTypeCategory::STRING:
                return engine.string(field_variant.string());
            case VariantTypeCategory::INT32:
                return engine.number(field_variant.int32());
            case VariantTypeCategory::UINT32:
                return engine.number(field_variant.uint32());
            case VariantTypeCategory::INT64:
                return engine.int64_cdata(field_variant.int64());
            case VariantTypeCategory::UINT64:
                return engine.uint64_cdata(field_variant.uint64());
            case VariantTypeCategory::FLOAT32:
                return engine.number(static_cast<double>(field_variant.float32()));
            case VariantTypeCategory::FLOAT64:
                return engine.number(field_variant.float64());
            case VariantTypeCategory::COMPLEX:
            case VariantTypeCategory::COMPONENT:
                return engine.native_type_copy("Variant", field_variant);
            case VariantTypeCategory::REFERENCE:
                return engine.native_type_copy("Variant", field_variant.variant());
            case VariantTypeCategory::LIST:
                return engine.native_type_copy("Variant", field_variant);
            case VariantTypeCategory::INVALID:
                throw std::runtime_error("attempted to get variant of invalid type");
            default:
                return engine.nil();
            }
        };

        variant_typedesc.set_func = [&](const smen::LuaEngine & engine, const Variant & target, const LuaObject & key, const LuaObject & value) -> LuaResult {
            auto key_type = key.type();

            std::optional<Variant> created_variant;
            Variant * field_variant = nullptr;

            if (target.type().category == VariantTypeCategory::LIST) {
                if (key_type != LuaType::NUMBER) return LuaResult::error("variant list index must be a number");

                auto index = static_cast<size_t>(key.number());
                if (index > target.list_size()) {
                    return LuaResult::error("index out of bounds: " + std::to_string(index) + " (list size: " + std::to_string(target.list_size()) + ")");
                }

                if (index == 0) {
                    return LuaResult::error("0 is not a valid variant list index");
                }
                index -= 1;

                field_variant = &target.list_ref(index);
            } else {
                if (key_type != LuaType::STRING && key_type != LuaType::NUMBER) return LuaResult::error("variant fields can only have string or numeric keys (invalid key " + key.to_string_lua() + ")");

                const VariantTypeField * field_ptr = nullptr;
                if (key_type == LuaType::STRING) {
                    field_ptr = &target.type().field(key.string());
                } else if (key_type == LuaType::NUMBER) {
                    auto field_id = key.number() - 1;
                    if (field_id < 0 || static_cast<double>(static_cast<size_t>(field_id)) != field_id) return LuaResult::error("numeric variant field index must be a non-negative integer (invalid key " + key.to_string_lua() + ")");
                    field_ptr = &target.type().field(static_cast<size_t>(field_id));
                }
                auto & field = *field_ptr;

                if (!field.valid()) {
                    if (key_type == LuaType::STRING) {
                        return LuaResult::error("variant type " + target.type().name + " does not have a field called " + key.to_string_lua());
                    } else {
                        return LuaResult::error("variant type " + target.type().name + " does not have a field at index " + key.to_string_lua());
                    }
                }

                created_variant = target.get_field(field);
                field_variant = &(*created_variant);
            }

            auto & field_type = field_variant->type();

            auto value_type = value.type();

            if (value_type == LuaType::TABLE) {
                for (auto & pair : value) {
                    auto pair_key_type = pair.key.type();
                    if (pair_key_type != LuaType::STRING && pair_key_type != LuaType::NUMBER) {
                        return LuaResult::error("table form of variant assignment requires that all of the keys in the table are strings or non-negative integer field indices (invalid key " + pair.key.to_string_lua() + " with value " + pair.value.to_string_lua() + ")");
                    }
                    auto result = variant_typedesc.set_func(engine, *field_variant, pair.key, pair.value);
                    if (result.fail()) return result;
                }
            } else if (value_type == LuaType::NUMBER) {
                switch(field_type.category) {
                case VariantTypeCategory::INT32:
                    field_variant->set_int32(static_cast<int32_t>(value.number()));
                    break;
                case VariantTypeCategory::UINT32:
                    field_variant->set_uint32(static_cast<uint32_t>(value.number()));
                    break;
                case VariantTypeCategory::INT64:
                    field_variant->set_int64(static_cast<int64_t>(value.number()));
                    break;
                case VariantTypeCategory::UINT64:
                    field_variant->set_uint64(static_cast<uint64_t>(value.number()));
                    break;
                case VariantTypeCategory::FLOAT32:
                    field_variant->set_float32(static_cast<float>(value.number()));
                    break;
                case VariantTypeCategory::FLOAT64:
                    field_variant->set_float32(static_cast<float>(value.number()));
                    break;
                default:
                    return LuaResult::error("attempted to set field of type " + field_type.name + " to a number");
                }
            } else if (value_type == LuaType::STRING) {
                switch(field_type.category) {
                case VariantTypeCategory::STRING:
                    field_variant->set_string(value.string());
                    break;
                default:
                    return LuaResult::error("attempted to set field of type " + field_type.name + " to a string");
                }
            } else if (value_type == LuaType::BOOLEAN) {
                switch(field_type.category) {
                case VariantTypeCategory::BOOLEAN:
                    field_variant->set_boolean(value.boolean());
                    break;
                default:
                    return LuaResult::error("attempted to set field of type " + field_type.name + " to a boolean");
                }
            } else if (auto cdata = _engine->int64_cdata_value(value)) {
                switch(field_type.category) {
                case VariantTypeCategory::INT64:
                    field_variant->set_int64(*cdata);
                    break;
                default:
                    return LuaResult::error("attempted to set field of type " + field_type.name + " to int64 cdata");
                }
            } else if (auto cdata = _engine->uint64_cdata_value(value)) {
                switch(field_type.category) {
                case VariantTypeCategory::UINT64:
                    field_variant->set_uint64(*cdata);
                    break;
                default:
                    return LuaResult::error("attempted to set field of type " + field_type.name + " to uint64 cdata");
                }
            } else if (_engine->is_native_type(value, "Variant")) {
                auto & value_variant = *value.userdata<Variant>();
                if (field_type.category == VariantTypeCategory::REFERENCE) {
                    auto value_ref = value_variant.reference_to();
                    if (value_ref) {
                        field_variant->set_reference(*value_ref);
                    } else {
                        return LuaResult::error("attempted to set Variant of type " + field_type.name + " (a reference type) to a variant through automatic reference-grabing, but the variant does not have a reference representation");
                    }
                } else {
                    field_variant->set_variant(value_variant);
                }
            } else if (value_type == LuaType::NIL) {
                if (field_variant->type().category == VariantTypeCategory::REFERENCE) {
                    field_variant->set_reference(VariantReference::NULL_REFERENCE);
                } else {
                    return LuaResult::error("attempted to set Variant of type '" + field_type.name + "' (not a reference type) to nil");
                }
            } else {
                return LuaResult::error("invalid variant value: " + value.to_string_lua());
            }

            return LuaResult();
        };

        variant_typedesc.length([](const LuaEngine & engine, const Variant & obj) -> size_t {
            if (obj.type().category == VariantTypeCategory::LIST) {
                return obj.list_size();
            } else {
                throw std::runtime_error("attempted to get length of a non-list Variant type (" + obj.type().name + ")");
            }
        });

        auto add_closure = LuaNativeFunction([](LuaEngine & engine, const LuaNativeFunctionArgs & args) -> LuaResult {
            auto & typedesc = *args[0].userdata<LuaNativeTypeDescriptor<Variant>>();

            if (args.size() < 2) return LuaResult::error("missing self argument to Variant:add");
            if (args.size() < 3) return LuaResult::error("missing argument #1 to Variant:add (value)");
            if (!engine.is_native_type(args[1], "Variant")) return LuaResult::error("self argument of Variant:add is not a Variant");
            auto & self = *args[1].userdata<Variant>();
            auto & self_type = self.type();
            if (self_type.generic_category != VariantGenericCategory::GENERIC_SPECIALIZED) return LuaResult::error("self argument of Variant:add is not a generic Variant type specialization (its type is " + self_type.name + ")");
            if (self_type.category != VariantTypeCategory::LIST) return LuaResult::error("self argument of Variant:add is not a Variant of a list category type (its type is " + self_type.name + ")");

            auto value = args[2];
            auto size = self.list_size();
            auto new_index_num = engine.number(static_cast<LuaNumber>(size + 1));

            self.list_add(Variant::create(self.container(), self.type().element_type_id));
            auto result = typedesc.set_func(engine, self, new_index_num, value);
            if (result.fail()) return result;

            return LuaResult(new_index_num);
        });

        auto remove_func = LuaNativeFunction([](LuaEngine & engine, const LuaNativeFunctionArgs & args) -> LuaResult {
            if (args.size() < 1) return LuaResult::error("missing self argument to Variant:remove");
            if (args.size() < 2) return LuaResult::error("missing argument #1 to Variant:remove (index)");
            if (!engine.is_native_type(args[0], "Variant")) return LuaResult::error("self argument of Variant:remove is not a Variant");
            auto & self = *args[0].userdata<Variant>();
            auto & self_type = self.type();

            if (self_type.generic_category != VariantGenericCategory::GENERIC_SPECIALIZED) return LuaResult::error("self argument of Variant:remove is not a generic Variant type specialization (its type is " + self_type.name + ")");
            if (self_type.category != VariantTypeCategory::LIST) return LuaResult::error("self argument of Variant:remove is not a Variant of a list category type (its type is " + self_type.name + ")");

            auto value = args[1];
            if (value.type() != LuaType::NUMBER) {
                return LuaResult::error("argument #1 of Variant:remove (index) must be a numeric index");
            }
            auto index = static_cast<size_t>(value.number());
            if (index == 0) {
                return LuaResult::error("argument #1 of Variant:remove (index) must be a numeric index (0 is not a valid index)");
            }
            index -= 1;

            auto size = self.list_size();
            if (index > size) {
                return LuaResult::error("index out of bounds: " + std::to_string(index) + " (list size: " + std::to_string(size) + ")");
            }

            self.list_remove(index);
            return LuaResult(engine.number(size - 1));
        });

        variant_typedesc.add_shared_object("add", _engine->closure(add_closure, { _engine->new_light_userdata(&variant_typedesc) }));
        variant_typedesc.add_shared_object("remove", _engine->function(remove_func));
        table.set("Variant", variant_typedesc.metatype);
    }

    void LuaVariantLibrary::_load_vector2_support(LuaObject & table) {
        auto & vec2_typedesc = _engine->create_native_type<Vector2>("Vector2")
            .copy_ctor([](const smen::LuaEngine & engine, const Vector2 & source, Vector2 * target) {
                new (target) Vector2(source);
            })

            .lua_ctor([&](const smen::LuaEngine & engine, LuaObject & target_obj, Vector2 * target_ptr, const std::span<const LuaObject> & args) -> LuaResult {
                // ctor is called through __call in a table
                // - first argument is the meta-type table itself
                // (useless)

                if (args.size() != 3) return LuaResult::error("Vector2 constructor requires 2 arguments (x, y), got " + std::to_string(args.size()));

                auto x = args[1];
                auto y = args[2];

                if (x.type() != LuaType::NUMBER) return LuaResult::error("invalid Vector2 constructor call - received " + x.to_string_lua() + " for the #1 argument (x)");
                if (y.type() != LuaType::NUMBER) return LuaResult::error("invalid Vector2 constructor call - received " + y.to_string_lua() + " for the #2 argument (y)");

                new (target_ptr) Vector2(static_cast<Vector2::ElementType>(x.number()), static_cast<Vector2::ElementType>(y.number()));

                return LuaResult(target_obj);
            })

            // dtor unnecessary
            .to_string([](const smen::LuaEngine & engine, const Vector2 & target) -> std::string {
                std::ostringstream s;
                s << "Vector2(";
                s << target.x << ", " << target.y;
                s << ")";
                return s.str();
            })

            .get([](const smen::LuaEngine & engine, const Vector2 & target, const LuaObject & key) -> LuaResult {
                if (key.type() != LuaType::STRING) return engine.nil();
                auto key_str = key.string();
                if (key_str == "x") return engine.number(target.x);
                if (key_str == "y") return engine.number(target.y);
                return engine.nil();
            })

            .set([](const smen::LuaEngine & engine, const Vector2 & target, const LuaObject & key, const LuaObject & value) -> LuaResult {
                return LuaResult::error("Vector2 instances are immutable");
            })

            .eq([](const smen::LuaEngine & engine, const Vector2 & lhs, const Vector2 & rhs) -> bool {
                return lhs.x == rhs.x && lhs.y == rhs.y;
            });


        vec2_typedesc.metatype_content.set(
            "ZERO", vec2_typedesc.create(Vector2(0, 0))
        );


        table.set("Vector2", vec2_typedesc.metatype);
    }

    void LuaVariantLibrary::_load_vector2f_support(LuaObject & table) {
        auto & vec2_typedesc = _engine->create_native_type<FloatVector2>("Vector2f")
            .copy_ctor([](const smen::LuaEngine & engine, const FloatVector2 & source, FloatVector2 * target) {
                new (target) FloatVector2(source);
            })

            .lua_ctor([&](const smen::LuaEngine & engine, LuaObject & target_obj, FloatVector2 * target_ptr, const std::span<const LuaObject> & args) -> LuaResult {
                // ctor is called through __call in a table
                // - first argument is the meta-type table itself
                // (useless)

                if (args.size() != 3) return LuaResult::error("FloatVector2 constructor requires 2 arguments (x, y), got " + std::to_string(args.size()));

                auto x = args[1];
                auto y = args[2];

                if (x.type() != LuaType::NUMBER) return LuaResult::error("invalid FloatVector2 constructor call - received " + x.to_string_lua() + " for the #1 argument (x)");
                if (y.type() != LuaType::NUMBER) return LuaResult::error("invalid FloatVector2 constructor call - received " + y.to_string_lua() + " for the #2 argument (y)");

                new (target_ptr) FloatVector2(static_cast<FloatVector2::ElementType>(x.number()), static_cast<FloatVector2::ElementType>(y.number()));

                return LuaResult(target_obj);
            })

            // dtor unnecessary
            .to_string([](const smen::LuaEngine & engine, const FloatVector2 & target) -> std::string {
                std::ostringstream s;
                s << "Vector2f(";
                s << target.x << "f, " << target.y << "f";
                s << ")";
                return s.str();
            })

            .get([](const smen::LuaEngine & engine, const FloatVector2 & target, const LuaObject & key) -> LuaResult {
                if (key.type() != LuaType::STRING) return engine.nil();
                auto key_str = key.string();
                if (key_str == "x") return engine.number(static_cast<LuaNumber>(target.x));
                if (key_str == "y") return engine.number(static_cast<LuaNumber>(target.y));
                return engine.nil();
            })

            .set([](const smen::LuaEngine & engine, const FloatVector2 & target, const LuaObject & key, const LuaObject & value) -> LuaResult {
                return LuaResult::error("FloatVector2 instances are immutable");
            })

            .eq([](const smen::LuaEngine & engine, const FloatVector2 & lhs, const FloatVector2 & rhs) -> bool {
                return lhs.x == rhs.x && lhs.y == rhs.y;
            });

        vec2_typedesc.metatype_content.set(
            "ZERO", vec2_typedesc.create(FloatVector2(0, 0))
        );

        table.set("Vector2f", vec2_typedesc.metatype);
    }

    void LuaVariantLibrary::_load_size2_support(LuaObject & table) {
        auto & typedesc = _engine->create_native_type<Size2>("Size2")
            .copy_ctor([](const smen::LuaEngine & engine, const Size2 & source, Size2 * target) {
                new (target) Size2(source);
            })

            .lua_ctor([&](const smen::LuaEngine & engine, LuaObject & target_obj, Size2 * target_ptr, const std::span<const LuaObject> & args) -> LuaResult {
                // ctor is called through __call in a table
                // - first argument is the meta-type table itself
                // (useless)

                if (args.size() != 3) return LuaResult::error("Size2 constructor requires 2 arguments (x, y), got " + std::to_string(args.size()));

                auto x = args[1];
                auto y = args[2];

                if (x.type() != LuaType::NUMBER) return LuaResult::error("invalid Size2 constructor call - received " + x.to_string_lua() + " for the #1 argument (x)");
                if (y.type() != LuaType::NUMBER) return LuaResult::error("invalid Size2 constructor call - received " + y.to_string_lua() + " for the #2 argument (y)");

                new (target_ptr) Size2(static_cast<Size2::ElementType>(x.number()), static_cast<Size2::ElementType>(y.number()));

                return LuaResult(target_obj);
            })

            // dtor unnecessary
            .to_string([](const smen::LuaEngine & engine, const Size2 & target) -> std::string {
                std::ostringstream s;
                s << "Size2(";
                s << target.w << ", " << target.h;
                s << ")";
                return s.str();
            })

            .get([](const smen::LuaEngine & engine, const Size2 & target, const LuaObject & key) -> LuaResult {
                if (key.type() != LuaType::STRING) return engine.nil();
                auto key_str = key.string();
                if (key_str == "w") return engine.number(target.w);
                if (key_str == "h") return engine.number(target.h);
                return engine.nil();
            })

            .set([](const smen::LuaEngine & engine, const Size2 & target, const LuaObject & key, const LuaObject & value) -> LuaResult {
                return LuaResult::error("Size2 instances are immutable");
            })

            .eq([](const smen::LuaEngine & engine, const Size2 & lhs, const Size2 & rhs) -> bool {
                return lhs.w == rhs.w && lhs.h == rhs.h;
            });

        typedesc.metatype_content.set(
            "ZERO", typedesc.create(Size2(0, 0))
        );

        typedesc.metatype_content.set(
            "UNIT", typedesc.create(Size2(1, 1))
        );

        table.set("Size2", typedesc.metatype);
    }

    void LuaVariantLibrary::_load_size2f_support(LuaObject & table) {
        auto & typedesc = _engine->create_native_type<FloatSize2>("Size2f")
            .copy_ctor([](const smen::LuaEngine & engine, const FloatSize2 & source, FloatSize2 * target) {
                new (target) FloatSize2(source);
            })

            .lua_ctor([&](const smen::LuaEngine & engine, LuaObject & target_obj, FloatSize2 * target_ptr, const std::span<const LuaObject> & args) -> LuaResult {
                // ctor is called through __call in a table
                // - first argument is the meta-type table itself
                // (useless)

                if (args.size() != 3) return LuaResult::error("FloatSize2 constructor requires 2 arguments (x, y), got " + std::to_string(args.size()));

                auto x = args[1];
                auto y = args[2];

                if (x.type() != LuaType::NUMBER) return LuaResult::error("invalid FloatSize2 constructor call - received " + x.to_string_lua() + " for the #1 argument (x)");
                if (y.type() != LuaType::NUMBER) return LuaResult::error("invalid FloatSize2 constructor call - received " + y.to_string_lua() + " for the #2 argument (y)");

                new (target_ptr) FloatSize2(static_cast<FloatSize2::ElementType>(x.number()), static_cast<FloatSize2::ElementType>(y.number()));

                return LuaResult(target_obj);
            })

            // dtor unnecessary
            .to_string([](const smen::LuaEngine & engine, const FloatSize2 & target) -> std::string {
                std::ostringstream s;
                s << "Size2f(";
                s << target.w << "f, " << target.h << "f";
                s << ")";
                return s.str();
            })

            .get([](const smen::LuaEngine & engine, const FloatSize2 & target, const LuaObject & key) -> LuaResult {
                if (key.type() != LuaType::STRING) return engine.nil();
                auto key_str = key.string();
                if (key_str == "w") return engine.number(static_cast<LuaNumber>(target.w));
                if (key_str == "h") return engine.number(static_cast<LuaNumber>(target.h));
                return engine.nil();
            })

            .set([](const smen::LuaEngine & engine, const FloatSize2 & target, const LuaObject & key, const LuaObject & value) -> LuaResult {
                return LuaResult::error("FloatSize2 instances are immutable");
            })

            .eq([](const smen::LuaEngine & engine, const FloatSize2 & lhs, const FloatSize2 & rhs) -> bool {
                return lhs.w == rhs.w && lhs.h == rhs.h;
            });

        typedesc.metatype_content.set(
            "ZERO", typedesc.create(FloatSize2(0, 0))
        );

        typedesc.metatype_content.set(
            "UNIT", typedesc.create(FloatSize2(1, 1))
        );

        table.set("Size2f", typedesc.metatype);
    }

    LuaResult LuaVariantLibrary::_scene_spawn_func(LuaEngine & engine, const LuaNativeFunctionArgs & args) {
        static const auto diag = LuaNativeFunctionDiagnostics {
            .name = "spawn",
            .self_type = "Scene",
            .return_type = "entity_id",

            .args = {
                { .name = "name", .type = "string" }
            }
        };

        LuaResult r;
        if (!diag.check_self(r, engine, args)) return r;
        std::string name = "";
        if (diag.check(r, engine, args, 1, { LuaType::STRING })) {
            name = diag.arg(args, 1).string();
        }

        auto & self = **args[0].userdata<Scene *>();
        return LuaResult(engine.number(self.spawn(name)));
    }

    LuaResult LuaVariantLibrary::_scene_all_entities_func(LuaEngine & engine, const LuaNativeFunctionArgs & args) {
        static const auto diag = LuaNativeFunctionDiagnostics {
            .name = "all_entities",
            .self_type = "Scene",
            .return_type = "table[entity_id]",

            .args = {}
        };

        LuaResult r;
        if (!diag.check_self(r, engine, args)) return r;

        auto table = engine.new_table();
        auto & self = **args[0].userdata<Scene *>();
        LuaNumber index = 0;
        for (auto & ent_id : self) {
            index += 1;
            table.set(index, engine.number(ent_id));
        }
        return table;
    }

    LuaResult LuaVariantLibrary::_scene_add_component_func(LuaEngine & engine, const LuaNativeFunctionArgs & args) {
        static const auto diag = LuaNativeFunctionDiagnostics {
            .name = "add_component",
            .self_type = "Scene",
            .return_type = "Variant",

            .args = {
                { .name = "entity_id" },
                { .name = "component", .type = "string | Variant" }
            }
        };

        LuaResult r;
        if (!diag.check_self(r, engine, args)) return r;
        if (!diag.check(r, engine, args, 1, { LuaType::NUMBER })) return r;

        if (!diag.check(r, engine, args, 2)) return r;

        auto & scene = ** diag.self(args).userdata<Scene *>();
        auto entity_id = static_cast<EntityID>(diag.arg(args, 1).number());

        auto & component_or_type = diag.arg(args, 2);
        if (component_or_type.type() == LuaType::STRING) {
            auto component_type = diag.arg(args, 2).string();

            auto variant = scene.add_component(entity_id, component_type);
            return engine.native_type_copy("Variant", variant);
        } else if (engine.is_native_type(component_or_type, "Variant")) {
            scene.add_component(entity_id, *component_or_type.userdata<Variant>());
            return component_or_type;
        } else {
            return diag.error_expected(2, "- expected string type name or Variant instance of component, got " + component_or_type.to_string_lua());
        }

    }

    LuaResult LuaVariantLibrary::_scene_has_component_func(LuaEngine & engine, const LuaNativeFunctionArgs & args) {
        static const auto diag = LuaNativeFunctionDiagnostics {
            .name = "has_component",
            .self_type = "Scene",
            .return_type = "boolean",

            .args = {
                { .name = "entity_id" },
                { .name = "component", .type = "string" }
            }
        };

        LuaResult r;
        if (!diag.check_self(r, engine, args)) return r;
        if (!diag.check(r, engine, args, 1, { LuaType::NUMBER })) return r;

        if (!diag.check(r, engine, args, 2, { LuaType::STRING })) return r;

        auto & scene = ** diag.self(args).userdata<Scene *>();
        auto entity_id = static_cast<EntityID>(diag.arg(args, 1).number());

        auto component_type = diag.arg(args, 2).string();
        return engine.boolean(scene.has_component(entity_id, component_type));
    }

    LuaResult LuaVariantLibrary::_scene_entities_with_component_func(LuaEngine & engine, const LuaNativeFunctionArgs & args) {
        static const auto diag = LuaNativeFunctionDiagnostics {
            .name = "entities_with_component",
            .self_type = "Scene",
            .return_type = "Variant",

            .args = {
                { .name = "component", .type = "string" }
            }
        };

        LuaResult r;
        if (!diag.check_self(r, engine, args)) return r;

        if (!diag.check(r, engine, args, 1, { LuaType::STRING })) return r;

        auto & scene = ** diag.self(args).userdata<Scene *>();

        auto component_type = diag.arg(args, 1).string();
        std::vector<EntityID> entity_ids = scene.entities_with_component(component_type);

        auto table = engine.new_table();
        LuaNumber index = 0;

        for (auto & ent_id : entity_ids) {
            index += 1;
            table.set(index, ent_id);
        }
        return table;
    }

    LuaResult LuaVariantLibrary::_scene_get_component_func(LuaEngine & engine, const LuaNativeFunctionArgs & args) {
        static const auto diag = LuaNativeFunctionDiagnostics {
            .name = "get_component",
            .self_type = "Scene",
            .return_type = "Variant",

            .args = {
                { .name = "entity_id" },
                { .name = "component", .type = "string" }
            }
        };

        LuaResult r;
        if (!diag.check_self(r, engine, args)) return r;
        if (!diag.check(r, engine, args, 1, { LuaType::NUMBER })) return r;

        if (!diag.check(r, engine, args, 2, { LuaType::STRING })) return r;

        auto & scene = ** diag.self(args).userdata<Scene *>();
        auto entity_id = static_cast<EntityID>(diag.arg(args, 1).number());

        auto component_type = diag.arg(args, 2).string();
        auto comp = scene.get_component(entity_id, component_type);

        if (!comp) return engine.nil();

        return engine.native_type_copy("Variant", *comp);
    }

    LuaResult LuaVariantLibrary::_scene_get_components_func(LuaEngine & engine, const LuaNativeFunctionArgs & args) {
        static const auto diag = LuaNativeFunctionDiagnostics {
            .name = "get_components",
            .self_type = "Scene",
            .return_type = "table[Variant]",

            .args = {
                { .name = "entity_id" },
                { .name = "component", .type = "string|nil" }
            }
        };

        LuaResult r;
        if (!diag.check_self(r, engine, args)) return r;
        if (!diag.check(r, engine, args, 1, { LuaType::NUMBER })) return r;

        auto & scene = ** diag.self(args).userdata<Scene *>();
        auto entity_id = static_cast<EntityID>(diag.arg(args, 1).number());

        if (diag.check(r, engine, args, 2)) {
            auto component_arg = diag.arg(args, 2);
            if (component_arg.type() != LuaType::STRING) {
                return diag.error_expected(2, "- expected string type name, got " + component_arg.to_string_lua());
            }

            auto component_type = component_arg.string();
            auto components = scene.get_components(entity_id, component_type);

            auto table = engine.new_table();
            LuaNumber index = 0;
            for (auto & comp : components) {
                index += 1;
                table.set(index, engine.native_type_copy("Variant", comp));
            }
            return table;
        } else {
            auto components = scene.get_components(entity_id);
            auto table = engine.new_table();
            LuaNumber index = 0;
            for (auto & comp : components) {
                index += 1;
                table.set(index, engine.native_type_copy("Variant", comp));
            }
            return table;
        }
    }

    LuaResult LuaVariantLibrary::_scene_children_of_func(LuaEngine & engine, const LuaNativeFunctionArgs & args) {
        static const auto diag = LuaNativeFunctionDiagnostics {
            .name = "children_of",
            .self_type = "Scene",
            .return_type = "table[entity_id]",

            .args = {
                { .name = "entity_id" },
            }
        };

        LuaResult r;
        if (!diag.check_self(r, engine, args)) return r;
        if (!diag.check(r, engine, args, 1, { LuaType::NUMBER })) return r;

        auto & self = ** diag.self(args).userdata<Scene *>();
        auto entity_id = static_cast<EntityID>(diag.arg(args, 1).number());

        auto table = engine.new_table();
        auto component_type = diag.arg(args, 2).string();
        auto & child_entity_ids = self.children_of(entity_id);
        LuaNumber index = 0;
        for (auto & ent_id : child_entity_ids) {
            index += 1;
            table.set(index, engine.number(ent_id));
        }
        return table;
    }

    LuaResult LuaVariantLibrary::_scene_has_children_func(LuaEngine & engine, const LuaNativeFunctionArgs & args) {
        static const auto diag = LuaNativeFunctionDiagnostics {
            .name = "has_children",
            .self_type = "Scene",
            .return_type = "boolean",

            .args = {
                { .name = "entity_id" },
            }
        };

        LuaResult r;
        if (!diag.check_self(r, engine, args)) return r;
        if (!diag.check(r, engine, args, 1, { LuaType::NUMBER })) return r;

        auto & self = ** diag.self(args).userdata<Scene *>();
        auto entity_id = static_cast<EntityID>(diag.arg(args, 1).number());

        return engine.boolean(self.has_children(entity_id));
    }

    LuaResult LuaVariantLibrary::_scene_parent_of_func(LuaEngine & engine, const LuaNativeFunctionArgs & args) {
        static const auto diag = LuaNativeFunctionDiagnostics {
            .name = "parent_of",
            .self_type = "Scene",
            .return_type = "entity_id|nil",

            .args = {
                { .name = "entity_id" },
            }
        };

        LuaResult r;
        if (!diag.check_self(r, engine, args)) return r;
        if (!diag.check(r, engine, args, 1, { LuaType::NUMBER })) return r;

        auto & self = ** diag.self(args).userdata<Scene *>();
        auto entity_id = static_cast<EntityID>(diag.arg(args, 1).number());

        auto parent_id = self.parent_of(entity_id);
        if (parent_id) {
            return engine.number(*parent_id);
        } else {
            return engine.nil();
        }
    }

    LuaResult LuaVariantLibrary::_scene_is_child_of_func(LuaEngine & engine, const LuaNativeFunctionArgs & args) {
        static const auto diag = LuaNativeFunctionDiagnostics {
            .name = "parent_of",
            .self_type = "Scene",
            .return_type = "boolean",

            .args = {
                { .name = "parent_id", .type = "entity_id" },
                { .name = "child_id", .type = "entity_id" },
            }
        };

        LuaResult r;
        if (!diag.check_self(r, engine, args)) return r;
        if (!diag.check(r, engine, args, 1, { LuaType::NUMBER })) return r;
        if (!diag.check(r, engine, args, 2, { LuaType::NUMBER })) return r;

        auto & self = ** diag.self(args).userdata<Scene *>();

        auto parent_id = static_cast<EntityID>(diag.arg(args, 1).number());
        auto child_id = static_cast<EntityID>(diag.arg(args, 2).number());

        return engine.boolean(self.is_child_of(parent_id, child_id));
    }

    LuaResult LuaVariantLibrary::_scene_has_parent_func(LuaEngine & engine, const LuaNativeFunctionArgs & args) {
        static const auto diag = LuaNativeFunctionDiagnostics {
            .name = "has_parent",
            .self_type = "Scene",
            .return_type = "boolean",

            .args = {
                { .name = "entity_id" }
            }
        };

        LuaResult r;
        if (!diag.check_self(r, engine, args)) return r;
        if (!diag.check(r, engine, args, 1, { LuaType::NUMBER })) return r;

        auto & self = ** diag.self(args).userdata<Scene *>();

        auto entity_id = static_cast<EntityID>(diag.arg(args, 1).number());

        return engine.boolean(self.has_parent(entity_id));
    }

    LuaResult LuaVariantLibrary::_scene_adopt_child_func(LuaEngine & engine, const LuaNativeFunctionArgs & args) {
        static const auto diag = LuaNativeFunctionDiagnostics {
            .name = "adopt_child",
            .self_type = "Scene",

            .args = {
                { .name = "parent_id", .type = "entity_id" },
                { .name = "child_id", .type = "entity_id" },
            }
        };

        LuaResult r;
        if (!diag.check_self(r, engine, args)) return r;
        if (!diag.check(r, engine, args, 1, { LuaType::NUMBER })) return r;
        if (!diag.check(r, engine, args, 2, { LuaType::NUMBER })) return r;

        auto & self = ** diag.self(args).userdata<Scene *>();

        auto parent_id = static_cast<EntityID>(diag.arg(args, 1).number());
        auto child_id = static_cast<EntityID>(diag.arg(args, 2).number());

        self.adopt_child(parent_id, child_id);
        return LuaResult();
    }

    LuaResult LuaVariantLibrary::_scene_spawn_child_func(LuaEngine & engine, const LuaNativeFunctionArgs & args) {
        static const auto diag = LuaNativeFunctionDiagnostics {
            .name = "spawn_child",
            .self_type = "Scene",
            .return_type = "entity_id",

            .args = {
                { .name = "parent_id", .type = "entity_id" },
                { .name = "name", .type = "string|nil" },
            }
        };

        LuaResult r;
        if (!diag.check_self(r, engine, args)) return r;
        if (!diag.check(r, engine, args, 1, { LuaType::NUMBER })) return r;

        std::string name = "";
        if (diag.check(r, engine, args, 2, { LuaType::STRING })) {
            name = diag.arg(args, 2).string();
        }

        auto & self = ** diag.self(args).userdata<Scene *>();

        auto parent_id = static_cast<EntityID>(diag.arg(args, 1).number());

        return engine.number(self.spawn_child(parent_id, name));
    }

    LuaResult LuaVariantLibrary::_scene_make_orphan_func(LuaEngine & engine, const LuaNativeFunctionArgs & args) {
        static const auto diag = LuaNativeFunctionDiagnostics {
            .name = "make_orphan",
            .self_type = "Scene",

            .args = {
                { .name = "entity_id" },
            }
        };

        LuaResult r;
        if (!diag.check_self(r, engine, args)) return r;
        if (!diag.check(r, engine, args, 1, { LuaType::NUMBER })) return r;

        auto & self = ** diag.self(args).userdata<Scene *>();

        auto entity_id = static_cast<EntityID>(diag.arg(args, 1).number());

        self.make_orphan(entity_id);
        return LuaResult();
    }

    LuaResult LuaVariantLibrary::_scene_remove_component_func(LuaEngine & engine, const LuaNativeFunctionArgs & args) {
        static const auto diag = LuaNativeFunctionDiagnostics {
            .name = "remove_component",
            .self_type = "Scene",
            .return_type = "boolean",

            .args = {
                { .name = "entity_id" },
                { .name = "component", .type = "string | Variant" }
            }
        };

        LuaResult r;
        if (!diag.check_self(r, engine, args)) return r;
        if (!diag.check(r, engine, args, 1, { LuaType::NUMBER })) return r;

        if (!diag.check(r, engine, args, 2)) return r;

        auto & scene = ** diag.self(args).userdata<Scene *>();

        auto entity_id = static_cast<EntityID>(diag.arg(args, 1).number());

        auto & component_or_type = diag.arg(args, 2);
        if (component_or_type.type() == LuaType::STRING) {
            auto component_type = diag.arg(args, 2).string();

            return engine.boolean(scene.remove_component(entity_id, component_type));
        } else if (engine.is_native_type(component_or_type, "Variant")) {
            return engine.boolean(scene.remove_component(entity_id, *component_or_type.userdata<Variant>()));
        } else {
            return diag.error_expected(2, "- expected string type name or Variant instance of component, got " + component_or_type.to_string_lua());
        }
    }

    LuaResult LuaVariantLibrary::_scene_find_by_name_func(LuaEngine & engine, const LuaNativeFunctionArgs & args) {
        static const auto diag = LuaNativeFunctionDiagnostics {
            .name = "find_by_name",
            .self_type = "Scene",
            .return_type = "table[entity_id]",

            .args = {
                { .name = "name", .type = "string" },
            }
        };

        LuaResult r;
        if (!diag.check_self(r, engine, args)) return r;
        if (!diag.check(r, engine, args, 1, { LuaType::STRING })) return r;

        auto & self = ** diag.self(args).userdata<Scene *>();
        auto name = diag.arg(args, 1).string();

        auto vec = self.find_by_name(name);
        auto table = engine.new_table();
        LuaNumber index = 0;

        for (auto & ent_id : vec) {
            index += 1;
            table.set(index, ent_id);
        }

        return table;
    }

    LuaResult LuaVariantLibrary::_scene_find_first_by_name_func(LuaEngine & engine, const LuaNativeFunctionArgs & args) {
        static const auto diag = LuaNativeFunctionDiagnostics {
            .name = "find_first_by_name",
            .self_type = "Scene",
            .return_type = "entity_id|nil",

            .args = {
                { .name = "name", .type = "string" },
            }
        };

        LuaResult r;
        if (!diag.check_self(r, engine, args)) return r;
        if (!diag.check(r, engine, args, 1, { LuaType::STRING })) return r;

        auto & self = ** diag.self(args).userdata<Scene *>();
        auto name = diag.arg(args, 1).string();

        auto vec = self.find_by_name(name);
        auto table = engine.new_table();
        auto ent_id = self.find_first_by_name(name);
        if (ent_id == 0) return engine.nil();
        return engine.number(ent_id);
    }

    LuaResult LuaVariantLibrary::_scene_name_of_func(LuaEngine & engine, const LuaNativeFunctionArgs & args) {
        static const auto diag = LuaNativeFunctionDiagnostics {
            .name = "name_of",
            .self_type = "Scene",
            .return_type = "string",

            .args = {
                { .name = "entity_id" },
            }
        };

        LuaResult r;
        if (!diag.check_self(r, engine, args)) return r;
        if (!diag.check(r, engine, args, 1, { LuaType::NUMBER })) return r;

        auto & self = ** diag.self(args).userdata<Scene *>();

        auto entity_id = static_cast<EntityID>(diag.arg(args, 1).number());

        return engine.string(self.name_of(entity_id));
    }

    LuaResult LuaVariantLibrary::_scene_set_name_func(LuaEngine & engine, const LuaNativeFunctionArgs & args) {
        static const auto diag = LuaNativeFunctionDiagnostics {
            .name = "set_name",
            .self_type = "Scene",

            .args = {
                { .name = "entity_id" },
                { .name = "name", .type = "string" },
            }
        };

        LuaResult r;
        if (!diag.check_self(r, engine, args)) return r;
        if (!diag.check(r, engine, args, 1, { LuaType::NUMBER })) return r;

        auto & self = ** diag.self(args).userdata<Scene *>();

        auto entity_id = static_cast<EntityID>(diag.arg(args, 1).number());
        auto name = diag.arg(args, 2).string();

        self.set_name(entity_id, name);
        return LuaResult();
    }

    LuaResult LuaVariantLibrary::_scene_kill_func(LuaEngine & engine, const LuaNativeFunctionArgs & args) {
        static const auto diag = LuaNativeFunctionDiagnostics {
            .name = "kill",
            .self_type = "Scene",

            .args = {
                { .name = "entity_id" },
            }
        };

        LuaResult r;
        if (!diag.check_self(r, engine, args)) return r;
        if (!diag.check(r, engine, args, 1, { LuaType::NUMBER })) return r;

        auto & self = ** diag.self(args).userdata<Scene *>();

        auto entity_id = static_cast<EntityID>(diag.arg(args, 1).number());

        self.kill(entity_id);
        return LuaResult();
    }

    void LuaVariantLibrary::_load_scene_support(LuaObject & table) {
        auto & typedesc = _engine->create_native_type<Scene *>("Scene")
            .copy_ctor([](const smen::LuaEngine & engine, Scene * const & source, Scene * * target) {
                new (target) Scene *(source);
            })

            // dtor unnecessary
            
            .to_string([](const smen::LuaEngine & engine, Scene * const & target) -> std::string {
                std::ostringstream s;
                s << "Scene(";
                s << target->name();
                s << ")";
                return s.str();
            })

            .get([](const smen::LuaEngine & engine, Scene * const & target, const LuaObject & key) -> LuaResult {
                if (key.type() != LuaType::STRING) return engine.nil();
                auto key_str = key.string();
                if (key_str == "name") return engine.string(target->name());
                if (key_str == "enabled") return engine.boolean(target->enabled());
                return engine.nil();
            })

            .eq([](const smen::LuaEngine & engine, Scene * const & lhs, Scene * const & rhs) -> bool {
                    return lhs == rhs;
            });

        typedesc.add_shared_object("spawn", _engine->function(_scene_spawn_func));
        typedesc.add_shared_object("all_entities", _engine->function(_scene_all_entities_func));
        typedesc.add_shared_object("add_component", _engine->function(_scene_add_component_func));
        typedesc.add_shared_object("has_component", _engine->function(_scene_has_component_func));
        typedesc.add_shared_object("entities_with_component", _engine->function(_scene_entities_with_component_func));
        typedesc.add_shared_object("get_component", _engine->function(_scene_get_component_func));
        typedesc.add_shared_object("get_components", _engine->function(_scene_get_components_func));
        typedesc.add_shared_object("children_of", _engine->function(_scene_children_of_func));
        typedesc.add_shared_object("has_children", _engine->function(_scene_has_children_func));
        typedesc.add_shared_object("parent_of", _engine->function(_scene_parent_of_func));
        typedesc.add_shared_object("is_child_of", _engine->function(_scene_is_child_of_func));
        typedesc.add_shared_object("has_parent", _engine->function(_scene_has_parent_func));

        typedesc.add_shared_object("adopt_child", _engine->function(_scene_adopt_child_func));
        typedesc.add_shared_object("spawn_child", _engine->function(_scene_spawn_child_func));
        typedesc.add_shared_object("make_orphan", _engine->function(_scene_make_orphan_func));

        typedesc.add_shared_object("remove_component", _engine->function(_scene_remove_component_func));
        typedesc.add_shared_object("find_first_by_name", _engine->function(_scene_find_first_by_name_func));
        typedesc.add_shared_object("find_by_name", _engine->function(_scene_find_by_name_func));
        typedesc.add_shared_object("name_of", _engine->function(_scene_name_of_func));
        typedesc.add_shared_object("set_name", _engine->function(_scene_set_name_func));
        typedesc.add_shared_object("kill", _engine->function(_scene_kill_func));
    }

    LuaResult LuaVariantLibrary::_renderer_render_func(LuaEngine & engine, const LuaNativeFunctionArgs & args) {
        static const auto diag = LuaNativeFunctionDiagnostics {
            .name = "render",
            .self_type = "Renderer",

            .args = {
                { .name = "texture", .type = "Texture" },
                { .name = "position", .type = "Vector2" },
                { .name = "scale", .type = "Size2f|Size2" },
            }
        };

        LuaResult r;
        if (!diag.check_self(r, engine, args)) return r;
        if (!diag.check(r, engine, args, 1, "Texture")) return r;
        auto position = Vector2(0, 0);
        auto scale = FloatSize2(1, 1);

        if (diag.check(r, engine, args, 2)) {
            auto pos_arg = diag.arg(args, 2);
            if (!engine.is_native_type(pos_arg, "Vector2")) {
            return diag.error_expected(2, "- expected Vector2 position, got " + pos_arg.to_string_lua());
            }

            position = *pos_arg.userdata<Vector2>();
        }

        if (diag.check(r, engine, args, 3)) {
            auto scale_arg = diag.arg(args, 3);
            if (engine.is_native_type(scale_arg, "Size2")) {
                auto & int_scale = *scale_arg.userdata<Size2>();
                scale = FloatSize2(static_cast<float>(int_scale.w), static_cast<float>(int_scale.h));
            } else if (engine.is_native_type(scale_arg, "Size2f")) {
                scale = *scale_arg.userdata<FloatSize2>();
            } else {
            return diag.error_expected(2, "- expected Size2f/Size scale, got " + scale_arg.to_string_lua());
            }
        }

        auto & self = ** diag.self(args).userdata<Renderer *>();
        auto & tex = *diag.arg(args, 1).userdata<Texture>();

        self.render(tex, position, scale);
        return LuaResult();
    }

    void LuaVariantLibrary::_load_renderer_support(LuaObject & table) {
        auto & typedesc = _engine->create_native_type<Renderer *>("Renderer")
            .copy_ctor([](const smen::LuaEngine & engine, Renderer * const & source, Renderer * * target) {
                new (target) Renderer *(source);
            })

            // dtor unnecessary
            
            .to_string([](const smen::LuaEngine & engine, Renderer * const & target) -> std::string {
                std::ostringstream s;
                s << "Renderer";
                return s.str();
            })

            .get([](const smen::LuaEngine & engine, Renderer * const & target, const LuaObject & key) -> LuaResult {
                if (key.type() != LuaType::STRING) return engine.nil();

                auto key_str = key.string();
                if (key_str == "logical_size") return engine.native_type_copy("Size2", target->logical_size());

                return engine.nil();
            })

            .set([](const smen::LuaEngine & engine, Renderer * const & target, const LuaObject & key, const LuaObject & value) -> LuaResult {
                if (key.type() != LuaType::STRING) return LuaResult::error("cannot est non-string key on Renderer: '" + key.to_string_lua() + "'");

                auto key_str = key.string();
                if (key_str == "logical_size") {
                    if (!engine.is_native_type(value, "Size2")) {
                        return LuaResult::error("Renderer.logical_size must be a Size2 instance, got: " + value.to_string_lua());
                    }

                    target->set_logical_size(*value.userdata<Size2>());
                } else {
                    return LuaResult::error("attempted to set invalid Renderer field: '" + key_str + "'");
                }

                return LuaResult();
            });

        typedesc.add_shared_object("render", _engine->function(_renderer_render_func));
    }

    LuaResult LuaVariantLibrary::_gui_begin_func(LuaEngine & engine, const LuaNativeFunctionArgs & args) {
        static const auto diag = LuaNativeFunctionDiagnostics {
            .name = "begin",
            .self_type = "GUIContext",

            .args = {
                { .name = "format_or_text", .type = "string" },
                { .name = "text", .type = "string|nil" },
            }
        };

        LuaResult r;
        if (!diag.check_self(r, engine, args)) return r;

        if (diag.self(args).userdata<GUIContext *>() == nullptr) return LuaResult::error("GUI is not available (smen.gui.available == false)");

        auto & self = ** diag.self(args).userdata<GUIContext *>();
        if (!self.drawing()) return LuaResult::error("GUI function cannot be used outside of draw routine (smen.gui.drawing == false)");

        std::string format = "%s";
        std::string text;

        if (!diag.check(r, engine, args, 1, { LuaType::STRING })) return r;

        if (diag.check(r, engine, args, 2)) {
            auto text_arg = diag.arg(args, 2);
            if (text_arg.type() != LuaType::STRING) {
                return diag.error_expected(2, "- expected title text, got " + text_arg.to_string_lua());
            }
            format = diag.arg(args, 1).string();
            text = diag.arg(args, 2).string();
        } else {
            text = diag.arg(args, 1).string();
        }

        ImGui::Text(format.c_str(), text.c_str());
        return LuaResult();
    }

    void LuaVariantLibrary::_load_gui_support(LuaObject & table) {
        auto & typedesc = _engine->create_native_type<GUIContext *>("GUIContext")
            .copy_ctor([](const smen::LuaEngine & engine, GUIContext * const & source, GUIContext * * target) {
                new (target) GUIContext *(source);
            })

            // dtor unnecessary
            
            .to_string([](const smen::LuaEngine & engine, GUIContext * const & target) -> std::string {
                std::ostringstream s;
                s << "GUIContext";
                return s.str();
            })

            .get([](const smen::LuaEngine & engine, GUIContext * const & target, const LuaObject & key) -> LuaResult {
                if (key.type() != LuaType::STRING) return engine.nil();

                auto key_str = key.string();
                if (key_str == "available") return engine.boolean(target != nullptr);
                else if (key_str == "drawing") return engine.boolean(target == nullptr ? false : target->drawing());
                if (target == nullptr) return engine.nil();

                return engine.nil();
            });
    }

    void LuaVariantLibrary::_load_window_support(LuaObject & table) {
        auto & typedesc = _engine->create_native_type<Window *>("Window")
            .copy_ctor([](const smen::LuaEngine & engine, Window * const & source, Window * * target) {
                new (target) Window *(source);
            })

            // dtor unnecessary
            
            .to_string([](const smen::LuaEngine & engine, Window * const & target) -> std::string {
                std::ostringstream s;
                s << "Window(";
                s << target->title();
                s << ")";
                return s.str();
            })

            .get([](const smen::LuaEngine & engine, Window * const & target, const LuaObject & key) -> LuaResult {
                if (key.type() != LuaType::STRING) return engine.nil();

                auto key_str = key.string();
                if (key_str == "size") return engine.native_type_copy("Size2", target->size());
                else if (key_str == "title") return engine.string(target->title());

                return engine.nil();
            })

            .set([](const smen::LuaEngine & engine, Window * const & target, const LuaObject & key, const LuaObject & value) -> LuaResult {
                if (key.type() != LuaType::STRING) return LuaResult::error("cannot est non-string key on Window: '" + key.to_string_lua() + "'");

                auto key_str = key.string();
                if (key_str == "size") {
                    if (!engine.is_native_type(value, "Size2")) {
                        return LuaResult::error("Window.size must be a Size2 instance, got: " + value.to_string_lua());
                    }

                    target->set_size(*value.userdata<Size2>());
                } else if (key_str == "title") {
                    if (value.type() != LuaType::STRING) {
                        return LuaResult::error("Window.title must be a string, got: " + value.to_string_lua());
                    }

                    target->set_title(value.string());
                } else {
                    return LuaResult::error("attempted to set invalid Window field: '" + key_str + "'");
                }
                return LuaResult();
            });
    }

    LuaResult LuaVariantLibrary::_texture_load_func(LuaEngine & engine, const LuaNativeFunctionArgs & args) {
        static const auto diag = LuaNativeFunctionDiagnostics {
            .name = "Texture:load",

            .args = {
                { .name = "path", .type = "string" },
            },

            .upvalues = 1,
        };

        auto & lib = *args[0].userdata<LuaVariantLibrary>();

        LuaResult r;
        if (!diag.check(r, engine, args, 1, { LuaType::STRING })) return r;
        auto path = diag.arg(args, 1).string();

        auto tex = Texture(*lib._current_renderer_ptr, path);
        if (!tex.valid()) return engine.nil();
        auto obj = engine.native_type_move("Texture", std::move(tex));
        return obj;
    }

    void LuaVariantLibrary::_load_texture_support(LuaObject & table) {
        auto & typedesc = _engine->create_native_type<Texture>("Texture")
            .move_ctor([](const smen::LuaEngine & engine, Texture && source, Texture * target) {
                new (target) Texture(std::move(source));
            })

            .dtor([](const smen::LuaEngine & engine, Texture & target) {
                target.~Texture();
            })

            .to_string([](const smen::LuaEngine & engine, const Texture & target) -> std::string {
                std::ostringstream s;
                auto tex_size = target.size();
                s << "Texture(";
                s << tex_size.w << ", " << tex_size.h;
                s << ")";
                return s.str();
            })

            .get([](const smen::LuaEngine & engine, const Texture & target, const LuaObject & key) -> LuaResult {
                if (key.type() != LuaType::STRING) return engine.nil();
                auto key_str = key.string();
                if (key_str == "width") return engine.number(target.width());
                if (key_str == "height") return engine.number(target.height());
                if (key_str == "size") return engine.native_type_copy("Size2", target.size());
                return engine.nil();
            })

            .eq([](const smen::LuaEngine & engine, const Texture & lhs, const Texture & rhs) -> bool {
                return &lhs == &rhs;
            });

        

        typedesc.metatype_content.set(
            "load", _engine->closure(_texture_load_func, { _engine->new_light_userdata(this) })
        );

        table.set("Texture", typedesc.metatype);
    }

    void LuaVariantLibrary::_load_system_callback_support(LuaObject & table) {
        auto func_db = _engine->new_table();

        table.set("func_db", func_db);

        auto register_closure = LuaNativeFunction([](LuaEngine & engine, const LuaNativeFunctionArgs & args) -> LuaResult {
            static auto valid_function_dbs = std::vector<std::string> {
                "system.process (ent_id : EntityID)",
                "system.render (ent_id : EntityID)"
            };

            auto & lib = *args[0].userdata<LuaVariantLibrary>();
            if (lib._current_scene_ptr == nullptr) return LuaResult::error("script is not running in an active scene");

            if (args.size() == 1) return LuaResult::error("missing argument #1 to func_db.register (db)");
            if (args.size() == 2) return LuaResult::error("missing argument #2 to func_db.register (name)");
            if (args.size() == 3) return LuaResult::error("missing argument #3 to func_db.register (func)");

            if (args[1].type() != LuaType::STRING) return LuaResult::error("expected argument #1 to func_db.register (db) to be a string, got " + args[1].to_string_lua());
            if (args[2].type() != LuaType::STRING) return LuaResult::error("expected argument #2 to func_db.register (name) to be a string, got " + args[2].to_string_lua());
            if (args[3].type() != LuaType::FUNCTION) return LuaResult::error("expected argument #3 to func_db.register (func) to be a function, got " + args[3].to_string_lua());

            auto db = args[1].string();
            auto name = args[2].string();
            auto func = args[3];
            
            auto & system_container = lib._current_scene_ptr->system_container();

            if (db == "system.process") {
                auto new_entry = system_container.process_db.reg(name, [name = name, func = func, & engine = engine](Scene &, EntityID ent_id, double delta) {
                    auto result = func.call({ engine.number(ent_id), engine.number(delta) });
                    if (result.fail()) throw std::runtime_error("error in system.process function '" + name + "': " + result.error_msg());
                });

                lib._func_db_process_list.emplace_back(std::move(new_entry));
            } else if (db == "system.render") {
                auto new_entry = system_container.render_db.reg(name, [name = name, func = func, & engine = engine](Scene &, EntityID ent_id) {
                    auto result = func.call({ engine.number(ent_id) });
                    if (result.fail()) throw std::runtime_error("error in system.render function '" + name + "': " + result.error_msg());
                });

                lib._func_db_render_list.emplace_back(std::move(new_entry));
            } else {
                auto s = std::ostringstream();
                s << "invalid function database: '" << db << "', ";
                s << "the following databases are available:";
                for (auto & db_info : valid_function_dbs) {
                    s << "\n- " << db_info;
                }
                return LuaResult::error(s.str());
            }

            return LuaResult();
        });

        func_db.set("register", _engine->closure(register_closure, { _engine->new_light_userdata(this) }));
    }


    LuaObject LuaVariantLibrary::_create_key_table() {
        // autogenerated!
        std::string key_table_script = 
            "return {\n"
            "['a'] = 4,\n"
            "['b'] = 5,\n"
            "['c'] = 6,\n"
            "['d'] = 7,\n"
            "['e'] = 8,\n"
            "['f'] = 9,\n"
            "['g'] = 10,\n"
            "['h'] = 11,\n"
            "['i'] = 12,\n"
            "['j'] = 13,\n"
            "['k'] = 14,\n"
            "['l'] = 15,\n"
            "['m'] = 16,\n"
            "['n'] = 17,\n"
            "['o'] = 18,\n"
            "['p'] = 19,\n"
            "['q'] = 20,\n"
            "['r'] = 21,\n"
            "['s'] = 22,\n"
            "['t'] = 23,\n"
            "['u'] = 24,\n"
            "['v'] = 25,\n"
            "['w'] = 26,\n"
            "['x'] = 27,\n"
            "['y'] = 28,\n"
            "['z'] = 29,\n"
            "['1'] = 30,\n"
            "['2'] = 31,\n"
            "['3'] = 32,\n"
            "['4'] = 33,\n"
            "['5'] = 34,\n"
            "['6'] = 35,\n"
            "['7'] = 36,\n"
            "['8'] = 37,\n"
            "['9'] = 38,\n"
            "['0'] = 39,\n"
            "['return'] = 40,\n"
            "['escape'] = 41,\n"
            "['backspace'] = 42,\n"
            "['tab'] = 43,\n"
            "['space'] = 44,\n"
            "['minus'] = 45,\n"
            "['equals'] = 46,\n"
            "['leftbracket'] = 47,\n"
            "['rightbracket'] = 48,\n"
            "['backslash'] = 49,\n"
            "['nonushash'] = 50,\n"
            "['semicolon'] = 51,\n"
            "['apostrophe'] = 52,\n"
            "['grave'] = 53,\n"
            "['comma'] = 54,\n"
            "['period'] = 55,\n"
            "['slash'] = 56,\n"
            "['capslock'] = 57,\n"
            "['f1'] = 58,\n"
            "['f2'] = 59,\n"
            "['f3'] = 60,\n"
            "['f4'] = 61,\n"
            "['f5'] = 62,\n"
            "['f6'] = 63,\n"
            "['f7'] = 64,\n"
            "['f8'] = 65,\n"
            "['f9'] = 66,\n"
            "['f10'] = 67,\n"
            "['f11'] = 68,\n"
            "['f12'] = 69,\n"
            "['printscreen'] = 70,\n"
            "['scrolllock'] = 71,\n"
            "['pause'] = 72,\n"
            "['insert'] = 73,\n"
            "['home'] = 74,\n"
            "['pageup'] = 75,\n"
            "['delete'] = 76,\n"
            "['end'] = 77,\n"
            "['pagedown'] = 78,\n"
            "['right'] = 79,\n"
            "['left'] = 80,\n"
            "['down'] = 81,\n"
            "['up'] = 82,\n"
            "['numlockclear'] = 83,\n"
            "['kp_divide'] = 84,\n"
            "['kp_multiply'] = 85,\n"
            "['kp_minus'] = 86,\n"
            "['kp_plus'] = 87,\n"
            "['kp_enter'] = 88,\n"
            "['kp_1'] = 89,\n"
            "['kp_2'] = 90,\n"
            "['kp_3'] = 91,\n"
            "['kp_4'] = 92,\n"
            "['kp_5'] = 93,\n"
            "['kp_6'] = 94,\n"
            "['kp_7'] = 95,\n"
            "['kp_8'] = 96,\n"
            "['kp_9'] = 97,\n"
            "['kp_0'] = 98,\n"
            "['kp_period'] = 99,\n"
            "['nonusbackslash'] = 100,\n"
            "['application'] = 101,\n"
            "['power'] = 102,\n"
            "['kp_equals'] = 103,\n"
            "['f13'] = 104,\n"
            "['f14'] = 105,\n"
            "['f15'] = 106,\n"
            "['f16'] = 107,\n"
            "['f17'] = 108,\n"
            "['f18'] = 109,\n"
            "['f19'] = 110,\n"
            "['f20'] = 111,\n"
            "['f21'] = 112,\n"
            "['f22'] = 113,\n"
            "['f23'] = 114,\n"
            "['f24'] = 115,\n"
            "['execute'] = 116,\n"
            "['help'] = 117,\n"
            "['menu'] = 118,\n"
            "['select'] = 119,\n"
            "['stop'] = 120,\n"
            "['again'] = 121,\n"
            "['undo'] = 122,\n"
            "['cut'] = 123,\n"
            "['copy'] = 124,\n"
            "['paste'] = 125,\n"
            "['find'] = 126,\n"
            "['mute'] = 127,\n"
            "['volumeup'] = 128,\n"
            "['volumedown'] = 129,\n"
            "['kp_comma'] = 133,\n"
            "['kp_equalsas400'] = 134,\n"
            "['international1'] = 135,\n"
            "['international2'] = 136,\n"
            "['international3'] = 137,\n"
            "['international4'] = 138,\n"
            "['international5'] = 139,\n"
            "['international6'] = 140,\n"
            "['international7'] = 141,\n"
            "['international8'] = 142,\n"
            "['international9'] = 143,\n"
            "['lang1'] = 144,\n"
            "['lang2'] = 145,\n"
            "['lang3'] = 146,\n"
            "['lang4'] = 147,\n"
            "['lang5'] = 148,\n"
            "['lang6'] = 149,\n"
            "['lang7'] = 150,\n"
            "['lang8'] = 151,\n"
            "['lang9'] = 152,\n"
            "['alterase'] = 153,\n"
            "['sysreq'] = 154,\n"
            "['cancel'] = 155,\n"
            "['clear'] = 156,\n"
            "['prior'] = 157,\n"
            "['return2'] = 158,\n"
            "['separator'] = 159,\n"
            "['out'] = 160,\n"
            "['oper'] = 161,\n"
            "['clearagain'] = 162,\n"
            "['crsel'] = 163,\n"
            "['exsel'] = 164,\n"
            "['kp_00'] = 176,\n"
            "['kp_000'] = 177,\n"
            "['thousandsseparator'] = 178,\n"
            "['decimalseparator'] = 179,\n"
            "['currencyunit'] = 180,\n"
            "['currencysubunit'] = 181,\n"
            "['kp_leftparen'] = 182,\n"
            "['kp_rightparen'] = 183,\n"
            "['kp_leftbrace'] = 184,\n"
            "['kp_rightbrace'] = 185,\n"
            "['kp_tab'] = 186,\n"
            "['kp_backspace'] = 187,\n"
            "['kp_a'] = 188,\n"
            "['kp_b'] = 189,\n"
            "['kp_c'] = 190,\n"
            "['kp_d'] = 191,\n"
            "['kp_e'] = 192,\n"
            "['kp_f'] = 193,\n"
            "['kp_xor'] = 194,\n"
            "['kp_power'] = 195,\n"
            "['kp_percent'] = 196,\n"
            "['kp_less'] = 197,\n"
            "['kp_greater'] = 198,\n"
            "['kp_ampersand'] = 199,\n"
            "['kp_dblampersand'] = 200,\n"
            "['kp_verticalbar'] = 201,\n"
            "['kp_dblverticalbar'] = 202,\n"
            "['kp_colon'] = 203,\n"
            "['kp_hash'] = 204,\n"
            "['kp_space'] = 205,\n"
            "['kp_at'] = 206,\n"
            "['kp_exclam'] = 207,\n"
            "['kp_memstore'] = 208,\n"
            "['kp_memrecall'] = 209,\n"
            "['kp_memclear'] = 210,\n"
            "['kp_memadd'] = 211,\n"
            "['kp_memsubtract'] = 212,\n"
            "['kp_memmultiply'] = 213,\n"
            "['kp_memdivide'] = 214,\n"
            "['kp_plusminus'] = 215,\n"
            "['kp_clear'] = 216,\n"
            "['kp_clearentry'] = 217,\n"
            "['kp_binary'] = 218,\n"
            "['kp_octal'] = 219,\n"
            "['kp_decimal'] = 220,\n"
            "['kp_hexadecimal'] = 221,\n"
            "['lctrl'] = 224,\n"
            "['lshift'] = 225,\n"
            "['lalt'] = 226,\n"
            "['lgui'] = 227,\n"
            "['rctrl'] = 228,\n"
            "['rshift'] = 229,\n"
            "['ralt'] = 230,\n"
            "['rgui'] = 231,\n"
            "['mode'] = 257,\n"
            "['audionext'] = 258,\n"
            "['audioprev'] = 259,\n"
            "['audiostop'] = 260,\n"
            "['audioplay'] = 261,\n"
            "['audiomute'] = 262,\n"
            "['mediaselect'] = 263,\n"
            "['www'] = 264,\n"
            "['mail'] = 265,\n"
            "['calculator'] = 266,\n"
            "['computer'] = 267,\n"
            "['ac_search'] = 268,\n"
            "['ac_home'] = 269,\n"
            "['ac_back'] = 270,\n"
            "['ac_forward'] = 271,\n"
            "['ac_stop'] = 272,\n"
            "['ac_refresh'] = 273,\n"
            "['ac_bookmarks'] = 274,\n"
            "['brightnessdown'] = 275,\n"
            "['brightnessup'] = 276,\n"
            "['displayswitch'] = 277,\n"
            "['kbdillumtoggle'] = 278,\n"
            "['kbdillumdown'] = 279,\n"
            "['kbdillumup'] = 280,\n"
            "['eject'] = 281,\n"
            "['sleep'] = 282,\n"
            "['app1'] = 283,\n"
            "['app2'] = 284,\n"
            "['audiorewind'] = 285,\n"
            "['audiofastforward'] = 286,\n"
            "}"
        ;

        return _engine->load_string(key_table_script).value().call().value();
    }

    LuaResult LuaVariantLibrary::_key_down_func(LuaEngine & engine, const LuaNativeFunctionArgs & args) {
        static const auto diag = LuaNativeFunctionDiagnostics {
            .name = "key_down",
            .return_type = "boolean",

            .args = {
                { .name = "key_id", .type = "number" },
            },
        };

        LuaResult r;
        if (!diag.check(r, engine, args, 1, { LuaType::NUMBER })) return r;
        auto key_id = static_cast<int>(diag.arg(args, 1).number());

        int num_keys = 0;
        auto * kb_state = SDL_GetKeyboardState(&num_keys);
        if (key_id < 0) {
            return LuaResult::error("key ID must be at least 0");
        } else if (key_id >= num_keys) {
            return LuaResult::error("invalid key ID (out of bounds)");
        }

        return engine.boolean(kb_state[static_cast<SDL_Scancode>(key_id)] != 0);
    }

    LuaResult LuaVariantLibrary::_key_up_func(LuaEngine & engine, const LuaNativeFunctionArgs & args) {
        static const auto diag = LuaNativeFunctionDiagnostics {
            .name = "key_up",
            .return_type = "boolean",

            .args = {
                { .name = "key_id", .type = "number" },
            },
        };

        LuaResult r;
        if (!diag.check(r, engine, args, 1, { LuaType::NUMBER })) return r;
        auto key_id = static_cast<int>(diag.arg(args, 1).number());

        int num_keys = 0;
        auto * kb_state = SDL_GetKeyboardState(&num_keys);
        if (key_id < 0) {
            return LuaResult::error("key ID must be at least 0");
        } else if (key_id >= num_keys) {
            return LuaResult::error("invalid key ID (out of bounds)");
        }

        return engine.boolean(kb_state[static_cast<SDL_Scancode>(key_id)] == 0);
    }

    void LuaVariantLibrary::_load_keyboard_support(LuaObject & table) {
        table.set("keys", _create_key_table());
        table.set("key_down", _engine->function(_key_down_func));
        table.set("key_up", _engine->function(_key_up_func));
    }

    void LuaVariantLibrary::_load_metatype_tools(LuaObject & table) {
        auto type_func = LuaNativeFunction([](LuaEngine & engine, const std::span<const smen::LuaObject> & args) -> LuaResult {
            // upvalue [0] => actual lua type() function

            if (args.size() <= 1) return LuaResult::error("missing argument to smen.type");
            auto lua_type_func = args[0];
            auto obj = args[1];
            auto mt = obj.metatable();
            if (mt.type() == LuaType::NIL) return lua_type_func.call({ obj });

            auto type_name = mt.get("__typename");
            if (type_name.type() != LuaType::STRING) return lua_type_func.call({ obj });

            return type_name;
        });

        table.set("type", _engine->closure(type_func, { _type_func }));
    }

    void LuaVariantLibrary::set_current_scene(Scene * scene) {
        auto table = load();

        _current_scene_ptr = scene;
        table.set("scene", _engine->native_type_copy("Scene", scene));
    }

    void LuaVariantLibrary::set_current_renderer(Renderer * renderer) {
        auto table = load();

        _current_renderer_ptr = renderer;
        table.set("renderer", _engine->native_type_copy("Renderer", renderer));
    }

    void LuaVariantLibrary::set_current_window(Window * window) {
        auto table = load();

        _current_window_ptr = window;
        table.set("window", _engine->native_type_copy("Window", window));
    }

    void LuaVariantLibrary::set_current_gui_context(GUIContext * gui_context) {
        auto table = load();

        _current_gui_context_ptr = gui_context;
        table.set("gui", _engine->native_type_copy("GUIContext", gui_context));
    }
}
