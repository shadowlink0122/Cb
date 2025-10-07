#include "enum_parser.h"
#include "../recursive_parser.h"
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>

/**
 * @file enum_parser.cpp
 * @brief enum宣言を担当するEnumParserクラスの実装
 * @note recursive_parser.cppから移行（Phase 6-1）
 */

// ========================================
// コンストラクタ
// ========================================

EnumParser::EnumParser(RecursiveParser *parser) : parser_(parser) {}

// ========================================
// enum宣言の解析
// ========================================

ASTNode *EnumParser::parseEnumDeclaration() {
    parser_->consume(TokenType::TOK_ENUM, "Expected 'enum'");

    if (!parser_->check(TokenType::TOK_IDENTIFIER)) {
        parser_->error("Expected enum name");
        return nullptr;
    }

    std::string enum_name = parser_->current_token_.value;
    parser_->advance(); // consume enum name

    parser_->consume(TokenType::TOK_LBRACE, "Expected '{' after enum name");

    ASTNode *enum_decl = new ASTNode(ASTNodeType::AST_ENUM_DECL);
    enum_decl->name = enum_name;
    parser_->setLocation(enum_decl, parser_->current_token_);

    EnumDefinition enum_def(enum_name);
    int64_t current_value = 0; // デフォルトの開始値

    // 空のenumはエラー
    if (parser_->check(TokenType::TOK_RBRACE)) {
        parser_->error("Empty enum is not allowed");
        return nullptr;
    }

    // enumメンバーをパース
    while (!parser_->check(TokenType::TOK_RBRACE) && !parser_->isAtEnd()) {
        if (!parser_->check(TokenType::TOK_IDENTIFIER)) {
            parser_->error("Expected enum member name");
            return nullptr;
        }

        std::string member_name = parser_->current_token_.value;
        parser_->advance(); // consume member name

        bool explicit_value = false;
        int64_t member_value = current_value;

        // 明示的な値の指定をチェック
        if (parser_->match(TokenType::TOK_ASSIGN)) {
            // 負の数値をチェック
            bool is_negative = false;
            if (parser_->check(TokenType::TOK_MINUS)) {
                is_negative = true;
                parser_->advance(); // consume '-'
            }

            if (!parser_->check(TokenType::TOK_NUMBER)) {
                parser_->error("Expected number after '=' in enum member");
                return nullptr;
            }

            member_value = std::stoll(parser_->current_token_.value);
            if (is_negative) {
                member_value = -member_value;
            }
            explicit_value = true;
            current_value = member_value;
            parser_->advance(); // consume number
        }

        // enumメンバーを追加
        enum_def.add_member(member_name, member_value, explicit_value);
        current_value++; // 次の暗黙的な値を準備

        // カンマまたは }をチェック
        if (parser_->match(TokenType::TOK_COMMA)) {
            // 次のメンバーがある場合は続行
            if (parser_->check(TokenType::TOK_RBRACE)) {
                parser_->error("Trailing comma in enum is not allowed");
                return nullptr;
            }
        } else if (!parser_->check(TokenType::TOK_RBRACE)) {
            parser_->error("Expected ',' or '}' after enum member");
            return nullptr;
        }
    }

    parser_->consume(TokenType::TOK_RBRACE, "Expected '}' after enum members");
    parser_->consume(TokenType::TOK_SEMICOLON,
                     "Expected ';' after enum declaration");

    // 値の重複チェック
    if (enum_def.has_duplicate_values()) {
        parser_->error("Enum has duplicate values - this is not allowed");
        return nullptr;
    }

    // enum定義を保存
    parser_->enum_definitions_[enum_name] = enum_def;

    // ASTノードにenum定義情報を埋め込む
    enum_decl->enum_definition = enum_def;

    return enum_decl;
}

ASTNode *EnumParser::parseEnumTypedefDeclaration() {
    parser_->consume(TokenType::TOK_ENUM, "Expected 'enum'");

    parser_->consume(TokenType::TOK_LBRACE,
                     "Expected '{' after 'typedef enum'");

    EnumDefinition enum_def;
    int64_t next_value = 0;

    // enumメンバの解析
    while (!parser_->check(TokenType::TOK_RBRACE) && !parser_->isAtEnd()) {
        if (!parser_->check(TokenType::TOK_IDENTIFIER)) {
            parser_->error("Expected enum member name");
            return nullptr;
        }

        std::string member_name = parser_->current_token_.value;
        parser_->advance();

        int64_t member_value = next_value;
        bool explicit_value = false;

        // 明示的な値の指定があるかチェック
        if (parser_->check(TokenType::TOK_ASSIGN)) {
            parser_->advance(); // consume '='

            // 負の数をサポート
            bool is_negative = false;
            if (parser_->check(TokenType::TOK_MINUS)) {
                is_negative = true;
                parser_->advance(); // consume '-'
            }

            if (!parser_->check(TokenType::TOK_NUMBER)) {
                parser_->error("Expected number after '=' in enum member");
                return nullptr;
            }

            member_value = std::stoll(parser_->current_token_.value);
            if (is_negative) {
                member_value = -member_value;
            }
            explicit_value = true;
            parser_->advance();
        }

        // enumメンバを追加
        enum_def.add_member(member_name, member_value, explicit_value);
        next_value = member_value + 1;

        if (parser_->check(TokenType::TOK_COMMA)) {
            parser_->advance(); // consume ','
        } else if (!parser_->check(TokenType::TOK_RBRACE)) {
            parser_->error("Expected ',' or '}' after enum member");
            return nullptr;
        }
    }

    parser_->consume(TokenType::TOK_RBRACE, "Expected '}' after enum members");

    // typedef エイリアス名を取得
    if (!parser_->check(TokenType::TOK_IDENTIFIER)) {
        parser_->error("Expected typedef alias name");
        return nullptr;
    }

    std::string alias_name = parser_->current_token_.value;
    parser_->advance();

    parser_->consume(TokenType::TOK_SEMICOLON,
                     "Expected ';' after typedef enum declaration");

    // enum定義を保存（エイリアス名で）
    enum_def.name = alias_name;
    parser_->enum_definitions_[alias_name] = enum_def;

    // typedef マッピングも追加
    parser_->typedef_map_[alias_name] = "enum " + alias_name;

    // ASTノードを作成
    ASTNode *node = new ASTNode(ASTNodeType::AST_ENUM_TYPEDEF_DECL);
    node->name = alias_name;
    node->type_info = TYPE_ENUM;

    // enum定義情報をASTに格納
    for (const auto &member : enum_def.members) {
        ASTNode *member_node = new ASTNode(ASTNodeType::AST_VAR_DECL);
        member_node->name = member.name;
        member_node->int_value = member.value;
        member_node->type_info = TYPE_INT; // enum値は整数
        node->arguments.push_back(std::unique_ptr<ASTNode>(member_node));
    }

    parser_->setLocation(node, parser_->current_token_);

    return node;
}
