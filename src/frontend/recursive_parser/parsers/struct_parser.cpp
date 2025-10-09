// Struct Parser - 構造体解析を担当
// Phase 2-3: RecursiveParserへの委譲実装 + ドキュメント化
// Phase 10: StructMembersとCircularReference実装
//
// 【概要】:
// StructParserは、構造体、Union、Enumの解析を担当します。
//
// 【サポートする型定義】:
// 1. 構造体: struct Point { int x; int y; }
// 2. Union: union Value { int i; float f; }
// 3. Enum: enum Color { RED, GREEN, BLUE }
// 4. 前方宣言: struct Node;
// 5. typedef struct/enum/union
//
#include "struct_parser.h"
#include "../recursive_parser.h"

StructParser::StructParser(RecursiveParser *parser) : parser_(parser) {}

// ========================================
// 構造体宣言
// ========================================

/**
 * @brief 構造体宣言を解析
 * @return 解析されたAST構造体宣言ノード
 *
 * サポートする構文:
 * - 通常の構造体: struct Point { int x; int y; }
 * - 前方宣言: struct Node;
 * - ネスト構造体: struct Outer { struct Inner { ... } inner; }
 *
 * 機能:
 * - メンバー変数の解析
 * - 値型メンバーのサポート
 * - 循環参照の検出
 */
ASTNode *StructParser::parseStructDeclaration() {
    return parser_->parseStructDeclaration();
}

/**
 * @brief typedef struct宣言を解析
 * @return 解析されたAST構造体typedef宣言ノード
 *
 * 構文: typedef struct { ... } Name;
 * または: typedef struct Name { ... } Name;
 */
ASTNode *StructParser::parseStructTypedefDeclaration() {
    return parser_->parseStructTypedefDeclaration();
}

// ========================================
// 前方宣言
// ========================================

/**
 * @brief 構造体の前方宣言を解析
 * @return 解析されたAST前方宣言ノード
 *
 * 構文: struct Name;
 *
 * 用途:
 * - 相互参照する構造体の定義
 * - ポインタメンバーの型宣言
 *
 * 注意:
 * - 前方宣言された構造体は、完全定義前はポインタとしてのみ使用可能
 * - 値型メンバーとして使用する場合、後で完全定義が必要
 */
ASTNode *StructParser::parseForwardDeclaration() {
    // 前方宣言は parseStructDeclaration 内で処理される
    return parser_->parseStructDeclaration();
}

// ========================================
// Union宣言
// ========================================

/**
 * @brief Union宣言を解析
 * @return 解析されたASTUnion宣言ノード
 *
 * 構文: union Name { type1 member1; type2 member2; }
 *
 * 注意: C言語風のunion宣言は現在未サポート
 * TypeScript風Union型（typedef Status = 200 | 404 | 500）のみサポート
 *
 * C言語風unionの代わりに、typedef構文を使用してください：
 * typedef Data = int | string;
 */
ASTNode *StructParser::parseUnionDeclaration() {
    parser_->error("C-style union declarations are not supported. Use typedef "
                   "union syntax instead: typedef Name = Type1 | Type2;");
    return nullptr;
}

/**
 * @brief typedef union宣言を解析
 * @return 解析されたASTUnion typedef宣言ノード
 *
 * 構文: typedef union { ... } Name;
 *
 * TypeScript風Union型のサポート:
 * - typedef Status = 200 | 404 | 500;
 * - typedef StringOrInt = string | int;
 */
ASTNode *StructParser::parseUnionTypedefDeclaration() {
    return parser_->parseUnionTypedefDeclaration();
}

// ========================================
// Enum宣言
// ========================================

/**
 * @brief Enum宣言を解析
 * @return 解析されたASTEnum宣言ノード
 *
 * 構文: enum Color { RED, GREEN, BLUE }
 *
 * 機能:
 * - 自動値割り当て（0から開始）
 * - スコープアクセス（Color::RED）
 * - 型安全性
 */
ASTNode *StructParser::parseEnumDeclaration() {
    return parser_->parseEnumDeclaration();
}

/**
 * @brief typedef enum宣言を解析
 * @return 解析されたASTEnum typedef宣言ノード
 *
 * 構文: typedef enum { RED, GREEN, BLUE } Color;
 * または: typedef enum Color { RED, GREEN, BLUE } Color;
 */
ASTNode *StructParser::parseEnumTypedefDeclaration() {
    return parser_->parseEnumTypedefDeclaration();
}

// ========================================
// メンバー解析
// ========================================

/**
 * @brief 構造体のメンバーを解析
 * @param struct_def 解析対象の構造体定義
 *
 * 構造体の中括弧内のメンバー変数を解析します。
 *
 * サポートする要素:
 * - 基本型メンバー
 * - 配列メンバー
 * - 構造体メンバー（値型、ポインタ）
 * - ポインタメンバー
 * - private修飾子
 * - const修飾子
 */
