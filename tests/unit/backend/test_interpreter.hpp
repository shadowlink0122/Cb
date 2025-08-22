#pragma once
#include "../framework/test_framework.h"
#include "../../../src/backend/interpreter.h"
#include <memory>

inline void test_interpreter_creation() {
    Interpreter interpreter(false);
    // インタープリターが正常に作成されることを確認
    // オブジェクトが例外なく作成されればテスト成功
}

inline void test_simple_number_evaluation() {
    Interpreter interpreter(false);
    
    // 単純な数値リテラルを評価
    auto literal = std::make_unique<ASTNode>(ASTNodeType::AST_NUMBER);
    literal->int_value = 42;
    literal->type_info = TYPE_INT;
    
    int64_t result = interpreter.evaluate(literal.get());
    ASSERT_EQ(42, result);
}

inline void test_string_literal_evaluation() {
    Interpreter interpreter(false);
    
    // 文字列リテラルの評価
    auto string_literal = std::make_unique<ASTNode>(ASTNodeType::AST_STRING_LITERAL);
    string_literal->str_value = "Hello World";
    string_literal->type_info = TYPE_STRING;
    
    // 文字列の場合は戻り値は使用されないが、例外が発生しなければOK
    try {
        interpreter.evaluate(string_literal.get());
    } catch (const std::exception& e) {
        // 例外が発生した場合はテスト失敗
        ASSERT_TRUE(false);
    }
}

inline void test_simple_ast_evaluation() {
    Interpreter interpreter(false);
    
    // 単純な二項演算をテスト: 10 + 20 = 30
    auto left = std::make_unique<ASTNode>(ASTNodeType::AST_NUMBER);
    left->int_value = 10;
    
    auto right = std::make_unique<ASTNode>(ASTNodeType::AST_NUMBER);
    right->int_value = 20;
    
    auto binop = std::make_unique<ASTNode>(ASTNodeType::AST_BINARY_OP);
    binop->op = "+";
    binop->left = std::move(left);
    binop->right = std::move(right);
    
    // 評価実行
    int64_t result = interpreter.evaluate(binop.get());
    ASSERT_EQ(30, result);
}

inline void register_interpreter_tests() {
    test_runner.add_test("backend", "interpreter_creation", test_interpreter_creation);
    test_runner.add_test("backend", "simple_number_evaluation", test_simple_number_evaluation);
    test_runner.add_test("backend", "string_literal_evaluation", test_string_literal_evaluation);
    test_runner.add_test("backend", "simple_ast_evaluation", test_simple_ast_evaluation);
}
