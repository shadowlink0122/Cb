%{
// C++ヘッダ
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <map>
#include <string>

#include "ast/ast.h"
using namespace std;

void yyerror(const char *s);
// eval.cppのdebug_printを利用
extern void debug_print(const char* fmt, ...);
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
%token VOID TINY SHORT INT LONG
%token PLUS MINUS MUL DIV ASSIGN SEMICOLON PRINT RETURN
%token '{' '}' '(' ')' '[' ']'

%type <ptr> expr term factor statement program funcdef typelist paramlist paramlist_nonempty returnstmt type type_list_items arglist

%%
program:
      /* 空 */ {
        std::vector<ASTNode*>* stmts = new std::vector<ASTNode*>();
        ASTNode* list = new ASTNode(ASTNode::AST_STMTLIST);
        list->stmts = *stmts;
        delete stmts;
        $$ = (void*)list;
        root = list;
      }
    | program statement {
        ASTNode* list = (ASTNode*)$1;
        list->stmts.push_back((ASTNode*)$2);
        $$ = $1;
        root = list;
      }
    | program funcdef {
        ASTNode* list = (ASTNode*)$1;
        list->stmts.push_back((ASTNode*)$2);
        $$ = $1;
        root = list;
      }
    ;
// 関数定義: (型,...) fname(引数,...) { ... } または 型 fname(引数,...) { ... }
funcdef:
      typelist IDENTIFIER '(' paramlist ')' '{' program '}' {
        ASTNode* fn = new ASTNode(ASTNode::AST_FUNCDEF);
        fn->rettypes = ((ASTNode*)$1)->rettypes; delete (ASTNode*)$1;
        fn->sval = std::string($2);
        fn->params = * ((std::vector<ASTNode*>*)$4); delete (std::vector<ASTNode*>*)$4;
        fn->body = (ASTNode*)$7;
        $$ = (void*)fn;
        free($2);
      }
    | type IDENTIFIER '(' paramlist ')' '{' program '}' {
        ASTNode* fn = new ASTNode(ASTNode::AST_FUNCDEF);
        ASTNode* typelist = (ASTNode*)$1;
        debug_print("DEBUG: funcdef %s, typelist=%p, type_info=%d\n", $2, typelist, typelist ? typelist->type_info : -1);
        fn->rettypes.push_back(typelist);
        fn->sval = std::string($2);
        fn->params = * ((std::vector<ASTNode*>*)$4); delete (std::vector<ASTNode*>*)$4;
        fn->body = (ASTNode*)$7;
        $$ = (void*)fn;
        free($2);
      }

// 型リスト: (型,型,...)
typelist:
      '(' type_list_items ')' {
        ASTNode* tlist = new ASTNode(ASTNode::AST_TYPELIST);
        tlist->rettypes = ((ASTNode*)$2)->rettypes; delete (ASTNode*)$2;
        $$ = (void*)tlist;
      }
    | type { $$ = $1; }
    ;

type_list_items:
      type { 
        ASTNode* tlist = new ASTNode(ASTNode::AST_TYPELIST);
        tlist->type_info = ((ASTNode*)$1)->type_info;
        tlist->rettypes.push_back((ASTNode*)$1);
        $$ = (void*)tlist;
      }
    | type_list_items ',' type {
        ASTNode* tlist = (ASTNode*)$1;
        ASTNode* t = (ASTNode*)$3;
        tlist->rettypes.push_back(t);
        $$ = (void*)tlist;
      }
    ;


type:
      VOID   { $$ = (void*)(new ASTNode(ASTNode::AST_TYPELIST)); ((ASTNode*)$$)->type_info = 0; }
    | TINY   { $$ = (void*)(new ASTNode(ASTNode::AST_TYPELIST)); ((ASTNode*)$$)->type_info = 1; }
    | SHORT  { $$ = (void*)(new ASTNode(ASTNode::AST_TYPELIST)); ((ASTNode*)$$)->type_info = 2; }
    | INT    { $$ = (void*)(new ASTNode(ASTNode::AST_TYPELIST)); ((ASTNode*)$$)->type_info = 3; }
    | LONG   { $$ = (void*)(new ASTNode(ASTNode::AST_TYPELIST)); ((ASTNode*)$$)->type_info = 4; }
    ;

// return文
returnstmt:
      RETURN expr SEMICOLON {
        ASTNode* ret = new ASTNode(ASTNode::AST_RETURN);
        ret->lhs = (ASTNode*)$2;
        $$ = (void*)ret;
      }
    | RETURN SEMICOLON {
        ASTNode* ret = new ASTNode(ASTNode::AST_RETURN);
        ret->lhs = nullptr;
        $$ = (void*)ret;
      }
    ;

// 文（ステートメント）
statement:
    type IDENTIFIER ASSIGN expr SEMICOLON {
        ASTNode* assign = new ASTNode(ASTNode::AST_ASSIGN);
        assign->sval = std::string($2);
        assign->rhs = (ASTNode*)$4;
        assign->type_info = ((ASTNode*)$1)->type_info;
        $$ = (void*)assign;
        delete (ASTNode*)$1;
        free($2);
    }
    | IDENTIFIER ASSIGN expr SEMICOLON {
        ASTNode* assign = new ASTNode(ASTNode::AST_ASSIGN);
        assign->sval = std::string($1);
        assign->rhs = (ASTNode*)$3;
        assign->type_info = ((ASTNode*)$3)->type_info;
        $$ = (void*)assign;
        free($1);
      }
    | PRINT expr SEMICOLON { $$ = (void*)(new ASTNode(ASTNode::AST_PRINT)); ((ASTNode*)$$)->lhs = (ASTNode*)$2; }
    | expr SEMICOLON { $$ = $1; }
    | returnstmt { $$ = $1; }
    ;

// 引数リスト: (型 ident, ...)
paramlist:
      /* 空 */ { $$ = (void*)(new std::vector<ASTNode*>()); }
    | paramlist_nonempty { $$ = $1; }
    ;

paramlist_nonempty:
      type IDENTIFIER {
        std::vector<ASTNode*>* params = new std::vector<ASTNode*>();
        ASTNode* param = new ASTNode(ASTNode::AST_FUNCPARAM);
        param->type_info = ((ASTNode*)$1)->type_info;
        param->sval = std::string($2);
        params->push_back(param);
        delete (ASTNode*)$1;
        free($2);
        $$ = (void*)params;
      }
    | paramlist_nonempty ',' type IDENTIFIER {
        std::vector<ASTNode*>* params = (std::vector<ASTNode*>*)$1;
        ASTNode* param = new ASTNode(ASTNode::AST_FUNCPARAM);
        param->type_info = ((ASTNode*)$3)->type_info;
        param->sval = std::string($4);
        params->push_back(param);
        delete (ASTNode*)$3;
        free($4);
        $$ = (void*)params;
      }
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
        num->type_info = ((ASTNode*)$1)->type_info;
        delete (ASTNode*)$1;
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
    | IDENTIFIER '(' arglist ')' {
        ASTNode* call = new ASTNode(ASTNode::AST_FUNCCALL);
        call->sval = std::string($1);
        if ($3 != nullptr) {
          call->params = std::move(*((std::vector<ASTNode*>*)$3));
          delete (std::vector<ASTNode*>*)$3;
        } else {
          call->params = std::vector<ASTNode*>();
        }
        free($1);
        $$ = (void*)call;
      }
    | IDENTIFIER '(' ')' {
        ASTNode* call = new ASTNode(ASTNode::AST_FUNCCALL);
        call->sval = std::string($1);
        call->params = std::vector<ASTNode*>(); // 空vectorで初期化
        $$ = (void*)call;
        free($1);
      }
    | IDENTIFIER { $$ = (void*)(new ASTNode(ASTNode::AST_VAR)); ((ASTNode*)$$)->sval = std::string($1); free($1); }
    ;

// 関数呼び出しの引数リスト: expr, expr, ...
arglist:
      expr { std::vector<ASTNode*>* v = new std::vector<ASTNode*>(); v->push_back((ASTNode*)$1); $$ = (void*)v; }
    | arglist ',' expr { std::vector<ASTNode*>* v = (std::vector<ASTNode*>*)$1; v->push_back((ASTNode*)$3); $$ = (void*)v; }
    ;
%%
