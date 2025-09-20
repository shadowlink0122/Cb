#pragma once
#include "../framework/test_framework.hpp"
#include "../../../src/backend/interpreter.h"
#include <memory>
#include <climits>

inline void test_boundary_tiny_values() {
    Interpreter interpreter(false);
    
    // tiny型の境界値テスト (signed char: -128 to 127)
    auto min_literal = std::make_unique<ASTNode>(ASTNodeType::AST_NUMBER);
    min_literal->int_value = -128;
    min_literal->type_info = TYPE_TINY;
    
    auto max_literal = std::make_unique<ASTNode>(ASTNodeType::AST_NUMBER);
    max_literal->int_value = 127;
    max_literal->type_info = TYPE_TINY;
    
    int64_t min_result = interpreter.evaluate(min_literal.get());
    int64_t max_result = interpreter.evaluate(max_literal.get());
    
    ASSERT_EQ(-128, min_result);
    ASSERT_EQ(127, max_result);
}

inline void test_boundary_short_values() {
    Interpreter interpreter(false);
    
    // short型の境界値テスト (-32768 to 32767)
    auto min_literal = std::make_unique<ASTNode>(ASTNodeType::AST_NUMBER);
    min_literal->int_value = -32768;
    min_literal->type_info = TYPE_SHORT;
    
    auto max_literal = std::make_unique<ASTNode>(ASTNodeType::AST_NUMBER);
    max_literal->int_value = 32767;
    max_literal->type_info = TYPE_SHORT;
    
    int64_t min_result = interpreter.evaluate(min_literal.get());
    int64_t max_result = interpreter.evaluate(max_literal.get());
    
    ASSERT_EQ(-32768, min_result);
    ASSERT_EQ(32767, max_result);
}

inline void test_boundary_int_values() {
    Interpreter interpreter(false);
    
    // int型の境界値テスト (-2147483648 to 2147483647)
    auto min_literal = std::make_unique<ASTNode>(ASTNodeType::AST_NUMBER);
    min_literal->int_value = INT_MIN;
    min_literal->type_info = TYPE_INT;
    
    auto max_literal = std::make_unique<ASTNode>(ASTNodeType::AST_NUMBER);
    max_literal->int_value = INT_MAX;
    max_literal->type_info = TYPE_INT;
    
    int64_t min_result = interpreter.evaluate(min_literal.get());
    int64_t max_result = interpreter.evaluate(max_literal.get());
    
    ASSERT_EQ(INT_MIN, min_result);
    ASSERT_EQ(INT_MAX, max_result);
}

inline void test_boundary_long_values() {
    Interpreter interpreter(false);
    
    // long型の境界値テスト
    auto min_literal = std::make_unique<ASTNode>(ASTNodeType::AST_NUMBER);
    min_literal->int_value = LLONG_MIN;
    min_literal->type_info = TYPE_LONG;
    
    auto max_literal = std::make_unique<ASTNode>(ASTNodeType::AST_NUMBER);
    max_literal->int_value = LLONG_MAX;
    max_literal->type_info = TYPE_LONG;
    
    int64_t min_result = interpreter.evaluate(min_literal.get());
    int64_t max_result = interpreter.evaluate(max_literal.get());
    
    ASSERT_EQ(LLONG_MIN, min_result);
    ASSERT_EQ(LLONG_MAX, max_result);
}

inline void test_boundary_overflow_detection() {
    Interpreter interpreter(false);
    
    // オーバーフロー近似テスト（実際のオーバーフロー検出は統合テストで）
    auto large_value = std::make_unique<ASTNode>(ASTNodeType::AST_NUMBER);
    large_value->int_value = 1000000;
    large_value->type_info = TYPE_INT;
    
    // 大きな値でも正常に評価されることを確認
    int64_t result = interpreter.evaluate(large_value.get());
    ASSERT_EQ(1000000, result);
}

inline void register_boundary_tests() {
    RUN_TEST("boundary_tiny_values", test_boundary_tiny_values);
    RUN_TEST("boundary_short_values", test_boundary_short_values);
    RUN_TEST("boundary_int_values", test_boundary_int_values);
    RUN_TEST("boundary_long_values", test_boundary_long_values);
    RUN_TEST("boundary_overflow_detection", test_boundary_overflow_detection);
}
