/* A Bison parser, made by GNU Bison 2.3.  */

/* Skeleton implementation for Bison's Yacc-like parsers in C

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

/* C LALR(1) parser skeleton written by Richard Stallman, by
   simplifying the original so-called "semantic" parser.  */

/* All symbols defined below should begin with yy or YY, to avoid
   infringing on user name space.  This should be done even for local
   variables, as they might otherwise be expanded by user macros.
   There are some unavoidable exceptions within include files to
   define necessary library symbols; they are noted "INFRINGES ON
   USER NAME SPACE" below.  */

/* Identify Bison output.  */
#define YYBISON 1

/* Bison version.  */
#define YYBISON_VERSION "2.3"

/* Skeleton name.  */
#define YYSKELETON_NAME "yacc.c"

/* Pure parsers.  */
#define YYPURE 0

/* Using locations.  */
#define YYLSP_NEEDED 0



/* Tokens.  */
#ifndef YYTOKENTYPE
# define YYTOKENTYPE
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




/* Copy the first part of user declarations.  */
#line 1 "src/frontend/parser.y"

#include "../common/ast.h"
#include "../frontend/parser_utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <cstring>

extern int yylex();
extern void yyerror(const char* s);
extern std::unique_ptr<ASTNode> root_node;

int yylex();
std::unique_ptr<ASTNode> root_node = nullptr;
extern "C" {
    char *yyfilename = NULL;
}


/* Enabling traces.  */
#ifndef YYDEBUG
# define YYDEBUG 0
#endif

/* Enabling verbose error messages.  */
#ifdef YYERROR_VERBOSE
# undef YYERROR_VERBOSE
# define YYERROR_VERBOSE 1
#else
# define YYERROR_VERBOSE 0
#endif

/* Enabling the token table.  */
#ifndef YYTOKEN_TABLE
# define YYTOKEN_TABLE 0
#endif

#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED
typedef union YYSTYPE
#line 20 "src/frontend/parser.y"
{
    long long lval;
    char* sval;
    void* ptr;
}
/* Line 193 of yacc.c.  */
#line 215 "src/frontend/parser.c"
	YYSTYPE;
# define yystype YYSTYPE /* obsolescent; will be withdrawn */
# define YYSTYPE_IS_DECLARED 1
# define YYSTYPE_IS_TRIVIAL 1
#endif



/* Copy the second part of user declarations.  */


/* Line 216 of yacc.c.  */
#line 228 "src/frontend/parser.c"

#ifdef short
# undef short
#endif

#ifdef YYTYPE_UINT8
typedef YYTYPE_UINT8 yytype_uint8;
#else
typedef unsigned char yytype_uint8;
#endif

#ifdef YYTYPE_INT8
typedef YYTYPE_INT8 yytype_int8;
#elif (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
typedef signed char yytype_int8;
#else
typedef short int yytype_int8;
#endif

#ifdef YYTYPE_UINT16
typedef YYTYPE_UINT16 yytype_uint16;
#else
typedef unsigned short int yytype_uint16;
#endif

#ifdef YYTYPE_INT16
typedef YYTYPE_INT16 yytype_int16;
#else
typedef short int yytype_int16;
#endif

#ifndef YYSIZE_T
# ifdef __SIZE_TYPE__
#  define YYSIZE_T __SIZE_TYPE__
# elif defined size_t
#  define YYSIZE_T size_t
# elif ! defined YYSIZE_T && (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
#  include <stddef.h> /* INFRINGES ON USER NAME SPACE */
#  define YYSIZE_T size_t
# else
#  define YYSIZE_T unsigned int
# endif
#endif

#define YYSIZE_MAXIMUM ((YYSIZE_T) -1)

#ifndef YY_
# if defined YYENABLE_NLS && YYENABLE_NLS
#  if ENABLE_NLS
#   include <libintl.h> /* INFRINGES ON USER NAME SPACE */
#   define YY_(msgid) dgettext ("bison-runtime", msgid)
#  endif
# endif
# ifndef YY_
#  define YY_(msgid) msgid
# endif
#endif

/* Suppress unused-variable warnings by "using" E.  */
#if ! defined lint || defined __GNUC__
# define YYUSE(e) ((void) (e))
#else
# define YYUSE(e) /* empty */
#endif

/* Identity function, used to suppress warnings about constant conditions.  */
#ifndef lint
# define YYID(n) (n)
#else
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static int
YYID (int i)
#else
static int
YYID (i)
    int i;
#endif
{
  return i;
}
#endif

#if ! defined yyoverflow || YYERROR_VERBOSE

/* The parser invokes alloca or malloc; define the necessary symbols.  */

# ifdef YYSTACK_USE_ALLOCA
#  if YYSTACK_USE_ALLOCA
#   ifdef __GNUC__
#    define YYSTACK_ALLOC __builtin_alloca
#   elif defined __BUILTIN_VA_ARG_INCR
#    include <alloca.h> /* INFRINGES ON USER NAME SPACE */
#   elif defined _AIX
#    define YYSTACK_ALLOC __alloca
#   elif defined _MSC_VER
#    include <malloc.h> /* INFRINGES ON USER NAME SPACE */
#    define alloca _alloca
#   else
#    define YYSTACK_ALLOC alloca
#    if ! defined _ALLOCA_H && ! defined _STDLIB_H && (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
#     include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
#     ifndef _STDLIB_H
#      define _STDLIB_H 1
#     endif
#    endif
#   endif
#  endif
# endif

# ifdef YYSTACK_ALLOC
   /* Pacify GCC's `empty if-body' warning.  */
#  define YYSTACK_FREE(Ptr) do { /* empty */; } while (YYID (0))
#  ifndef YYSTACK_ALLOC_MAXIMUM
    /* The OS might guarantee only one guard page at the bottom of the stack,
       and a page size can be as small as 4096 bytes.  So we cannot safely
       invoke alloca (N) if N exceeds 4096.  Use a slightly smaller number
       to allow for a few compiler-allocated temporary stack slots.  */
#   define YYSTACK_ALLOC_MAXIMUM 4032 /* reasonable circa 2006 */
#  endif
# else
#  define YYSTACK_ALLOC YYMALLOC
#  define YYSTACK_FREE YYFREE
#  ifndef YYSTACK_ALLOC_MAXIMUM
#   define YYSTACK_ALLOC_MAXIMUM YYSIZE_MAXIMUM
#  endif
#  if (defined __cplusplus && ! defined _STDLIB_H \
       && ! ((defined YYMALLOC || defined malloc) \
	     && (defined YYFREE || defined free)))
#   include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
#   ifndef _STDLIB_H
#    define _STDLIB_H 1
#   endif
#  endif
#  ifndef YYMALLOC
#   define YYMALLOC malloc
#   if ! defined malloc && ! defined _STDLIB_H && (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
void *malloc (YYSIZE_T); /* INFRINGES ON USER NAME SPACE */
#   endif
#  endif
#  ifndef YYFREE
#   define YYFREE free
#   if ! defined free && ! defined _STDLIB_H && (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
void free (void *); /* INFRINGES ON USER NAME SPACE */
#   endif
#  endif
# endif
#endif /* ! defined yyoverflow || YYERROR_VERBOSE */


#if (! defined yyoverflow \
     && (! defined __cplusplus \
	 || (defined YYSTYPE_IS_TRIVIAL && YYSTYPE_IS_TRIVIAL)))

/* A type that is properly aligned for any stack member.  */
union yyalloc
{
  yytype_int16 yyss;
  YYSTYPE yyvs;
  };

/* The size of the maximum gap between one aligned stack and the next.  */
# define YYSTACK_GAP_MAXIMUM (sizeof (union yyalloc) - 1)

/* The size of an array large to enough to hold all stacks, each with
   N elements.  */
# define YYSTACK_BYTES(N) \
     ((N) * (sizeof (yytype_int16) + sizeof (YYSTYPE)) \
      + YYSTACK_GAP_MAXIMUM)

/* Copy COUNT objects from FROM to TO.  The source and destination do
   not overlap.  */
# ifndef YYCOPY
#  if defined __GNUC__ && 1 < __GNUC__
#   define YYCOPY(To, From, Count) \
      __builtin_memcpy (To, From, (Count) * sizeof (*(From)))
#  else
#   define YYCOPY(To, From, Count)		\
      do					\
	{					\
	  YYSIZE_T yyi;				\
	  for (yyi = 0; yyi < (Count); yyi++)	\
	    (To)[yyi] = (From)[yyi];		\
	}					\
      while (YYID (0))
#  endif
# endif

/* Relocate STACK from its old location to the new one.  The
   local variables YYSIZE and YYSTACKSIZE give the old and new number of
   elements in the stack, and YYPTR gives the new location of the
   stack.  Advance YYPTR to a properly aligned location for the next
   stack.  */
# define YYSTACK_RELOCATE(Stack)					\
    do									\
      {									\
	YYSIZE_T yynewbytes;						\
	YYCOPY (&yyptr->Stack, Stack, yysize);				\
	Stack = &yyptr->Stack;						\
	yynewbytes = yystacksize * sizeof (*Stack) + YYSTACK_GAP_MAXIMUM; \
	yyptr += yynewbytes / sizeof (*yyptr);				\
      }									\
    while (YYID (0))

#endif

/* YYFINAL -- State number of the termination state.  */
#define YYFINAL  18
/* YYLAST -- Last index in YYTABLE.  */
#define YYLAST   778

/* YYNTOKENS -- Number of terminals.  */
#define YYNTOKENS  55
/* YYNNTS -- Number of nonterminals.  */
#define YYNNTS  26
/* YYNRULES -- Number of rules.  */
#define YYNRULES  102
/* YYNRULES -- Number of states.  */
#define YYNSTATES  202

/* YYTRANSLATE(YYLEX) -- Bison symbol number corresponding to YYLEX.  */
#define YYUNDEFTOK  2
#define YYMAXUTOK   302

#define YYTRANSLATE(YYX)						\
  ((unsigned int) (YYX) <= YYMAXUTOK ? yytranslate[YYX] : YYUNDEFTOK)

/* YYTRANSLATE[YYLEX] -- Bison symbol number corresponding to YYLEX.  */
static const yytype_uint8 yytranslate[] =
{
       0,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
      50,    51,     2,     2,    54,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,    52,     2,    53,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,    48,     2,    49,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     1,     2,     3,     4,
       5,     6,     7,     8,     9,    10,    11,    12,    13,    14,
      15,    16,    17,    18,    19,    20,    21,    22,    23,    24,
      25,    26,    27,    28,    29,    30,    31,    32,    33,    34,
      35,    36,    37,    38,    39,    40,    41,    42,    43,    44,
      45,    46,    47
};

#if YYDEBUG
/* YYPRHS[YYN] -- Index of the first RHS symbol of rule number YYN in
   YYRHS.  */
static const yytype_uint16 yyprhs[] =
{
       0,     0,     3,     5,     7,    10,    12,    16,    20,    22,
      24,    26,    30,    33,    36,    38,    40,    42,    44,    46,
      48,    50,    52,    54,    56,    60,    65,    71,    78,    87,
      95,   104,   112,   113,   116,   121,   122,   125,   127,   130,
     134,   140,   148,   154,   164,   173,   177,   180,   183,   187,
     191,   193,   195,   197,   201,   208,   212,   216,   220,   224,
     228,   230,   234,   236,   240,   242,   246,   250,   252,   256,
     260,   264,   268,   270,   274,   278,   280,   284,   288,   292,
     294,   297,   300,   303,   306,   308,   311,   314,   319,   324,
     328,   330,   332,   335,   337,   339,   341,   343,   347,   349,
     353,   355,   359
};

