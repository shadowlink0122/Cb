%{
#include <vector>
#include "ast/ast.h"

// C++ヘッダ
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <map>
#include <string>

#include "ast/util.h"
using namespace std;

extern void yyerror(const char *s);
extern void yyerror(const char *s, const char *error);
// eval.cppのdebug_printfを利用
extern void debug_printf(const char* fmt, ...);
int yylex();
ASTNode* root = nullptr;
extern "C" {
    char *yyfilename = NULL;
}
%}

%union {
    int ival;
    long long lval;
    char* sval;
    void* ptr;
}

%token <lval> NUMBER
%token <sval> IDENTIFIER STRING STRING_LITERAL
%token VOID TINY SHORT INT LONG BOOL
%token TRUE FALSE NULL_LIT
%token PLUS MINUS MUL DIV ASSIGN SEMICOLON PRINT RETURN
%token FOR WHILE BREAK
%token IF ELSE
%token EQ NEQ GE LE GT LT OR AND NOT MOD
%token ADD_ASSIGN SUB_ASSIGN MUL_ASSIGN DIV_ASSIGN MOD_ASSIGN
%token INC_OP DEC_OP
%token '{' '}' '(' ')' '[' ']' ','

%type <ptr> expr term factor statement program funcdef typelist paramlist paramlist_nonempty returnstmt type type_list_items arglist opt_statement opt_expr init_statement opt_update
%type <ptr> if_stmt compound_assign array_literal array_init_list

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
        debug_printf("DEBUG: funcdef %s, typelist=%p, type_info=%d\n", $2, typelist, typelist ? typelist->type_info : -1);
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
      VOID   { ASTNode* n = new ASTNode(ASTNode::AST_TYPELIST); n->type_info = 0; $$ = (void*)n; }
    | TINY   { ASTNode* n = new ASTNode(ASTNode::AST_TYPELIST); n->type_info = 1; $$ = (void*)n; }
    | SHORT  { ASTNode* n = new ASTNode(ASTNode::AST_TYPELIST); n->type_info = 2; $$ = (void*)n; }
    | INT    { ASTNode* n = new ASTNode(ASTNode::AST_TYPELIST); n->type_info = 3; $$ = (void*)n; }
    | LONG   { ASTNode* n = new ASTNode(ASTNode::AST_TYPELIST); n->type_info = 4; $$ = (void*)n; }
    | STRING { ASTNode* n = new ASTNode(ASTNode::AST_TYPELIST); n->type_info = 5; $$ = (void*)n; }
    | BOOL   { ASTNode* n = new ASTNode(ASTNode::AST_TYPELIST); n->type_info = 6; $$ = (void*)n; }
    ;

