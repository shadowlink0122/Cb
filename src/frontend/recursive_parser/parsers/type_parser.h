// 型パーサー - 型情報の解析と解決
// Type Parser - Parse and resolve type information

#ifndef TYPE_PARSER_H
#define TYPE_PARSER_H

#include "../recursive_parser.h" // ParsedTypeInfo の完全な定義が必要
#include <string>
#include <utility>
#include <vector>

// 型解析を担当するクラス
class TypeParser {
  public:
    explicit TypeParser(RecursiveParser *parser);

    // 型の解析
    ParsedTypeInfo parseType();

    // 型の解決（typedef等を考慮）
    TypeInfo resolveParsedTypeInfo(const ParsedTypeInfo &type_info);

    // 配列型の解決
    std::string
    resolveArrayType(const std::string &base_type,
                     const std::vector<std::pair<int, bool>> &dimensions);

    // ポインタレベルの取得
    int getPointerLevel(const ParsedTypeInfo &type_info);

    // 型のバリデーション
    bool isValidType(const std::string &type_name);
    bool isStructType(const std::string &type_name);
    bool isEnumType(const std::string &type_name);

  private:
    RecursiveParser *parser_;
};

#endif // TYPE_PARSER_H
