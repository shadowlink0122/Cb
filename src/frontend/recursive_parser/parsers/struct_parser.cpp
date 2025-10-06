// Struct Parser - 構造体解析を担当
// Phase 2-3: RecursiveParserへの委譲実装 + ドキュメント化
//
// このファイルは、構造体、Union、Enumの解析を担当します。
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

StructParser::StructParser(RecursiveParser* parser) 
    : parser_(parser) {
}

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
ASTNode* StructParser::parseStructDeclaration() {
    return parser_->parseStructDeclaration();
}

/**
 * @brief typedef struct宣言を解析
 * @return 解析されたAST構造体typedef宣言ノード
 * 
 * 構文: typedef struct { ... } Name;
 * または: typedef struct Name { ... } Name;
 */
ASTNode* StructParser::parseStructTypedefDeclaration() {
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
ASTNode* StructParser::parseForwardDeclaration() {
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
 * 構文: union Value { int i; float f; }
 * 
 * 注意: 現在は基本的なUnion型のみサポート
 * TypeScript風Union型（200 | 404 | 500）とは異なる
 */
ASTNode* StructParser::parseUnionDeclaration() {
    // TODO: RecursiveParserに対応するメソッドを追加
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
ASTNode* StructParser::parseUnionTypedefDeclaration() {
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
ASTNode* StructParser::parseEnumDeclaration() {
    return parser_->parseEnumDeclaration();
}

/**
 * @brief typedef enum宣言を解析
 * @return 解析されたASTEnum typedef宣言ノード
 * 
 * 構文: typedef enum { RED, GREEN, BLUE } Color;
 * または: typedef enum Color { RED, GREEN, BLUE } Color;
 */
ASTNode* StructParser::parseEnumTypedefDeclaration() {
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
 */
void StructParser::parseStructMembers(StructDefinition* struct_def) {
    // TODO: RecursiveParserから実装を移行
    // 現時点では何もしない
}

/**
 * @brief Unionのメンバーを解析
 * @param union_def 解析対象のUnion定義
 * 
 * Union型の中括弧内のメンバー変数を解析します。
 */
void StructParser::parseUnionMembers(UnionDefinition* union_def) {
    // TODO: RecursiveParserから実装を移行
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
 */
void StructParser::detectCircularReference(
    const std::string& struct_name,
    const std::string& member_type,
    int pointer_level
) {
    // RecursiveParserの detectCircularReference を呼び出す
    // ただし、このメソッドはprivateなので直接呼べない
    // TODO: Phase 3で実装を移行
}
