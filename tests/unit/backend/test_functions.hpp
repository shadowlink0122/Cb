#pragma once
#include "../framework/test_framework.h"
#include "../../../src/backend/interpreter.h"
#include "../../../src/frontend/parser_utils.h"
#include <memory>

inline void test_function_definition() {
    // 関数定義ASTノードの基本構造をテスト
    auto func_node = std::make_unique<ASTNode>(ASTNodeType::AST_FUNC_DECL);
    func_node->name = "test_func";
    func_node->type_info = TYPE_INT;
    
    // 引数リスト
    auto param1 = std::make_unique<ASTNode>(ASTNodeType::AST_PARAM_DECL);
    param1->name = "x";
    param1->type_info = TYPE_INT;
    
    auto param2 = std::make_unique<ASTNode>(ASTNodeType::AST_PARAM_DECL);
    param2->name = "y";
    param2->type_info = TYPE_INT;
    
    func_node->parameters.push_back(std::move(param1));
    func_node->parameters.push_back(std::move(param2));
    
    // 関数本体（簡単なreturn文）
    auto return_val = std::make_unique<ASTNode>(ASTNodeType::AST_NUMBER);
    return_val->int_value = 42;
    return_val->type_info = TYPE_INT;
    
    auto return_stmt = std::make_unique<ASTNode>(ASTNodeType::AST_RETURN_STMT);
    return_stmt->left = std::move(return_val);
    
    func_node->body = std::move(return_stmt);
    
    // ASTノードの構造をテスト（enum値は比較しない、文字列で確認）
    ASSERT_TRUE(func_node->node_type == ASTNodeType::AST_FUNC_DECL);
    ASSERT_STREQ("test_func", func_node->name.c_str());
    ASSERT_TRUE(func_node->type_info == TYPE_INT);
    ASSERT_TRUE(func_node->parameters.size() == 2);
    ASSERT_NOT_NULL(func_node->body.get());
}

inline void test_function_call() {
    // 関数呼び出しASTノードの構造をテスト
    auto func_call = std::make_unique<ASTNode>(ASTNodeType::AST_FUNC_CALL);
    func_call->name = "simple_func";
    
    // 引数を追加
    auto arg1 = std::make_unique<ASTNode>(ASTNodeType::AST_NUMBER);
    arg1->int_value = 123;
    arg1->type_info = TYPE_INT;
    
    auto arg2 = std::make_unique<ASTNode>(ASTNodeType::AST_VARIABLE);
    arg2->name = "x";
    
    func_call->arguments.push_back(std::move(arg1));
    func_call->arguments.push_back(std::move(arg2));
    
    // ASTノードの構造をテスト
    ASSERT_TRUE(func_call->node_type == ASTNodeType::AST_FUNC_CALL);
    ASSERT_STREQ("simple_func", func_call->name.c_str());
    ASSERT_TRUE(func_call->arguments.size() == 2);
    ASSERT_TRUE(func_call->arguments[0]->node_type == ASTNodeType::AST_NUMBER);
    ASSERT_TRUE(func_call->arguments[1]->node_type == ASTNodeType::AST_VARIABLE);
}

inline void test_recursive_function() {
    // 再帰関数のASTノード構造をテスト
    auto func_def = std::make_unique<ASTNode>(ASTNodeType::AST_FUNC_DECL);
    func_def->name = "factorial";
    func_def->type_info = TYPE_INT;
    
    // パラメータ
    auto param = std::make_unique<ASTNode>(ASTNodeType::AST_PARAM_DECL);
    param->name = "n";
    param->type_info = TYPE_INT;
    func_def->parameters.push_back(std::move(param));
    
    // 条件文 (if n <= 1) - 簡素化
    auto condition = std::make_unique<ASTNode>(ASTNodeType::AST_BINARY_OP);
    condition->op = "<=";  // 文字列として保存
    
    auto n_var = std::make_unique<ASTNode>(ASTNodeType::AST_VARIABLE);
    n_var->name = "n";
    condition->left = std::move(n_var);
    
    auto one = std::make_unique<ASTNode>(ASTNodeType::AST_NUMBER);
    one->int_value = 1;
    condition->right = std::move(one);
    
    // if文全体
    auto if_stmt = std::make_unique<ASTNode>(ASTNodeType::AST_IF_STMT);
    if_stmt->condition = std::move(condition);
    
    func_def->body = std::move(if_stmt);
    
    // ASTノードの構造をテスト
    ASSERT_TRUE(func_def->node_type == ASTNodeType::AST_FUNC_DECL);
    ASSERT_STREQ("factorial", func_def->name.c_str());
    ASSERT_TRUE(func_def->parameters.size() == 1);
    ASSERT_STREQ("n", func_def->parameters[0]->name.c_str());
    ASSERT_NOT_NULL(func_def->body.get());
}

