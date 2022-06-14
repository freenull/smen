#ifndef SMEN_SER_SERIALIZER_HPP
#define SMEN_SER_SERIALIZER_HPP

#include <iostream>
#include <unordered_set>
#include <smen/lua/script.hpp>
#include <smen/variant/types.hpp>
#include <smen/ecs/scene.hpp>
#include <smen/ecs/system.hpp>

namespace smen {
    bool is_unquoted_string_char(char c);
    bool is_unquoted_string(const std::string & s);

    void write_escaped_string(std::ostream & s, const std::string & str);

    class SerializationException : public std::runtime_error {
    public:
        inline SerializationException(const std::string & msg)
            : std::runtime_error(msg) {}
    };

    class TypeSerializer {
    private:
        std::unordered_set<VariantTypeID> _serialized_type_ids;
        VariantTypeDirectory & _dir;
        std::ostream & _s;


    public:
        TypeSerializer(std::ostream & s, VariantTypeDirectory & dir);
        void serialize(const VariantTypeField & field);
        void serialize(const VariantType & type);
        void serialize_all();
    };

    class VariantSerializer {
    private:
        std::unordered_map<VariantReference, size_t, VariantReference::HashFunction> _ref_map;
        size_t _ref_counter;
        std::ostream & _s;
        VariantContainer & _container;
        void _serialize_references(const Variant & variant, const VariantType & type);
        void _serialize(const std::string & field_key, const Variant & variant, const VariantType & type);

    public:
        VariantSerializer(std::ostream & s, VariantContainer & container);
        void serialize_reference(const VariantReference & ref);
        void serialize_references(const Variant & variant);
        void serialize(const Variant & variant);
    };

    class SceneSerializer {
    private:
        const Scene & _scene;
        std::ostream & _s;
        VariantSerializer _variant_ser;
        size_t _parent_id_counter;
        std::unordered_map<EntityID, size_t> _parent_id_map;

    public:
        SceneSerializer(std::ostream & s, Scene & scene);
        void serialize_system_query(const SystemQuery & query);
        void serialize_system(const System & system);
        void serialize_entity(EntityID ent_id);
        void serialize_script(const std::string & id, const LuaScript & script);
        void serialize_entities();
        void serialize_systems();
        void serialize_scripts();
        void serialize_all();
    };
}

#endif//SMEN_SER_SERIALIZER_HPP
