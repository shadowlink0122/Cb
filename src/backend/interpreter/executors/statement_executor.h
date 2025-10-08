#pragma once
#include "../../../common/ast.h"
#include "../core/type_inference.h"

// 前方宣言
class Interpreter;
class ExpressionEvaluator;

// Statement実行エンジンクラス
class StatementExecutor {
  public:
    StatementExecutor(Interpreter &interpreter);

    // Statement実行の主要メソッド
    void execute_statement(const ASTNode *node);
    void execute(const ASTNode *node);

    // 専用の実行メソッド
    void execute_multiple_var_decl(const ASTNode *node);
    void execute_array_decl(const ASTNode *node);
    void execute_struct_array_literal_init(const std::string &array_name,
                                           const ASTNode *array_literal,
                                           const std::string &struct_type);

    // 個別の実行メソッド（ハンドラから呼び出されるためpublic）
    void execute_assignment(const ASTNode *node);
    void execute_ternary_assignment(const ASTNode *node);
    void execute_ternary_variable_initialization(const ASTNode *var_decl_node,
                                                 const ASTNode *ternary_node);
    void execute_variable_declaration(const ASTNode *node);
    void execute_union_assignment(const std::string &var_name,
                                  const ASTNode *value_node);
    void execute_member_array_assignment(const ASTNode *node);
    void execute_member_assignment(const ASTNode *node);
    void execute_arrow_assignment(const ASTNode *node);
    void execute_member_array_literal_assignment(const ASTNode *node);
    void execute_self_member_assignment(const std::string &member_name,
                                        const ASTNode *value_node);

    // ヘルパーメソッド
    Variable *evaluate_nested_member_access(const ASTNode *member_access_node);

  private:
    Interpreter &interpreter_; // インタープリターへの参照
};
