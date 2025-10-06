#ifndef TYPE_UTILITY_PARSER_H
#define TYPE_UTILITY_PARSER_H

#include <string>
#include <unordered_set>
#include <vector>
#include "../../../common/ast.h"

class RecursiveParser;
struct ParsedTypeInfo;

class TypeUtilityParser {
public:
    explicit TypeUtilityParser(RecursiveParser* parser);
    
    // 型文字列の解析と処理
    std::string parseType();
    TypeInfo getTypeInfoFromString(const std::string& type_name);
    std::string resolveTypedefChain(const std::string& typedef_name);
    std::string extractBaseType(const std::string& type_name);
    bool detectCircularReference(const std::string& struct_name, 
                                const std::string& member_type,
                                std::unordered_set<std::string>& visited,
                                std::vector<std::string>& path);
    
private:
    RecursiveParser* parser_;
};

#endif // TYPE_UTILITY_PARSER_H
