#include <cassert>
#include <iostream>

#include "src/ast/ast.h"
#include "src/eval/eval.h"

extern "C" void test_unit_cross_type_tiny() {
    // tiny→short
    ASTNode *n = new ASTNode(ASTNode::AST_NUM);
    n->type_info = 1; // tiny
    n->lval64 = 42;
    ASTNode *as_short = new ASTNode(ASTNode::AST_NUM);
    as_short->type_info = 2; // short
    as_short->lval64 = n->lval64;
    assert(eval(n) == 42);
    assert(eval(as_short) == 42);
    delete n;
    delete as_short;

    // tiny→int
    ASTNode *n2 = new ASTNode(ASTNode::AST_NUM);
    n2->type_info = 1;
    n2->lval64 = -10;
    ASTNode *as_int = new ASTNode(ASTNode::AST_NUM);
    as_int->type_info = 3; // int
    as_int->lval64 = n2->lval64;
    assert(eval(n2) == -10);
    assert(eval(as_int) == -10);
    delete n2;
    delete as_int;

    // tiny→long
    ASTNode *n3 = new ASTNode(ASTNode::AST_NUM);
    n3->type_info = 1;
    n3->lval64 = 100;
    ASTNode *as_long = new ASTNode(ASTNode::AST_NUM);
    as_long->type_info = 4; // long
    as_long->lval64 = n3->lval64;
    assert(eval(n3) == 100);
    assert(eval(as_long) == 100);
    delete n3;
    delete as_long;
}
