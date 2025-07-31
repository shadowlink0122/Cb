#include <cassert>
#include <iostream>

#include "src/ast/ast.h"
#include "src/eval/eval.h"

extern "C" void test_unit_boundary_int() {
    // int型の境界値をAST+evalで
    ASTNode *n_min = new ASTNode(ASTNode::AST_NUM);
    n_min->type_info = 3;
    n_min->lval64 = -2147483648LL;
    ASTNode *n_max = new ASTNode(ASTNode::AST_NUM);
    n_max->type_info = 3;
    n_max->lval64 = 2147483647LL;
    assert(eval(n_min) == -2147483648LL);
    assert(eval(n_max) == 2147483647LL);
    delete n_min;
    delete n_max;
}