/* YYRHS -- A `-1'-separated list of the rules' RHS.  */
static const yytype_int8 yyrhs[] =
{
      56,     0,    -1,    57,    -1,    58,    -1,    57,    58,    -1,
      64,    -1,    59,    63,    23,    -1,    62,    63,    23,    -1,
      60,    -1,    61,    -1,    62,    -1,    60,    61,    62,    -1,
      60,    62,    -1,    61,    62,    -1,     7,    -1,     6,    -1,
       8,    -1,     9,    -1,    10,    -1,    11,    -1,    12,    -1,
      14,    -1,    13,    -1,     4,    -1,     4,    22,    69,    -1,
       4,    52,    68,    53,    -1,     4,    52,    53,    22,    80,
      -1,     4,    52,    68,    53,    22,    80,    -1,    59,     4,
      50,    65,    51,    48,    66,    49,    -1,    59,     4,    50,
      51,    48,    66,    49,    -1,    62,     4,    50,    65,    51,
      48,    66,    49,    -1,    62,     4,    50,    51,    48,    66,
      49,    -1,    -1,    62,     4,    -1,    65,    54,    62,     4,
      -1,    -1,    66,    67,    -1,    58,    -1,    68,    23,    -1,
      24,    68,    23,    -1,    29,    50,    68,    51,    67,    -1,
      29,    50,    68,    51,    67,    30,    67,    -1,    27,    50,
      68,    51,    67,    -1,    26,    50,    68,    23,    68,    23,
      68,    51,    67,    -1,    26,    50,    58,    68,    23,    68,
      51,    67,    -1,    25,    68,    23,    -1,    25,    23,    -1,
      28,    23,    -1,    28,    68,    23,    -1,    48,    66,    49,
      -1,    23,    -1,    69,    -1,    70,    -1,     4,    22,    69,
      -1,     4,    52,    68,    53,    22,    69,    -1,     4,    41,
      69,    -1,     4,    42,    69,    -1,     4,    43,    69,    -1,
       4,    44,    69,    -1,     4,    45,    69,    -1,    71,    -1,
      70,    37,    71,    -1,    72,    -1,    71,    38,    72,    -1,
      73,    -1,    72,    31,    73,    -1,    72,    32,    73,    -1,
      74,    -1,    73,    36,    74,    -1,    73,    35,    74,    -1,
      73,    34,    74,    -1,    73,    33,    74,    -1,    75,    -1,
      74,    18,    75,    -1,    74,    19,    75,    -1,    76,    -1,
      75,    20,    76,    -1,    75,    21,    76,    -1,    75,    40,
      76,    -1,    77,    -1,    46,     4,    -1,    47,     4,    -1,
      39,    76,    -1,    19,    76,    -1,    78,    -1,     4,    46,
      -1,     4,    47,    -1,     4,    52,    68,    53,    -1,     4,
      50,    79,    51,    -1,     4,    50,    51,    -1,     4,    -1,
       3,    -1,    62,     3,    -1,     5,    -1,    15,    -1,    16,
      -1,    17,    -1,    50,    68,    51,    -1,    69,    -1,    79,
      54,    69,    -1,    69,    -1,    52,    79,    53,    -1,    52,
      53,    -1
};

/* YYRLINE[YYN] -- source line where rule number YYN was defined.  */
static const yytype_uint16 yyrline[] =
{
       0,    54,    54,    61,    66,    73,    74,    80,    90,    91,
      92,    93,    98,   102,   109,   113,   117,   118,   119,   120,
     121,   122,   123,   127,   131,   135,   139,   143,   150,   156,
     161,   168,   177,   178,   185,   194,   195,   202,   203,   204,
     205,   208,   211,   214,   217,   220,   221,   222,   223,   224,
     225,   229,   233,   234,   238,   242,   246,   250,   254,   258,
     265,   266,   272,   273,   279,   280,   283,   289,   290,   293,
     296,   299,   305,   306,   309,   315,   316,   319,   322,   328,
     329,   330,   331,   332,   336,   337,   338,   339,   343,   347,
     354,   355,   363,   367,   371,   372,   373,   374,   378,   383,
     390,   391,   392
};
#endif

#if YYDEBUG || YYERROR_VERBOSE || YYTOKEN_TABLE
/* YYTNAME[SYMBOL-NUM] -- String name of the symbol SYMBOL-NUM.
   First, the terminals, then, starting at YYNTOKENS, nonterminals.  */
static const char *const yytname[] =
{
  "$end", "error", "$undefined", "NUMBER", "IDENTIFIER", "STRING_LITERAL",
  "CONST", "STATIC", "VOID", "TINY", "SHORT", "INT", "LONG", "BOOL",
  "STRING", "TRUE", "FALSE", "NULL_LIT", "PLUS", "MINUS", "MUL", "DIV",
  "ASSIGN", "SEMICOLON", "PRINT", "RETURN", "FOR", "WHILE", "BREAK", "IF",
  "ELSE", "EQ", "NEQ", "GE", "LE", "GT", "LT", "OR", "AND", "NOT", "MOD",
  "ADD_ASSIGN", "SUB_ASSIGN", "MUL_ASSIGN", "DIV_ASSIGN", "MOD_ASSIGN",
  "INC_OP", "DEC_OP", "'{'", "'}'", "'('", "')'", "'['", "']'", "','",
  "$accept", "program", "declaration_list", "declaration",
  "declaration_specifiers", "storage_class_specifier", "type_qualifier",
  "type_specifier", "declarator", "function_definition", "parameter_list",
  "statement_list", "statement", "expression", "assignment_expression",
  "logical_or_expression", "logical_and_expression", "equality_expression",
  "relational_expression", "additive_expression",
  "multiplicative_expression", "unary_expression", "postfix_expression",
  "primary_expression", "argument_list", "initializer", 0
};
#endif

# ifdef YYPRINT
/* YYTOKNUM[YYLEX-NUM] -- Internal token number corresponding to
   token YYLEX-NUM.  */
static const yytype_uint16 yytoknum[] =
{
       0,   256,   257,   258,   259,   260,   261,   262,   263,   264,
     265,   266,   267,   268,   269,   270,   271,   272,   273,   274,
     275,   276,   277,   278,   279,   280,   281,   282,   283,   284,
     285,   286,   287,   288,   289,   290,   291,   292,   293,   294,
     295,   296,   297,   298,   299,   300,   301,   302,   123,   125,
      40,    41,    91,    93,    44
};
# endif

/* YYR1[YYN] -- Symbol number of symbol that rule YYN derives.  */
static const yytype_uint8 yyr1[] =
{
       0,    55,    56,    57,    57,    58,    58,    58,    59,    59,
      59,    59,    59,    59,    60,    61,    62,    62,    62,    62,
      62,    62,    62,    63,    63,    63,    63,    63,    64,    64,
      64,    64,    65,    65,    65,    66,    66,    67,    67,    67,
      67,    67,    67,    67,    67,    67,    67,    67,    67,    67,
      67,    68,    69,    69,    69,    69,    69,    69,    69,    69,
      70,    70,    71,    71,    72,    72,    72,    73,    73,    73,
      73,    73,    74,    74,    74,    75,    75,    75,    75,    76,
      76,    76,    76,    76,    77,    77,    77,    77,    77,    77,
      78,    78,    78,    78,    78,    78,    78,    78,    79,    79,
      80,    80,    80
};

/* YYR2[YYN] -- Number of symbols composing right hand side of rule YYN.  */
static const yytype_uint8 yyr2[] =
{
       0,     2,     1,     1,     2,     1,     3,     3,     1,     1,
       1,     3,     2,     2,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     3,     4,     5,     6,     8,     7,
       8,     7,     0,     2,     4,     0,     2,     1,     2,     3,
       5,     7,     5,     9,     8,     3,     2,     2,     3,     3,
       1,     1,     1,     3,     6,     3,     3,     3,     3,     3,
       1,     3,     1,     3,     1,     3,     3,     1,     3,     3,
       3,     3,     1,     3,     3,     1,     3,     3,     3,     1,
       2,     2,     2,     2,     1,     2,     2,     4,     4,     3,
       1,     1,     2,     1,     1,     1,     1,     3,     1,     3,
       1,     3,     2
};

/* YYDEFACT[STATE-NAME] -- Default rule to reduce with in state
   STATE-NUM when YYTABLE doesn't specify something else to do.  Zero
   means the default is an error.  */
static const yytype_uint8 yydefact[] =
{
       0,    15,    14,    16,    17,    18,    19,    20,    22,    21,
       0,     2,     3,     0,     8,     9,     0,     5,     1,     4,
      23,     0,     0,    12,    13,    23,     0,     0,    32,     0,
       6,    11,    32,     7,    91,    90,    93,    94,    95,    96,
       0,     0,     0,     0,     0,     0,    24,    52,    60,    62,
      64,    67,    72,    75,    79,    84,     0,     0,     0,     0,
       0,    51,     0,     0,     0,     0,     0,     0,     0,     0,
      85,    86,     0,     0,    90,    83,    82,    80,    81,     0,
      92,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,    35,    33,     0,     0,     0,    25,
      35,     0,    53,    55,    56,    57,    58,    59,    89,    98,
       0,     0,     0,    97,    61,    63,    65,    66,    71,    70,
      69,    68,    73,    74,    76,    77,    78,     0,    35,     0,
       0,   100,    26,     0,     0,    35,    88,     0,    87,     0,
      50,     0,     0,     0,     0,     0,     0,    35,    29,    37,
       0,    36,     0,     0,    34,   102,     0,    27,    31,     0,
      99,     0,    87,     0,    46,     0,     0,     0,    47,     0,
       0,     0,    38,    28,   101,    30,    54,    39,    45,     0,
       0,     0,    48,     0,    49,     0,     0,     0,     0,     0,
       0,    42,    40,     0,     0,     0,     0,     0,    41,    44,
       0,    43
};

/* YYDEFGOTO[NTERM-NUM].  */
static const yytype_int16 yydefgoto[] =
{
      -1,    10,    11,   149,    13,    14,    15,    45,    26,    17,
      58,   127,   151,   152,    61,    47,    48,    49,    50,    51,
      52,    53,    54,    55,   110,   132
};

/* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
   STATE-NUM.  */
#define YYPACT_NINF -103
static const yytype_int16 yypact[] =
{
     275,  -103,  -103,  -103,  -103,  -103,  -103,  -103,  -103,  -103,
      16,   275,  -103,    14,   138,   282,    19,  -103,  -103,  -103,
      -1,     2,   282,  -103,  -103,    38,     6,   680,    26,    53,
    -103,  -103,    70,  -103,  -103,   133,  -103,  -103,  -103,  -103,
     728,   728,    27,    29,   680,    86,  -103,     4,    66,   -23,
      89,    56,   -14,  -103,  -103,  -103,    25,    87,   -41,   104,
      61,  -103,    84,   -34,   680,   680,   680,   680,   680,   680,
    -103,  -103,   261,   680,    55,  -103,  -103,  -103,  -103,    82,
    -103,   728,   728,   728,   728,   728,   728,   728,   728,   728,
     728,   728,   728,   728,  -103,  -103,    88,   282,   211,   113,
    -103,    90,  -103,  -103,  -103,  -103,  -103,  -103,  -103,  -103,
     -24,   103,   680,  -103,    66,   -23,    89,    89,    56,    56,
      56,    56,   -14,   -14,  -103,  -103,  -103,   310,  -103,   136,
     194,  -103,  -103,   211,   358,  -103,  -103,   680,   115,   105,
    -103,   680,   615,    93,   107,   663,   110,  -103,  -103,  -103,
      92,  -103,   139,   406,  -103,  -103,    59,  -103,  -103,   454,
    -103,   680,  -103,   140,  -103,   142,   598,   680,  -103,   145,
     680,   502,  -103,  -103,  -103,  -103,  -103,  -103,  -103,   680,
     147,   121,  -103,   122,  -103,   158,   680,   550,   550,   680,
     159,  -103,   154,   135,   680,   550,   550,   141,  -103,  -103,
     550,  -103
};

