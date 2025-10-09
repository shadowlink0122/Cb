#ifndef RETURN_HANDLER_H
#define RETURN_HANDLER_H

#include <string>

struct ASTNode;
struct Variable;
class Interpreter;

// return文の処理を担当するクラス
class ReturnHandler {
  public:
    explicit ReturnHandler(Interpreter *interpreter)
        : interpreter_(interpreter) {}

    // return文の実行
    // ReturnExceptionをスローすることで戻り値を返す
    void execute_return_statement(const ASTNode *node);

  private:
    Interpreter *interpreter_;

    // 配列リテラルのreturn処理
    void handle_array_literal_return(const ASTNode *node);

    // 識別子のreturn処理（変数、self等）
    void handle_identifier_return(const ASTNode *node);

    // 変数のreturn処理
    void handle_variable_return(const ASTNode *node);

    // 配列変数のreturn処理
    void handle_array_variable_return(const ASTNode *node, Variable *var);

    // メンバーアクセスのreturn処理
    void handle_member_access_return(const ASTNode *node);

    // 式のreturn処理（デフォルト）
    void handle_expression_return(const ASTNode *node);
};

#endif // RETURN_HANDLER_H
