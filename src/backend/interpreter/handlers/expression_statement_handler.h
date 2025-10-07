#pragma once

// 前方宣言
struct ASTNode;
class Interpreter;

/**
 * @brief 式文処理ハンドラ
 * 
 * 他のどの文型にも当てはまらない場合の処理（default case）。
 * 式として評価を試みる。
 */
class ExpressionStatementHandler {
  public:
    explicit ExpressionStatementHandler(Interpreter *interpreter);
    ~ExpressionStatementHandler() = default;

    // 式文を処理
    void handle_expression_statement(const ASTNode *node);

  private:
    Interpreter *interpreter_;
};
