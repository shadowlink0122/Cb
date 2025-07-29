%option noyywrap

%{
#include "parser.h"
%}

%%
[ \t\n]+           ;
[0-9]+            { yylval.ival = atoi(yytext); return NUMBER; }
[a-zA-Z_][a-zA-Z0-9_]* {
    if (strcmp(yytext, "print") == 0) return PRINT;
    yylval.sval = strdup(yytext);
    return IDENTIFIER;
}
\+                { return PLUS; }
\-                { return MINUS; }
\*                { return MUL; }
\/                 { return DIV; }
=                 { return ASSIGN; }
;                 { return SEMICOLON; }
.                 { return yytext[0]; }
%%
