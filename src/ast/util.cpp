#include "util.h"
#include "ast.h"
#include <cstdio>
#include <stdio.h>
#include <string>

extern int yyparse();
extern FILE *yyin;
extern ASTNode *root;
extern int yylineno;
extern char *yyfilename;

ASTNode *parse_to_ast(const std::string &filename) {
    root = nullptr; // パース前に必ず初期化
    yyin = fopen(filename.c_str(), "r");
    yyfilename = (char *)filename.c_str();
    if (!yyin) {
        perror(filename.c_str());
        return nullptr;
    }
    int parse_result = yyparse();
    fclose(yyin);
    if (parse_result != 0 || root == nullptr) {
        root = nullptr;
        return nullptr;
    }
    ASTNode *result = root;
    root = nullptr; // 呼び出し側でdeleteすること
    return result;
}
