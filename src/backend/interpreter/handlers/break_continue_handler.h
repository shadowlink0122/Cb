#ifndef CB_INTERPRETER_BREAK_CONTINUE_HANDLER_H
#define CB_INTERPRETER_BREAK_CONTINUE_HANDLER_H

class ASTNode;
class Interpreter;

/**
 * @brief break/continue文の実行を管理するクラス
 *
 * AST_BREAK_STMT と AST_CONTINUE_STMT の処理を担当。
 * execute_statement から分離して単一責任原則を実現。
 */
class BreakContinueHandler {
  public:
    explicit BreakContinueHandler(Interpreter *interpreter);

    /**
     * @brief break文(AST_BREAK_STMT)を実行
     * @param node AST_BREAK_STMT ノード
     *
     * BreakException をスローしてループを抜ける。
     */
    void handle_break(const ASTNode *node);

    /**
     * @brief continue文(AST_CONTINUE_STMT)を実行
     * @param node AST_CONTINUE_STMT ノード
     *
     * ContinueException をスローしてループの次のイテレーションに進む。
     */
    void handle_continue(const ASTNode *node);

  private:
    Interpreter *interpreter_;
};

#endif // CB_INTERPRETER_BREAK_CONTINUE_HANDLER_H
