// Struct Parser - 構造体解析を担当
// Phase 2: RecursiveParserへの委譲実装
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
