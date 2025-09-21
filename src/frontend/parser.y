%{
#include "../common/ast.h"
#include "../frontend/parser_utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <cstring>
#include <memory>

extern int yylex();
extern void yyerror(const char* s);
extern std::unique_ptr<ASTNode> root_node;

int yylex();
std::unique_ptr<ASTNode> root_node = nullptr;
extern "C" {
    char *yyfilename = NULL;
}
%}

%union {
    long long lval;
    char* sval;
    void* ptr;
}

%token <lval> NUMBER
%token <sval> IDENTIFIER STRING_LITERAL
%token CONST STATIC TYPEDEF
%token VOID TINY SHORT INT LONG BOOL STRING
%token TRUE FALSE NULL_LIT
%token PLUS MINUS MUL DIV ASSIGN SEMICOLON PRINT PRINTLN RETURN
%token FOR WHILE BREAK IF ELSE
%token IMPORT EXPORT MODULE
%token TRY CATCH FINALLY THROW
%token EQ NEQ GE LE GT LT OR AND NOT MOD
%token ADD_ASSIGN SUB_ASSIGN MUL_ASSIGN DIV_ASSIGN MOD_ASSIGN
%token INC_OP DEC_OP
%token '{' '}' '(' ')' '[' ']' ','

%type <ptr> program declaration_list declaration
%type <ptr> function_definition parameter_list
%type <ptr> statement_list statement 
%type <ptr> expression assignment_expression
%type <ptr> logical_or_expression logical_and_expression equality_expression
%type <ptr> relational_expression additive_expression multiplicative_expression
%type <ptr> unary_expression postfix_expression primary_expression
%type <ptr> declarator type_specifier storage_class_specifier type_qualifier
%type <ptr> declaration_specifiers
%type <ptr> argument_list initializer
%type <ptr> import_statement export_statement module_declaration
%type <ptr> try_statement catch_statement finally_statement throw_statement
%type <ptr> typedef_declaration array_dimensions
%type <sval> qualified_name

%start program

%%

program:
    declaration_list {
        $$ = $1;
        root_node = std::unique_ptr<ASTNode>((ASTNode*)$$);
    }
    ;

declaration_list:
    declaration {
        ASTNode* list = create_stmt_list();
        add_statement(list, (ASTNode*)$1);
        $$ = list;
    }
    | declaration_list declaration {
        add_statement((ASTNode*)$1, (ASTNode*)$2);
        $$ = $1;
    }
    ;

declaration:
    function_definition { $$ = $1; }
    | import_statement { $$ = $1; }
    | export_statement { $$ = $1; }
    | module_declaration { $$ = $1; }
    | typedef_declaration { $$ = $1; }
    | declaration_specifiers declarator SEMICOLON {
        ASTNode* decl = (ASTNode*)$2;
        set_declaration_attributes(decl, (ASTNode*)$1, nullptr);
        delete_node((ASTNode*)$1);
        $$ = decl;
    }
    | declaration_specifiers declarator '=' initializer SEMICOLON {
        ASTNode* declarator_node = (ASTNode*)$2;
        ASTNode* decl = create_var_decl_with_init(declarator_node->name.c_str(), (ASTNode*)$4);
        set_declaration_attributes(decl, (ASTNode*)$1, (ASTNode*)$4);
        delete_node((ASTNode*)$1);
        delete_node(declarator_node);
        $$ = decl;
    }
    | type_specifier declarator SEMICOLON {
        ASTNode* decl = (ASTNode*)$2;
        ASTNode* decl_spec = create_decl_spec(nullptr, nullptr, (ASTNode*)$1);
        set_declaration_attributes(decl, decl_spec, nullptr);
        delete_node(decl_spec);
        $$ = decl;
    }
    | type_specifier declarator '=' initializer SEMICOLON {
        ASTNode* declarator_node = (ASTNode*)$2;
        ASTNode* decl = create_var_decl_with_init(declarator_node->name.c_str(), (ASTNode*)$4);
        ASTNode* decl_spec = create_decl_spec(nullptr, nullptr, (ASTNode*)$1);
        set_declaration_attributes(decl, decl_spec, (ASTNode*)$4);
        delete_node(decl_spec);
        delete_node(declarator_node);
        $$ = decl;
    }
    ;