// return文
returnstmt:
      RETURN expr SEMICOLON {
        ASTNode* ret = new ASTNode(ASTNode::AST_RETURN);
        ret->lhs = (ASTNode*)$2;
        // 戻り値型チェック: 関数定義ノードのrettypes[0]->type_infoと比較
        extern ASTNode* root;
        ASTNode* func = nullptr;
        // スタックの一番上の関数定義ノードを探索
        for (auto it = root->stmts.rbegin(); it != root->stmts.rend(); ++it) {
          if (*it && ((ASTNode*)*it)->type == ASTNode::AST_FUNCDEF) {
            func = (ASTNode*)*it;
            break;
          }
        }
        debug_printf("DEBUG: returnstmt expected=%d, actual=%d\n", func && !func->rettypes.empty() && func->rettypes[0] ? func->rettypes[0]->type_info : -1, ((ASTNode*)$2)->type_info);
        if (func && !func->rettypes.empty() && func->rettypes[0]) {
          int expected = func->rettypes[0]->type_info;
          int actual = ((ASTNode*)$2)->type_info;
          if (expected != actual) {
            // string型(5)同士はOK
            if (!(expected == 5 && actual == 5)) {
              yyerror("return文の型が関数の戻り値型と一致しません", "");
            }
          }
        }
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
    if_stmt { $$ = $1; }
    | type IDENTIFIER ASSIGN expr SEMICOLON {
        ASTNode* assign = new ASTNode(ASTNode::AST_ASSIGN);
        assign->sval = std::string($2);
        assign->rhs = (ASTNode*)$4;
        assign->type_info = ((ASTNode*)$1)->type_info;
        $$ = (void*)assign;
        delete (ASTNode*)$1;
        free($2);
    }
    // 配列宣言: int a[5];
    | type IDENTIFIER '[' NUMBER ']' SEMICOLON {
        ASTNode* arr = new ASTNode(ASTNode::AST_ARRAY_DECL);
        arr->sval = std::string($2);
        arr->array_size = $4;
        arr->elem_type_info = ((ASTNode*)$1)->type_info;
        $$ = (void*)arr;
        delete (ASTNode*)$1;
        free($2);
    }
    // 配列宣言＋初期化: int a[] = [1,2,3];
    | type IDENTIFIER '[' ']' ASSIGN array_literal SEMICOLON {
        ASTNode* arr = new ASTNode(ASTNode::AST_ARRAY_DECL);
        arr->sval = std::string($2);
        arr->array_size = ((ASTNode*)$6)->elements.size();
        arr->elem_type_info = ((ASTNode*)$1)->type_info;
        arr->elements = ((ASTNode*)$6)->elements;
        $$ = (void*)arr;
        delete (ASTNode*)$1;
        delete (ASTNode*)$6;
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
    | IDENTIFIER '[' expr ']' ASSIGN expr SEMICOLON {
        ASTNode* assign = new ASTNode(ASTNode::AST_ASSIGN);
        assign->sval = ""; // 通常変数代入以外は空
        assign->lhs = new ASTNode(ASTNode::AST_ARRAY_REF);
        assign->lhs->sval = std::string($1);
        assign->lhs->array_index = (ASTNode*)$3;
        assign->rhs = (ASTNode*)$6;
        assign->type_info = ((ASTNode*)$6)->type_info;
        $$ = (void*)assign;
        free($1);
      }
    | PRINT expr SEMICOLON { $$ = (void*)(new ASTNode(ASTNode::AST_PRINT)); ((ASTNode*)$$)->lhs = (ASTNode*)$2; }
    | expr SEMICOLON { $$ = $1; }
    | returnstmt { $$ = $1; }
    | BREAK SEMICOLON {
        ASTNode* br = new ASTNode(ASTNode::AST_BREAK);
        br->lhs = nullptr;
        $$ = (void*)br;
      }
    | BREAK expr SEMICOLON {
        ASTNode* br = new ASTNode(ASTNode::AST_BREAK);
        br->lhs = (ASTNode*)$2;
        $$ = (void*)br;
      }
    | FOR '(' init_statement SEMICOLON opt_expr SEMICOLON opt_update ')' '{' program '}' {
        ASTNode* forNode = new ASTNode(ASTNode::AST_FOR);
        forNode->for_init = (ASTNode*)$3;
        forNode->for_cond = (ASTNode*)$5;
        forNode->for_update = (ASTNode*)$7;
        forNode->for_body = (ASTNode*)$10;
        $$ = (void*)forNode;
      }
    | WHILE '(' expr ')' '{' program '}' {
        ASTNode* whileNode = new ASTNode(ASTNode::AST_WHILE);
        whileNode->for_cond = (ASTNode*)$3;
        whileNode->for_body = (ASTNode*)$6;
        $$ = (void*)whileNode;
      }
    | '{' program '}' { $$ = $2; }
    ;

// if文
if_stmt:
    IF '(' expr ')' statement {
        ASTNode* ifnode = new ASTNode(ASTNode::AST_IF);
        ifnode->if_cond = (ASTNode*)$3;
        ifnode->if_then = (ASTNode*)$5;
        ifnode->if_else = nullptr;
        $$ = (void*)ifnode;
    }
    | IF '(' expr ')' '{' program '}' {
        ASTNode* ifnode = new ASTNode(ASTNode::AST_IF);
        ifnode->if_cond = (ASTNode*)$3;
        ifnode->if_then = (ASTNode*)$6;
        ifnode->if_else = nullptr;
        $$ = (void*)ifnode;
    }
    | IF '(' expr ')' statement ELSE statement {
        ASTNode* ifnode = new ASTNode(ASTNode::AST_IF);
        ifnode->if_cond = (ASTNode*)$3;
        ifnode->if_then = (ASTNode*)$5;
        ifnode->if_else = (ASTNode*)$7;
        $$ = (void*)ifnode;
    }
    | IF '(' expr ')' statement ELSE if_stmt {
        ASTNode* ifnode = new ASTNode(ASTNode::AST_IF);
        ifnode->if_cond = (ASTNode*)$3;
        ifnode->if_then = (ASTNode*)$5;
        ifnode->if_else = (ASTNode*)$7;
        $$ = (void*)ifnode;
    }
    | IF '(' expr ')' '{' program '}' ELSE statement {
        ASTNode* ifnode = new ASTNode(ASTNode::AST_IF);
        ifnode->if_cond = (ASTNode*)$3;
        ifnode->if_then = (ASTNode*)$6;
        ifnode->if_else = (ASTNode*)$9;
        $$ = (void*)ifnode;
    }
    | IF '(' expr ')' '{' program '}' ELSE '{' program '}' {
        ASTNode* ifnode = new ASTNode(ASTNode::AST_IF);
        ifnode->if_cond = (ASTNode*)$3;
        ifnode->if_then = (ASTNode*)$6;
        ifnode->if_else = (ASTNode*)$10;
        $$ = (void*)ifnode;
    }
    | IF '(' expr ')' '{' program '}' ELSE if_stmt {
        ASTNode* ifnode = new ASTNode(ASTNode::AST_IF);
        ifnode->if_cond = (ASTNode*)$3;
        ifnode->if_then = (ASTNode*)$6;
        ifnode->if_else = (ASTNode*)$9;
        $$ = (void*)ifnode;
    }
  ;

opt_update:
      expr { $$ = $1; }
    | /* 空 */ { $$ = nullptr; }
    ;

init_statement:
      type IDENTIFIER ASSIGN expr { 
        ASTNode* assign = new ASTNode(ASTNode::AST_ASSIGN);
        assign->sval = std::string($2);
        assign->rhs = (ASTNode*)$4;
        assign->type_info = ((ASTNode*)$1)->type_info;
        $$ = (void*)assign;
        delete (ASTNode*)$1;
        free($2);
      }
    | IDENTIFIER ASSIGN expr {
        ASTNode* assign = new ASTNode(ASTNode::AST_ASSIGN);
        assign->sval = std::string($1);
        assign->rhs = (ASTNode*)$3;
        assign->type_info = ((ASTNode*)$3)->type_info;
        $$ = (void*)assign;
        free($1);
      }
    | compound_assign { $$ = $1; }
    | /* 空 */ { $$ = nullptr; }
    ;

opt_statement:
      statement { $$ = $1; }
    | expr { $$ = $1; }
    | /* 空 */ { $$ = nullptr; }
    ;

opt_expr:
      expr { $$ = $1; }
    | compound_assign { $$ = $1; }
    | /* 空 */ { $$ = nullptr; }
    ;
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
      IDENTIFIER ASSIGN expr {
          ASTNode* assign = new ASTNode(ASTNode::AST_ASSIGN);
          assign->sval = std::string($1);
          assign->rhs = (ASTNode*)$3;
          assign->type_info = ((ASTNode*)$3)->type_info;
          $$ = (void*)assign;
          free($1);
      }
    | compound_assign { $$ = $1; }
    | expr EQ term {
        ASTNode* node = new ASTNode(ASTNode::AST_BINOP);
        node->op = "==";
        node->lhs = (ASTNode*)$1;
        node->rhs = (ASTNode*)$3;
        node->type_info = 6; // bool
        $$ = (void*)node;
      }
    | expr NEQ term {
        ASTNode* node = new ASTNode(ASTNode::AST_BINOP);
        node->op = "!=";
        node->lhs = (ASTNode*)$1;
        node->rhs = (ASTNode*)$3;
        node->type_info = 6; // bool
        $$ = (void*)node;
      }
    | expr GT term {
        ASTNode* node = new ASTNode(ASTNode::AST_BINOP);
        node->op = ">";
        node->lhs = (ASTNode*)$1;
        node->rhs = (ASTNode*)$3;
        node->type_info = 6; // bool
        $$ = (void*)node;
      }
    | expr LT term {
        ASTNode* node = new ASTNode(ASTNode::AST_BINOP);
        node->op = "<";
        node->lhs = (ASTNode*)$1;
        node->rhs = (ASTNode*)$3;
        node->type_info = 6; // bool
        $$ = (void*)node;
      }
    | expr GE term {
        ASTNode* node = new ASTNode(ASTNode::AST_BINOP);
        node->op = ">=";
        node->lhs = (ASTNode*)$1;
        node->rhs = (ASTNode*)$3;
        node->type_info = 6; // bool
        $$ = (void*)node;
      }
    | expr LE term {
        ASTNode* node = new ASTNode(ASTNode::AST_BINOP);
        node->op = "<=";
        node->lhs = (ASTNode*)$1;
        node->rhs = (ASTNode*)$3;
        node->type_info = 6; // bool
        $$ = (void*)node;
      }
    | expr OR term {
        ASTNode* node = new ASTNode(ASTNode::AST_BINOP);
        node->op = "||";
        node->lhs = (ASTNode*)$1;
        node->rhs = (ASTNode*)$3;
        node->type_info = 6; // bool
        $$ = (void*)node;
      }
    | expr AND term {
        ASTNode* node = new ASTNode(ASTNode::AST_BINOP);
        node->op = "&&";
        node->lhs = (ASTNode*)$1;
        node->rhs = (ASTNode*)$3;
        node->type_info = 6; // bool
        $$ = (void*)node;
      }
    | expr PLUS term {
        debug_printf("DEBUG: expr PLUS term\n");
        ASTNode* node = new ASTNode(ASTNode::AST_BINOP);
        node->op = "+";
        node->lhs = (ASTNode*)$1;
        node->rhs = (ASTNode*)$3;
        int ltype = node->lhs ? node->lhs->type_info : 3;
        int rtype = node->rhs ? node->rhs->type_info : 3;
        node->type_info = (ltype > rtype) ? ltype : rtype;
        $$ = (void*)node;
      }
    | expr MINUS term {
        debug_printf("DEBUG: expr MINUS term\n");
        ASTNode* node = new ASTNode(ASTNode::AST_BINOP);
        node->op = "-";
        node->lhs = (ASTNode*)$1;
        node->rhs = (ASTNode*)$3;
        int ltype = node->lhs ? node->lhs->type_info : 3;
        int rtype = node->rhs ? node->rhs->type_info : 3;
        node->type_info = (ltype > rtype) ? ltype : rtype;
        $$ = (void*)node;
      }
    | term {
        debug_printf("DEBUG: expr -> term\n");
        $$ = $1;
      }
    ;

compound_assign:
      IDENTIFIER ADD_ASSIGN expr {
        ASTNode* node = new ASTNode(ASTNode::AST_ASSIGN);
        node->sval = std::string($1);
        ASTNode* rhs = new ASTNode(ASTNode::AST_BINOP);
        rhs->op = "+";
        rhs->lhs = new ASTNode(ASTNode::AST_VAR); rhs->lhs->sval = std::string($1);
        rhs->rhs = (ASTNode*)$3;
        node->rhs = rhs;
        node->type_info = ((ASTNode*)$3)->type_info;
        $$ = (void*)node;
        free($1);
      }
    | IDENTIFIER SUB_ASSIGN expr {
        ASTNode* node = new ASTNode(ASTNode::AST_ASSIGN);
        node->sval = std::string($1);
        ASTNode* rhs = new ASTNode(ASTNode::AST_BINOP);
        rhs->op = "-";
        rhs->lhs = new ASTNode(ASTNode::AST_VAR); rhs->lhs->sval = std::string($1);
        rhs->rhs = (ASTNode*)$3;
        node->rhs = rhs;
        node->type_info = ((ASTNode*)$3)->type_info;
        $$ = (void*)node;
        free($1);
    }
    | IDENTIFIER MUL_ASSIGN expr {
        ASTNode* node = new ASTNode(ASTNode::AST_ASSIGN);
        node->sval = std::string($1);
        ASTNode* rhs = new ASTNode(ASTNode::AST_BINOP);
        rhs->op = "*";
        rhs->lhs = new ASTNode(ASTNode::AST_VAR); rhs->lhs->sval = std::string($1);
        rhs->rhs = (ASTNode*)$3;
        node->rhs = rhs;
        node->type_info = ((ASTNode*)$3)->type_info;
        $$ = (void*)node;
        free($1);
    }
    | IDENTIFIER DIV_ASSIGN expr {
        ASTNode* node = new ASTNode(ASTNode::AST_ASSIGN);
        node->sval = std::string($1);
        ASTNode* rhs = new ASTNode(ASTNode::AST_BINOP);
        rhs->op = "/";
        rhs->lhs = new ASTNode(ASTNode::AST_VAR); rhs->lhs->sval = std::string($1);
        rhs->rhs = (ASTNode*)$3;
        node->rhs = rhs;
        node->type_info = ((ASTNode*)$3)->type_info;
        $$ = (void*)node;
        free($1);
    }
    | IDENTIFIER MOD_ASSIGN expr {
        ASTNode* node = new ASTNode(ASTNode::AST_ASSIGN);
        node->sval = std::string($1);
        ASTNode* rhs = new ASTNode(ASTNode::AST_BINOP);
        rhs->op = "%";
        rhs->lhs = new ASTNode(ASTNode::AST_VAR); rhs->lhs->sval = std::string($1);
        rhs->rhs = (ASTNode*)$3;
        node->rhs = rhs;
        node->type_info = ((ASTNode*)$3)->type_info;
        $$ = (void*)node;
        free($1);
    }

term:
      term MUL factor {
        debug_printf("DEBUG: term MUL factor\n");
        ASTNode* node = new ASTNode(ASTNode::AST_BINOP);
        node->op = "*";
        node->lhs = (ASTNode*)$1;
        node->rhs = (ASTNode*)$3;
        int ltype = node->lhs ? node->lhs->type_info : 3;
        int rtype = node->rhs ? node->rhs->type_info : 3;
        node->type_info = (ltype > rtype) ? ltype : rtype;
        $$ = (void*)node;
      }
    | term DIV factor {
        debug_printf("DEBUG: term DIV factor\n");
        ASTNode* node = new ASTNode(ASTNode::AST_BINOP);
        node->op = "/";
        node->lhs = (ASTNode*)$1;
        node->rhs = (ASTNode*)$3;
        int ltype = node->lhs ? node->lhs->type_info : 3;
        int rtype = node->rhs ? node->rhs->type_info : 3;
        node->type_info = (ltype > rtype) ? ltype : rtype;
        $$ = (void*)node;
      }
    | term MOD factor {
        debug_printf("DEBUG: term MOD factor\n");
        ASTNode* node = new ASTNode(ASTNode::AST_BINOP);
        node->op = "%";
        node->lhs = (ASTNode*)$1;
        node->rhs = (ASTNode*)$3;
        int ltype = node->lhs ? node->lhs->type_info : 3;
        int rtype = node->rhs ? node->rhs->type_info : 3;
        node->type_info = (ltype > rtype) ? ltype : rtype;
        $$ = (void*)node;
      }
    | factor {
        debug_printf("DEBUG: term -> factor\n");
        $$ = $1;
      }
    ;

// 型情報を持つリテラルをサポート
factor:
    INC_OP IDENTIFIER {
        // 前置インクリメント ++a
        ASTNode* node = new ASTNode(ASTNode::AST_PRE_INCDEC);
        node->op = "++";
        node->lhs = new ASTNode(ASTNode::AST_VAR); node->lhs->sval = std::string($2);
        $$ = (void*)node;
        free($2);
    }
    | DEC_OP IDENTIFIER {
        // 前置デクリメント --a
        ASTNode* node = new ASTNode(ASTNode::AST_PRE_INCDEC);
        node->op = "--";
        node->lhs = new ASTNode(ASTNode::AST_VAR); node->lhs->sval = std::string($2);
        $$ = (void*)node;
        free($2);
    }
    | IDENTIFIER INC_OP {
        // 後置インクリメント a++
        ASTNode* node = new ASTNode(ASTNode::AST_POST_INCDEC);
        node->op = "++";
        node->lhs = new ASTNode(ASTNode::AST_VAR); node->lhs->sval = std::string($1);
        $$ = (void*)node;
        free($1);
    }
    | IDENTIFIER DEC_OP {
        // 後置デクリメント a--
        ASTNode* node = new ASTNode(ASTNode::AST_POST_INCDEC);
        node->op = "--";
        node->lhs = new ASTNode(ASTNode::AST_VAR); node->lhs->sval = std::string($1);
        $$ = (void*)node;
        free($1);
    }
    | NOT factor {
        // NOTは右結合で、factorの先頭でのみ受け付けることで優先順位を正しくする
        ASTNode* node = new ASTNode(ASTNode::AST_UNARYOP);
        node->op = "!";
        node->lhs = (ASTNode*)$2;
        node->type_info = 6; // bool
        $$ = (void*)node;
      }
    | IDENTIFIER '[' expr ']' {
        ASTNode* ref = new ASTNode(ASTNode::AST_ARRAY_REF);
        ref->sval = std::string($1);
        ref->array_index = (ASTNode*)$3;
        $$ = (void*)ref;
        free($1);
    }
    | array_literal { $$ = $1; }
    | '(' expr ')' { $$ = $2; }
    | type NUMBER {
        ASTNode* num = new ASTNode(ASTNode::AST_NUM);
        num->lval64 = $2;
        num->type_info = ((ASTNode*)$1)->type_info;
        // 型範囲チェック
        bool out_of_range = false;
        switch (num->type_info) {
          case 1: // tiny
            if (num->lval64 < -128 || num->lval64 > 127) out_of_range = true;
            break;
          case 2: // short
            if (num->lval64 < -32768 || num->lval64 > 32767) out_of_range = true;
            break;
          case 3: // int
            if (num->lval64 < -2147483648LL || num->lval64 > 2147483647LL) out_of_range = true;
            break;
          case 4: // long
            // int64_tの範囲は十分広いのでチェック不要
            break;
          case 6: // bool
            num->lval64 = (num->lval64 != 0) ? 1 : 0;
            break;
          default:
            break;
        }
        if (out_of_range) {
          yyerror("型の範囲外の値を代入しようとしました", "");
          YYABORT;
        }
        delete (ASTNode*)$1;
        $$ = (void*)num;
      }
    | TRUE {
        ASTNode* b = new ASTNode(ASTNode::AST_NUM);
        b->lval64 = 1;
        b->type_info = 6;
        $$ = (void*)b;
      }
    | FALSE {
        ASTNode* b = new ASTNode(ASTNode::AST_NUM);
        b->lval64 = 0;
        b->type_info = 6;
        $$ = (void*)b;
      }
    | NULL_LIT {
        ASTNode* b = new ASTNode(ASTNode::AST_NUM);
        b->lval64 = 0;
        b->type_info = 6;
        $$ = (void*)b;
      }
    | STRING_LITERAL {
        debug_printf("DEBUG: factor STRING_LITERAL sval=%s\n", $1 ? $1 : "NULL");
        ASTNode* str = new ASTNode(ASTNode::AST_STRING_LITERAL);
        str->sval = std::string($1);
        str->type_info = 5; // string型
        debug_printf("DEBUG: factor STRING_LITERAL type_info=%d\n", str->type_info);
        $$ = (void*)str;
        free($1);
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

// 配列リテラル: [expr, expr, ...]
array_literal:
      '[' array_init_list ']' {
        ASTNode* arr = new ASTNode(ASTNode::AST_ARRAY_LITERAL);
        arr->elements = ((ASTNode*)$2)->elements;
        delete (ASTNode*)$2;
        $$ = (void*)arr;
      }
    | '[' ']' {
        ASTNode* arr = new ASTNode(ASTNode::AST_ARRAY_LITERAL);
        arr->elements = std::vector<ASTNode*>();
        $$ = (void*)arr;
      }
    ;

// 配列初期化リスト: expr, expr, ...
array_init_list:
      expr {
        ASTNode* arr = new ASTNode(ASTNode::AST_ARRAY_LITERAL);
        arr->elements.push_back((ASTNode*)$1);
        $$ = (void*)arr;
      }
    | array_init_list ',' expr {
        ASTNode* arr = (ASTNode*)$1;
        arr->elements.push_back((ASTNode*)$3);
        $$ = (void*)arr;
      }
    ;
%%
