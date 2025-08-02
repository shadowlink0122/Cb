#pragma once
#include "ast.h"
#include <cstdio>
#include <string>

inline void dump_ast(const ASTNode *node, int indent = 0) {
    if (!node)
        return;
    std::string ind(indent, ' ');
    printf("%s[Type=%d", ind.c_str(), node->type);
    if (!node->sval.empty())
        printf(", sval=%s", node->sval.c_str());
    if (node->type == ASTNode::AST_NUM)
        printf(", lval64=%lld", node->lval64);
    if (node->type_info)
        printf(", type_info=%d", node->type_info);
    printf("]\n");
    if (node->lhs) {
        printf("%s  lhs:\n", ind.c_str());
        dump_ast(node->lhs, indent + 4);
    }
    if (node->rhs) {
        printf("%s  rhs:\n", ind.c_str());
        dump_ast(node->rhs, indent + 4);
    }
    if (!node->stmts.empty()) {
        printf("%s  stmts:\n", ind.c_str());
        for (auto *s : node->stmts)
            dump_ast(s, indent + 4);
    }
    if (!node->params.empty()) {
        printf("%s  params:\n", ind.c_str());
        for (auto *p : node->params)
            dump_ast(p, indent + 4);
    }
    if (!node->rettypes.empty()) {
        printf("%s  rettypes:\n", ind.c_str());
        for (auto *r : node->rettypes)
            dump_ast(r, indent + 4);
    }
    if (node->body) {
        printf("%s  body:\n", ind.c_str());
        dump_ast(node->body, indent + 4);
    }
    if (node->type == ASTNode::AST_FOR) {
        if (node->for_init) {
            printf("%s  for_init:\n", ind.c_str());
            dump_ast(node->for_init, indent + 4);
        }
        if (node->for_cond) {
            printf("%s  for_cond:\n", ind.c_str());
            dump_ast(node->for_cond, indent + 4);
        }
        if (node->for_update) {
            printf("%s  for_update:\n", ind.c_str());
            dump_ast(node->for_update, indent + 4);
        }
        if (node->for_body) {
            printf("%s  for_body:\n", ind.c_str());
            dump_ast(node->for_body, indent + 4);
        }
    }
}
