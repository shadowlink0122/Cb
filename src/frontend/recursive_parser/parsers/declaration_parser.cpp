// Declaration Parser - 宣言解析を担当
// Phase 2: RecursiveParserへの委譲実装
#include "declaration_parser.h"
#include "../recursive_parser.h"

DeclarationParser::DeclarationParser(RecursiveParser* parser) 
    : parser_(parser) {
}

// Phase 2: 委譲パターンの実装

ASTNode* DeclarationParser::parseVariableDeclaration() {
    return parser_->parseVariableDeclaration();
}

ASTNode* DeclarationParser::parseTypedefVariableDeclaration() {
    return parser_->parseTypedefVariableDeclaration();
}

ASTNode* DeclarationParser::parseFunctionDeclaration() {
    return parser_->parseFunctionDeclaration();
}

ASTNode* DeclarationParser::parseFunctionDeclarationAfterName(
    const std::string& return_type,
    const std::string& function_name
) {
    return parser_->parseFunctionDeclarationAfterName(return_type, function_name);
}

ASTNode* DeclarationParser::parseTypedefDeclaration() {
    return parser_->parseTypedefDeclaration();
}

ASTNode* DeclarationParser::parseFunctionPointerTypedefDeclaration() {
    return parser_->parseFunctionPointerTypedefDeclaration();
}
