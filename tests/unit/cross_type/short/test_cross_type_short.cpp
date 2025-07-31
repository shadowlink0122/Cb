#include <cassert>
#include <iostream>

#include "src/ast/ast.h"
#include "src/eval/eval.h"

extern "C" void test_unit_cross_type_short() {
    // short→int
    ASTNode *n = new ASTNode(ASTNode::AST_NUM);
    n->type_info = 2; // short
    n->lval64 = -100;
    ASTNode *as_int = new ASTNode(ASTNode::AST_NUM);
    as_int->type_info = 3; // int
    as_int->lval64 = n->lval64;
    assert(eval(n) == -100);
    assert(eval(as_int) == -100);
    delete n;
    delete as_int;

    // short→long
    ASTNode *n2 = new ASTNode(ASTNode::AST_NUM);
    n2->type_info = 2;
    n2->lval64 = 1234;
    ASTNode *as_long = new ASTNode(ASTNode::AST_NUM);
    as_long->type_info = 4; // long
    as_long->lval64 = n2->lval64;
    assert(eval(n2) == 1234);
    assert(eval(as_long) == 1234);
    delete n2;
    delete as_long;

    // short→tiny（範囲内のみ）
    ASTNode *n3 = new ASTNode(ASTNode::AST_NUM);
    n3->type_info = 2;
    n3->lval64 = 42;
    ASTNode *as_tiny = new ASTNode(ASTNode::AST_NUM);
    as_tiny->type_info = 1; // tiny
    as_tiny->lval64 = n3->lval64;
    assert(eval(n3) == 42);
    assert(eval(as_tiny) == 42);
    delete n3;
    delete as_tiny;
}
