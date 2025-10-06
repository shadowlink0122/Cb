// Declaration Parser - 宣言解析を担当
// Phase 2: RecursiveParserへの委譲実装（v0.9.1）
//
// このファイルは、宣言に関連する6個のメソッドを管理します。
//
// 管理するメソッド:
// - parseVariableDeclaration: 変数宣言（int x = 10;）
// - parseTypedefVariableDeclaration: typedef変数宣言
// - parseFunctionDeclaration: 関数宣言
// - parseFunctionDeclarationAfterName: 関数名解析後の処理
// - parseTypedefDeclaration: typedef宣言（typedef int MyInt;）
// - parseFunctionPointerTypedefDeclaration: 関数ポインタtypedef
//
// Phase 3での実装移行予定:
// - 推定行数: 600-800行
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
