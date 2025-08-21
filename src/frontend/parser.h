/* A Bison parser, made by GNU Bison 2.3.  */

/* Skeleton interface for Bison's Yacc-like parsers in C

   Copyright (C) 1984, 1989, 1990, 2000, 2001, 2002, 2003, 2004, 2005, 2006
   Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.  */

/* As a special exception, you may create a larger work that contains
   part or all of the Bison parser skeleton and distribute that work
   under terms of your choice, so long as that work isn't itself a
   parser generator using the skeleton or a modified version thereof
   as a parser skeleton.  Alternatively, if you modify or redistribute
   the parser skeleton itself, you may (at your option) remove this
   special exception, which will cause the skeleton and the resulting
   Bison output files to be licensed under the GNU General Public
   License without this special exception.

   This special exception was added by the Free Software Foundation in
   version 2.2 of Bison.  */

/* Tokens.  */
#ifndef YYTOKENTYPE
#define YYTOKENTYPE
/* Put the tokens into the symbol table, so that GDB and other debuggers
   know about them.  */
enum yytokentype {
    NUMBER = 258,
    IDENTIFIER = 259,
    STRING_LITERAL = 260,
    CONST = 261,
    STATIC = 262,
    VOID = 263,
    TINY = 264,
    SHORT = 265,
    INT = 266,
    LONG = 267,
    BOOL = 268,
    STRING = 269,
    TRUE = 270,
    FALSE = 271,
    NULL_LIT = 272,
    PLUS = 273,
    MINUS = 274,
    MUL = 275,
    DIV = 276,
    ASSIGN = 277,
    SEMICOLON = 278,
    PRINT = 279,
    RETURN = 280,
    FOR = 281,
    WHILE = 282,
    BREAK = 283,
    IF = 284,
    ELSE = 285,
    EQ = 286,
    NEQ = 287,
    GE = 288,
    LE = 289,
    GT = 290,
    LT = 291,
    OR = 292,
    AND = 293,
    NOT = 294,
    MOD = 295,
    ADD_ASSIGN = 296,
    SUB_ASSIGN = 297,
    MUL_ASSIGN = 298,
    DIV_ASSIGN = 299,
    MOD_ASSIGN = 300,
    INC_OP = 301,
    DEC_OP = 302
};
#endif
/* Tokens.  */
#define NUMBER 258
#define IDENTIFIER 259
#define STRING_LITERAL 260
#define CONST 261
#define STATIC 262
#define VOID 263
#define TINY 264
#define SHORT 265
#define INT 266
#define LONG 267
#define BOOL 268
#define STRING 269
#define TRUE 270
#define FALSE 271
#define NULL_LIT 272
#define PLUS 273
#define MINUS 274
#define MUL 275
#define DIV 276
#define ASSIGN 277
#define SEMICOLON 278
#define PRINT 279
#define RETURN 280
#define FOR 281
#define WHILE 282
#define BREAK 283
#define IF 284
#define ELSE 285
#define EQ 286
#define NEQ 287
#define GE 288
#define LE 289
#define GT 290
#define LT 291
#define OR 292
#define AND 293
#define NOT 294
#define MOD 295
#define ADD_ASSIGN 296
#define SUB_ASSIGN 297
#define MUL_ASSIGN 298
#define DIV_ASSIGN 299
#define MOD_ASSIGN 300
#define INC_OP 301
#define DEC_OP 302

#if !defined YYSTYPE && !defined YYSTYPE_IS_DECLARED
typedef union YYSTYPE
#line 20 "src/frontend/parser.y"
{
    long long lval;
    char *sval;
    void *ptr;
}
/* Line 1529 of yacc.c.  */
#line 149 "src/frontend/parser.h"
YYSTYPE;
#define yystype YYSTYPE /* obsolescent; will be withdrawn */
#define YYSTYPE_IS_DECLARED 1
#define YYSTYPE_IS_TRIVIAL 1
#endif

extern YYSTYPE yylval;
