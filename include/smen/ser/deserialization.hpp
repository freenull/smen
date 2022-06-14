#ifndef SMEN_SER_DESERIALIZER_HPP
#define SMEN_SER_DESERIALIZER_HPP

#include <iostream>
#include <unordered_set>
#include <smen/variant/types.hpp>
#include <smen/ser/serialization.hpp>
#include <smen/variant/type_builder.hpp>

namespace smen {
    struct FileRegion {
    public:
        size_t start_row;
        size_t start_col;
        size_t end_row;
        size_t end_col;

        inline FileRegion(size_t row, size_t col)
        : start_row(row)
        , start_col(col)
        , end_row(row)
        , end_col(col)
        {}

        inline FileRegion(size_t start_row, size_t start_col, size_t end_row, size_t end_col)
        : start_row(start_row)
        , start_col(start_col)
        , end_row(end_row)
        , end_col(end_col)
        {}

        inline void write_string(std::ostream & s) const {
            s << "[";
            if (start_row == end_row) {
                if (start_col == end_col) {
                   s << start_row << ":" << start_col;
                } else {
                   s << start_row << ":" << start_col << "-" << end_col;
                }
            } else {
                   s << start_row << ":" << start_col << "-" << end_row << ":" << end_col;
            }
            s << "]";
        }

        inline std::string to_string() const {
            std::ostringstream s;
            write_string(s);
            return s.str();
        }

        inline friend std::ostream & operator <<(std::ostream & s, const FileRegion & region) {
            region.write_string(s);
            return s;
        }
    };

    class DeserializationException : public std::runtime_error {
    public:
        inline DeserializationException(const std::string & msg)
            : std::runtime_error("exception thrown while deserializing: " + msg) {}

        inline DeserializationException(const FileRegion & region, const std::string & msg)
            : std::runtime_error("exception thrown while deserializing: " + region.to_string() + " " + msg) {}
    };

    class Lexer {
    public:
        enum class TokenType {
            HEADER_LIST_BEGIN,
            HEADER_LIST_END,
            OBJECT_BEGIN,
            OBJECT_END,
            STRING,
            ASSIGN,
            REFERENCE,
            END_OF_FILE
        };

        struct Token {
        public:
            TokenType type;
            std::string content;
            FileRegion region;

            inline Token(TokenType type, const std::string & content, const FileRegion & region)
            : type(type)
            , content(content)
            , region(region)
            {}
        };

    private:
        std::istream & _s;
        bool _eof;
        char _cur;

        size_t _cur_row;
        size_t _cur_col;
        size_t _prev_col;

        std::optional<Token> _peek_token;
        std::optional<Token> _peek2_token; // ugly

        static bool _is_whitespace(char c);

        char _peek();
        void _move(size_t n = 1);
        void _skip_whitespace();
        FileRegion _start_region();
        FileRegion _end_region(const FileRegion & region);
        FileRegion _end_region_before(const FileRegion & region);

    public:
        Lexer(std::istream & s);

        std::string read_unquoted();
        std::string read_quoted();
        std::string read_string();

        Token next();
        void check(const Token & tok, TokenType type, const std::string & name);
        Token next_expect(TokenType type, const std::string & name);

        inline void check_header_begin(const Token & tok, const std::string & name = "") {
            check(tok, TokenType::HEADER_LIST_BEGIN, name.size() == 0 ? "header begin marker ('[')" : name);
        }

        inline void check_header_end(const Token & tok, const std::string & name = "") {
            check(tok, TokenType::HEADER_LIST_END, name.size() == 0 ? "header end marker ('[')" : name);
        }

        inline void check_list_begin(const Token & tok, const std::string & name = "") {
            check(tok, TokenType::HEADER_LIST_BEGIN, name.size() == 0 ? "list begin marker ('[')" : name);
        }

        inline void check_list_end(const Token & tok, const std::string & name = "") {
            check(tok, TokenType::HEADER_LIST_END, name.size() == 0 ? "list end marker ('[')" : name);
        }

