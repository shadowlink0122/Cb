#include "eval.h"
#include <map>
#include <stdio.h>
#include <string>

#ifndef UNIT_TEST_BUILD
extern std::map<std::string, int> vars;
void yyerror(const char *s, const char *error);
#else
#include <map>
#include <string>
std::map<std::string, int> vars;
void yyerror(const char *s, const char *error);
#endif

int eval_num(ASTNode *node) { return node->ival; }
int eval_var(ASTNode *node) { return vars[node->sval]; }

int eval_binop(ASTNode *node) {
    int l = eval(node->lhs);
    int r = eval(node->rhs);
    if (node->op == "+")
        return l + r;
    if (node->op == "-")
        return l - r;
    if (node->op == "*")
        return l * r;
    if (node->op == "/") {
        if (r == 0) {
            yyerror("Error", "0除算が発生しました");
            exit(1);
        }
        return l / r;
    }
    return 0;
}

int eval_assign(ASTNode *node) {
    vars[node->sval] = eval(node->rhs);
    return vars[node->sval];
}

int eval_print(ASTNode *node) {
    printf("%d\n", eval(node->lhs));
    return 0;
}

int eval_stmtlist(ASTNode *node) {
    for (std::vector<ASTNode *>::iterator it = node->stmts.begin();
         it != node->stmts.end(); ++it)
        eval(*it);
    return 0;
}

int eval(ASTNode *node) {
    if (!node)
        return 0;
    switch (node->type) {
    case ASTNode::AST_NUM:
        return eval_num(node);
    case ASTNode::AST_VAR:
        return eval_var(node);
    case ASTNode::AST_BINOP:
        return eval_binop(node);
    case ASTNode::AST_ASSIGN:
        return eval_assign(node);
    case ASTNode::AST_PRINT:
        return eval_print(node);
    case ASTNode::AST_STMTLIST:
        return eval_stmtlist(node);
    }
    return 0;
}
