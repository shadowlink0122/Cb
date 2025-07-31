#include <cassert>
#include <iostream>

#include "src/ast/ast.h"
#include "src/eval/eval.h"

extern "C" void test_unit_boundary_long() {
    // long型の境界値をAST+evalで
    ASTNode *n_min = new ASTNode(ASTNode::AST_NUM);
    n_min->type_info = 4;
    n_min->lval64 = -9223372036854775807LL - 1;
    ASTNode *n_max = new ASTNode(ASTNode::AST_NUM);
    n_max->type_info = 4;
    n_max->lval64 = 9223372036854775807LL;
    assert(eval(n_min) == -9223372036854775807LL - 1);
    assert(eval(n_max) == 9223372036854775807LL);
    delete n_min;
    delete n_max;
}