declaration_specifiers:
    type_specifier { $$ = create_decl_spec(nullptr, nullptr, (ASTNode*)$1); }
    | storage_class_specifier type_qualifier type_specifier {
        $$ = create_decl_spec((ASTNode*)$1, (ASTNode*)$2, (ASTNode*)$3);
        delete_node((ASTNode*)$1);
        delete_node((ASTNode*)$2);
    }
    | storage_class_specifier type_specifier {
        $$ = create_decl_spec((ASTNode*)$1, nullptr, (ASTNode*)$2);
        delete_node((ASTNode*)$1);
    }
    | type_qualifier type_specifier {
        $$ = create_decl_spec(nullptr, (ASTNode*)$1, (ASTNode*)$2);
        delete_node((ASTNode*)$1);
    }
    ;

storage_class_specifier:
    STATIC { $$ = create_storage_spec(true, false); }
    ;

type_qualifier:
    CONST { $$ = create_storage_spec(false, true); }
    ;

type_specifier:
    VOID   { $$ = create_type_node(TYPE_VOID); }
    | TINY   { $$ = create_type_node(TYPE_TINY); }
    | SHORT  { $$ = create_type_node(TYPE_SHORT); }
    | INT    { $$ = create_type_node(TYPE_INT); }
    | LONG   { $$ = create_type_node(TYPE_LONG); }
    | STRING { $$ = create_type_node(TYPE_STRING); }
    | BOOL   { $$ = create_type_node(TYPE_BOOL); }
    | IDENTIFIER { 
        // 型エイリアスかチェック
        $$ = create_type_alias_node($1); 
        free($1);
    }
    ;

declarator:
    IDENTIFIER {
        $$ = create_var_decl($1);
        free($1);
    }
    | IDENTIFIER ASSIGN assignment_expression {
        $$ = create_var_init($1, (ASTNode*)$3);
        free($1);
    }
    | IDENTIFIER '[' expression ']' {
        $$ = create_array_decl($1, (ASTNode*)$3);
        free($1);
    }
    | IDENTIFIER '[' ']' ASSIGN initializer {
        $$ = create_array_init($1, (ASTNode*)$5);
        free($1);
    }
    | IDENTIFIER '[' expression ']' ASSIGN initializer {
        $$ = create_array_init_with_size($1, (ASTNode*)$3, (ASTNode*)$6);
        free($1);
    }
    ;

function_definition:
    declaration_specifiers IDENTIFIER '(' parameter_list ')' '{' statement_list '}' {
        $$ = create_function_def($2, (ASTNode*)$1, nullptr, (ASTNode*)$4, (ASTNode*)$7);
        delete_node((ASTNode*)$1);
        // delete_node((ASTNode*)$4); // create_function_def内でparamsが削除される
        free($2);
    }
    | declaration_specifiers IDENTIFIER '(' ')' '{' statement_list '}' {
        $$ = create_function_def($2, (ASTNode*)$1, nullptr, nullptr, (ASTNode*)$6);
        delete_node((ASTNode*)$1);
        free($2);
    }
    | type_specifier IDENTIFIER '(' parameter_list ')' '{' statement_list '}' {
        ASTNode* decl_spec = create_decl_spec(nullptr, nullptr, (ASTNode*)$1);
        $$ = create_function_def($2, decl_spec, nullptr, (ASTNode*)$4, (ASTNode*)$7);
        delete_node(decl_spec);
        // delete_node((ASTNode*)$4); // create_function_def内でdeleteされる
        free($2);
    }
    | type_specifier IDENTIFIER '(' ')' '{' statement_list '}' {
        ASTNode* decl_spec = create_decl_spec(nullptr, nullptr, (ASTNode*)$1);
        $$ = create_function_def($2, decl_spec, nullptr, nullptr, (ASTNode*)$6);
        delete_node(decl_spec);
        free($2);
    }
    ;

parameter_list:
    /* empty */ { $$ = create_param_list(); }
    | type_specifier IDENTIFIER {
        ASTNode* list = create_param_list();
        add_parameter(list, create_parameter((ASTNode*)$1, $2));
        delete_node((ASTNode*)$1);
        free($2);
        $$ = list;
    }
    | parameter_list ',' type_specifier IDENTIFIER {
        add_parameter((ASTNode*)$1, create_parameter((ASTNode*)$3, $4));
        delete_node((ASTNode*)$3);
        free($4);
        $$ = $1;
    }
    ;

