// Struct Parser - 構造体解析を担当
// Phase 2: RecursiveParserへの委譲実装（v0.9.1）
//
// このファイルは、構造体・Union・Enum解析に関連する10個のメソッドを管理します。
//
// 管理するメソッド:
// - parseStructDeclaration: 構造体宣言（struct Point { int x; int y; };）
// - parseStructTypedefDeclaration: typedef struct宣言
// - parseForwardDeclaration: 前方宣言（struct Name;）
// - parseUnionDeclaration: Union宣言
// - parseUnionTypedefDeclaration: typedef union宣言
// - parseEnumDeclaration: enum宣言（enum Color { RED, GREEN, BLUE };）
// - parseEnumTypedefDeclaration: typedef enum宣言
// - parseStructMembers: 構造体メンバーの解析
// - parseUnionMembers: Unionメンバーの解析
// - detectCircularReference: 循環参照の検出
//
// 特記事項:
// - 前方宣言のサポート（v0.10.0で実装）
// - 循環参照の自動検出機能
//
// Phase 3での実装移行予定:
// - 推定行数: 500-600行
#include "struct_parser.h"
#include "../recursive_parser.h"

StructParser::StructParser(RecursiveParser* parser) 
    : parser_(parser) {
}

// Phase 2: 委譲パターンの実装

ASTNode* StructParser::parseStructDeclaration() {
    return parser_->parseStructDeclaration();
}

ASTNode* StructParser::parseStructTypedefDeclaration() {
    return parser_->parseStructTypedefDeclaration();
}

ASTNode* StructParser::parseForwardDeclaration() {
    // 前方宣言は parseStructDeclaration 内で処理される
    return parser_->parseStructDeclaration();
}

ASTNode* StructParser::parseUnionDeclaration() {
    // TODO: RecursiveParserに対応するメソッドを追加
    return nullptr;
}

ASTNode* StructParser::parseUnionTypedefDeclaration() {
    return parser_->parseUnionTypedefDeclaration();
}

ASTNode* StructParser::parseEnumDeclaration() {
    return parser_->parseEnumDeclaration();
}

ASTNode* StructParser::parseEnumTypedefDeclaration() {
    return parser_->parseEnumTypedefDeclaration();
}

void StructParser::parseStructMembers(StructDefinition* struct_def) {
    // TODO: RecursiveParserから実装を移行
    // 現時点では何もしない
}

void StructParser::parseUnionMembers(UnionDefinition* union_def) {
    // TODO: RecursiveParserから実装を移行
}

void StructParser::detectCircularReference(
    const std::string& struct_name,
    const std::string& member_type,
    int pointer_level
) {
    // RecursiveParserの detectCircularReference を呼び出す
    // ただし、このメソッドはprivateなので直接呼べない
    // TODO: Phase 3で実装を移行
}
