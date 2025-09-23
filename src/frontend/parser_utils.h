#pragma once
#include "../common/ast.h"
#include "debug.h"

// パーサーヘルパー関数の宣言
extern "C" {

// ノード作成関数
ASTNode *create_stmt_list();
ASTNode *create_type_node(TypeInfo type);
ASTNode *create_storage_spec(bool is_static, bool is_const);
ASTNode *create_var_decl(const char *name);
ASTNode *create_var_init(const char *name, ASTNode *init_expr);
ASTNode *create_array_decl(const char *name, ASTNode *size_expr);
ASTNode *create_array_init(const char *name, ASTNode *init_list);
ASTNode *create_array_init_with_type(const char *name, TypeInfo element_type,
                                     ASTNode *init_list);
ASTNode *create_array_init_with_size(const char *name, ASTNode *size_expr,
                                     ASTNode *init_list);
ASTNode *create_function_def(const char *name, ASTNode *storage, ASTNode *type,
                             ASTNode *params, ASTNode *body);
ASTNode *create_param_list();
ASTNode *create_parameter(ASTNode *type, const char *name);
ASTNode *create_print_stmt(ASTNode *expr);
ASTNode *create_println_stmt(ASTNode *expr);
ASTNode *create_println_empty();
ASTNode *create_printf_stmt(ASTNode *format_str, ASTNode *arg_list);
ASTNode *create_printlnf_stmt(ASTNode *format_str, ASTNode *arg_list);
ASTNode *create_if_stmt(ASTNode *condition, ASTNode *then_stmt,
                        ASTNode *else_stmt);
ASTNode *create_while_stmt(ASTNode *condition, ASTNode *body);
ASTNode *create_for_stmt(ASTNode *init, ASTNode *condition, ASTNode *update,
                         ASTNode *body);
ASTNode *create_for_stmt_with_decl(ASTNode *decl, ASTNode *condition,
                                   ASTNode *update, ASTNode *body);
ASTNode *create_return_stmt(ASTNode *expr);
ASTNode *create_break_stmt(ASTNode *expr);
ASTNode *create_assign_expr(const char *name, ASTNode *expr);
ASTNode *create_array_assign(const char *name, ASTNode *index, ASTNode *expr);
ASTNode *create_compound_assign(const char *name, const char *op,
                                ASTNode *expr);
ASTNode *create_binop(const char *op, ASTNode *left, ASTNode *right);
ASTNode *create_unary(const char *op, ASTNode *operand);
ASTNode *create_pre_incdec(const char *op, const char *name);
ASTNode *create_post_incdec(const char *op, const char *name);
ASTNode *create_array_ref(const char *name, ASTNode *index);
ASTNode *create_func_call(const char *name, ASTNode *args);
ASTNode *create_var_ref(const char *name);
ASTNode *create_number(int64_t value, TypeInfo type);
ASTNode *create_string_literal(const char *str);
ASTNode *create_decl_spec(ASTNode *storage_class, ASTNode *type_qualifier,
                          ASTNode *type_spec);
ASTNode *create_arg_list();
ASTNode *create_array_literal(ASTNode *elements);

// 多次元配列用関数
ASTNode *create_multidim_array_decl(const char *name, ASTNode *dimensions);
ASTNode *create_multidim_array_init(const char *name, ASTNode *dimensions,
                                    ASTNode *init);
ASTNode *create_multidim_array_ref(const char *name, ASTNode *indices);
ASTNode *create_multidim_array_assign(const char *name, ASTNode *indices,
                                      ASTNode *expr);
ASTNode *create_dimension_list();
ASTNode *create_index_list();
ASTNode *create_nested_init_list();
void add_dimension(ASTNode *list, ASTNode *size_expr);
void add_index(ASTNode *list, ASTNode *index_expr);
void add_nested_initializer(ASTNode *list, ASTNode *init);

// typedef用関数
ASTNode *create_typedef_decl(const char *alias_name, ASTNode *base_type,
                             ASTNode *unused);
ASTNode *create_typedef_array_decl(const char *alias_name, ASTNode *base_type,
                                   ASTNode *dimensions);
ASTNode *create_typedef_type(const char *type_name);
ASTNode *create_typedef_array_var(const char *name, ASTNode *size_expr);

// ユーティリティ関数
void add_statement(ASTNode *list, ASTNode *stmt);
void add_parameter(ASTNode *list, ASTNode *param);
void add_argument(ASTNode *list, ASTNode *arg);
void set_declaration_attributes(ASTNode *decl, ASTNode *storage, ASTNode *type);
TypeInfo get_type_info(ASTNode *type_node);
void set_current_type(TypeInfo type);
TypeInfo get_current_type();
void delete_node(ASTNode *node);

// エラー処理
void yyerror(const char *s);
}