/* YYPGOTO[NTERM-NUM].  */
static const yytype_int16 yypgoto[] =
{
    -103,  -103,  -103,     1,  -103,  -103,   175,     0,   177,  -103,
     161,   -76,  -102,   -25,   -22,  -103,   120,   109,    35,    43,
      20,   -38,  -103,  -103,    64,    79
};

/* YYTABLE[YYPACT[STATE-NUM]].  What to do in state STATE-NUM.  If
   positive, shift that token.  If negative, reduce the rule which
   number is the opposite.  If zero, do what YYDEFACT says.
   If YYTABLE_NINF, syntax error.  */
#define YYTABLE_NINF -1
static const yytype_uint8 yytable[] =
{
      16,    12,    75,    76,    60,    46,    91,    92,    83,    84,
      96,    16,    19,    97,    23,    24,    18,   101,    20,    79,
      97,    27,    31,    25,   134,    30,    93,   136,    57,    33,
     137,    77,    57,    78,     3,     4,     5,     6,     7,     8,
       9,    81,   102,   103,   104,   105,   106,   107,   111,    28,
     109,    29,   153,   124,   125,   126,    34,    35,    36,   159,
      27,     3,     4,     5,     6,     7,     8,     9,    37,    38,
      39,   171,    40,    94,    89,    90,   131,    56,     3,     4,
       5,     6,     7,     8,     9,   191,   192,   139,    32,    80,
      29,    95,    41,   198,   199,    80,    25,   129,   201,    42,
      43,    70,    71,    44,    82,    72,    59,   112,   109,   122,
     123,   131,   174,   137,    99,   160,   163,   165,   116,   117,
     169,    62,    85,    86,    87,    88,    98,   150,   118,   119,
     120,   121,   100,   113,   150,   133,   128,   161,   135,   176,
     154,   180,   181,   166,     1,   183,     3,     4,     5,     6,
       7,     8,     9,   150,   185,    64,   138,   167,   162,   150,
     170,   190,   172,   177,   193,   178,   150,   179,   182,   197,
     186,   150,   187,   188,    65,    66,    67,    68,    69,    70,
      71,   189,   194,    72,   195,    73,   196,   150,   150,    22,
      21,   115,   200,    63,   156,   150,   150,    34,    35,    36,
     150,   114,     3,     4,     5,     6,     7,     8,     9,    37,
      38,    39,   157,    40,    34,    35,    36,     0,     0,     3,
       4,     5,     6,     7,     8,     9,    37,    38,    39,     0,
      40,     0,     0,    41,     0,     0,     0,     0,     0,     0,
      42,    43,     0,     0,    44,     0,     0,   155,     0,     0,
      41,     0,     0,     0,     0,     0,     0,    42,    43,     0,
       0,    44,     0,   130,    34,    35,    36,     0,     0,     3,
       4,     5,     6,     7,     8,     9,    37,    38,    39,     0,
      40,     1,     2,     3,     4,     5,     6,     7,     8,     9,
       3,     4,     5,     6,     7,     8,     9,     0,     0,     0,
      41,     0,     0,     0,     0,     0,     0,    42,    43,     0,
       0,    44,   108,    34,    35,    36,     1,     2,     3,     4,
       5,     6,     7,     8,     9,    37,    38,    39,     0,    40,
       0,     0,     0,   140,   141,   142,   143,   144,   145,   146,
       0,     0,     0,     0,     0,     0,     0,     0,     0,    41,
       0,     0,     0,     0,     0,     0,    42,    43,   147,   148,
      44,    34,    35,    36,     1,     2,     3,     4,     5,     6,
       7,     8,     9,    37,    38,    39,     0,    40,     0,     0,
       0,   140,   141,   142,   143,   144,   145,   146,     0,     0,
       0,     0,     0,     0,     0,     0,     0,    41,     0,     0,
       0,     0,     0,     0,    42,    43,   147,   158,    44,    34,
      35,    36,     1,     2,     3,     4,     5,     6,     7,     8,
       9,    37,    38,    39,     0,    40,     0,     0,     0,   140,
     141,   142,   143,   144,   145,   146,     0,     0,     0,     0,
       0,     0,     0,     0,     0,    41,     0,     0,     0,     0,
       0,     0,    42,    43,   147,   173,    44,    34,    35,    36,
       1,     2,     3,     4,     5,     6,     7,     8,     9,    37,
      38,    39,     0,    40,     0,     0,     0,   140,   141,   142,
     143,   144,   145,   146,     0,     0,     0,     0,     0,     0,
       0,     0,     0,    41,     0,     0,     0,     0,     0,     0,
      42,    43,   147,   175,    44,    34,    35,    36,     1,     2,
       3,     4,     5,     6,     7,     8,     9,    37,    38,    39,
       0,    40,     0,     0,     0,   140,   141,   142,   143,   144,
     145,   146,     0,     0,     0,     0,     0,     0,     0,     0,
       0,    41,     0,     0,     0,     0,     0,     0,    42,    43,
     147,   184,    44,    34,    35,    36,     1,     2,     3,     4,
       5,     6,     7,     8,     9,    37,    38,    39,     0,    40,
       0,     0,     0,   140,   141,   142,   143,   144,   145,   146,
       0,     0,     0,     0,     0,     0,     0,     0,     0,    41,
       0,     0,     0,     0,     0,     0,    42,    43,   147,     0,
      44,    34,    35,    36,     1,     2,     3,     4,     5,     6,
       7,     8,     9,    37,    38,    39,     0,    40,    34,    35,
      36,     0,     0,     3,     4,     5,     6,     7,     8,     9,
      37,    38,    39,     0,    40,     0,     0,    41,   164,     0,
       0,     0,     0,     0,    42,    43,     0,     0,    44,     0,
       0,     0,     0,     0,    41,     0,     0,     0,     0,     0,
       0,    42,    43,     0,     0,    44,    34,    35,    36,     0,
       0,     3,     4,     5,     6,     7,     8,     9,    37,    38,
      39,     0,    40,    34,    35,    36,   168,     0,     3,     4,
       5,     6,     7,     8,     9,    37,    38,    39,     0,    40,
       0,     0,    41,     0,     0,     0,     0,     0,     0,    42,
      43,     0,     0,    44,     0,     0,     0,     0,     0,    41,
       0,     0,     0,     0,     0,     0,    42,    43,     0,     0,
      44,    34,    74,    36,     0,     0,     3,     4,     5,     6,
       7,     8,     9,    37,    38,    39,     0,    40,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,    41,     0,     0,
       0,     0,     0,     0,    42,    43,     0,     0,    44
};

static const yytype_int16 yycheck[] =
{
       0,     0,    40,    41,    29,    27,    20,    21,    31,    32,
      51,    11,    11,    54,    14,    15,     0,    51,     4,    44,
      54,    22,    22,     4,   100,    23,    40,    51,    28,    23,
      54,     4,    32,     4,     8,     9,    10,    11,    12,    13,
      14,    37,    64,    65,    66,    67,    68,    69,    73,    50,
      72,    52,   128,    91,    92,    93,     3,     4,     5,   135,
      22,     8,     9,    10,    11,    12,    13,    14,    15,    16,
      17,   147,    19,    48,    18,    19,    98,    51,     8,     9,
      10,    11,    12,    13,    14,   187,   188,   112,    50,     3,
      52,     4,    39,   195,   196,     3,     4,    97,   200,    46,
      47,    46,    47,    50,    38,    50,    53,    52,   130,    89,
      90,   133,    53,    54,    53,   137,   141,   142,    83,    84,
     145,    51,    33,    34,    35,    36,    22,   127,    85,    86,
      87,    88,    48,    51,   134,    22,    48,    22,    48,   161,
       4,   166,   167,    50,     6,   170,     8,     9,    10,    11,
      12,    13,    14,   153,   179,    22,    53,    50,    53,   159,
      50,   186,    23,    23,   189,    23,   166,   166,    23,   194,
      23,   171,    51,    51,    41,    42,    43,    44,    45,    46,
      47,    23,    23,    50,    30,    52,    51,   187,   188,    14,
      13,    82,    51,    32,   130,   195,   196,     3,     4,     5,
     200,    81,     8,     9,    10,    11,    12,    13,    14,    15,
      16,    17,   133,    19,     3,     4,     5,    -1,    -1,     8,
       9,    10,    11,    12,    13,    14,    15,    16,    17,    -1,
      19,    -1,    -1,    39,    -1,    -1,    -1,    -1,    -1,    -1,
      46,    47,    -1,    -1,    50,    -1,    -1,    53,    -1,    -1,
      39,    -1,    -1,    -1,    -1,    -1,    -1,    46,    47,    -1,
      -1,    50,    -1,    52,     3,     4,     5,    -1,    -1,     8,
       9,    10,    11,    12,    13,    14,    15,    16,    17,    -1,
      19,     6,     7,     8,     9,    10,    11,    12,    13,    14,
       8,     9,    10,    11,    12,    13,    14,    -1,    -1,    -1,
      39,    -1,    -1,    -1,    -1,    -1,    -1,    46,    47,    -1,
      -1,    50,    51,     3,     4,     5,     6,     7,     8,     9,
      10,    11,    12,    13,    14,    15,    16,    17,    -1,    19,
      -1,    -1,    -1,    23,    24,    25,    26,    27,    28,    29,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    39,
      -1,    -1,    -1,    -1,    -1,    -1,    46,    47,    48,    49,
      50,     3,     4,     5,     6,     7,     8,     9,    10,    11,
      12,    13,    14,    15,    16,    17,    -1,    19,    -1,    -1,
      -1,    23,    24,    25,    26,    27,    28,    29,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    39,    -1,    -1,
      -1,    -1,    -1,    -1,    46,    47,    48,    49,    50,     3,
       4,     5,     6,     7,     8,     9,    10,    11,    12,    13,
      14,    15,    16,    17,    -1,    19,    -1,    -1,    -1,    23,
      24,    25,    26,    27,    28,    29,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    39,    -1,    -1,    -1,    -1,
      -1,    -1,    46,    47,    48,    49,    50,     3,     4,     5,
       6,     7,     8,     9,    10,    11,    12,    13,    14,    15,
      16,    17,    -1,    19,    -1,    -1,    -1,    23,    24,    25,
      26,    27,    28,    29,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    39,    -1,    -1,    -1,    -1,    -1,    -1,
      46,    47,    48,    49,    50,     3,     4,     5,     6,     7,
       8,     9,    10,    11,    12,    13,    14,    15,    16,    17,
      -1,    19,    -1,    -1,    -1,    23,    24,    25,    26,    27,
      28,    29,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    39,    -1,    -1,    -1,    -1,    -1,    -1,    46,    47,
      48,    49,    50,     3,     4,     5,     6,     7,     8,     9,
      10,    11,    12,    13,    14,    15,    16,    17,    -1,    19,
      -1,    -1,    -1,    23,    24,    25,    26,    27,    28,    29,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    39,
      -1,    -1,    -1,    -1,    -1,    -1,    46,    47,    48,    -1,
      50,     3,     4,     5,     6,     7,     8,     9,    10,    11,
      12,    13,    14,    15,    16,    17,    -1,    19,     3,     4,
       5,    -1,    -1,     8,     9,    10,    11,    12,    13,    14,
      15,    16,    17,    -1,    19,    -1,    -1,    39,    23,    -1,
      -1,    -1,    -1,    -1,    46,    47,    -1,    -1,    50,    -1,
      -1,    -1,    -1,    -1,    39,    -1,    -1,    -1,    -1,    -1,
      -1,    46,    47,    -1,    -1,    50,     3,     4,     5,    -1,
      -1,     8,     9,    10,    11,    12,    13,    14,    15,    16,
      17,    -1,    19,     3,     4,     5,    23,    -1,     8,     9,
      10,    11,    12,    13,    14,    15,    16,    17,    -1,    19,
      -1,    -1,    39,    -1,    -1,    -1,    -1,    -1,    -1,    46,
      47,    -1,    -1,    50,    -1,    -1,    -1,    -1,    -1,    39,
      -1,    -1,    -1,    -1,    -1,    -1,    46,    47,    -1,    -1,
      50,     3,     4,     5,    -1,    -1,     8,     9,    10,    11,
      12,    13,    14,    15,    16,    17,    -1,    19,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    39,    -1,    -1,
      -1,    -1,    -1,    -1,    46,    47,    -1,    -1,    50
};

