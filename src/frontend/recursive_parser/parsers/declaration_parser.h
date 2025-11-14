// 宣言パーサー - 変数、関数、型の宣言を解析
// Declaration Parser - Parse variable, function, and type declarations

#ifndef DECLARATION_PARSER_H
#define DECLARATION_PARSER_H

#include "../../common/ast.h"
#include "variable_declaration_parser.h"
#include <memory>

class RecursiveParser;

// 宣言の解析を担当するクラス
class DeclarationParser {
  public:
    explicit DeclarationParser(RecursiveParser *parser);

    // 変数宣言
    ASTNode *parseVariableDeclaration();
    ASTNode *parseTypedefVariableDeclaration();

    // 関数宣言
    ASTNode *parseFunctionDeclaration();
    ASTNode *
    parseFunctionDeclarationAfterName(const std::string &return_type,
                                      const std::string &function_name);

    // Typedef
    ASTNode *parseTypedefDeclaration();
    ASTNode *parseFunctionPointerTypedefDeclaration();

    // v0.13.0: FFI (Foreign Function Interface)
    ASTNode *parseForeignModuleDecl();
    ForeignFunctionDecl parseForeignFunctionDecl();

  private:
    RecursiveParser *parser_;
    std::unique_ptr<VariableDeclarationParser> variable_declaration_parser_;
};

#endif // DECLARATION_PARSER_H
