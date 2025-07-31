#pragma once
#include "../ast/ast.h"
#include <map>
#include <string>

// テスト用バイナリでもリンクできるようにエラー関数extern宣言
extern void yyerror(const char *s, const char *error);

// 関数定義テーブル（main.cpp等から参照する場合用）
extern std::map<std::string, ASTNode *> function_table;

int64_t eval(ASTNode *node);
int64_t eval_num(ASTNode *node);
int64_t eval_var(ASTNode *node);
int64_t eval_binop(ASTNode *node);
int64_t eval_assign(ASTNode *node);
int eval_print(ASTNode *node);
int eval_stmtlist(ASTNode *node);
