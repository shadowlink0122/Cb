#include "ast.h"
#include "eval.h"
#include "parser.h"
#include <map>
#include <stdio.h>
#include <string>

extern int yyparse();
extern FILE *yyin;
extern ASTNode *root;
extern int yylineno;
char *yyfilename = NULL;

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
}

// Bison用のシグネチャ
void yyerror(const char *s) { yyerror("構文エラー", s); }

std::map<std::string, int> vars;

int main(int argc, char **argv) {
    if (argc > 1) {
        yyin = fopen(argv[1], "r");
        yyfilename = argv[1];
        if (!yyin) {
            perror(argv[1]);
            return 1;
        }
    }
    yyparse();
    eval(root);
    if (argc > 1)
        fclose(yyin);
    delete root;
    return 0;
}
