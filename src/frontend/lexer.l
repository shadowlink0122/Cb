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
void             { return VOID; }
tiny             { return TINY; }
short            { return SHORT; }
int              { return INT; }
long             { return LONG; }
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
import           { return IMPORT; }
export           { return EXPORT; }
module           { return MODULE; }
try              { return TRY; }
catch            { return CATCH; }
finally          { return FINALLY; }
throw            { return THROW; }

\"([^\"\\]|\\.)*\"  { 
    yylval.sval = strdup(yytext + 1);
    yylval.sval[strlen(yylval.sval) - 1] = '\0';
    
    // エスケープシーケンスを処理
    char *src = yylval.sval;
    char *dst = yylval.sval;
    while (*src) {
        if (*src == '\\' && *(src + 1)) {
            switch (*(src + 1)) {
                case 'n': *dst++ = '\n'; src += 2; break;
                case 't': *dst++ = '\t'; src += 2; break;
                case 'r': *dst++ = '\r'; src += 2; break;
                case '\\': *dst++ = '\\'; src += 2; break;
                case '"': *dst++ = '"'; src += 2; break;
                case '\'': *dst++ = '\''; src += 2; break;
                case '0': *dst++ = '\0'; src += 2; break;
                case 'a': *dst++ = '\a'; src += 2; break;
                case 'b': *dst++ = '\b'; src += 2; break;
                case 'f': *dst++ = '\f'; src += 2; break;
                case 'v': *dst++ = '\v'; src += 2; break;
                default: *dst++ = *src++; break;
            }
        } else {
            *dst++ = *src++;
        }
    }
    *dst = '\0';
    
    return STRING_LITERAL; 
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
