#include "ast.h"
#include "eval.h"
#include "parser.h"
#include <map>
#include <stdio.h>
#include <string>
extern int yyparse();
extern FILE *yyin;
extern ASTNode *root;

std::map<std::string, int> vars;

int main(int argc, char **argv) {
    if (argc > 1) {
        yyin = fopen(argv[1], "r");
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
