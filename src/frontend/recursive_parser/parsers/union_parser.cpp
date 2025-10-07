#include "union_parser.h"
#include "../../../common/debug.h"
#include "../recursive_parser.h"
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>

UnionParser::UnionParser(RecursiveParser *parser) : parser_(parser) {}

bool UnionParser::parseUnionValue(UnionDefinition &union_def) {
    if (parser_->check(TokenType::TOK_NUMBER)) {
        // 数値リテラル (int)
        std::string value_str = parser_->current_token_.value;
        int64_t int_value = std::stoll(value_str);
        parser_->advance();

        UnionValue union_val(int_value);
        union_def.add_literal_value(union_val);
        return true;

    } else if (parser_->check(TokenType::TOK_STRING)) {
        // 文字列リテラル
        std::string str_value = parser_->current_token_.value;
        parser_->advance();

        UnionValue union_val(str_value);
        union_def.add_literal_value(union_val);
        return true;

    } else if (parser_->check(TokenType::TOK_CHAR)) {
        // 文字リテラル
        std::string char_str = parser_->current_token_.value;
        parser_->advance();

        // Lexer already removed quotes, so char_str contains just the character
        if (char_str.length() == 1) {
            char char_value = char_str[0];
            UnionValue union_val(char_value);
            union_def.add_literal_value(union_val);
            return true;
        } else if (char_str.length() >= 3 && char_str[0] == '\'' &&
                   char_str[char_str.length() - 1] == '\'') {
            // Handle case where quotes are still present
            char char_value = char_str[1];
            UnionValue union_val(char_value);
            union_def.add_literal_value(union_val);
            return true;
        }
        parser_->error("Invalid character literal: '" + char_str +
                       "' (length: " + std::to_string(char_str.length()) + ")");
        return false;

    } else if (parser_->check(TokenType::TOK_TRUE)) {
        // boolean true
        parser_->advance();

        UnionValue union_val(true);
        union_def.add_literal_value(union_val);
        return true;

    } else if (parser_->check(TokenType::TOK_FALSE)) {
        // boolean false
        parser_->advance();

        UnionValue union_val(false);
        union_def.add_literal_value(union_val);
        return true;

    } else if (parser_->check(TokenType::TOK_INT)) {
        // int type keyword
        parser_->advance();
        // Check for array type (int[size])
        if (parser_->check(TokenType::TOK_LBRACKET)) {
            parser_->advance(); // consume '['
            if (parser_->check(TokenType::TOK_NUMBER)) {
                std::string size = parser_->current_token_.value;
                parser_->advance(); // consume the size number
                if (parser_->check(TokenType::TOK_RBRACKET)) {
                    parser_->advance(); // consume ']'
                    union_def.add_allowed_array_type("int[" + size + "]");
                    return true;
                } else {
                    parser_->error("Expected ']' after array size");
                    return false;
                }
            } else {
                parser_->error("Expected array size after '[' in array type");
                return false;
            }
        }
        union_def.add_allowed_type(TYPE_INT);
        return true;

    } else if (parser_->check(TokenType::TOK_LONG)) {
        // long type keyword
        parser_->advance();
        union_def.add_allowed_type(TYPE_LONG);
        return true;

    } else if (parser_->check(TokenType::TOK_SHORT)) {
        // short type keyword
        parser_->advance();
        union_def.add_allowed_type(TYPE_SHORT);
        return true;

    } else if (parser_->check(TokenType::TOK_TINY)) {
        // tiny type keyword
        parser_->advance();
        union_def.add_allowed_type(TYPE_TINY);
        return true;

    } else if (parser_->check(TokenType::TOK_BOOL)) {
        // bool type keyword
        parser_->advance();
        // Check for array type (bool[size])
        if (parser_->check(TokenType::TOK_LBRACKET)) {
            parser_->advance(); // consume '['
            if (parser_->check(TokenType::TOK_NUMBER)) {
                std::string size = parser_->current_token_.value;
                parser_->advance(); // consume the size number
                if (parser_->check(TokenType::TOK_RBRACKET)) {
                    parser_->advance(); // consume ']'
                    union_def.add_allowed_array_type("bool[" + size + "]");
                    return true;
                } else {
                    parser_->error("Expected ']' after array size");
                    return false;
                }
            } else {
                parser_->error("Expected array size after '[' in array type");
                return false;
            }
        }
        union_def.add_allowed_type(TYPE_BOOL);
        return true;

    } else if (parser_->check(TokenType::TOK_STRING_TYPE)) {
        // string type keyword
        parser_->advance();
        // Check for array type (string[size])
        if (parser_->check(TokenType::TOK_LBRACKET)) {
            parser_->advance(); // consume '['
            if (parser_->check(TokenType::TOK_NUMBER)) {
                std::string size = parser_->current_token_.value;
                parser_->advance(); // consume the size number
                if (parser_->check(TokenType::TOK_RBRACKET)) {
                    parser_->advance(); // consume ']'
                    union_def.add_allowed_array_type("string[" + size + "]");
                    return true;
                } else {
                    parser_->error("Expected ']' after array size");
                    return false;
                }
            } else {
                parser_->error("Expected array size after '[' in array type");
                return false;
            }
        }
        union_def.add_allowed_type(TYPE_STRING);
        return true;

    } else if (parser_->check(TokenType::TOK_CHAR_TYPE)) {
        // char type keyword
        parser_->advance();
        union_def.add_allowed_type(TYPE_CHAR);
        return true;

    } else if (parser_->check(TokenType::TOK_VOID)) {
        // void type keyword
        parser_->advance();
        union_def.add_allowed_type(TYPE_VOID);
        return true;

    } else if (parser_->check(TokenType::TOK_IDENTIFIER)) {
        // Type name (for type unions like user-defined types)
        std::string type_name = parser_->current_token_.value;
        parser_->advance();

        // Check for array type (type_name[size])
        if (parser_->check(TokenType::TOK_LBRACKET)) {
            parser_->advance(); // consume '['
            if (parser_->check(TokenType::TOK_NUMBER)) {
                std::string size = parser_->current_token_.value;
                parser_->advance(); // consume the size number
                if (parser_->check(TokenType::TOK_RBRACKET)) {
                    parser_->advance(); // consume ']'
                    // This is an array type with size
                    union_def.add_allowed_array_type(type_name + "[" + size +
                                                     "]");
                    return true;
                } else {
                    parser_->error("Expected ']' after array size");
                    return false;
                }
            } else {
                parser_->error("Expected array size after '[' in array type");
                return false;
            }
        }

        // Could be typedef, struct, or enum type
        if (parser_->typedef_map_.find(type_name) !=
            parser_->typedef_map_.end()) {
            // This is a typedef - add as custom type
            union_def.add_allowed_custom_type(type_name);
            debug_print(
                "UNION_PARSE_DEBUG: Added typedef custom type '%s' to union\n",
                type_name.c_str());
            return true;
        }

        if (parser_->struct_definitions_.find(type_name) !=
            parser_->struct_definitions_.end()) {
            // This is a struct - add as custom type
            union_def.add_allowed_custom_type(type_name);
            debug_print(
                "UNION_PARSE_DEBUG: Added struct custom type '%s' to union\n",
                type_name.c_str());
            return true;
        }

        if (parser_->enum_definitions_.find(type_name) !=
            parser_->enum_definitions_.end()) {
            // This is an enum - add as custom type
            union_def.add_allowed_custom_type(type_name);
            debug_print(
                "UNION_PARSE_DEBUG: Added enum custom type '%s' to union\n",
                type_name.c_str());
            return true;
        }

        // Unknown custom type - still add it (might be defined later)
        union_def.add_allowed_custom_type(type_name);
        debug_print(
            "UNION_PARSE_DEBUG: Added unknown custom type '%s' to union\n",
            type_name.c_str());
        return true;
    }

    parser_->error("Expected literal value or type name in union");
    return false;
}

// メンバアクセスの解析: obj.member
