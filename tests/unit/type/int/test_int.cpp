#include <cassert>
#include <iostream>
#include "src/eval.h"

void test_unit_int() {
    // int型の範囲内
    ASTNode n1; n1.type_info = 3; n1.lval64 = 2147483647; n1.type = ASTNode::AST_NUM; n1.lhs = nullptr; n1.rhs = nullptr;
    assert(eval_num(&n1) == 2147483647);
    ASTNode n2; n2.type_info = 3; n2.lval64 = -2147483648LL; n2.type = ASTNode::AST_NUM; n2.lhs = nullptr; n2.rhs = nullptr;
    assert(eval_num(&n2) == -2147483648LL);
    // ※delete等の明示的な解放は一切行わない
    // 範囲外（値が変化しないことを確認）
    ASTNode* lhs = new ASTNode(); lhs->type_info = 3; lhs->sval = "i"; lhs->type = ASTNode::AST_VAR; lhs->lval64 = 0;
    ASTNode* rhs = new ASTNode(); rhs->type_info = 3; rhs->lval64 = 2147483648LL; rhs->type = ASTNode::AST_NUM;
    ASTNode assign; assign.type_info = 3; assign.lhs = lhs; assign.rhs = rhs; assign.type = ASTNode::AST_ASSIGN; assign.sval = "i";
    eval_assign(&assign);
    assert(lhs->lval64 == 0 && "int型オーバーフロー時は値が変化しないこと");
    std::cout << "[unit] int overflow test passed\n";
    std::cout << "[unit] int test passed\n";
}
