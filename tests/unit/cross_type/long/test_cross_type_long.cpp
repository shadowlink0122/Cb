#include <cassert>
#include <iostream>

#include "src/ast/ast.h"
#include "src/eval/eval.h"

extern "C" void test_unit_cross_type_long() {
    // long→int（範囲内のみ）
    ASTNode *n = new ASTNode(ASTNode::AST_NUM);
    n->type_info = 4; // long
    n->lval64 = 77;
    ASTNode *as_int = new ASTNode(ASTNode::AST_NUM);
    as_int->type_info = 3; // int
    as_int->lval64 = n->lval64;
    assert(eval(n) == 77);
    assert(eval(as_int) == 77);
    delete n;
    delete as_int;

    // long→short（範囲内のみ）
    ASTNode *n2 = new ASTNode(ASTNode::AST_NUM);
    n2->type_info = 4;
    n2->lval64 = -100;
    ASTNode *as_short = new ASTNode(ASTNode::AST_NUM);
    as_short->type_info = 2; // short
    as_short->lval64 = n2->lval64;
    assert(eval(n2) == -100);
    assert(eval(as_short) == -100);
    delete n2;
    delete as_short;

    // long→tiny（範囲内のみ）
    ASTNode *n3 = new ASTNode(ASTNode::AST_NUM);
    n3->type_info = 4;
    n3->lval64 = 42;
    ASTNode *as_tiny = new ASTNode(ASTNode::AST_NUM);
    as_tiny->type_info = 1; // tiny
    as_tiny->lval64 = n3->lval64;
    assert(eval(n3) == 42);
    assert(eval(as_tiny) == 42);
    delete n3;
    delete as_tiny;
}