void StructParser::parseStructMembers(StructDefinition *struct_def) {
    // メンバ変数の解析
    while (!parser_->check(TokenType::TOK_RBRACE) && !parser_->isAtEnd()) {
        bool is_private_member = false;
        if (parser_->check(TokenType::TOK_PRIVATE)) {
            is_private_member = true;
            parser_->advance();
        }

        // const修飾子のチェック
        bool is_const_member = false;
        if (parser_->check(TokenType::TOK_CONST)) {
            is_const_member = true;
            parser_->advance();
        }

        // メンバの型を解析
        std::string member_type = parser_->parseType();

        if (member_type.empty()) {
            parser_->error("Expected member type in struct definition");
            return;
        }

        ParsedTypeInfo member_parsed = parser_->getLastParsedTypeInfo();

        // メンバ名のリストを解析（int[2][2] a, b; のような複数宣言に対応）
        do {
            if (!parser_->check(TokenType::TOK_IDENTIFIER)) {
                parser_->error("Expected member name");
                return;
            }

            std::string member_name = parser_->current_token_.value;
            parser_->advance();

            ParsedTypeInfo var_parsed = member_parsed;
            TypeInfo member_type_info =
                parser_->resolveParsedTypeInfo(var_parsed);

            // 自己再帰構造体チェック:
            // 自分自身の型のメンバーはポインタでなければならない
            std::string member_base_type = var_parsed.base_type;
            if (member_base_type.empty()) {
                member_base_type = var_parsed.full_type;
            }
            // "struct " プレフィックスを除去
            if (member_base_type.rfind("struct ", 0) == 0) {
                member_base_type = member_base_type.substr(7);
            }

            if (member_base_type == struct_def->name &&
                !var_parsed.is_pointer) {
                parser_->error("Self-recursive struct member '" + member_name +
                               "' must be a pointer type. Use '" +
                               struct_def->name + "* " + member_name +
                               ";' instead of '" + struct_def->name + " " +
                               member_name + ";'");
                return;
            }

            struct_def->add_member(
                member_name, member_type_info, var_parsed.full_type,
                var_parsed.is_pointer, var_parsed.pointer_depth,
                var_parsed.base_type, var_parsed.base_type_info,
                is_private_member, var_parsed.is_reference,
                var_parsed.is_unsigned, is_const_member);

            if (var_parsed.is_array) {
                StructMember &added = struct_def->members.back();
                added.array_info = var_parsed.array_info;
            }

            // 旧式の配列宣言をチェック（int data[2][2];）- エラーとして処理
            if (parser_->check(TokenType::TOK_LBRACKET)) {
                parser_->error(
                    "Old-style array declaration is not supported in struct "
                    "members. Use 'int[2][2] member_name;' instead of 'int "
                    "member_name[2][2];'");
                return;
            }

            if (parser_->check(TokenType::TOK_COMMA)) {
                parser_->advance(); // カンマをスキップ
                continue;
            } else {
                break;
            }
        } while (true);

        parser_->consume(TokenType::TOK_SEMICOLON,
                         "Expected ';' after struct member");
    }
}

/**
 * @brief Unionのメンバーを解析
 * @param union_def 解析対象のUnion定義
 *
 * Union型の中括弧内のメンバー変数を解析します。
 *
 * 注意: C言語風のunionメンバー解析は現在未サポート
 * TypeScript風Union型は値またはtype定義として解析されます
 */
void StructParser::parseUnionMembers(UnionDefinition *union_def) {
    parser_->error("C-style union member parsing is not supported. Union types "
                   "use the typedef syntax: typedef Name = Value1 | Value2;");
}

// ========================================
// 循環参照検出
// ========================================

/**
 * @brief 構造体の循環参照を検出
 * @param struct_name 構造体名
 * @param member_type メンバーの型
 * @param pointer_level ポインタのレベル
 *
 * 構造体が自分自身を値型メンバーとして持つ場合、
 * 無限再帰を防ぐために循環参照を検出します。
 *
 * 許可されるケース:
 * - ポインタメンバー: struct Node { Node* next; }
 *
 * 禁止されるケース:
 * - 値型メンバー: struct Node { Node next; } // エラー
 *
 * 検出方法:
 * - 深さ優先探索でメンバーの型を再帰的にチェック
 * - 訪問済みの型を記録して循環を検出
 *
 * 注意: この実装はRecursiveParser::parseStructDeclaration()内で
 * 直接実行されています。このメソッドは将来の拡張用です。
 */
void StructParser::detectCircularReference(const std::string &struct_name,
                                           const std::string &member_type,
                                           int pointer_level) {
    // ポインタメンバーは循環参照しない
    if (pointer_level > 0) {
        return;
    }

    // TypeUtilityParserのdetectCircularReferenceメソッドを活用
    // ただし、実際の循環参照チェックはparseStructDeclaration()内で実行されています
    // このメソッドは互換性維持のために残されています

    std::unordered_set<std::string> visited;
    std::vector<std::string> path;
    path.push_back(struct_name);

    // RecursiveParser経由でTypeUtilityParserのdetectCircularReferenceを呼び出し
    if (parser_->detectCircularReference(struct_name, member_type, visited,
                                         path)) {
        std::string cycle_path = struct_name;
        for (size_t i = 1; i < path.size(); ++i) {
            cycle_path += " -> " + path[i];
        }
        parser_->error("Circular reference detected in struct value members: " +
                       cycle_path + ". Use pointers to break the cycle.");
    }
}