/* YYSTOS[STATE-NUM] -- The (internal number of the) accessing
   symbol of state STATE-NUM.  */
static const yytype_uint8 yystos[] =
{
       0,     6,     7,     8,     9,    10,    11,    12,    13,    14,
      56,    57,    58,    59,    60,    61,    62,    64,     0,    58,
       4,    63,    61,    62,    62,     4,    63,    22,    50,    52,
      23,    62,    50,    23,     3,     4,     5,    15,    16,    17,
      19,    39,    46,    47,    50,    62,    69,    70,    71,    72,
      73,    74,    75,    76,    77,    78,    51,    62,    65,    53,
      68,    69,    51,    65,    22,    41,    42,    43,    44,    45,
      46,    47,    50,    52,     4,    76,    76,     4,     4,    68,
       3,    37,    38,    31,    32,    33,    34,    35,    36,    18,
      19,    20,    21,    40,    48,     4,    51,    54,    22,    53,
      48,    51,    69,    69,    69,    69,    69,    69,    51,    69,
      79,    68,    52,    51,    71,    72,    73,    73,    74,    74,
      74,    74,    75,    75,    76,    76,    76,    66,    48,    62,
      52,    69,    80,    22,    66,    48,    51,    54,    53,    68,
      23,    24,    25,    26,    27,    28,    29,    48,    49,    58,
      62,    67,    68,    66,     4,    53,    79,    80,    49,    66,
      69,    22,    53,    68,    23,    68,    50,    50,    23,    68,
      50,    66,    23,    49,    53,    49,    69,    23,    23,    58,
      68,    68,    23,    68,    49,    68,    23,    51,    51,    23,
      68,    67,    67,    68,    23,    30,    51,    68,    67,    67,
      51,    67
};

#define yyerrok		(yyerrstatus = 0)
#define yyclearin	(yychar = YYEMPTY)
#define YYEMPTY		(-2)
#define YYEOF		0

#define YYACCEPT	goto yyacceptlab
#define YYABORT		goto yyabortlab
#define YYERROR		goto yyerrorlab


/* Like YYERROR except do call yyerror.  This remains here temporarily
   to ease the transition to the new meaning of YYERROR, for GCC.
   Once GCC version 2 has supplanted version 1, this can go.  */

#define YYFAIL		goto yyerrlab

#define YYRECOVERING()  (!!yyerrstatus)

#define YYBACKUP(Token, Value)					\
do								\
  if (yychar == YYEMPTY && yylen == 1)				\
    {								\
      yychar = (Token);						\
      yylval = (Value);						\
      yytoken = YYTRANSLATE (yychar);				\
      YYPOPSTACK (1);						\
      goto yybackup;						\
    }								\
  else								\
    {								\
      yyerror (YY_("syntax error: cannot back up")); \
      YYERROR;							\
    }								\
while (YYID (0))


#define YYTERROR	1
#define YYERRCODE	256


/* YYLLOC_DEFAULT -- Set CURRENT to span from RHS[1] to RHS[N].
   If N is 0, then set CURRENT to the empty location which ends
   the previous symbol: RHS[0] (always defined).  */

#define YYRHSLOC(Rhs, K) ((Rhs)[K])
#ifndef YYLLOC_DEFAULT
# define YYLLOC_DEFAULT(Current, Rhs, N)				\
    do									\
      if (YYID (N))                                                    \
	{								\
	  (Current).first_line   = YYRHSLOC (Rhs, 1).first_line;	\
	  (Current).first_column = YYRHSLOC (Rhs, 1).first_column;	\
	  (Current).last_line    = YYRHSLOC (Rhs, N).last_line;		\
	  (Current).last_column  = YYRHSLOC (Rhs, N).last_column;	\
	}								\
      else								\
	{								\
	  (Current).first_line   = (Current).last_line   =		\
	    YYRHSLOC (Rhs, 0).last_line;				\
	  (Current).first_column = (Current).last_column =		\
	    YYRHSLOC (Rhs, 0).last_column;				\
	}								\
    while (YYID (0))
#endif


/* YY_LOCATION_PRINT -- Print the location on the stream.
   This macro was not mandated originally: define only if we know
   we won't break user code: when these are the locations we know.  */

#ifndef YY_LOCATION_PRINT
# if defined YYLTYPE_IS_TRIVIAL && YYLTYPE_IS_TRIVIAL
#  define YY_LOCATION_PRINT(File, Loc)			\
     fprintf (File, "%d.%d-%d.%d",			\
	      (Loc).first_line, (Loc).first_column,	\
	      (Loc).last_line,  (Loc).last_column)
# else
#  define YY_LOCATION_PRINT(File, Loc) ((void) 0)
# endif
#endif


/* YYLEX -- calling `yylex' with the right arguments.  */

#ifdef YYLEX_PARAM
# define YYLEX yylex (YYLEX_PARAM)
#else
# define YYLEX yylex ()
#endif

/* Enable debugging if requested.  */
#if YYDEBUG

# ifndef YYFPRINTF
#  include <stdio.h> /* INFRINGES ON USER NAME SPACE */
#  define YYFPRINTF fprintf
# endif

# define YYDPRINTF(Args)			\
do {						\
  if (yydebug)					\
    YYFPRINTF Args;				\
} while (YYID (0))

# define YY_SYMBOL_PRINT(Title, Type, Value, Location)			  \
do {									  \
  if (yydebug)								  \
    {									  \
      YYFPRINTF (stderr, "%s ", Title);					  \
      yy_symbol_print (stderr,						  \
		  Type, Value); \
      YYFPRINTF (stderr, "\n");						  \
    }									  \
} while (YYID (0))


/*--------------------------------.
| Print this symbol on YYOUTPUT.  |
`--------------------------------*/

/*ARGSUSED*/
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yy_symbol_value_print (FILE *yyoutput, int yytype, YYSTYPE const * const yyvaluep)
#else
static void
yy_symbol_value_print (yyoutput, yytype, yyvaluep)
    FILE *yyoutput;
    int yytype;
    YYSTYPE const * const yyvaluep;
#endif
{
  if (!yyvaluep)
    return;
# ifdef YYPRINT
  if (yytype < YYNTOKENS)
    YYPRINT (yyoutput, yytoknum[yytype], *yyvaluep);
# else
  YYUSE (yyoutput);
# endif
  switch (yytype)
    {
      default:
	break;
    }
}


/*--------------------------------.
| Print this symbol on YYOUTPUT.  |
`--------------------------------*/

#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yy_symbol_print (FILE *yyoutput, int yytype, YYSTYPE const * const yyvaluep)
#else
static void
yy_symbol_print (yyoutput, yytype, yyvaluep)
    FILE *yyoutput;
    int yytype;
    YYSTYPE const * const yyvaluep;
#endif
{
  if (yytype < YYNTOKENS)
    YYFPRINTF (yyoutput, "token %s (", yytname[yytype]);
  else
    YYFPRINTF (yyoutput, "nterm %s (", yytname[yytype]);

  yy_symbol_value_print (yyoutput, yytype, yyvaluep);
  YYFPRINTF (yyoutput, ")");
}

/*------------------------------------------------------------------.
| yy_stack_print -- Print the state stack from its BOTTOM up to its |
| TOP (included).                                                   |
`------------------------------------------------------------------*/

#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yy_stack_print (yytype_int16 *bottom, yytype_int16 *top)
#else
static void
yy_stack_print (bottom, top)
    yytype_int16 *bottom;
    yytype_int16 *top;
#endif
{
  YYFPRINTF (stderr, "Stack now");
  for (; bottom <= top; ++bottom)
    YYFPRINTF (stderr, " %d", *bottom);
  YYFPRINTF (stderr, "\n");
}

# define YY_STACK_PRINT(Bottom, Top)				\
do {								\
  if (yydebug)							\
    yy_stack_print ((Bottom), (Top));				\
} while (YYID (0))


/*------------------------------------------------.
| Report that the YYRULE is going to be reduced.  |
`------------------------------------------------*/

#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yy_reduce_print (YYSTYPE *yyvsp, int yyrule)
#else
static void
yy_reduce_print (yyvsp, yyrule)
    YYSTYPE *yyvsp;
    int yyrule;
#endif
{
  int yynrhs = yyr2[yyrule];
  int yyi;
  unsigned long int yylno = yyrline[yyrule];
  YYFPRINTF (stderr, "Reducing stack by rule %d (line %lu):\n",
	     yyrule - 1, yylno);
  /* The symbols being reduced.  */
  for (yyi = 0; yyi < yynrhs; yyi++)
    {
      fprintf (stderr, "   $%d = ", yyi + 1);
      yy_symbol_print (stderr, yyrhs[yyprhs[yyrule] + yyi],
		       &(yyvsp[(yyi + 1) - (yynrhs)])
		       		       );
      fprintf (stderr, "\n");
    }
}

# define YY_REDUCE_PRINT(Rule)		\
do {					\
  if (yydebug)				\
    yy_reduce_print (yyvsp, Rule); \
} while (YYID (0))

/* Nonzero means print parse trace.  It is left uninitialized so that
   multiple parsers can coexist.  */
int yydebug;
#else /* !YYDEBUG */
# define YYDPRINTF(Args)
# define YY_SYMBOL_PRINT(Title, Type, Value, Location)
# define YY_STACK_PRINT(Bottom, Top)
# define YY_REDUCE_PRINT(Rule)
#endif /* !YYDEBUG */


/* YYINITDEPTH -- initial size of the parser's stacks.  */
#ifndef	YYINITDEPTH
# define YYINITDEPTH 200
#endif

/* YYMAXDEPTH -- maximum size the stacks can grow to (effective only
   if the built-in stack extension method is used).

   Do not make this value too large; the results are undefined if
   YYSTACK_ALLOC_MAXIMUM < YYSTACK_BYTES (YYMAXDEPTH)
   evaluated with infinite-precision integer arithmetic.  */

#ifndef YYMAXDEPTH
# define YYMAXDEPTH 10000
#endif



#if YYERROR_VERBOSE

# ifndef yystrlen
#  if defined __GLIBC__ && defined _STRING_H
#   define yystrlen strlen
#  else
/* Return the length of YYSTR.  */
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static YYSIZE_T
yystrlen (const char *yystr)
#else
static YYSIZE_T
yystrlen (yystr)
    const char *yystr;
#endif
{
  YYSIZE_T yylen;
  for (yylen = 0; yystr[yylen]; yylen++)
    continue;
  return yylen;
}
#  endif
# endif

# ifndef yystpcpy
#  if defined __GLIBC__ && defined _STRING_H && defined _GNU_SOURCE
#   define yystpcpy stpcpy
#  else
/* Copy YYSRC to YYDEST, returning the address of the terminating '\0' in
   YYDEST.  */
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static char *
yystpcpy (char *yydest, const char *yysrc)
#else
static char *
yystpcpy (yydest, yysrc)
    char *yydest;
    const char *yysrc;
