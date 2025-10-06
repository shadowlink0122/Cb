// Type Parser - 型解析を担当
// Phase 2: RecursiveParserへの委譲実装
#include "type_parser.h"
#include "../recursive_parser.h"

TypeParser::TypeParser(RecursiveParser* parser) 
    : parser_(parser) {
}

// Phase 2: 委譲パターンの実装
// 注: parseTypeは文字列を返すため、直接委譲

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
