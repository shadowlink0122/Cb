#include "ast/util.h"
#include "eval/eval.h"
#include "parser.h"
#include <map>
#include <stdio.h>
#include <string>

extern int yyparse();
extern FILE *yyin;
extern ASTNode *root;
extern int yylineno;

int main(int argc, char **argv) {
    if (argc > 1) {
        ASTNode *ast = parse_to_ast(argv[1]);
        if (!ast)
            return 1;
        eval(ast);
        delete ast;
        return 0;
    } else {
        fprintf(stderr, "Usage: %s <input.cb>\n", argv[0]);
        return 1;
    }
}