statement_list:
    /* empty */ { $$ = create_stmt_list(); }
    | statement_list statement {
        if ($2) add_statement((ASTNode*)$1, (ASTNode*)$2);
        $$ = $1;
    }
    ;

statement:
    declaration { $$ = $1; }
    | expression SEMICOLON { $$ = $1; }
    | PRINT '(' expression ')' SEMICOLON { 
        $$ = create_print_stmt((ASTNode*)$3);
    }
    | PRINTLN '(' expression ')' SEMICOLON { 
        $$ = create_println_stmt((ASTNode*)$3);
    }
    | PRINTLN '(' ')' SEMICOLON { 
        $$ = create_println_empty();
    }
    | PRINTLN '(' argument_list ')' SEMICOLON {
        $$ = create_println_multi_stmt((ASTNode*)$3);
    }
    | PRINTLN '(' STRING_LITERAL ',' argument_list ')' SEMICOLON { 
        ASTNode* format_str = create_string_literal($3);
        $$ = create_printlnf_stmt(format_str, (ASTNode*)$5);
    }
    | PRINT '(' argument_list ')' SEMICOLON {
        $$ = create_print_multi_stmt((ASTNode*)$3);
    }
    | PRINT '(' STRING_LITERAL ',' argument_list ')' SEMICOLON { 
        ASTNode* format_str = create_string_literal($3);
        $$ = create_printf_stmt(format_str, (ASTNode*)$5);
    }
    | IF '(' expression ')' statement {
        $$ = create_if_stmt((ASTNode*)$3, (ASTNode*)$5, nullptr);
    }
    | IF '(' expression ')' statement ELSE statement {
        $$ = create_if_stmt((ASTNode*)$3, (ASTNode*)$5, (ASTNode*)$7);
    }
    | WHILE '(' expression ')' statement {
        $$ = create_while_stmt((ASTNode*)$3, (ASTNode*)$5);
    }
    | FOR '(' expression SEMICOLON expression SEMICOLON expression ')' statement {
        $$ = create_for_stmt((ASTNode*)$3, (ASTNode*)$5, (ASTNode*)$7, (ASTNode*)$9);
    }
    | FOR '(' declaration expression SEMICOLON expression ')' statement {
        $$ = create_for_stmt_with_decl((ASTNode*)$3, (ASTNode*)$4, (ASTNode*)$6, (ASTNode*)$8);
    }
    | RETURN expression SEMICOLON { $$ = create_return_stmt((ASTNode*)$2); }
    | RETURN SEMICOLON { $$ = create_return_stmt(nullptr); }
    | BREAK SEMICOLON { $$ = create_break_stmt(nullptr); }
    | BREAK expression SEMICOLON { $$ = create_break_stmt((ASTNode*)$2); }
    | try_statement { $$ = $1; }
    | throw_statement { $$ = $1; }
    | '{' statement_list '}' { $$ = $2; }
    | SEMICOLON { $$ = nullptr; }
    ;

expression:
    assignment_expression { $$ = $1; }
    ;

assignment_expression:
    logical_or_expression { $$ = $1; }
    | IDENTIFIER ASSIGN assignment_expression {
        $$ = create_assign_expr($1, (ASTNode*)$3);
        free($1);
    }
    | IDENTIFIER '[' expression ']' ASSIGN assignment_expression {
        $$ = create_array_assign($1, (ASTNode*)$3, (ASTNode*)$6);
        free($1);
    }
    | IDENTIFIER ADD_ASSIGN assignment_expression {
        $$ = create_compound_assign($1, "+", (ASTNode*)$3);
        free($1);
    }
    | IDENTIFIER SUB_ASSIGN assignment_expression {
        $$ = create_compound_assign($1, "-", (ASTNode*)$3);
        free($1);
    }
    | IDENTIFIER MUL_ASSIGN assignment_expression {
        $$ = create_compound_assign($1, "*", (ASTNode*)$3);
        free($1);
    }
    | IDENTIFIER DIV_ASSIGN assignment_expression {
        $$ = create_compound_assign($1, "/", (ASTNode*)$3);
        free($1);
    }
    | IDENTIFIER MOD_ASSIGN assignment_expression {
        $$ = create_compound_assign($1, "%", (ASTNode*)$3);
        free($1);
    }
    ;

