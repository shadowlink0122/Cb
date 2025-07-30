#pragma once
#include "ast.h"
// テスト用バイナリでもリンクできるようにエラー関数extern宣言
extern void yyerror(const char *s, const char *error);

int64_t eval(ASTNode *node);
int64_t eval_num(ASTNode *node);
int64_t eval_var(ASTNode *node);
int64_t eval_binop(ASTNode *node);
int64_t eval_assign(ASTNode *node);
int eval_print(ASTNode *node);
int eval_stmtlist(ASTNode *node);
