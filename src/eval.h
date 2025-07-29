#pragma once
#include "ast.h"

int eval(ASTNode *node);
int eval_num(ASTNode *node);
int eval_var(ASTNode *node);
int eval_binop(ASTNode *node);
int eval_assign(ASTNode *node);
int eval_print(ASTNode *node);
int eval_stmtlist(ASTNode *node);
