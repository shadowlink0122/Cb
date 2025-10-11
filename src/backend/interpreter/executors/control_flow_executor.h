#ifndef CONTROL_FLOW_EXECUTOR_H
#define CONTROL_FLOW_EXECUTOR_H

struct ASTNode;
class Interpreter;

// 制御フロー文（IF, WHILE, FOR）を実行するクラス
class ControlFlowExecutor {
  public:
    explicit ControlFlowExecutor(Interpreter *interpreter)
        : interpreter_(interpreter) {}

    // IF文の実行
    void execute_if_statement(const ASTNode *node);

    // WHILE文の実行
    void execute_while_statement(const ASTNode *node);

    // FOR文の実行
    void execute_for_statement(const ASTNode *node);

    // SWITCH文の実行
    void execute_switch_statement(const ASTNode *node);

  private:
    Interpreter *interpreter_;

    // Switch文のヘルパー関数
    bool match_case_value(const ASTNode *switch_expr_value,
                          const ASTNode *case_value);
};

#endif // CONTROL_FLOW_EXECUTOR_H
