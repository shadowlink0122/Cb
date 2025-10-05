#pragma once
#include "../framework/test_framework.hpp"
#include "../../../src/backend/interpreter/core/interpreter.h"
#include <memory>

inline void test_arithmetic_addition() {
    Interpreter interpreter(false);
    
    // 異なる型での加算テスト
    auto left_tiny = std::make_unique<ASTNode>(ASTNodeType::AST_NUMBER);
    left_tiny->int_value = 10;
    left_tiny->type_info = TYPE_TINY;
    
    auto right_tiny = std::make_unique<ASTNode>(ASTNodeType::AST_NUMBER);
    right_tiny->int_value = 20;
    right_tiny->type_info = TYPE_TINY;
    
    auto add_op = std::make_unique<ASTNode>(ASTNodeType::AST_BINARY_OP);
    add_op->op = "+";
    add_op->left = std::move(left_tiny);
    add_op->right = std::move(right_tiny);
    
    int64_t result = interpreter.evaluate(add_op.get());
    ASSERT_EQ(30, result);
}

inline void test_arithmetic_subtraction() {
    Interpreter interpreter(false);
    
    auto left = std::make_unique<ASTNode>(ASTNodeType::AST_NUMBER);
    left->int_value = 50;
    
    auto right = std::make_unique<ASTNode>(ASTNodeType::AST_NUMBER);
    right->int_value = 20;
    
    auto sub_op = std::make_unique<ASTNode>(ASTNodeType::AST_BINARY_OP);
    sub_op->op = "-";
    sub_op->left = std::move(left);
    sub_op->right = std::move(right);
    
    int64_t result = interpreter.evaluate(sub_op.get());
    ASSERT_EQ(30, result);
}

inline void test_arithmetic_multiplication() {
    Interpreter interpreter(false);
    
    auto left = std::make_unique<ASTNode>(ASTNodeType::AST_NUMBER);
    left->int_value = 6;
    
    auto right = std::make_unique<ASTNode>(ASTNodeType::AST_NUMBER);
    right->int_value = 7;
    
    auto mul_op = std::make_unique<ASTNode>(ASTNodeType::AST_BINARY_OP);
    mul_op->op = "*";
    mul_op->left = std::move(left);
    mul_op->right = std::move(right);
    
    int64_t result = interpreter.evaluate(mul_op.get());
    ASSERT_EQ(42, result);
}

inline void test_arithmetic_division() {
    Interpreter interpreter(false);
    
    auto left = std::make_unique<ASTNode>(ASTNodeType::AST_NUMBER);
    left->int_value = 84;
    
    auto right = std::make_unique<ASTNode>(ASTNodeType::AST_NUMBER);
    right->int_value = 2;
    
    auto div_op = std::make_unique<ASTNode>(ASTNodeType::AST_BINARY_OP);
    div_op->op = "/";
    div_op->left = std::move(left);
    div_op->right = std::move(right);
    
    int64_t result = interpreter.evaluate(div_op.get());
    ASSERT_EQ(42, result);
}

inline void test_arithmetic_modulo() {
    Interpreter interpreter(false);
    
    auto left = std::make_unique<ASTNode>(ASTNodeType::AST_NUMBER);
    left->int_value = 17;
    
    auto right = std::make_unique<ASTNode>(ASTNodeType::AST_NUMBER);
    right->int_value = 5;
    
    auto mod_op = std::make_unique<ASTNode>(ASTNodeType::AST_BINARY_OP);
    mod_op->op = "%";
    mod_op->left = std::move(left);
    mod_op->right = std::move(right);
    
    int64_t result = interpreter.evaluate(mod_op.get());
    ASSERT_EQ(2, result);
}

inline void test_arithmetic_negative_numbers() {
    Interpreter interpreter(false);
    
    auto left = std::make_unique<ASTNode>(ASTNodeType::AST_NUMBER);
    left->int_value = -10;
    
    auto right = std::make_unique<ASTNode>(ASTNodeType::AST_NUMBER);
    right->int_value = 5;
    
    auto add_op = std::make_unique<ASTNode>(ASTNodeType::AST_BINARY_OP);
    add_op->op = "+";
    add_op->left = std::move(left);
    add_op->right = std::move(right);
    
    int64_t result = interpreter.evaluate(add_op.get());
    ASSERT_EQ(-5, result);
}

inline void test_arithmetic_large_numbers() {
    Interpreter interpreter(false);
    
    auto left = std::make_unique<ASTNode>(ASTNodeType::AST_NUMBER);
    left->int_value = 1000000;
    left->type_info = TYPE_LONG;
    
    auto right = std::make_unique<ASTNode>(ASTNodeType::AST_NUMBER);
    right->int_value = 2000000;
    right->type_info = TYPE_LONG;
    
    auto add_op = std::make_unique<ASTNode>(ASTNodeType::AST_BINARY_OP);
    add_op->op = "+";
    add_op->left = std::move(left);
    add_op->right = std::move(right);
    
    int64_t result = interpreter.evaluate(add_op.get());
    ASSERT_EQ(3000000, result);
}

inline void register_arithmetic_tests() {
    RUN_TEST("arithmetic_addition", test_arithmetic_addition);
    RUN_TEST("arithmetic_subtraction", test_arithmetic_subtraction);
    RUN_TEST("arithmetic_multiplication", test_arithmetic_multiplication);
    RUN_TEST("arithmetic_division", test_arithmetic_division);
    RUN_TEST("arithmetic_modulo", test_arithmetic_modulo);
    // RUN_TEST("arithmetic_negative_numbers", test_arithmetic_negative_numbers);  // 一時的に無効化
    // RUN_TEST("arithmetic_large_numbers", test_arithmetic_large_numbers);  // 一時的に無効化
}