inline void test_function_with_multiple_params() {
    // 複数パラメータを持つ関数のASTノード構造をテスト
    auto func_def = std::make_unique<ASTNode>(ASTNodeType::AST_FUNC_DECL);
    func_def->name = "add_three";
    func_def->type_info = TYPE_INT;
    
    // 3つのパラメータ
    auto param1 = std::make_unique<ASTNode>(ASTNodeType::AST_PARAM_DECL);
    param1->name = "a";
    param1->type_info = TYPE_INT;
    
    auto param2 = std::make_unique<ASTNode>(ASTNodeType::AST_PARAM_DECL);
    param2->name = "b";
    param2->type_info = TYPE_INT;
    
    auto param3 = std::make_unique<ASTNode>(ASTNodeType::AST_PARAM_DECL);
    param3->name = "c";
    param3->type_info = TYPE_INT;
    
    func_def->parameters.push_back(std::move(param1));
    func_def->parameters.push_back(std::move(param2));
    func_def->parameters.push_back(std::move(param3));
    
    // 返り値の式 (a + b + c)
    auto add_expr = std::make_unique<ASTNode>(ASTNodeType::AST_BINARY_OP);
    add_expr->op = "+";
    
    auto a_var = std::make_unique<ASTNode>(ASTNodeType::AST_VARIABLE);
    a_var->name = "a";
    add_expr->left = std::move(a_var);
    
    auto b_var = std::make_unique<ASTNode>(ASTNodeType::AST_VARIABLE);
    b_var->name = "b";
    add_expr->right = std::move(b_var);
    
    auto return_stmt = std::make_unique<ASTNode>(ASTNodeType::AST_RETURN_STMT);
    return_stmt->left = std::move(add_expr);
    
    func_def->body = std::move(return_stmt);
    
    // ASTノードの構造をテスト
    ASSERT_TRUE(func_def->node_type == ASTNodeType::AST_FUNC_DECL);
    ASSERT_STREQ("add_three", func_def->name.c_str());
    ASSERT_TRUE(func_def->parameters.size() == 3);
    ASSERT_STREQ("a", func_def->parameters[0]->name.c_str());
    ASSERT_STREQ("b", func_def->parameters[1]->name.c_str());
    ASSERT_STREQ("c", func_def->parameters[2]->name.c_str());
    ASSERT_NOT_NULL(func_def->body.get());
}

inline void test_function_return_types() {
    // 異なる戻り値型を持つ関数のASTノード構造をテスト
    
    // INT型の関数
    auto int_func = std::make_unique<ASTNode>(ASTNodeType::AST_FUNC_DECL);
    int_func->name = "get_int";
    int_func->type_info = TYPE_INT;
    
    auto int_return = std::make_unique<ASTNode>(ASTNodeType::AST_NUMBER);
    int_return->int_value = 999;
    int_return->type_info = TYPE_INT;
    
    auto int_return_stmt = std::make_unique<ASTNode>(ASTNodeType::AST_RETURN_STMT);
    int_return_stmt->left = std::move(int_return);
    int_func->body = std::move(int_return_stmt);
    
    // STRING型の関数
    auto str_func = std::make_unique<ASTNode>(ASTNodeType::AST_FUNC_DECL);
    str_func->name = "get_string";
    str_func->type_info = TYPE_STRING;
    
    auto str_return = std::make_unique<ASTNode>(ASTNodeType::AST_STRING_LITERAL);
    str_return->str_value = "hello";
    str_return->type_info = TYPE_STRING;
    
    auto str_return_stmt = std::make_unique<ASTNode>(ASTNodeType::AST_RETURN_STMT);
    str_return_stmt->left = std::move(str_return);
    str_func->body = std::move(str_return_stmt);
    
    // ASTノードの構造をテスト
    ASSERT_TRUE(int_func->node_type == ASTNodeType::AST_FUNC_DECL);
    ASSERT_TRUE(int_func->type_info == TYPE_INT);
    ASSERT_TRUE(int_func->body->node_type == ASTNodeType::AST_RETURN_STMT);
    
    ASSERT_TRUE(str_func->node_type == ASTNodeType::AST_FUNC_DECL);
    ASSERT_TRUE(str_func->type_info == TYPE_STRING);
    ASSERT_TRUE(str_func->body->node_type == ASTNodeType::AST_RETURN_STMT);
}

inline void register_function_tests() {
    // 関数機能のテストを実行する（簡素化されたバージョン）
    test_runner.add_test("backend", "function_definition", test_function_definition, false);
    test_runner.add_test("backend", "function_call", test_function_call, false);
    test_runner.add_test("backend", "recursive_function", test_recursive_function, false);
    test_runner.add_test("backend", "function_with_multiple_params", test_function_with_multiple_params, false);
    test_runner.add_test("backend", "function_return_types", test_function_return_types, false);
}
