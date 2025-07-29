%{
// C++ヘッダ
// C++ヘッダ
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <map>
#include <string>

#include "ast.h"
using namespace std;

void yyerror(const char *s);
int yylex();

ASTNode* root = nullptr;
%}

%union {
    int ival;
    char* sval;
    void* ptr;
}

%token <ival> NUMBER
%token <sval> IDENTIFIER
%token PLUS MINUS MUL DIV ASSIGN SEMICOLON PRINT

%type <ptr> expr term factor statement program

%%
program:
      /* 空 */ { $$ = (void*)(new std::vector<ASTNode*>()); root = new ASTNode(ASTNode::AST_STMTLIST); root->stmts = *((std::vector<ASTNode*>*)$$); }
    | program statement { ((std::vector<ASTNode*>*)$1)->push_back((ASTNode*)$2); root = new ASTNode(ASTNode::AST_STMTLIST); root->stmts = *((std::vector<ASTNode*>*)$1); $$ = $1; }
    ;

statement:
      IDENTIFIER ASSIGN expr SEMICOLON { $$ = (void*)(new ASTNode(ASTNode::AST_ASSIGN)); ((ASTNode*)$$)->sval = std::string($1); ((ASTNode*)$$)->rhs = (ASTNode*)$3; free($1); }
    | PRINT expr SEMICOLON { $$ = (void*)(new ASTNode(ASTNode::AST_PRINT)); ((ASTNode*)$$)->lhs = (ASTNode*)$2; }
    | expr SEMICOLON { $$ = $1; }
    ;

expr:
      expr PLUS term { $$ = (void*)(new ASTNode(ASTNode::AST_BINOP)); ((ASTNode*)$$)->op = "+"; ((ASTNode*)$$)->lhs = (ASTNode*)$1; ((ASTNode*)$$)->rhs = (ASTNode*)$3; }
    | expr MINUS term { $$ = (void*)(new ASTNode(ASTNode::AST_BINOP)); ((ASTNode*)$$)->op = "-"; ((ASTNode*)$$)->lhs = (ASTNode*)$1; ((ASTNode*)$$)->rhs = (ASTNode*)$3; }
    | term { $$ = $1; }
    ;

term:
      term MUL factor { $$ = (void*)(new ASTNode(ASTNode::AST_BINOP)); ((ASTNode*)$$)->op = "*"; ((ASTNode*)$$)->lhs = (ASTNode*)$1; ((ASTNode*)$$)->rhs = (ASTNode*)$3; }
    | term DIV factor { $$ = (void*)(new ASTNode(ASTNode::AST_BINOP)); ((ASTNode*)$$)->op = "/"; ((ASTNode*)$$)->lhs = (ASTNode*)$1; ((ASTNode*)$$)->rhs = (ASTNode*)$3; }
    | factor { $$ = $1; }
    ;

factor:
      NUMBER { $$ = (void*)(new ASTNode(ASTNode::AST_NUM)); ((ASTNode*)$$)->ival = $1; }
    | MINUS factor { $$ = (void*)(new ASTNode(ASTNode::AST_NUM)); ((ASTNode*)$$)->ival = -((ASTNode*)$2)->ival; delete (ASTNode*)$2; }
    | IDENTIFIER { $$ = (void*)(new ASTNode(ASTNode::AST_VAR)); ((ASTNode*)$$)->sval = std::string($1); free($1); }
    ;
%%

void yyerror(const char *s) {
    fprintf(stderr, "Error: %s\n", s);
}
