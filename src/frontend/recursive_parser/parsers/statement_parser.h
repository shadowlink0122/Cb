// 文パーサー - 制御構文とステートメントの解析
// Statement Parser - Parse control flow and statements

#ifndef STATEMENT_PARSER_H
#define STATEMENT_PARSER_H

#include "../../common/ast.h"
#include "../../common/debug.h"

class RecursiveParser;

// 文の解析を担当するクラス
class StatementParser {
  public:
    explicit StatementParser(RecursiveParser *parser);

    // 文の解析
    ASTNode *parseStatement();
    ASTNode *parseCompoundStatement();

    // 制御構文
    ASTNode *parseIfStatement();
    ASTNode *parseForStatement();
    ASTNode *parseWhileStatement();

    // ジャンプ文
    ASTNode *parseReturnStatement();
    ASTNode *parseBreakStatement();
    ASTNode *parseContinueStatement();

    // その他
    ASTNode *parseAssertStatement();
    ASTNode *parsePrintlnStatement();
    ASTNode *parsePrintStatement();

  private:
    RecursiveParser *parser_;

    // parseStatementのヘルパーメソッド
    ASTNode *parseDeclarationStatement(bool isStatic, bool isConst);
    ASTNode *parseTypedefTypeStatement(const std::string &type_name,
                                       bool isStatic, bool isConst);
    ASTNode *parseBasicTypeStatement(bool isStatic, bool isConst,
                                     bool isUnsigned);
    ASTNode *parseControlFlowStatement();
    ASTNode *parseExpressionOrAssignmentStatement();

    // 配列宣言と変数宣言のヘルパー
    ASTNode *parseArrayDeclaration(const std::string &base_type_name,
                                   const std::string &type_name,
                                   TypeInfo base_type_info,
                                   TypeInfo declared_type_info, bool isStatic,
                                   bool isConst, bool isUnsigned,
                                   bool is_reference);

    ASTNode *parseVariableDeclarationList(
        const std::string &first_var_name, const std::string &type_name,
        const std::string &base_type_name, TypeInfo base_type_info,
        TypeInfo declared_type_info, int pointer_depth, bool isStatic,
        bool isConst, bool isUnsigned, bool is_reference);
};

#endif // STATEMENT_PARSER_H
