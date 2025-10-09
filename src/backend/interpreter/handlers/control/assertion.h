#ifndef CB_INTERPRETER_ASSERTION_HANDLER_H
#define CB_INTERPRETER_ASSERTION_HANDLER_H

struct ASTNode;
class Interpreter;

/**
 * @brief アサーション文の実行を管理するクラス
 *
 * AST_ASSERT_STMT の処理を担当。
 * execute_statement から分離して単一責任原則を実現。
 */
class AssertionHandler {
  public:
    explicit AssertionHandler(Interpreter *interpreter);

    /**
     * @brief アサーション文(AST_ASSERT_STMT)を実行
     * @param node AST_ASSERT_STMT ノード
     *
     * アサーションが失敗した場合、プログラムを終了する。
     */
    void handle_assertion(const ASTNode *node);

  private:
    Interpreter *interpreter_;
};

#endif // CB_INTERPRETER_ASSERTION_HANDLER_H
