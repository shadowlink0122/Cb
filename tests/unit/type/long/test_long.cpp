#include <cassert>
#include <iostream>
#include "src/eval/eval.h"

void test_unit_long() {
    // long型の範囲内のみテスト（範囲外リテラル例外テストは行わない）
    ASTNode n1; n1.type_info = 4; n1.lval64 = 9223372036854775807LL; n1.type = ASTNode::AST_NUM; n1.lhs = nullptr; n1.rhs = nullptr;
    assert(eval_num(&n1) == 9223372036854775807LL);
    ASTNode n2; n2.type_info = 4; n2.lval64 = -9223372036854775807LL - 1; n2.type = ASTNode::AST_NUM; n2.lhs = nullptr; n2.rhs = nullptr;
    assert(eval_num(&n2) == -9223372036854775807LL - 1);
    std::cout << "[unit] long test passed\n";
}