#endif
{
  char *yyd = yydest;
  const char *yys = yysrc;

  while ((*yyd++ = *yys++) != '\0')
    continue;

  return yyd - 1;
}
#  endif
# endif

# ifndef yytnamerr
/* Copy to YYRES the contents of YYSTR after stripping away unnecessary
   quotes and backslashes, so that it's suitable for yyerror.  The
   heuristic is that double-quoting is unnecessary unless the string
   contains an apostrophe, a comma, or backslash (other than
   backslash-backslash).  YYSTR is taken from yytname.  If YYRES is
   null, do not copy; instead, return the length of what the result
   would have been.  */
static YYSIZE_T
yytnamerr (char *yyres, const char *yystr)
{
  if (*yystr == '"')
    {
      YYSIZE_T yyn = 0;
      char const *yyp = yystr;

      for (;;)
	switch (*++yyp)
	  {
	  case '\'':
	  case ',':
	    goto do_not_strip_quotes;

	  case '\\':
	    if (*++yyp != '\\')
	      goto do_not_strip_quotes;
	    /* Fall through.  */
	  default:
	    if (yyres)
	      yyres[yyn] = *yyp;
	    yyn++;
	    break;

	  case '"':
	    if (yyres)
	      yyres[yyn] = '\0';
	    return yyn;
	  }
    do_not_strip_quotes: ;
    }

  if (! yyres)
    return yystrlen (yystr);

  return yystpcpy (yyres, yystr) - yyres;
}
# endif

/* Copy into YYRESULT an error message about the unexpected token
   YYCHAR while in state YYSTATE.  Return the number of bytes copied,
   including the terminating null byte.  If YYRESULT is null, do not
   copy anything; just return the number of bytes that would be
   copied.  As a special case, return 0 if an ordinary "syntax error"
   message will do.  Return YYSIZE_MAXIMUM if overflow occurs during
   size calculation.  */
static YYSIZE_T
yysyntax_error (char *yyresult, int yystate, int yychar)
{
  int yyn = yypact[yystate];

  if (! (YYPACT_NINF < yyn && yyn <= YYLAST))
    return 0;
  else
    {
      int yytype = YYTRANSLATE (yychar);
      YYSIZE_T yysize0 = yytnamerr (0, yytname[yytype]);
      YYSIZE_T yysize = yysize0;
      YYSIZE_T yysize1;
      int yysize_overflow = 0;
      enum { YYERROR_VERBOSE_ARGS_MAXIMUM = 5 };
      char const *yyarg[YYERROR_VERBOSE_ARGS_MAXIMUM];
      int yyx;

# if 0
      /* This is so xgettext sees the translatable formats that are
	 constructed on the fly.  */
      YY_("syntax error, unexpected %s");
      YY_("syntax error, unexpected %s, expecting %s");
      YY_("syntax error, unexpected %s, expecting %s or %s");
      YY_("syntax error, unexpected %s, expecting %s or %s or %s");
      YY_("syntax error, unexpected %s, expecting %s or %s or %s or %s");
# endif
      char *yyfmt;
      char const *yyf;
      static char const yyunexpected[] = "syntax error, unexpected %s";
      static char const yyexpecting[] = ", expecting %s";
      static char const yyor[] = " or %s";
      char yyformat[sizeof yyunexpected
		    + sizeof yyexpecting - 1
		    + ((YYERROR_VERBOSE_ARGS_MAXIMUM - 2)
		       * (sizeof yyor - 1))];
      char const *yyprefix = yyexpecting;

      /* Start YYX at -YYN if negative to avoid negative indexes in
	 YYCHECK.  */
      int yyxbegin = yyn < 0 ? -yyn : 0;

      /* Stay within bounds of both yycheck and yytname.  */
      int yychecklim = YYLAST - yyn + 1;
      int yyxend = yychecklim < YYNTOKENS ? yychecklim : YYNTOKENS;
      int yycount = 1;

      yyarg[0] = yytname[yytype];
      yyfmt = yystpcpy (yyformat, yyunexpected);

      for (yyx = yyxbegin; yyx < yyxend; ++yyx)
	if (yycheck[yyx + yyn] == yyx && yyx != YYTERROR)
	  {
	    if (yycount == YYERROR_VERBOSE_ARGS_MAXIMUM)
	      {
		yycount = 1;
		yysize = yysize0;
		yyformat[sizeof yyunexpected - 1] = '\0';
		break;
	      }
	    yyarg[yycount++] = yytname[yyx];
	    yysize1 = yysize + yytnamerr (0, yytname[yyx]);
	    yysize_overflow |= (yysize1 < yysize);
	    yysize = yysize1;
	    yyfmt = yystpcpy (yyfmt, yyprefix);
	    yyprefix = yyor;
	  }

      yyf = YY_(yyformat);
      yysize1 = yysize + yystrlen (yyf);
      yysize_overflow |= (yysize1 < yysize);
      yysize = yysize1;

      if (yysize_overflow)
	return YYSIZE_MAXIMUM;

      if (yyresult)
	{
	  /* Avoid sprintf, as that infringes on the user's name space.
	     Don't have undefined behavior even if the translation
	     produced a string with the wrong number of "%s"s.  */
	  char *yyp = yyresult;
	  int yyi = 0;
	  while ((*yyp = *yyf) != '\0')
	    {
	      if (*yyp == '%' && yyf[1] == 's' && yyi < yycount)
		{
		  yyp += yytnamerr (yyp, yyarg[yyi++]);
		  yyf += 2;
		}
	      else
		{
		  yyp++;
		  yyf++;
		}
	    }
	}
      return yysize;
    }
}
#endif /* YYERROR_VERBOSE */


/*-----------------------------------------------.
| Release the memory associated to this symbol.  |
`-----------------------------------------------*/

/*ARGSUSED*/
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yydestruct (const char *yymsg, int yytype, YYSTYPE *yyvaluep)
#else
static void
yydestruct (yymsg, yytype, yyvaluep)
    const char *yymsg;
    int yytype;
    YYSTYPE *yyvaluep;
#endif
{
  YYUSE (yyvaluep);

  if (!yymsg)
    yymsg = "Deleting";
  YY_SYMBOL_PRINT (yymsg, yytype, yyvaluep, yylocationp);

  switch (yytype)
    {

      default:
	break;
    }
}


/* Prevent warnings from -Wmissing-prototypes.  */

#ifdef YYPARSE_PARAM
#if defined __STDC__ || defined __cplusplus
int yyparse (void *YYPARSE_PARAM);
#else
int yyparse ();
#endif
#else /* ! YYPARSE_PARAM */
#if defined __STDC__ || defined __cplusplus
int yyparse (void);
#else
int yyparse ();
#endif
#endif /* ! YYPARSE_PARAM */



/* The look-ahead symbol.  */
int yychar;

/* The semantic value of the look-ahead symbol.  */
YYSTYPE yylval;

/* Number of syntax errors so far.  */
int yynerrs;



/*----------.
| yyparse.  |
`----------*/

#ifdef YYPARSE_PARAM
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
int
yyparse (void *YYPARSE_PARAM)
#else
int
yyparse (YYPARSE_PARAM)
    void *YYPARSE_PARAM;
#endif
#else /* ! YYPARSE_PARAM */
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
int
yyparse (void)
#else
int
yyparse ()

