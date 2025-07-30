#include <cassert>
#include <iostream>
#include "src/eval.h"

void test_unit_long() {
    // long型の範囲内
    ASTNode n1; n1.type_info = 4; n1.lval64 = 9223372036854775807LL; n1.type = ASTNode::AST_NUM; n1.lhs = nullptr; n1.rhs = nullptr;
    assert(eval_num(&n1) == 9223372036854775807LL);
    ASTNode n2; n2.type_info = 4; n2.lval64 = -9223372036854775807LL - 1; n2.type = ASTNode::AST_NUM; n2.lhs = nullptr; n2.rhs = nullptr;
    assert(eval_num(&n2) == -9223372036854775807LL - 1);
    // ※delete等の明示的な解放は一切行わない
    // 範囲外（値が変化しないことを確認）
    ASTNode* lhs = new ASTNode(); lhs->type_info = 4; lhs->sval = "l"; lhs->type = ASTNode::AST_VAR; lhs->lval64 = 0;
    ASTNode* rhs = new ASTNode(); rhs->type_info = 4; rhs->lval64 = -9223372036854775808ULL; rhs->type = ASTNode::AST_NUM;
    ASTNode assign; assign.type_info = 4; assign.lhs = lhs; assign.rhs = rhs; assign.type = ASTNode::AST_ASSIGN; assign.sval = "l";
    eval_assign(&assign);
    assert(lhs->lval64 == 0 && "long型オーバーフロー時は値が変化しないこと");
    std::cout << "[unit] long overflow test passed\n";
    std::cout << "[unit] long test passed\n";
}
