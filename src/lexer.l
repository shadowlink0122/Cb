%option noyywrap

%{
#include "parser.h"
extern int yylineno;
%}

%%
[ 	]+           ;
[ \t]+           ;
[\/][\/].*         ; // C++スタイルコメント無視
[/][*]([^*]*[*]+)*?[/]   ; // Cスタイルコメント無視
\n               { yylineno++; }
void             { return VOID; }
tiny             { return TINY; }
short            { return SHORT; }
int              { return INT; }
long             { return LONG; }
string           { return STRING; }
bool             { return BOOL; }
true             { return TRUE; }
false            { return FALSE; }
break            { return BREAK; }
while            { return WHILE; }
null             { return NULL_LIT; }
for              { return FOR; }
if               { return IF; }
else             { return ELSE; }
\"([^\\\"]|\\.)*\"  { yylval.sval = strdup(yytext); return STRING_LITERAL; }
[0-9]+            { yylval.lval = strtoll(yytext, NULL, 10); return NUMBER; }
return            { return RETURN; }
==                { return EQ; }
!=                { return NEQ; }
!                 { return NOT; }
\|\|              { return OR; }
&&                { return AND; }
\+=               { return ADD_ASSIGN; }
\-=               { return SUB_ASSIGN; }
\*=               { return MUL_ASSIGN; }
\/=               { return DIV_ASSIGN; }
\%=               { return MOD_ASSIGN; }
[a-zA-Z_][a-zA-Z0-9_]* {
    if (strcmp(yytext, "print") == 0) return PRINT;
    yylval.sval = strdup(yytext);
    return IDENTIFIER;
}
\+                { return PLUS; }
\-                { return MINUS; }
\*                { return MUL; }
\/                { return DIV; }
%                 { return MOD; }
\(                { return '('; }
\)                { return ')'; }
\{                { return '{'; }
\}                { return '}'; }
\[                { return '['; }
\]                { return ']'; }
\>=               { return GE; }
\<=               { return LE; }
\>                { return GT; }
\<                { return LT; }
\=                { return ASSIGN; }
\;                { return SEMICOLON; }
\.                { return yytext[0]; }
%%
