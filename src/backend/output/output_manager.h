#pragma once
#include "../../common/ast.h"
#include <string>

// 前方宣言
class Interpreter;
struct Variable;
class ReturnException;

// 出力管理クラス
class OutputManager {
public:
    OutputManager(Interpreter* interpreter);
    
    // 基本出力機能
    void print_value(const ASTNode *expr);
    void print_value_with_newline(const ASTNode *expr);
    void print_multiple_with_newline(const ASTNode *arg_list);
    void print_formatted_with_newline(const ASTNode *format_str, const ASTNode *arg_list);
    void print_formatted(const ASTNode *format_str, const ASTNode *arg_list);
    void print_multiple(const ASTNode *arg_list);

private:
    Interpreter* interpreter_;  // インタープリターへの参照
    
    // ヘルパーメソッド
    Variable* find_variable(const std::string& name);
    int64_t evaluate_expression(const ASTNode* node);
    const ASTNode* find_function(const std::string& name);
};
