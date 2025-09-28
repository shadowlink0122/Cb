#pragma once
#include "../../../common/ast.h"

// 前方宣言
class Interpreter;

// Statement実行エンジンクラス
class StatementExecutor {
public:
    StatementExecutor(Interpreter& interpreter);
    
    // Statement実行の主要メソッド
    void execute_statement(const ASTNode *node);
    void execute(const ASTNode *node);
    
    // 専用の実行メソッド
    void execute_multiple_var_decl(const ASTNode *node);
    void execute_array_decl(const ASTNode *node);
    void execute_struct_array_literal_init(const std::string& array_name, const ASTNode* array_literal, const std::string& struct_type);
    
private:
    Interpreter& interpreter_;  // インタープリターへの参照
    
    // 個別の実行メソッド
    void execute_assignment(const ASTNode* node);
    void execute_variable_declaration(const ASTNode* node);
    void execute_union_assignment(const std::string& var_name, const ASTNode* value_node);
    void execute_member_array_assignment(const ASTNode* node);
    void execute_member_assignment(const ASTNode* node);
    void execute_member_array_literal_assignment(const ASTNode* node);
};
