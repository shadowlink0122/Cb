#pragma once
#include "ast.h"
#include <string>

ASTNode *parse_to_ast(const std::string &filename);
void yyerror(const char *s, const char *error);
void yyerror(const char *s);
extern char *yyfilename;
