#include <cassert>
#include <iostream>
#include "src/eval/eval.h"

void test_unit_tiny() {
    // tiny型の範囲内
    ASTNode n1; n1.type_info = 1; n1.lval64 = 127; n1.type = ASTNode::AST_NUM; n1.lhs = nullptr; n1.rhs = nullptr;
    assert(eval_num(&n1) == 127);
    ASTNode n2; n2.type_info = 1; n2.lval64 = -128; n2.type = ASTNode::AST_NUM; n2.lhs = nullptr; n2.rhs = nullptr;
    assert(eval_num(&n2) == -128);
    // ※delete等の明示的な解放は一切行わない
    // 範囲外（値が変化しないことを確認）
    ASTNode* lhs = new ASTNode(); lhs->type_info = 1; lhs->sval = "t"; lhs->type = ASTNode::AST_VAR; lhs->lval64 = 0;
    ASTNode* rhs = new ASTNode(); rhs->type_info = 1; rhs->lval64 = 128; rhs->type = ASTNode::AST_NUM;
    ASTNode assign; assign.type_info = 1; assign.lhs = lhs; assign.rhs = rhs; assign.type = ASTNode::AST_ASSIGN; assign.sval = "t";
    eval_assign(&assign);
    assert(lhs->lval64 == 0 && "tiny型オーバーフロー時は値が変化しないこと");
    std::cout << "[unit] tiny overflow test passed\n";
    std::cout << "[unit] tiny test passed\n";
}
