#include <cassert>
#include <iostream>

#include "src/ast/ast.h"
#include "src/eval/eval.h"

extern "C" void test_unit_boundary_tiny() {
    // tiny型の境界値をAST+evalで
    ASTNode *n_min = new ASTNode(ASTNode::AST_NUM);
    n_min->type_info = 1;
    n_min->lval64 = -128;
    ASTNode *n_max = new ASTNode(ASTNode::AST_NUM);
    n_max->type_info = 1;
    n_max->lval64 = 127;
    assert(eval(n_min) == -128);
    assert(eval(n_max) == 127);
    delete n_min;
    delete n_max;
}
