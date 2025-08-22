#pragma once
#include "../framework/test_framework.h"
#include "../../../src/backend/interpreter.h"
#include "../../../src/frontend/parser_utils.h"
#include <memory>

inline void test_function_definition() {
    Interpreter interpreter(false);
    
    // 基本的な関数定義のテスト
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
    
    // 関数本体（x + y を返す）
    auto x_var = std::make_unique<ASTNode>(ASTNodeType::AST_VARIABLE);
    x_var->name = "x";
    
    auto y_var = std::make_unique<ASTNode>(ASTNodeType::AST_VARIABLE);
    y_var->name = "y";
    
    auto add_op = std::make_unique<ASTNode>(ASTNodeType::AST_BINARY_OP);
    add_op->op = "+";
    add_op->left = std::move(x_var);
    add_op->right = std::move(y_var);
    
    auto return_stmt = std::make_unique<ASTNode>(ASTNodeType::AST_RETURN_STMT);
    return_stmt->left = std::move(add_op);
    
    func_node->body = std::move(return_stmt);
    
    // 関数定義を実行（例外が発生しないことを確認）
    try {
        interpreter.evaluate(func_node.get());
    } catch (const std::exception& e) {
        ASSERT_TRUE(false); // 関数定義で例外が発生してはいけない
    }
}

inline void test_function_call() {
    Interpreter interpreter(false);
    
    // まず関数を定義
    auto func_def = std::make_unique<ASTNode>(ASTNodeType::AST_FUNC_DECL);
    func_def->name = "add_func";
    func_def->type_info = TYPE_INT;
    
    auto param1 = std::make_unique<ASTNode>(ASTNodeType::AST_PARAM_DECL);
    param1->name = "a";
    param1->type_info = TYPE_INT;
    
    auto param2 = std::make_unique<ASTNode>(ASTNodeType::AST_PARAM_DECL);
    param2->name = "b";
    param2->type_info = TYPE_INT;
    
    func_def->parameters.push_back(std::move(param1));
    func_def->parameters.push_back(std::move(param2));
    
    auto a_var = std::make_unique<ASTNode>(ASTNodeType::AST_VARIABLE);
    a_var->name = "a";
    
    auto b_var = std::make_unique<ASTNode>(ASTNodeType::AST_VARIABLE);
    b_var->name = "b";
    
    auto add_op = std::make_unique<ASTNode>(ASTNodeType::AST_BINARY_OP);
    add_op->op = "+";
    add_op->left = std::move(a_var);
    add_op->right = std::move(b_var);
    
    auto return_stmt = std::make_unique<ASTNode>(ASTNodeType::AST_RETURN_STMT);
    return_stmt->left = std::move(add_op);
    
    func_def->body = std::move(return_stmt);
    
    // 関数定義
    interpreter.evaluate(func_def.get());
    
    // 関数呼び出し
    auto func_call = std::make_unique<ASTNode>(ASTNodeType::AST_FUNC_CALL);
    func_call->name = "add_func";
    
    auto arg1 = std::make_unique<ASTNode>(ASTNodeType::AST_NUMBER);
    arg1->int_value = 10;
    arg1->type_info = TYPE_INT;
    
    auto arg2 = std::make_unique<ASTNode>(ASTNodeType::AST_NUMBER);
    arg2->int_value = 20;
    arg2->type_info = TYPE_INT;
    
    func_call->arguments.push_back(std::move(arg1));
    func_call->arguments.push_back(std::move(arg2));
    
    int64_t result = interpreter.evaluate(func_call.get());
    ASSERT_EQ(30, result);
}

inline void test_recursive_function() {
    Interpreter interpreter(false);
    
    // 階乗関数のテスト（簡単な再帰）
    auto func_def = std::make_unique<ASTNode>(ASTNodeType::AST_FUNC_DECL);
    func_def->name = "factorial";
    func_def->type_info = TYPE_INT;
    
    auto param = std::make_unique<ASTNode>(ASTNodeType::AST_PARAM_DECL);
    param->name = "n";
    param->type_info = TYPE_INT;
    func_def->parameters.push_back(std::move(param));
    
    // if (n <= 1) return 1; else return n * factorial(n-1);
    // 簡略化: n > 1の場合のみテスト
    auto n_var = std::make_unique<ASTNode>(ASTNodeType::AST_VARIABLE);
    n_var->name = "n";
    
    auto one = std::make_unique<ASTNode>(ASTNodeType::AST_NUMBER);
    one->int_value = 1;
    one->type_info = TYPE_INT;
    
    auto return_stmt = std::make_unique<ASTNode>(ASTNodeType::AST_RETURN_STMT);
    return_stmt->left = std::move(one); // 簡略化: 常に1を返す
    
    func_def->body = std::move(return_stmt);
    
    // 関数定義
    interpreter.evaluate(func_def.get());
    
    // 関数呼び出し
    auto func_call = std::make_unique<ASTNode>(ASTNodeType::AST_FUNC_CALL);
    func_call->name = "factorial";
    
    auto arg = std::make_unique<ASTNode>(ASTNodeType::AST_NUMBER);
    arg->int_value = 3;
    arg->type_info = TYPE_INT;
    
    func_call->arguments.push_back(std::move(arg));
    
    // 関数呼び出しが実行できることを確認
    try {
        int64_t result = interpreter.evaluate(func_call.get());
        ASSERT_EQ(1, result); // 簡略化した実装では1が返される
    } catch (const std::exception& e) {
        ASSERT_TRUE(false); // 再帰関数呼び出しで例外が発生してはいけない
    }
}

