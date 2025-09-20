#pragma once
#include "../framework/test_framework.hpp"
#include "../../../src/backend/interpreter.h"
#include <memory>

inline void test_cross_type_tiny_to_int() {
    Interpreter interpreter(false);
    
    // tiny値をint演算で使用
    auto tiny_val = std::make_unique<ASTNode>(ASTNodeType::AST_NUMBER);
    tiny_val->int_value = 100;
    tiny_val->type_info = TYPE_TINY;
    
    auto int_val = std::make_unique<ASTNode>(ASTNodeType::AST_NUMBER);
    int_val->int_value = 200;
    int_val->type_info = TYPE_INT;
    
    auto add_op = std::make_unique<ASTNode>(ASTNodeType::AST_BINARY_OP);
    add_op->op = "+";
    add_op->left = std::move(tiny_val);
    add_op->right = std::move(int_val);
    
    int64_t result = interpreter.evaluate(add_op.get());
    ASSERT_EQ(300, result);
}

inline void test_cross_type_short_to_long() {
    Interpreter interpreter(false);
    
    auto short_val = std::make_unique<ASTNode>(ASTNodeType::AST_NUMBER);
    short_val->int_value = 30000;
    short_val->type_info = TYPE_SHORT;
    
    auto long_val = std::make_unique<ASTNode>(ASTNodeType::AST_NUMBER);
    long_val->int_value = 70000;
    long_val->type_info = TYPE_LONG;
    
    auto add_op = std::make_unique<ASTNode>(ASTNodeType::AST_BINARY_OP);
    add_op->op = "+";
    add_op->left = std::move(short_val);
    add_op->right = std::move(long_val);
    
    int64_t result = interpreter.evaluate(add_op.get());
    ASSERT_EQ(100000, result);
}

inline void test_cross_type_int_to_long() {
    Interpreter interpreter(false);
    
    auto int_val = std::make_unique<ASTNode>(ASTNodeType::AST_NUMBER);
    int_val->int_value = 1000000;
    int_val->type_info = TYPE_INT;
    
    auto long_val = std::make_unique<ASTNode>(ASTNodeType::AST_NUMBER);
    long_val->int_value = 2000000;
    long_val->type_info = TYPE_LONG;
    
    auto mul_op = std::make_unique<ASTNode>(ASTNodeType::AST_BINARY_OP);
    mul_op->op = "*";
    mul_op->left = std::move(int_val);
    mul_op->right = std::move(long_val);
    
    int64_t result = interpreter.evaluate(mul_op.get());
    ASSERT_EQ(2000000000000LL, result);
}

inline void test_cross_type_mixed_arithmetic() {
    Interpreter interpreter(false);
    
    // (tiny + short) * int の複合演算
    auto tiny_val = std::make_unique<ASTNode>(ASTNodeType::AST_NUMBER);
    tiny_val->int_value = 5;
    tiny_val->type_info = TYPE_TINY;
    
    auto short_val = std::make_unique<ASTNode>(ASTNodeType::AST_NUMBER);
    short_val->int_value = 10;
    short_val->type_info = TYPE_SHORT;
    
    auto add_op = std::make_unique<ASTNode>(ASTNodeType::AST_BINARY_OP);
    add_op->op = "+";
    add_op->left = std::move(tiny_val);
    add_op->right = std::move(short_val);
    
    auto int_val = std::make_unique<ASTNode>(ASTNodeType::AST_NUMBER);
    int_val->int_value = 3;
    int_val->type_info = TYPE_INT;
    
    auto mul_op = std::make_unique<ASTNode>(ASTNodeType::AST_BINARY_OP);
    mul_op->op = "*";
    mul_op->left = std::move(add_op);
    mul_op->right = std::move(int_val);
    
    int64_t result = interpreter.evaluate(mul_op.get());
    ASSERT_EQ(45, result); // (5 + 10) * 3 = 45
}

inline void test_cross_type_comparison() {
    Interpreter interpreter(false);
    
    // 異なる型同士の比較（等価）
    auto int_val = std::make_unique<ASTNode>(ASTNodeType::AST_NUMBER);
    int_val->int_value = 100;
    int_val->type_info = TYPE_INT;
    
    auto long_val = std::make_unique<ASTNode>(ASTNodeType::AST_NUMBER);
    long_val->int_value = 100;
    long_val->type_info = TYPE_LONG;
    
    auto eq_op = std::make_unique<ASTNode>(ASTNodeType::AST_BINARY_OP);
    eq_op->op = "==";
    eq_op->left = std::move(int_val);
    eq_op->right = std::move(long_val);
    
    // 比較演算の結果は統合テストで確認するが、例外が発生しないことを確認
    try {
        interpreter.evaluate(eq_op.get());
    } catch (const std::exception& e) {
        ASSERT_TRUE(false); // 例外が発生してはいけない
    }
}

inline void register_cross_type_tests() {
    RUN_TEST("cross_type_tiny_to_int", test_cross_type_tiny_to_int);
    RUN_TEST("cross_type_short_to_long", test_cross_type_short_to_long);
    RUN_TEST("cross_type_int_to_long", test_cross_type_int_to_long);
    RUN_TEST("cross_type_mixed_arithmetic", test_cross_type_mixed_arithmetic);
    RUN_TEST("cross_type_comparison", test_cross_type_comparison);
}
