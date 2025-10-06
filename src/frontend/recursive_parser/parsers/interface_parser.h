// Interface Parser - Parse interface and impl declarations
// InterfaceParser - interface/impl宣言の解析

#ifndef INTERFACE_PARSER_H
#define INTERFACE_PARSER_H

#include "../../common/ast.h"

class RecursiveParser;

// interface/impl宣言の解析を担当するクラス
class InterfaceParser {
public:
    explicit InterfaceParser(RecursiveParser* parser);
    
    // interface/impl宣言の解析
    ASTNode* parseInterfaceDeclaration();
    ASTNode* parseImplDeclaration();
    
private:
    RecursiveParser* parser_;
};

#endif // INTERFACE_PARSER_H
