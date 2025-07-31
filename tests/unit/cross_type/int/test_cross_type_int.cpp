#include <cassert>
#include <iostream>

#include "src/ast/ast.h"
#include "src/eval/eval.h"

extern "C" void test_unit_cross_type_int() {
    // int→long
    ASTNode *n = new ASTNode(ASTNode::AST_NUM);
    n->type_info = 3; // int
    n->lval64 = 123;
    ASTNode *as_long = new ASTNode(ASTNode::AST_NUM);
    as_long->type_info = 4; // long
    as_long->lval64 = n->lval64;
    assert(eval(n) == 123);
    assert(eval(as_long) == 123);
    delete n;
    delete as_long;

    // int→short（範囲内のみ）
    ASTNode *n2 = new ASTNode(ASTNode::AST_NUM);
    n2->type_info = 3;
    n2->lval64 = -100;
    ASTNode *as_short = new ASTNode(ASTNode::AST_NUM);
    as_short->type_info = 2; // short
    as_short->lval64 = n2->lval64;
    assert(eval(n2) == -100);
    assert(eval(as_short) == -100);
    delete n2;
    delete as_short;

    // int→tiny（範囲内のみ）
    ASTNode *n3 = new ASTNode(ASTNode::AST_NUM);
    n3->type_info = 3;
    n3->lval64 = 42;
    ASTNode *as_tiny = new ASTNode(ASTNode::AST_NUM);
    as_tiny->type_info = 1; // tiny
    as_tiny->lval64 = n3->lval64;
    assert(eval(n3) == 42);
    assert(eval(as_tiny) == 42);
    delete n3;
    delete as_tiny;
}