#endif
#endif
{
  
  int yystate;
  int yyn;
  int yyresult;
  /* Number of tokens to shift before error messages enabled.  */
  int yyerrstatus;
  /* Look-ahead token as an internal (translated) token number.  */
  int yytoken = 0;
#if YYERROR_VERBOSE
  /* Buffer for error messages, and its allocated size.  */
  char yymsgbuf[128];
  char *yymsg = yymsgbuf;
  YYSIZE_T yymsg_alloc = sizeof yymsgbuf;
#endif

  /* Three stacks and their tools:
     `yyss': related to states,
     `yyvs': related to semantic values,
     `yyls': related to locations.

     Refer to the stacks thru separate pointers, to allow yyoverflow
     to reallocate them elsewhere.  */

  /* The state stack.  */
  yytype_int16 yyssa[YYINITDEPTH];
  yytype_int16 *yyss = yyssa;
  yytype_int16 *yyssp;

  /* The semantic value stack.  */
  YYSTYPE yyvsa[YYINITDEPTH];
  YYSTYPE *yyvs = yyvsa;
  YYSTYPE *yyvsp;



#define YYPOPSTACK(N)   (yyvsp -= (N), yyssp -= (N))

  YYSIZE_T yystacksize = YYINITDEPTH;

  /* The variables used to return semantic value and location from the
     action routines.  */
  YYSTYPE yyval;


  /* The number of symbols on the RHS of the reduced rule.
     Keep to zero when no symbol should be popped.  */
  int yylen = 0;

  YYDPRINTF ((stderr, "Starting parse\n"));

  yystate = 0;
  yyerrstatus = 0;
  yynerrs = 0;
  yychar = YYEMPTY;		/* Cause a token to be read.  */

  /* Initialize stack pointers.
     Waste one element of value and location stack
     so that they stay on the same level as the state stack.
     The wasted elements are never initialized.  */

  yyssp = yyss;
  yyvsp = yyvs;

  goto yysetstate;

/*------------------------------------------------------------.
| yynewstate -- Push a new state, which is found in yystate.  |
`------------------------------------------------------------*/
 yynewstate:
  /* In all cases, when you get here, the value and location stacks
     have just been pushed.  So pushing a state here evens the stacks.  */
  yyssp++;

 yysetstate:
  *yyssp = yystate;

  if (yyss + yystacksize - 1 <= yyssp)
    {
      /* Get the current used size of the three stacks, in elements.  */
      YYSIZE_T yysize = yyssp - yyss + 1;

#ifdef yyoverflow
      {
	/* Give user a chance to reallocate the stack.  Use copies of
	   these so that the &'s don't force the real ones into
	   memory.  */
	YYSTYPE *yyvs1 = yyvs;
	yytype_int16 *yyss1 = yyss;


	/* Each stack pointer address is followed by the size of the
	   data in use in that stack, in bytes.  This used to be a
	   conditional around just the two extra args, but that might
	   be undefined if yyoverflow is a macro.  */
	yyoverflow (YY_("memory exhausted"),
		    &yyss1, yysize * sizeof (*yyssp),
		    &yyvs1, yysize * sizeof (*yyvsp),

		    &yystacksize);

	yyss = yyss1;
	yyvs = yyvs1;
      }
#else /* no yyoverflow */
# ifndef YYSTACK_RELOCATE
      goto yyexhaustedlab;
# else
      /* Extend the stack our own way.  */
      if (YYMAXDEPTH <= yystacksize)
	goto yyexhaustedlab;
      yystacksize *= 2;
      if (YYMAXDEPTH < yystacksize)
	yystacksize = YYMAXDEPTH;

      {
	yytype_int16 *yyss1 = yyss;
	union yyalloc *yyptr =
	  (union yyalloc *) YYSTACK_ALLOC (YYSTACK_BYTES (yystacksize));
	if (! yyptr)
	  goto yyexhaustedlab;
	YYSTACK_RELOCATE (yyss);
	YYSTACK_RELOCATE (yyvs);

#  undef YYSTACK_RELOCATE
	if (yyss1 != yyssa)
	  YYSTACK_FREE (yyss1);
      }
# endif
#endif /* no yyoverflow */

      yyssp = yyss + yysize - 1;
      yyvsp = yyvs + yysize - 1;


      YYDPRINTF ((stderr, "Stack size increased to %lu\n",
		  (unsigned long int) yystacksize));

      if (yyss + yystacksize - 1 <= yyssp)
	YYABORT;
    }

  YYDPRINTF ((stderr, "Entering state %d\n", yystate));

  goto yybackup;

/*-----------.
| yybackup.  |
`-----------*/
yybackup:

  /* Do appropriate processing given the current state.  Read a
     look-ahead token if we need one and don't already have one.  */

  /* First try to decide what to do without reference to look-ahead token.  */
  yyn = yypact[yystate];
  if (yyn == YYPACT_NINF)
    goto yydefault;

  /* Not known => get a look-ahead token if don't already have one.  */

  /* YYCHAR is either YYEMPTY or YYEOF or a valid look-ahead symbol.  */
  if (yychar == YYEMPTY)
    {
      YYDPRINTF ((stderr, "Reading a token: "));
      yychar = YYLEX;
    }

  if (yychar <= YYEOF)
    {
      yychar = yytoken = YYEOF;
      YYDPRINTF ((stderr, "Now at end of input.\n"));
    }
  else
    {
      yytoken = YYTRANSLATE (yychar);
      YY_SYMBOL_PRINT ("Next token is", yytoken, &yylval, &yylloc);
    }

  /* If the proper action on seeing token YYTOKEN is to reduce or to
     detect an error, take that action.  */
  yyn += yytoken;
  if (yyn < 0 || YYLAST < yyn || yycheck[yyn] != yytoken)
    goto yydefault;
  yyn = yytable[yyn];
  if (yyn <= 0)
    {
      if (yyn == 0 || yyn == YYTABLE_NINF)
	goto yyerrlab;
      yyn = -yyn;
      goto yyreduce;
    }

  if (yyn == YYFINAL)
    YYACCEPT;

  /* Count tokens shifted since error; after three, turn off error
     status.  */
  if (yyerrstatus)
    yyerrstatus--;

  /* Shift the look-ahead token.  */
  YY_SYMBOL_PRINT ("Shifting", yytoken, &yylval, &yylloc);

  /* Discard the shifted token unless it is eof.  */
  if (yychar != YYEOF)
    yychar = YYEMPTY;

  yystate = yyn;
  *++yyvsp = yylval;

  goto yynewstate;


/*-----------------------------------------------------------.
| yydefault -- do the default action for the current state.  |
`-----------------------------------------------------------*/
yydefault:
  yyn = yydefact[yystate];
  if (yyn == 0)
    goto yyerrlab;
  goto yyreduce;


/*-----------------------------.
| yyreduce -- Do a reduction.  |
`-----------------------------*/
yyreduce:
  /* yyn is the number of a rule to reduce with.  */
  yylen = yyr2[yyn];

  /* If YYLEN is nonzero, implement the default value of the action:
     `$$ = $1'.

     Otherwise, the following line sets YYVAL to garbage.
     This behavior is undocumented and Bison
     users should not rely upon it.  Assigning to YYVAL
     unconditionally makes the parser a bit smaller, and it avoids a
     GCC warning that YYVAL may be used uninitialized.  */
  yyval = yyvsp[1-yylen];


  YY_REDUCE_PRINT (yyn);
  switch (yyn)
    {
        case 2:
#line 54 "src/frontend/parser.y"
    {
        (yyval.ptr) = (yyvsp[(1) - (1)].ptr);
        root_node = std::unique_ptr<ASTNode>((ASTNode*)(yyval.ptr));
    ;}
    break;

  case 3:
#line 61 "src/frontend/parser.y"
    {
        ASTNode* list = create_stmt_list();
        add_statement(list, (ASTNode*)(yyvsp[(1) - (1)].ptr));
        (yyval.ptr) = list;
    ;}
    break;

  case 4:
#line 66 "src/frontend/parser.y"
    {
        add_statement((ASTNode*)(yyvsp[(1) - (2)].ptr), (ASTNode*)(yyvsp[(2) - (2)].ptr));
        (yyval.ptr) = (yyvsp[(1) - (2)].ptr);
    ;}
    break;

  case 5:
#line 73 "src/frontend/parser.y"
    { (yyval.ptr) = (yyvsp[(1) - (1)].ptr); ;}
    break;

  case 6:
#line 74 "src/frontend/parser.y"
    {
        ASTNode* decl = (ASTNode*)(yyvsp[(2) - (3)].ptr);
        set_declaration_attributes(decl, (ASTNode*)(yyvsp[(1) - (3)].ptr), nullptr);
        delete_node((ASTNode*)(yyvsp[(1) - (3)].ptr));
        (yyval.ptr) = decl;
    ;}
    break;

  case 7:
#line 80 "src/frontend/parser.y"
    {
        ASTNode* decl = (ASTNode*)(yyvsp[(2) - (3)].ptr);
        ASTNode* decl_spec = create_decl_spec(nullptr, nullptr, (ASTNode*)(yyvsp[(1) - (3)].ptr));
        set_declaration_attributes(decl, decl_spec, nullptr);
        delete_node(decl_spec);
        (yyval.ptr) = decl;
    ;}
    break;

  case 8:
#line 90 "src/frontend/parser.y"
    { (yyval.ptr) = (yyvsp[(1) - (1)].ptr); ;}
    break;

  case 9:
#line 91 "src/frontend/parser.y"
    { (yyval.ptr) = (yyvsp[(1) - (1)].ptr); ;}
    break;

  case 10:
#line 92 "src/frontend/parser.y"
    { (yyval.ptr) = create_decl_spec(nullptr, nullptr, (ASTNode*)(yyvsp[(1) - (1)].ptr)); ;}
    break;

  case 11:
#line 93 "src/frontend/parser.y"
    {
        (yyval.ptr) = create_decl_spec((ASTNode*)(yyvsp[(1) - (3)].ptr), (ASTNode*)(yyvsp[(2) - (3)].ptr), (ASTNode*)(yyvsp[(3) - (3)].ptr));
        delete_node((ASTNode*)(yyvsp[(1) - (3)].ptr));
        delete_node((ASTNode*)(yyvsp[(2) - (3)].ptr));
    ;}
    break;

  case 12:
#line 98 "src/frontend/parser.y"
    {
        (yyval.ptr) = create_decl_spec((ASTNode*)(yyvsp[(1) - (2)].ptr), nullptr, (ASTNode*)(yyvsp[(2) - (2)].ptr));
        delete_node((ASTNode*)(yyvsp[(1) - (2)].ptr));
    ;}
    break;

  case 13:
#line 102 "src/frontend/parser.y"
    {
        (yyval.ptr) = create_decl_spec(nullptr, (ASTNode*)(yyvsp[(1) - (2)].ptr), (ASTNode*)(yyvsp[(2) - (2)].ptr));
        delete_node((ASTNode*)(yyvsp[(1) - (2)].ptr));
    ;}
    break;

  case 14:
#line 109 "src/frontend/parser.y"
    { (yyval.ptr) = create_storage_spec(true, false); ;}
    break;

  case 15:
#line 113 "src/frontend/parser.y"
    { (yyval.ptr) = create_storage_spec(false, true); ;}
    break;

  case 16:
#line 117 "src/frontend/parser.y"
    { (yyval.ptr) = create_type_node(TYPE_VOID); ;}
    break;

  case 17:
#line 118 "src/frontend/parser.y"
    { (yyval.ptr) = create_type_node(TYPE_TINY); ;}
    break;

  case 18:
#line 119 "src/frontend/parser.y"
    { (yyval.ptr) = create_type_node(TYPE_SHORT); ;}
    break;

  case 19:
#line 120 "src/frontend/parser.y"
    { (yyval.ptr) = create_type_node(TYPE_INT); ;}
    break;

  case 20:
#line 121 "src/frontend/parser.y"
    { (yyval.ptr) = create_type_node(TYPE_LONG); ;}
    break;

  case 21:
#line 122 "src/frontend/parser.y"
    { (yyval.ptr) = create_type_node(TYPE_STRING); ;}
    break;

  case 22:
#line 123 "src/frontend/parser.y"
    { (yyval.ptr) = create_type_node(TYPE_BOOL); ;}
    break;

  case 23:
#line 127 "src/frontend/parser.y"
    {
        (yyval.ptr) = create_var_decl((yyvsp[(1) - (1)].sval));
        free((yyvsp[(1) - (1)].sval));
    ;}
    break;

  case 24:
#line 131 "src/frontend/parser.y"
    {
        (yyval.ptr) = create_var_init((yyvsp[(1) - (3)].sval), (ASTNode*)(yyvsp[(3) - (3)].ptr));
        free((yyvsp[(1) - (3)].sval));
    ;}
    break;

  case 25:
#line 135 "src/frontend/parser.y"
    {
        (yyval.ptr) = create_array_decl((yyvsp[(1) - (4)].sval), (ASTNode*)(yyvsp[(3) - (4)].ptr));
        free((yyvsp[(1) - (4)].sval));
    ;}
    break;

  case 26:
#line 139 "src/frontend/parser.y"
    {
        (yyval.ptr) = create_array_init((yyvsp[(1) - (5)].sval), (ASTNode*)(yyvsp[(5) - (5)].ptr));
        free((yyvsp[(1) - (5)].sval));
    ;}
    break;

  case 27:
#line 143 "src/frontend/parser.y"
    {
        (yyval.ptr) = create_array_init_with_size((yyvsp[(1) - (6)].sval), (ASTNode*)(yyvsp[(3) - (6)].ptr), (ASTNode*)(yyvsp[(6) - (6)].ptr));
        free((yyvsp[(1) - (6)].sval));
    ;}
    break;

  case 28:
#line 150 "src/frontend/parser.y"
    {
        (yyval.ptr) = create_function_def((yyvsp[(2) - (8)].sval), (ASTNode*)(yyvsp[(1) - (8)].ptr), nullptr, (ASTNode*)(yyvsp[(4) - (8)].ptr), (ASTNode*)(yyvsp[(7) - (8)].ptr));
        delete_node((ASTNode*)(yyvsp[(1) - (8)].ptr));
        // delete_node((ASTNode*)$4); // create_function_def内でparamsが削除される
        free((yyvsp[(2) - (8)].sval));
    ;}
    break;

  case 29:
#line 156 "src/frontend/parser.y"
    {
        (yyval.ptr) = create_function_def((yyvsp[(2) - (7)].sval), (ASTNode*)(yyvsp[(1) - (7)].ptr), nullptr, nullptr, (ASTNode*)(yyvsp[(6) - (7)].ptr));
        delete_node((ASTNode*)(yyvsp[(1) - (7)].ptr));
        free((yyvsp[(2) - (7)].sval));
    ;}
    break;

  case 30:
#line 161 "src/frontend/parser.y"
    {
        ASTNode* decl_spec = create_decl_spec(nullptr, nullptr, (ASTNode*)(yyvsp[(1) - (8)].ptr));
        (yyval.ptr) = create_function_def((yyvsp[(2) - (8)].sval), decl_spec, nullptr, (ASTNode*)(yyvsp[(4) - (8)].ptr), (ASTNode*)(yyvsp[(7) - (8)].ptr));
        delete_node(decl_spec);
        // delete_node((ASTNode*)$4); // create_function_def内でdeleteされる
        free((yyvsp[(2) - (8)].sval));
    ;}
    break;

  case 31:
#line 168 "src/frontend/parser.y"
    {
        ASTNode* decl_spec = create_decl_spec(nullptr, nullptr, (ASTNode*)(yyvsp[(1) - (7)].ptr));
        (yyval.ptr) = create_function_def((yyvsp[(2) - (7)].sval), decl_spec, nullptr, nullptr, (ASTNode*)(yyvsp[(6) - (7)].ptr));
        delete_node(decl_spec);
        free((yyvsp[(2) - (7)].sval));
    ;}
    break;

  case 32:
#line 177 "src/frontend/parser.y"
    { (yyval.ptr) = create_param_list(); ;}
    break;

  case 33:
#line 178 "src/frontend/parser.y"
    {
        ASTNode* list = create_param_list();
        add_parameter(list, create_parameter((ASTNode*)(yyvsp[(1) - (2)].ptr), (yyvsp[(2) - (2)].sval)));
        delete_node((ASTNode*)(yyvsp[(1) - (2)].ptr));
        free((yyvsp[(2) - (2)].sval));
        (yyval.ptr) = list;
    ;}
    break;

  case 34:
#line 185 "src/frontend/parser.y"
    {
        add_parameter((ASTNode*)(yyvsp[(1) - (4)].ptr), create_parameter((ASTNode*)(yyvsp[(3) - (4)].ptr), (yyvsp[(4) - (4)].sval)));
        delete_node((ASTNode*)(yyvsp[(3) - (4)].ptr));
        free((yyvsp[(4) - (4)].sval));
        (yyval.ptr) = (yyvsp[(1) - (4)].ptr);
    ;}
    break;

  case 35:
#line 194 "src/frontend/parser.y"
    { (yyval.ptr) = create_stmt_list(); ;}
    break;

  case 36:
#line 195 "src/frontend/parser.y"
    {
        if ((yyvsp[(2) - (2)].ptr)) add_statement((ASTNode*)(yyvsp[(1) - (2)].ptr), (ASTNode*)(yyvsp[(2) - (2)].ptr));
        (yyval.ptr) = (yyvsp[(1) - (2)].ptr);
    ;}
    break;

  case 37:
#line 202 "src/frontend/parser.y"
    { (yyval.ptr) = (yyvsp[(1) - (1)].ptr); ;}
    break;

  case 38:
#line 203 "src/frontend/parser.y"
    { (yyval.ptr) = (yyvsp[(1) - (2)].ptr); ;}
    break;

  case 39:
#line 204 "src/frontend/parser.y"
    { (yyval.ptr) = create_print_stmt((ASTNode*)(yyvsp[(2) - (3)].ptr)); ;}
    break;

  case 40:
#line 205 "src/frontend/parser.y"
    {
        (yyval.ptr) = create_if_stmt((ASTNode*)(yyvsp[(3) - (5)].ptr), (ASTNode*)(yyvsp[(5) - (5)].ptr), nullptr);
    ;}
    break;

  case 41:
#line 208 "src/frontend/parser.y"
    {
        (yyval.ptr) = create_if_stmt((ASTNode*)(yyvsp[(3) - (7)].ptr), (ASTNode*)(yyvsp[(5) - (7)].ptr), (ASTNode*)(yyvsp[(7) - (7)].ptr));
    ;}
    break;

  case 42:
#line 211 "src/frontend/parser.y"
    {
        (yyval.ptr) = create_while_stmt((ASTNode*)(yyvsp[(3) - (5)].ptr), (ASTNode*)(yyvsp[(5) - (5)].ptr));
    ;}
    break;

  case 43:
#line 214 "src/frontend/parser.y"
    {
        (yyval.ptr) = create_for_stmt((ASTNode*)(yyvsp[(3) - (9)].ptr), (ASTNode*)(yyvsp[(5) - (9)].ptr), (ASTNode*)(yyvsp[(7) - (9)].ptr), (ASTNode*)(yyvsp[(9) - (9)].ptr));
    ;}
    break;

  case 44:
#line 217 "src/frontend/parser.y"
    {
        (yyval.ptr) = create_for_stmt_with_decl((ASTNode*)(yyvsp[(3) - (8)].ptr), (ASTNode*)(yyvsp[(4) - (8)].ptr), (ASTNode*)(yyvsp[(6) - (8)].ptr), (ASTNode*)(yyvsp[(8) - (8)].ptr));
    ;}
    break;

  case 45:
#line 220 "src/frontend/parser.y"
    { (yyval.ptr) = create_return_stmt((ASTNode*)(yyvsp[(2) - (3)].ptr)); ;}
    break;

  case 46:
#line 221 "src/frontend/parser.y"
    { (yyval.ptr) = create_return_stmt(nullptr); ;}
    break;

  case 47:
#line 222 "src/frontend/parser.y"
    { (yyval.ptr) = create_break_stmt(nullptr); ;}
    break;

  case 48:
#line 223 "src/frontend/parser.y"
    { (yyval.ptr) = create_break_stmt((ASTNode*)(yyvsp[(2) - (3)].ptr)); ;}
    break;

  case 49:
#line 224 "src/frontend/parser.y"
    { (yyval.ptr) = (yyvsp[(2) - (3)].ptr); ;}
    break;

  case 50:
#line 225 "src/frontend/parser.y"
    { (yyval.ptr) = nullptr; ;}
    break;

  case 51:
#line 229 "src/frontend/parser.y"
    { (yyval.ptr) = (yyvsp[(1) - (1)].ptr); ;}
    break;

  case 52:
#line 233 "src/frontend/parser.y"
    { (yyval.ptr) = (yyvsp[(1) - (1)].ptr); ;}
    break;

  case 53:
#line 234 "src/frontend/parser.y"
    {
        (yyval.ptr) = create_assign_expr((yyvsp[(1) - (3)].sval), (ASTNode*)(yyvsp[(3) - (3)].ptr));
        free((yyvsp[(1) - (3)].sval));
    ;}
    break;

  case 54:
#line 238 "src/frontend/parser.y"
    {
        (yyval.ptr) = create_array_assign((yyvsp[(1) - (6)].sval), (ASTNode*)(yyvsp[(3) - (6)].ptr), (ASTNode*)(yyvsp[(6) - (6)].ptr));
        free((yyvsp[(1) - (6)].sval));
    ;}
    break;

  case 55:
#line 242 "src/frontend/parser.y"
    {
        (yyval.ptr) = create_compound_assign((yyvsp[(1) - (3)].sval), "+", (ASTNode*)(yyvsp[(3) - (3)].ptr));
        free((yyvsp[(1) - (3)].sval));
    ;}
    break;

  case 56:
#line 246 "src/frontend/parser.y"
    {
        (yyval.ptr) = create_compound_assign((yyvsp[(1) - (3)].sval), "-", (ASTNode*)(yyvsp[(3) - (3)].ptr));
        free((yyvsp[(1) - (3)].sval));
    ;}
    break;

  case 57:
#line 250 "src/frontend/parser.y"
    {
        (yyval.ptr) = create_compound_assign((yyvsp[(1) - (3)].sval), "*", (ASTNode*)(yyvsp[(3) - (3)].ptr));
        free((yyvsp[(1) - (3)].sval));
    ;}
    break;

  case 58:
#line 254 "src/frontend/parser.y"
    {
        (yyval.ptr) = create_compound_assign((yyvsp[(1) - (3)].sval), "/", (ASTNode*)(yyvsp[(3) - (3)].ptr));
        free((yyvsp[(1) - (3)].sval));
    ;}
    break;

  case 59:
#line 258 "src/frontend/parser.y"
    {
        (yyval.ptr) = create_compound_assign((yyvsp[(1) - (3)].sval), "%", (ASTNode*)(yyvsp[(3) - (3)].ptr));
        free((yyvsp[(1) - (3)].sval));
    ;}
    break;

  case 60:
#line 265 "src/frontend/parser.y"
    { (yyval.ptr) = (yyvsp[(1) - (1)].ptr); ;}
    break;

  case 61:
#line 266 "src/frontend/parser.y"
    {
        (yyval.ptr) = create_binop("||", (ASTNode*)(yyvsp[(1) - (3)].ptr), (ASTNode*)(yyvsp[(3) - (3)].ptr));
    ;}
    break;

  case 62:
#line 272 "src/frontend/parser.y"
    { (yyval.ptr) = (yyvsp[(1) - (1)].ptr); ;}
    break;

  case 63:
#line 273 "src/frontend/parser.y"
    {
        (yyval.ptr) = create_binop("&&", (ASTNode*)(yyvsp[(1) - (3)].ptr), (ASTNode*)(yyvsp[(3) - (3)].ptr));
    ;}
    break;

  case 64:
#line 279 "src/frontend/parser.y"
    { (yyval.ptr) = (yyvsp[(1) - (1)].ptr); ;}
    break;

  case 65:
#line 280 "src/frontend/parser.y"
    {
        (yyval.ptr) = create_binop("==", (ASTNode*)(yyvsp[(1) - (3)].ptr), (ASTNode*)(yyvsp[(3) - (3)].ptr));
    ;}
    break;

  case 66:
#line 283 "src/frontend/parser.y"
    {
        (yyval.ptr) = create_binop("!=", (ASTNode*)(yyvsp[(1) - (3)].ptr), (ASTNode*)(yyvsp[(3) - (3)].ptr));
    ;}
    break;

  case 67:
#line 289 "src/frontend/parser.y"
    { (yyval.ptr) = (yyvsp[(1) - (1)].ptr); ;}
    break;

  case 68:
#line 290 "src/frontend/parser.y"
    {
        (yyval.ptr) = create_binop("<", (ASTNode*)(yyvsp[(1) - (3)].ptr), (ASTNode*)(yyvsp[(3) - (3)].ptr));
    ;}
    break;

  case 69:
#line 293 "src/frontend/parser.y"
    {
        (yyval.ptr) = create_binop(">", (ASTNode*)(yyvsp[(1) - (3)].ptr), (ASTNode*)(yyvsp[(3) - (3)].ptr));
    ;}
    break;

  case 70:
#line 296 "src/frontend/parser.y"
    {
        (yyval.ptr) = create_binop("<=", (ASTNode*)(yyvsp[(1) - (3)].ptr), (ASTNode*)(yyvsp[(3) - (3)].ptr));
    ;}
    break;

  case 71:
#line 299 "src/frontend/parser.y"
    {
        (yyval.ptr) = create_binop(">=", (ASTNode*)(yyvsp[(1) - (3)].ptr), (ASTNode*)(yyvsp[(3) - (3)].ptr));
    ;}
    break;

  case 72:
#line 305 "src/frontend/parser.y"
    { (yyval.ptr) = (yyvsp[(1) - (1)].ptr); ;}
    break;

  case 73:
#line 306 "src/frontend/parser.y"
    {
        (yyval.ptr) = create_binop("+", (ASTNode*)(yyvsp[(1) - (3)].ptr), (ASTNode*)(yyvsp[(3) - (3)].ptr));
    ;}
    break;

  case 74:
#line 309 "src/frontend/parser.y"
    {
        (yyval.ptr) = create_binop("-", (ASTNode*)(yyvsp[(1) - (3)].ptr), (ASTNode*)(yyvsp[(3) - (3)].ptr));
    ;}
    break;

  case 75:
#line 315 "src/frontend/parser.y"
    { (yyval.ptr) = (yyvsp[(1) - (1)].ptr); ;}
    break;

  case 76:
#line 316 "src/frontend/parser.y"
    {
        (yyval.ptr) = create_binop("*", (ASTNode*)(yyvsp[(1) - (3)].ptr), (ASTNode*)(yyvsp[(3) - (3)].ptr));
    ;}
    break;

  case 77:
#line 319 "src/frontend/parser.y"
    {
        (yyval.ptr) = create_binop("/", (ASTNode*)(yyvsp[(1) - (3)].ptr), (ASTNode*)(yyvsp[(3) - (3)].ptr));
    ;}
    break;

  case 78:
#line 322 "src/frontend/parser.y"
    {
        (yyval.ptr) = create_binop("%", (ASTNode*)(yyvsp[(1) - (3)].ptr), (ASTNode*)(yyvsp[(3) - (3)].ptr));
    ;}
    break;

  case 79:
#line 328 "src/frontend/parser.y"
    { (yyval.ptr) = (yyvsp[(1) - (1)].ptr); ;}
    break;

  case 80:
#line 329 "src/frontend/parser.y"
    { (yyval.ptr) = create_pre_incdec("++", (yyvsp[(2) - (2)].sval)); free((yyvsp[(2) - (2)].sval)); ;}
    break;

  case 81:
#line 330 "src/frontend/parser.y"
    { (yyval.ptr) = create_pre_incdec("--", (yyvsp[(2) - (2)].sval)); free((yyvsp[(2) - (2)].sval)); ;}
    break;

  case 82:
#line 331 "src/frontend/parser.y"
    { (yyval.ptr) = create_unary("!", (ASTNode*)(yyvsp[(2) - (2)].ptr)); ;}
    break;

  case 83:
#line 332 "src/frontend/parser.y"
    { (yyval.ptr) = create_unary("-", (ASTNode*)(yyvsp[(2) - (2)].ptr)); ;}
    break;

  case 84:
#line 336 "src/frontend/parser.y"
    { (yyval.ptr) = (yyvsp[(1) - (1)].ptr); ;}
    break;

  case 85:
#line 337 "src/frontend/parser.y"
    { (yyval.ptr) = create_post_incdec("++", (yyvsp[(1) - (2)].sval)); free((yyvsp[(1) - (2)].sval)); ;}
    break;

  case 86:
#line 338 "src/frontend/parser.y"
    { (yyval.ptr) = create_post_incdec("--", (yyvsp[(1) - (2)].sval)); free((yyvsp[(1) - (2)].sval)); ;}
    break;

  case 87:
#line 339 "src/frontend/parser.y"
    {
        (yyval.ptr) = create_array_ref((yyvsp[(1) - (4)].sval), (ASTNode*)(yyvsp[(3) - (4)].ptr));
        free((yyvsp[(1) - (4)].sval));
    ;}
    break;

  case 88:
#line 343 "src/frontend/parser.y"
    {
        (yyval.ptr) = create_func_call((yyvsp[(1) - (4)].sval), (ASTNode*)(yyvsp[(3) - (4)].ptr));
        free((yyvsp[(1) - (4)].sval));
    ;}
    break;

  case 89:
#line 347 "src/frontend/parser.y"
    {
        (yyval.ptr) = create_func_call((yyvsp[(1) - (3)].sval), nullptr);
        free((yyvsp[(1) - (3)].sval));
    ;}
    break;

  case 90:
#line 354 "src/frontend/parser.y"
    { (yyval.ptr) = create_var_ref((yyvsp[(1) - (1)].sval)); free((yyvsp[(1) - (1)].sval)); ;}
    break;

  case 91:
#line 355 "src/frontend/parser.y"
    {
        // 大きな数値の場合はlongとして処理
        if ((yyvsp[(1) - (1)].lval) > 2147483647LL || (yyvsp[(1) - (1)].lval) < -2147483648LL) {
            (yyval.ptr) = create_number((yyvsp[(1) - (1)].lval), TYPE_LONG);
        } else {
            (yyval.ptr) = create_number((yyvsp[(1) - (1)].lval), TYPE_INT);
        }
    ;}
    break;

  case 92:
#line 363 "src/frontend/parser.y"
    {
        (yyval.ptr) = create_number((yyvsp[(2) - (2)].lval), get_type_info((ASTNode*)(yyvsp[(1) - (2)].ptr)));
        delete_node((ASTNode*)(yyvsp[(1) - (2)].ptr));
    ;}
    break;

  case 93:
#line 367 "src/frontend/parser.y"
    {
        (yyval.ptr) = create_string_literal((yyvsp[(1) - (1)].sval));
        free((yyvsp[(1) - (1)].sval));
    ;}
    break;

  case 94:
#line 371 "src/frontend/parser.y"
    { (yyval.ptr) = create_number(1, TYPE_BOOL); ;}
    break;

  case 95:
#line 372 "src/frontend/parser.y"
    { (yyval.ptr) = create_number(0, TYPE_BOOL); ;}
    break;

  case 96:
#line 373 "src/frontend/parser.y"
    { (yyval.ptr) = create_number(0, TYPE_BOOL); ;}
    break;

  case 97:
#line 374 "src/frontend/parser.y"
    { (yyval.ptr) = (yyvsp[(2) - (3)].ptr); ;}
    break;

  case 98:
#line 378 "src/frontend/parser.y"
    {
        ASTNode* list = create_arg_list();
        add_argument(list, (ASTNode*)(yyvsp[(1) - (1)].ptr));
        (yyval.ptr) = list;
    ;}
    break;

  case 99:
#line 383 "src/frontend/parser.y"
    {
        add_argument((ASTNode*)(yyvsp[(1) - (3)].ptr), (ASTNode*)(yyvsp[(3) - (3)].ptr));
        (yyval.ptr) = (yyvsp[(1) - (3)].ptr);
    ;}
    break;

  case 100:
#line 390 "src/frontend/parser.y"
    { (yyval.ptr) = (yyvsp[(1) - (1)].ptr); ;}
    break;

  case 101:
#line 391 "src/frontend/parser.y"
    { (yyval.ptr) = create_array_literal((ASTNode*)(yyvsp[(2) - (3)].ptr)); ;}
    break;

  case 102:
#line 392 "src/frontend/parser.y"
    { (yyval.ptr) = create_array_literal(nullptr); ;}
    break;


/* Line 1267 of yacc.c.  */
#line 2395 "src/frontend/parser.c"
      default: break;
    }
  YY_SYMBOL_PRINT ("-> $$ =", yyr1[yyn], &yyval, &yyloc);

  YYPOPSTACK (yylen);
  yylen = 0;
  YY_STACK_PRINT (yyss, yyssp);

  *++yyvsp = yyval;


  /* Now `shift' the result of the reduction.  Determine what state
     that goes to, based on the state we popped back to and the rule
     number reduced by.  */

  yyn = yyr1[yyn];

  yystate = yypgoto[yyn - YYNTOKENS] + *yyssp;
  if (0 <= yystate && yystate <= YYLAST && yycheck[yystate] == *yyssp)
    yystate = yytable[yystate];
  else
    yystate = yydefgoto[yyn - YYNTOKENS];

  goto yynewstate;


/*------------------------------------.
| yyerrlab -- here on detecting error |
`------------------------------------*/
yyerrlab:
  /* If not already recovering from an error, report this error.  */
  if (!yyerrstatus)
    {
      ++yynerrs;
#if ! YYERROR_VERBOSE
      yyerror (YY_("syntax error"));
#else
      {
	YYSIZE_T yysize = yysyntax_error (0, yystate, yychar);
	if (yymsg_alloc < yysize && yymsg_alloc < YYSTACK_ALLOC_MAXIMUM)
	  {
	    YYSIZE_T yyalloc = 2 * yysize;
	    if (! (yysize <= yyalloc && yyalloc <= YYSTACK_ALLOC_MAXIMUM))
	      yyalloc = YYSTACK_ALLOC_MAXIMUM;
	    if (yymsg != yymsgbuf)
	      YYSTACK_FREE (yymsg);
	    yymsg = (char *) YYSTACK_ALLOC (yyalloc);
	    if (yymsg)
	      yymsg_alloc = yyalloc;
	    else
	      {
		yymsg = yymsgbuf;
		yymsg_alloc = sizeof yymsgbuf;
	      }
	  }

	if (0 < yysize && yysize <= yymsg_alloc)
	  {
	    (void) yysyntax_error (yymsg, yystate, yychar);
	    yyerror (yymsg);
	  }
	else
	  {
	    yyerror (YY_("syntax error"));
	    if (yysize != 0)
	      goto yyexhaustedlab;
	  }
      }
#endif
    }



  if (yyerrstatus == 3)
    {
      /* If just tried and failed to reuse look-ahead token after an
	 error, discard it.  */

      if (yychar <= YYEOF)
	{
	  /* Return failure if at end of input.  */
	  if (yychar == YYEOF)
	    YYABORT;
	}
      else
	{
	  yydestruct ("Error: discarding",
		      yytoken, &yylval);
	  yychar = YYEMPTY;
	}
    }

  /* Else will try to reuse look-ahead token after shifting the error
     token.  */
  goto yyerrlab1;


/*---------------------------------------------------.
| yyerrorlab -- error raised explicitly by YYERROR.  |
`---------------------------------------------------*/
yyerrorlab:

  /* Pacify compilers like GCC when the user code never invokes
     YYERROR and the label yyerrorlab therefore never appears in user
     code.  */
  if (/*CONSTCOND*/ 0)
     goto yyerrorlab;

  /* Do not reclaim the symbols of the rule which action triggered
     this YYERROR.  */
  YYPOPSTACK (yylen);
  yylen = 0;
  YY_STACK_PRINT (yyss, yyssp);
  yystate = *yyssp;
  goto yyerrlab1;


/*-------------------------------------------------------------.
| yyerrlab1 -- common code for both syntax error and YYERROR.  |
`-------------------------------------------------------------*/
yyerrlab1:
  yyerrstatus = 3;	/* Each real token shifted decrements this.  */

  for (;;)
    {
      yyn = yypact[yystate];
      if (yyn != YYPACT_NINF)
	{
	  yyn += YYTERROR;
	  if (0 <= yyn && yyn <= YYLAST && yycheck[yyn] == YYTERROR)
	    {
	      yyn = yytable[yyn];
	      if (0 < yyn)
		break;
	    }
	}

      /* Pop the current state because it cannot handle the error token.  */
      if (yyssp == yyss)
	YYABORT;


      yydestruct ("Error: popping",
		  yystos[yystate], yyvsp);
      YYPOPSTACK (1);
      yystate = *yyssp;
      YY_STACK_PRINT (yyss, yyssp);
    }

  if (yyn == YYFINAL)
    YYACCEPT;

  *++yyvsp = yylval;


  /* Shift the error token.  */
  YY_SYMBOL_PRINT ("Shifting", yystos[yyn], yyvsp, yylsp);

  yystate = yyn;
  goto yynewstate;


/*-------------------------------------.
| yyacceptlab -- YYACCEPT comes here.  |
`-------------------------------------*/
yyacceptlab:
  yyresult = 0;
  goto yyreturn;

/*-----------------------------------.
| yyabortlab -- YYABORT comes here.  |
`-----------------------------------*/
yyabortlab:
  yyresult = 1;
  goto yyreturn;

#ifndef yyoverflow
/*-------------------------------------------------.
| yyexhaustedlab -- memory exhaustion comes here.  |
`-------------------------------------------------*/
yyexhaustedlab:
  yyerror (YY_("memory exhausted"));
  yyresult = 2;
  /* Fall through.  */
#endif

yyreturn:
  if (yychar != YYEOF && yychar != YYEMPTY)
     yydestruct ("Cleanup: discarding lookahead",
		 yytoken, &yylval);
  /* Do not reclaim the symbols of the rule which action triggered
     this YYABORT or YYACCEPT.  */
  YYPOPSTACK (yylen);
  YY_STACK_PRINT (yyss, yyssp);
  while (yyssp != yyss)
    {
      yydestruct ("Cleanup: popping",
		  yystos[*yyssp], yyvsp);
      YYPOPSTACK (1);
    }
#ifndef yyoverflow
  if (yyss != yyssa)
    YYSTACK_FREE (yyss);
#endif
#if YYERROR_VERBOSE
  if (yymsg != yymsgbuf)
    YYSTACK_FREE (yymsg);
#endif
  /* Make sure YYID is used.  */
  return YYID (yyresult);
}


#line 395 "src/frontend/parser.y"


