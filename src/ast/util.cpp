#include "util.h"
#include "ast.h"
#include <cstdio>
#include <stdio.h>
#include <string>

char *yyfilename = NULL;

extern int yyparse();
extern FILE *yyin;
extern ASTNode *root;
extern int yylineno;
extern char *yyfilename;

ASTNode *parse_to_ast(const std::string &filename) {
    yyin = fopen(filename.c_str(), "r");
    yyfilename = (char *)filename.c_str();
    if (!yyin) {
        perror(filename.c_str());
        return nullptr;
    }
    yyparse();
    fclose(yyin);
    ASTNode *result = root;
    root = nullptr; // 呼び出し側でdeleteすること
    return result;
}

void yyerror(const char *s, const char *error) {
    fprintf(stderr, "%s: %s\n", s, error);
    if (yyfilename) {
        FILE *fp = fopen(yyfilename, "r");
        if (fp) {
            char buf[1024];
            int line = 1;
            while (fgets(buf, sizeof(buf), fp)) {
                if (line == yylineno) {
                    fprintf(stderr, "%s:%d\n>> %s", yyfilename, line, buf);
                    break;
                }
                line++;
            }
            fclose(fp);
        }
    }
    exit(1);
}

void yyerror(const char *s) { yyerror("構文エラー", s); }