        inline void check_object_begin(const Token & tok, const std::string & name = "") {
            check(tok, TokenType::OBJECT_BEGIN, name.size() == 0 ? "object begin marker ('{')" : name);
        }

        inline void check_object_end(const Token & tok, const std::string & name = "") {
            check(tok, TokenType::OBJECT_END, name.size() == 0 ? "object end marker ('}')" : name);
        }

        inline void check_string(const Token & tok, const std::string & name = "") {
            check(tok, TokenType::STRING, name.size() == 0 ? "string" : name);
        }

        inline void check_reference(const Token & tok, const std::string & name = "") {
            check(tok, TokenType::REFERENCE, name.size() == 0 ? "reference" : name);
        }

        inline void check_assign(const Token & tok, const std::string & name = "") {
            check(tok, TokenType::ASSIGN, name.size() == 0 ? "assignment symbol ('=')" : name);
        }

        inline Token next_expect_header_begin(const std::string & name = "") {
            return next_expect(TokenType::HEADER_LIST_BEGIN, name.size() == 0 ? "header begin marker ('[')" : name);
        }

        inline Token next_expect_header_end(const std::string & name = "") {
            return next_expect(TokenType::HEADER_LIST_END, name.size() == 0 ? "header end marker ('[')" : name);
        }

        inline Token next_expect_list_begin(const std::string & name = "") {
            return next_expect(TokenType::HEADER_LIST_BEGIN, name.size() == 0 ? "list begin marker ('[')" : name);
        }

        inline Token next_expect_list_end(const std::string & name = "") {
            return next_expect(TokenType::HEADER_LIST_END, name.size() == 0 ? "list end marker ('[')" : name);
        }

        inline Token next_expect_object_begin(const std::string & name = "") {
            return next_expect(TokenType::OBJECT_BEGIN, name.size() == 0 ? "object begin marker ('{')" : name);
        }

        inline Token next_expect_object_end(const std::string & name = "") {
            return next_expect(TokenType::OBJECT_END, name.size() == 0 ? "object end marker ('}')" : name);
        }

        inline Token next_expect_string(const std::string & name = "") {
            return next_expect(TokenType::STRING, name.size() == 0 ? "string" : name);
        }

        inline Token next_expect_reference(const std::string & name = "") {
            return next_expect(TokenType::REFERENCE, name.size() == 0 ? "reference" : name);
        }

        inline Token next_expect_assign(const std::string & name = "") {
            return next_expect(TokenType::ASSIGN, name.size() == 0 ? "assignment symbol ('=')" : name);
        }

        Token peek();
        Token peek2();
    };

    class TypeDeserializer {
    private:
        VariantTypeDirectory & _dir;
        Lexer & _lexer;

    public:
        TypeDeserializer(VariantTypeDirectory & dir, Lexer & lexer);
        void deserialize_field(VariantTypeBuilder & builder);
        VariantType deserialize_next();
        void deserialize_all();
    };

    class SceneDeserializer {
    private:
        Scene & _scene;
        Lexer & _lexer;
        std::unordered_map<size_t, Variant> _variant_cache;
        std::unordered_map<size_t, EntityID> _entity_cache;

        Variant _get_referenced_variant_from_token(const Lexer::Token & ref);

    public:
        SceneDeserializer(Scene & scene, Lexer & lexer);

        void set_variant(Variant & variant, const std::string & field_text, const Lexer::Token & ref);
        void deserialize_variant_field(Variant & variant);
        void deserialize_variant_data(Variant & variant);
        void deserialize_entity_field(EntityID id);
        void deserialize_entity_data(EntityID id);
        void deserialize_system_field(System & system);
        void deserialize_system_data(System & system);
        SystemQuery deserialize_system_query();
        void deserialize_script_field(const std::string & id);
        void deserialize_script_data(const std::string & id);
        void deserialize_scene_field();
        void deserialize_scene_data();
        void deserialize_next();
        void deserialize_all();
    };
}

#endif//SMEN_SER_DESERIALIZER_HPP
