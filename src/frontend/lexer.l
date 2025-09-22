%option noyywrap

%{
#include "../common/ast.h"
#include "parser.h"
extern int yylineno;

// デバッグ用
#ifndef YYDEBUG
#define YYDEBUG 1
#endif

extern int yydebug;
%}

%%
[ \t]+           ;
[\/][\/].*       ;
[/][*]([^*]|[*]+[^*/])*[*]+[/]   ;
\n               { yylineno++; }

const            { return CONST; }
static           { return STATIC; }
typedef          { return TYPEDEF; }
void             { return VOID; }
tiny             { return TINY; }
short            { return SHORT; }
int              { return INT; }
long             { return LONG; }
char             { return CHAR; }
string           { return STRING; }
bool             { return BOOL; }
true             { yylval.lval = 1; return TRUE; }
false            { yylval.lval = 0; return FALSE; }
break            { return BREAK; }
while            { return WHILE; }
null             { return NULL_LIT; }
for              { return FOR; }
if               { return IF; }
else             { return ELSE; }
return           { return RETURN; }
print            { return PRINT; }
println          { return PRINTLN; }

\"([^\"\\]|\\.)*\"  { 
    yylval.sval = strdup(yytext + 1);
    yylval.sval[strlen(yylval.sval) - 1] = '\0';
    return STRING_LITERAL; 
}

'([^'\\]|\\.)' {
    // 文字リテラルの処理
    if (strlen(yytext) == 3) {
        // 通常の文字 'a'
        yylval.lval = yytext[1];
    } else if (strlen(yytext) == 4 && yytext[1] == '\\') {
        // エスケープ文字 '\n', '\t', etc.
        switch (yytext[2]) {
            case 'n': yylval.lval = '\n'; break;
            case 't': yylval.lval = '\t'; break;
            case 'r': yylval.lval = '\r'; break;
            case '\\': yylval.lval = '\\'; break;
            case '\'': yylval.lval = '\''; break;
            case '0': yylval.lval = '\0'; break;
            default: yylval.lval = yytext[2]; break;
        }
    } else {
        yylval.lval = 0; // エラーケース
    }
    return CHAR_LITERAL;
}

[0-9]+            { yylval.lval = strtoll(yytext, NULL, 10); return NUMBER; }

==                { return EQ; }
!=                { return NEQ; }
\>=               { return GE; }
\<=               { return LE; }
\>                { return GT; }
\<                { return LT; }
!                 { return NOT; }
\|\|              { return OR; }
&&                { return AND; }
\+\+              { return INC_OP; }
\-\-              { return DEC_OP; }
\+=               { return ADD_ASSIGN; }
\-=               { return SUB_ASSIGN; }
\*=               { return MUL_ASSIGN; }
\/=               { return DIV_ASSIGN; }
\%=               { return MOD_ASSIGN; }
\+                { return PLUS; }
\-                { return MINUS; }
\*                { return MUL; }
\/                { return DIV; }
%                 { return MOD; }
\=                { return ASSIGN; }

\(                { return '('; }
\)                { return ')'; }
\{                { return '{'; }
\}                { return '}'; }
\[                { return '['; }
\]                { return ']'; }
\;                { return SEMICOLON; }
\,                { return ','; }

[a-zA-Z_][a-zA-Z0-9_]* {
    yylval.sval = strdup(yytext);
    return IDENTIFIER;
}

.                 { return yytext[0]; }

%%
