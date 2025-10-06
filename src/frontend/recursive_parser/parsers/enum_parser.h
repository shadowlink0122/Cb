// Enum Parser - Parse enum declarations
// EnumParser - enum宣言の解析

#ifndef ENUM_PARSER_H
#define ENUM_PARSER_H

#include "../../common/ast.h"

class RecursiveParser;

// enum宣言の解析を担当するクラス
class EnumParser {
public:
    explicit EnumParser(RecursiveParser* parser);
    
    // enum宣言の解析
    ASTNode* parseEnumDeclaration();
    ASTNode* parseEnumTypedefDeclaration();
    
private:
    RecursiveParser* parser_;
};

#endif // ENUM_PARSER_H