logical_or_expression:
    logical_and_expression { $$ = $1; }
    | logical_or_expression OR logical_and_expression {
        $$ = create_binop("||", (ASTNode*)$1, (ASTNode*)$3);
    }
    ;

logical_and_expression:
    equality_expression { $$ = $1; }
    | logical_and_expression AND equality_expression {
        $$ = create_binop("&&", (ASTNode*)$1, (ASTNode*)$3);
    }
    ;

equality_expression:
    relational_expression { $$ = $1; }
    | equality_expression EQ relational_expression {
        $$ = create_binop("==", (ASTNode*)$1, (ASTNode*)$3);
    }
    | equality_expression NEQ relational_expression {
        $$ = create_binop("!=", (ASTNode*)$1, (ASTNode*)$3);
    }
    ;

relational_expression:
    additive_expression { $$ = $1; }
    | relational_expression LT additive_expression {
        $$ = create_binop("<", (ASTNode*)$1, (ASTNode*)$3);
    }
    | relational_expression GT additive_expression {
        $$ = create_binop(">", (ASTNode*)$1, (ASTNode*)$3);
    }
    | relational_expression LE additive_expression {
        $$ = create_binop("<=", (ASTNode*)$1, (ASTNode*)$3);
    }
    | relational_expression GE additive_expression {
        $$ = create_binop(">=", (ASTNode*)$1, (ASTNode*)$3);
    }
    ;

additive_expression:
    multiplicative_expression { $$ = $1; }
    | additive_expression PLUS multiplicative_expression {
        $$ = create_binop("+", (ASTNode*)$1, (ASTNode*)$3);
    }
    | additive_expression MINUS multiplicative_expression {
        $$ = create_binop("-", (ASTNode*)$1, (ASTNode*)$3);
    }
    ;

multiplicative_expression:
    unary_expression { $$ = $1; }
    | multiplicative_expression MUL unary_expression {
        $$ = create_binop("*", (ASTNode*)$1, (ASTNode*)$3);
    }
    | multiplicative_expression DIV unary_expression {
        $$ = create_binop("/", (ASTNode*)$1, (ASTNode*)$3);
    }
    | multiplicative_expression MOD unary_expression {
        $$ = create_binop("%", (ASTNode*)$1, (ASTNode*)$3);
    }
    ;

unary_expression:
    postfix_expression { $$ = $1; }
    | INC_OP IDENTIFIER { $$ = create_pre_incdec("++", $2); free($2); }
    | DEC_OP IDENTIFIER { $$ = create_pre_incdec("--", $2); free($2); }
    | NOT unary_expression { $$ = create_unary("!", (ASTNode*)$2); }
    | MINUS unary_expression { $$ = create_unary("-", (ASTNode*)$2); }
    ;

postfix_expression:
    primary_expression { $$ = $1; }
    | IDENTIFIER INC_OP { $$ = create_post_incdec("++", $1); free($1); }
    | IDENTIFIER DEC_OP { $$ = create_post_incdec("--", $1); free($1); }
    | IDENTIFIER '[' expression ']' {
        $$ = create_array_ref($1, (ASTNode*)$3);
        free($1);
    }
    | IDENTIFIER '(' argument_list ')' {
        $$ = create_func_call($1, (ASTNode*)$3);
        free($1);
    }
    | IDENTIFIER '(' ')' {
        $$ = create_func_call($1, nullptr);
        free($1);
    }
    | qualified_name '(' argument_list ')' {
        ASTNode* func_call = create_qualified_func_call($1, (ASTNode*)$3);
        free($1);
        $$ = func_call;
    }
    | qualified_name '(' ')' {
        ASTNode* func_call = create_qualified_func_call($1, nullptr);
        free($1);
        $$ = func_call;
    }
    ;

