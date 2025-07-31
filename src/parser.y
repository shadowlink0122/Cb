%{
// C++ヘッダ
// C++ヘッダ
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <map>
#include <string>

#include "ast/ast.h"
using namespace std;

void yyerror(const char *s);
int yylex();

ASTNode* root = nullptr;
%}

%union {
    int ival;
    long long lval;
    char* sval;
    void* ptr;
}

%token <lval> NUMBER
%token <sval> IDENTIFIER
%token TINY SHORT INT LONG
%token PLUS MINUS MUL DIV ASSIGN SEMICOLON PRINT

%type <ptr> expr term factor statement program
%type <ival> type

%%
program:
      /* 空 */ { $$ = (void*)(new std::vector<ASTNode*>()); root = new ASTNode(ASTNode::AST_STMTLIST); root->stmts = *((std::vector<ASTNode*>*)$$); }
    | program statement { ((std::vector<ASTNode*>*)$1)->push_back((ASTNode*)$2); root = new ASTNode(ASTNode::AST_STMTLIST); root->stmts = *((std::vector<ASTNode*>*)$1); $$ = $1; }
    ;

statement:
      type IDENTIFIER ASSIGN expr SEMICOLON { 
        ASTNode* assign = new ASTNode(ASTNode::AST_ASSIGN);
        assign->sval = std::string($2);
        assign->rhs = (ASTNode*)$4;
        assign->type_info = $1; /* 型情報: 1=tiny, 2=short, 3=int, 4=long */
        $$ = (void*)assign;
        free($2);
      }
    | IDENTIFIER ASSIGN expr SEMICOLON {
        yyerror("型宣言されていない変数への代入です");
        $$ = nullptr;
        free($1);
      }
    | PRINT expr SEMICOLON { $$ = (void*)(new ASTNode(ASTNode::AST_PRINT)); ((ASTNode*)$$)->lhs = (ASTNode*)$2; }
    | expr SEMICOLON { $$ = $1; }

type:
      TINY  { $$ = 1; }
    | SHORT { $$ = 2; }
    | INT   { $$ = 3; }
    | LONG  { $$ = 4; }
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

// 型情報を持つリテラルをサポート
factor:
      type NUMBER {
        ASTNode* num = new ASTNode(ASTNode::AST_NUM);
        num->lval64 = $2;
        num->type_info = $1;
        $$ = (void*)num;
      }
    | MINUS factor {
        ASTNode* num = new ASTNode(ASTNode::AST_NUM);
        ASTNode* rhs = (ASTNode*)$2;
        num->lval64 = -(rhs->lval64);
        num->type_info = rhs->type_info;
        delete rhs;
        $$ = (void*)num;
      }
    | NUMBER {
        ASTNode* num = new ASTNode(ASTNode::AST_NUM);
        num->lval64 = $1;
        num->type_info = 3; // デフォルトint型
        $$ = (void*)num;
      }
    | IDENTIFIER { $$ = (void*)(new ASTNode(ASTNode::AST_VAR)); ((ASTNode*)$$)->sval = std::string($1); free($1); }
    ;
%%
