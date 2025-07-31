#include <cassert>
#include <iostream>

#include "src/ast/ast.h"
#include "src/eval/eval.h"

extern "C" void test_unit_boundary_short() {
    // short型の境界値をAST+evalで
    ASTNode *n_min = new ASTNode(ASTNode::AST_NUM);
    n_min->type_info = 2;
    n_min->lval64 = -32768;
    ASTNode *n_max = new ASTNode(ASTNode::AST_NUM);
    n_max->type_info = 2;
    n_max->lval64 = 32767;
    assert(eval(n_min) == -32768);
    assert(eval(n_max) == 32767);
    delete n_min;
    delete n_max;
}
