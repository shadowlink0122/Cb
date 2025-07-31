#pragma once
#include "ast.h"
#include <string>

ASTNode *parse_to_ast(const std::string &filename);
void yyerror(const char *s, const char *error);
extern void yyerror(const char *s);
#ifdef __cplusplus
extern "C" {
#endif
extern char *yyfilename;
extern int yylineno;
#ifdef __cplusplus
}
#endif
