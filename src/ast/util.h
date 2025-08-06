#pragma once
#include "ast.h"
#include <string>

ASTNode *parse_to_ast(const std::string &filename);
#ifdef __cplusplus
extern "C" {
#endif
void yyerror(const char *s, const char *error);
#ifdef __cplusplus
}
static inline void yyerror(const char *s) { yyerror(s, ""); }
#endif
#ifdef __cplusplus
extern "C" {
#endif
extern char *yyfilename;
extern int yylineno;
#ifdef __cplusplus
}
#endif
