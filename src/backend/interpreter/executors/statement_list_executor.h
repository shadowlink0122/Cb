#ifndef CB_INTERPRETER_STATEMENT_LIST_EXECUTOR_H
#define CB_INTERPRETER_STATEMENT_LIST_EXECUTOR_H

struct ASTNode;
class Interpreter;

/**
 * @brief 文リスト・複合文の実行を管理するクラス
 *
 * AST_STMT_LIST と AST_COMPOUND_STMT の処理を担当。
 * execute_statement から分離して単一責任原則を実現。
 */
class StatementListExecutor {
  public:
    explicit StatementListExecutor(Interpreter *interpreter);

    /**
     * @brief 文リスト(AST_STMT_LIST)を実行
     * @param node AST_STMT_LIST ノード
     */
    void execute_statement_list(const ASTNode *node);

    /**
     * @brief 複合文(AST_COMPOUND_STMT)を実行
     * @param node AST_COMPOUND_STMT ノード
     */
    void execute_compound_statement(const ASTNode *node);

  private:
    Interpreter *interpreter_;
};

#endif // CB_INTERPRETER_STATEMENT_LIST_EXECUTOR_H