primary_expression:
    IDENTIFIER { $$ = create_var_ref($1); free($1); }
    | qualified_name { $$ = create_qualified_var_ref($1); free($1); }
    | NUMBER {
        // 大きな数値の場合はlongとして処理
        if ($1 > 2147483647LL || $1 < -2147483648LL) {
            $$ = create_number($1, TYPE_LONG);
        } else {
            $$ = create_number($1, TYPE_INT);
        }
    }
    | type_specifier NUMBER {
        $$ = create_number($2, get_type_info((ASTNode*)$1));
        delete_node((ASTNode*)$1);
    }
    | STRING_LITERAL {
        $$ = create_string_literal($1);
        free($1);
    }
    | TRUE { $$ = create_number(1, TYPE_BOOL); }
    | FALSE { $$ = create_number(0, TYPE_BOOL); }
    | NULL_LIT { $$ = create_number(0, TYPE_BOOL); }
    | '(' expression ')' { $$ = $2; }
    ;

argument_list:
    assignment_expression {
        ASTNode* list = create_arg_list();
        add_argument(list, (ASTNode*)$1);
        $$ = list;
    }
    | argument_list ',' assignment_expression {
        add_argument((ASTNode*)$1, (ASTNode*)$3);
        $$ = $1;
    }
    ;

initializer:
    assignment_expression { $$ = $1; }
    | '[' argument_list ']' { $$ = create_array_literal((ASTNode*)$2); }
    | '[' ']' { $$ = create_array_literal(nullptr); }
    ;

/* モジュールシステム */
qualified_name:
    IDENTIFIER {
        $$ = $1; // そのまま返す
    }
    | qualified_name '.' IDENTIFIER {
        // ドット記法を連結
        std::string result = std::string($1) + "." + std::string($3);
        char* str = strdup(result.c_str());
        free($1);
        free($3);
        $$ = str;
    }
    ;

import_statement:
    IMPORT qualified_name SEMICOLON {
        ASTNode* import_node = new ASTNode(ASTNodeType::AST_IMPORT_STMT);
        import_node->module_name = std::string($2);
        free($2);
        $$ = import_node;
    }
    ;

export_statement:
    EXPORT function_definition {
        ASTNode* func = (ASTNode*)$2;
        func->is_exported = true;
        $$ = func;
    }
    | EXPORT declaration_specifiers declarator SEMICOLON {
        ASTNode* decl = (ASTNode*)$3;
        
        // declaratorが初期化子を含む場合（AST_ASSIGN）、export const用に変換
        if (decl->node_type == ASTNodeType::AST_ASSIGN) {
            ASTNode* export_decl = create_export_var_init(decl->name.c_str(), decl->right.release());
            set_declaration_attributes(export_decl, (ASTNode*)$2, nullptr);
            export_decl->is_exported = true;
            delete_node((ASTNode*)$2);
            delete_node(decl);
            $$ = export_decl;
        } else {
            set_declaration_attributes(decl, (ASTNode*)$2, nullptr);
            decl->is_exported = true;
            delete_node((ASTNode*)$2);
            $$ = decl;
        }
    }
    | EXPORT declaration_specifiers declarator '=' initializer SEMICOLON {
        ASTNode* declarator_node = (ASTNode*)$3;
        ASTNode* decl;
        
        // declaratorが既に初期化子を含む場合（AST_ASSIGN）、export const用に変換
        if (declarator_node->node_type == ASTNodeType::AST_ASSIGN) {
            decl = create_export_var_init(declarator_node->name.c_str(), declarator_node->right.release());
        } else {
            decl = create_export_var_init(declarator_node->name.c_str(), (ASTNode*)$5);
        }
        
        set_declaration_attributes(decl, (ASTNode*)$2, nullptr);
        decl->is_exported = true;
        delete_node((ASTNode*)$2);
        delete_node(declarator_node);
        $$ = decl;
    }
    ;

module_declaration:
    MODULE IDENTIFIER '{' declaration_list '}' {
        ASTNode* module_node = new ASTNode(ASTNodeType::AST_MODULE_DECL);
        module_node->module_name = std::string($2);
        module_node->body = std::unique_ptr<ASTNode>((ASTNode*)$4);
        free($2);
        $$ = module_node;
    }
    ;

