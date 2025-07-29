#include "eval.h"
#include <cassert>
#include <iostream>

int main() {
    // 1 + 2 = 3
    ASTNode *n1 = new ASTNode(ASTNode::AST_NUM);
    n1->ival = 1;
    ASTNode *n2 = new ASTNode(ASTNode::AST_NUM);
    n2->ival = 2;
    ASTNode *add = new ASTNode(ASTNode::AST_BINOP);
    add->op = "+";
    add->lhs = n1;
    add->rhs = n2;
    assert(eval(add) == 3);
    delete add;

    // a = 5; print a;
    ASTNode *a = new ASTNode(ASTNode::AST_VAR);
    a->sval = "a";
    ASTNode *n5 = new ASTNode(ASTNode::AST_NUM);
    n5->ival = 5;
    ASTNode *assign = new ASTNode(ASTNode::AST_ASSIGN);
    assign->sval = "a";
    assign->rhs = n5;
    eval(assign);
    ASTNode *var = new ASTNode(ASTNode::AST_VAR);
    var->sval = "a";
    assert(eval(var) == 5);
    delete assign;
    delete var;

    std::cout << "All eval tests passed!\n";
    return 0;
}
