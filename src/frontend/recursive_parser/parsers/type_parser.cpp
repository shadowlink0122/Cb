// Type Parser - 型解析を担当
// Phase 2: RecursiveParserへの委譲実装（v0.9.1）
//
// このファイルは、型解析に関連する7個のメソッドを管理します。
//
// 管理するメソッド:
// - parseType: 型の解析（int, int*, int[5], struct Point等）
// - resolveParsedTypeInfo: 型情報の解決（typedefチェーン解決）
// - resolveArrayType: 配列型の解決
// - getPointerLevel: ポインタの深さ取得
// - isValidType: 型の妥当性チェック
// - isStructType: 構造体型かチェック
// - isEnumType: enum型かチェック
//
// 特記事項:
// - ParsedTypeInfo構造体を使用して詳細な型情報を管理
// - typedef解決、ポインタ深さ、配列情報を含む
//
// Phase 3での実装移行予定:
// - 推定行数: 300-400行
#include "type_parser.h"
#include "../recursive_parser.h"

TypeParser::TypeParser(RecursiveParser* parser) 
    : parser_(parser) {
}

// Phase 2: 委譲パターンの実装

ParsedTypeInfo TypeParser::parseType() {
    // parseType()は文字列を返すが、ParsedTypeInfoを返す必要がある
    // RecursiveParserのparseType()を呼び出し、last_parsed_type_info_を取得
    parser_->parseType();
    return parser_->getLastParsedTypeInfo();
}

TypeInfo TypeParser::resolveParsedTypeInfo(const ParsedTypeInfo& type_info) {
    return parser_->resolveParsedTypeInfo(type_info);
}

std::string TypeParser::resolveArrayType(
    const std::string& base_type,
    const std::vector<std::pair<int, bool>>& dimensions
) {
    // TODO: RecursiveParserに対応するメソッドを追加する必要がある
    // 現時点では空文字列を返す
    return "";
}

int TypeParser::getPointerLevel(const ParsedTypeInfo& type_info) {
    return type_info.pointer_depth;
}

bool TypeParser::isValidType(const std::string& type_name) {
    // TODO: RecursiveParserに対応するメソッドを追加
    return true;  // 暫定実装
}

bool TypeParser::isStructType(const std::string& type_name) {
    // struct定義が存在するかチェック
    const auto& struct_defs = parser_->get_struct_definitions();
    return struct_defs.find(type_name) != struct_defs.end();
}

bool TypeParser::isEnumType(const std::string& type_name) {
    // enum定義が存在するかチェック
    const auto& enum_defs = parser_->get_enum_definitions();
    return enum_defs.find(type_name) != enum_defs.end();
}