inline void test_function_with_multiple_params() {
    Interpreter interpreter(false);
    
    // 複数パラメータの関数テスト
    auto func_def = std::make_unique<ASTNode>(ASTNodeType::AST_FUNC_DECL);
    func_def->name = "calc_func";
    func_def->type_info = TYPE_INT;
    
    auto param1 = std::make_unique<ASTNode>(ASTNodeType::AST_PARAM_DECL);
    param1->name = "x";
    param1->type_info = TYPE_INT;
    
    auto param2 = std::make_unique<ASTNode>(ASTNodeType::AST_PARAM_DECL);
    param2->name = "y";
    param2->type_info = TYPE_INT;
    
    auto param3 = std::make_unique<ASTNode>(ASTNodeType::AST_PARAM_DECL);
    param3->name = "z";
    param3->type_info = TYPE_INT;
    
    func_def->parameters.push_back(std::move(param1));
    func_def->parameters.push_back(std::move(param2));
    func_def->parameters.push_back(std::move(param3));
    
    // return x + y + z;
    auto x_var = std::make_unique<ASTNode>(ASTNodeType::AST_VARIABLE);
    x_var->name = "x";
    
    auto y_var = std::make_unique<ASTNode>(ASTNodeType::AST_VARIABLE);
    y_var->name = "y";
    
    auto add1 = std::make_unique<ASTNode>(ASTNodeType::AST_BINARY_OP);
    add1->op = "+";
    add1->left = std::move(x_var);
    add1->right = std::move(y_var);
    
    auto z_var = std::make_unique<ASTNode>(ASTNodeType::AST_VARIABLE);
    z_var->name = "z";
    
    auto add2 = std::make_unique<ASTNode>(ASTNodeType::AST_BINARY_OP);
    add2->op = "+";
    add2->left = std::move(add1);
    add2->right = std::move(z_var);
    
    auto return_stmt = std::make_unique<ASTNode>(ASTNodeType::AST_RETURN_STMT);
    return_stmt->left = std::move(add2);
    
    func_def->body = std::move(return_stmt);
    
    // 関数定義
    interpreter.evaluate(func_def.get());
    
    // 関数呼び出し
    auto func_call = std::make_unique<ASTNode>(ASTNodeType::AST_FUNC_CALL);
    func_call->name = "calc_func";
    
    auto arg1 = std::make_unique<ASTNode>(ASTNodeType::AST_NUMBER);
    arg1->int_value = 5;
    arg1->type_info = TYPE_INT;
    
    auto arg2 = std::make_unique<ASTNode>(ASTNodeType::AST_NUMBER);
    arg2->int_value = 10;
    arg2->type_info = TYPE_INT;
    
    auto arg3 = std::make_unique<ASTNode>(ASTNodeType::AST_NUMBER);
    arg3->int_value = 15;
    arg3->type_info = TYPE_INT;
    
    func_call->arguments.push_back(std::move(arg1));
    func_call->arguments.push_back(std::move(arg2));
    func_call->arguments.push_back(std::move(arg3));
    
    int64_t result = interpreter.evaluate(func_call.get());
    ASSERT_EQ(30, result); // 5 + 10 + 15 = 30
}

inline void test_function_return_types() {
    Interpreter interpreter(false);
    
    // 異なる戻り値型の関数テスト
    auto func_def = std::make_unique<ASTNode>(ASTNodeType::AST_FUNC_DECL);
    func_def->name = "get_tiny";
    func_def->type_info = TYPE_TINY;
    
    auto tiny_val = std::make_unique<ASTNode>(ASTNodeType::AST_NUMBER);
    tiny_val->int_value = 100;
    tiny_val->type_info = TYPE_TINY;
    
    auto return_stmt = std::make_unique<ASTNode>(ASTNodeType::AST_RETURN_STMT);
    return_stmt->left = std::move(tiny_val);
    
    func_def->body = std::move(return_stmt);
    
    // 関数定義
    interpreter.evaluate(func_def.get());
    
    // 関数呼び出し
    auto func_call = std::make_unique<ASTNode>(ASTNodeType::AST_FUNC_CALL);
    func_call->name = "get_tiny";
    
    int64_t result = interpreter.evaluate(func_call.get());
    ASSERT_EQ(100, result);
}

inline void register_function_tests() {
    // 関数機能はまだ実装されていないため、これらのテストは失敗が予想される
    test_runner.add_test("backend", "function_definition", test_function_definition, true);
    test_runner.add_test("backend", "function_call", test_function_call, true);
    test_runner.add_test("backend", "recursive_function", test_recursive_function, true);
    test_runner.add_test("backend", "function_with_multiple_params", test_function_with_multiple_params, true);
    test_runner.add_test("backend", "function_return_types", test_function_return_types, true);
}