try_statement:
    TRY '{' statement_list '}' catch_statement {
        ASTNode* try_node = new ASTNode(ASTNodeType::AST_TRY_STMT);
        try_node->try_body = std::unique_ptr<ASTNode>((ASTNode*)$3);
        try_node->catch_body = std::unique_ptr<ASTNode>((ASTNode*)$5);
        $$ = try_node;
    }
    | TRY '{' statement_list '}' catch_statement finally_statement {
        ASTNode* try_node = new ASTNode(ASTNodeType::AST_TRY_STMT);
        try_node->try_body = std::unique_ptr<ASTNode>((ASTNode*)$3);
        try_node->catch_body = std::unique_ptr<ASTNode>((ASTNode*)$5);
        try_node->finally_body = std::unique_ptr<ASTNode>((ASTNode*)$6);
        $$ = try_node;
    }
    ;

catch_statement:
    CATCH '(' IDENTIFIER IDENTIFIER ')' '{' statement_list '}' {
        ASTNode* catch_node = new ASTNode(ASTNodeType::AST_CATCH_STMT);
        catch_node->exception_type = std::string($3);
        catch_node->exception_var = std::string($4);
        catch_node->catch_body = std::unique_ptr<ASTNode>((ASTNode*)$7);
        free($3);
        free($4);
        $$ = catch_node;
    }
    ;

finally_statement:
    FINALLY '{' statement_list '}' {
        ASTNode* finally_node = new ASTNode(ASTNodeType::AST_FINALLY_STMT);
        finally_node->finally_body = std::unique_ptr<ASTNode>((ASTNode*)$3);
        $$ = finally_node;
    }
    ;

throw_statement:
    THROW expression SEMICOLON {
        ASTNode* throw_node = new ASTNode(ASTNodeType::AST_THROW_STMT);
        throw_node->throw_expr = std::unique_ptr<ASTNode>((ASTNode*)$2);
        $$ = throw_node;
    }
    ;

typedef_declaration:
    TYPEDEF type_specifier IDENTIFIER SEMICOLON {
        ASTNode* typedef_node = create_typedef_decl($3, (ASTNode*)$2);
        free($3);
        $$ = typedef_node;
    }
    | TYPEDEF type_specifier array_dimensions IDENTIFIER SEMICOLON {
        // 配列typedef: typedef int[10][20] ArrayType;
        std::vector<std::unique_ptr<ASTNode>> dims;
        ASTNode* current = (ASTNode*)$3;
        
        // array_dimensionsから次元情報を抽出
        while (current) {
            if (current->array_size_expr) {
                dims.push_back(std::unique_ptr<ASTNode>(current->array_size_expr.release()));
            } else {
                // 空の括弧[] = 動的サイズ
                dims.push_back(nullptr);
            }
            current = current->left.get();
        }
        
        ASTNode* typedef_node = create_typedef_array_decl($4, (ASTNode*)$2, dims);
        free($4);
        delete (ASTNode*)$3; // array_dimensionsノードを削除
        $$ = typedef_node;
    }
    ;

array_dimensions:
    '[' expression ']' {
        // 固定サイズ次元: [10]
        ASTNode* dim_node = new ASTNode(ASTNodeType::AST_ARRAY_DECL);
        dim_node->array_size_expr = std::unique_ptr<ASTNode>((ASTNode*)$2);
        $$ = dim_node;
    }
    | '[' ']' {
        // 動的サイズ次元: []
        ASTNode* dim_node = new ASTNode(ASTNodeType::AST_ARRAY_DECL);
        // array_size_exprがnullptrなら動的サイズ
        $$ = dim_node;
    }
    | array_dimensions '[' expression ']' {
        // 多次元固定サイズ: [3][4]
        ASTNode* dim_node = new ASTNode(ASTNodeType::AST_ARRAY_DECL);
        dim_node->array_size_expr = std::unique_ptr<ASTNode>((ASTNode*)$3);
        dim_node->left = std::unique_ptr<ASTNode>((ASTNode*)$1);
        $$ = dim_node;
    }
    | array_dimensions '[' ']' {
        // 多次元動的サイズ: [3][]
        ASTNode* dim_node = new ASTNode(ASTNodeType::AST_ARRAY_DECL);
        // array_size_exprがnullptrなら動的サイズ
        dim_node->left = std::unique_ptr<ASTNode>((ASTNode*)$1);
        $$ = dim_node;
    }
    ;

%%
