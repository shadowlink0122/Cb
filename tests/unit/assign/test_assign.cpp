#include <cassert>
#include <iostream>

// Cb ASTノードを直接生成して代入・参照をテスト
#include "src/ast/ast.h"
#include "src/eval/eval.h"

// 各型ごとのassignテスト関数の宣言
extern "C" void test_unit_assign_tiny();
extern "C" void test_unit_assign_short();
extern "C" void test_unit_assign_int();
extern "C" void test_unit_assign_long();

// assignテスト統合関数
extern "C" void test_unit_assign() {
    test_unit_assign_tiny();
    test_unit_assign_short();
    test_unit_assign_int();
    test_unit_assign_long();
}
