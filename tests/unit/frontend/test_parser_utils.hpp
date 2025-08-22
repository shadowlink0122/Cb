#pragma once
#include "../framework/test_framework.h"
#include "../../../src/common/ast.h"
#include "../../../src/frontend/parser_utils.h"

inline void test_create_stmt_list() {
    ASTNode* node = create_stmt_list();
    ASSERT_NOT_NULL(node);
    ASSERT_EQ(static_cast<int>(ASTNodeType::AST_STMT_LIST), static_cast<int>(node->node_type));
    delete node;
}

inline void test_create_type_node() {
    ASTNode* node = create_type_node(TYPE_INT);
    ASSERT_NOT_NULL(node);
    ASSERT_EQ(static_cast<int>(ASTNodeType::AST_TYPE_SPEC), static_cast<int>(node->node_type));
    ASSERT_EQ(TYPE_INT, node->type_info);
    delete node;
    
    ASTNode* string_node = create_type_node(TYPE_STRING);
    ASSERT_NOT_NULL(string_node);
    ASSERT_EQ(TYPE_STRING, string_node->type_info);
    delete string_node;
}

inline void test_create_storage_spec() {
    ASTNode* node = create_storage_spec(true, false); // static, non-const
    ASSERT_NOT_NULL(node);
    ASSERT_EQ(static_cast<int>(ASTNodeType::AST_STORAGE_SPEC), static_cast<int>(node->node_type));
    ASSERT_TRUE(node->is_static);
    ASSERT_FALSE(node->is_const);
    delete node;
    
    ASTNode* const_node = create_storage_spec(false, true); // non-static, const
    ASSERT_NOT_NULL(const_node);
    ASSERT_FALSE(const_node->is_static);
    ASSERT_TRUE(const_node->is_const);
    delete const_node;
}

inline void test_create_var_decl() {
    ASTNode* node = create_var_decl("test_var");
    ASSERT_NOT_NULL(node);
    ASSERT_EQ(static_cast<int>(ASTNodeType::AST_VAR_DECL), static_cast<int>(node->node_type));
    ASSERT_STREQ("test_var", node->name.c_str());
    delete node;
}

inline void test_create_var_init() {
    // 数値リテラルノードを作成
    ASTNode* literal = new ASTNode(ASTNodeType::AST_NUMBER);
    literal->int_value = 42;
    
    ASTNode* node = create_var_init("initialized_var", literal);
    ASSERT_NOT_NULL(node);
    ASSERT_EQ(static_cast<int>(ASTNodeType::AST_ASSIGN), static_cast<int>(node->node_type));
    ASSERT_STREQ("initialized_var", node->name.c_str());
    ASSERT_NOT_NULL(node->right.get());
    ASSERT_EQ(42, node->right->int_value);
    delete node;
}

inline void test_create_array_decl() {
    // サイズ式として数値リテラルを作成
    ASTNode* size_expr = new ASTNode(ASTNodeType::AST_NUMBER);
    size_expr->int_value = 10;
    
    ASTNode* node = create_array_decl("test_array", size_expr);
    ASSERT_NOT_NULL(node);
    ASSERT_EQ(static_cast<int>(ASTNodeType::AST_ARRAY_DECL), static_cast<int>(node->node_type));
    ASSERT_STREQ("test_array", node->name.c_str());
    ASSERT_NOT_NULL(node->array_size_expr.get());
    ASSERT_EQ(10, node->array_size_expr->int_value);
    delete node;
}

inline void test_create_param_list() {
    ASTNode* node = create_param_list();
    ASSERT_NOT_NULL(node);
    ASSERT_EQ(static_cast<int>(ASTNodeType::AST_STMT_LIST), static_cast<int>(node->node_type));
    delete node;
}

inline void test_create_parameter() {
    ASTNode* type_node = create_type_node(TYPE_INT);
    ASTNode* param = create_parameter(type_node, "param_name");
    ASSERT_NOT_NULL(param);
    ASSERT_EQ(static_cast<int>(ASTNodeType::AST_PARAM_DECL), static_cast<int>(param->node_type));
    ASSERT_STREQ("param_name", param->name.c_str());
    ASSERT_EQ(TYPE_INT, param->type_info);
    delete param;
}

inline void register_parser_utils_tests() {
    test_runner.add_test("frontend", "create_stmt_list", test_create_stmt_list);
    test_runner.add_test("frontend", "create_type_node", test_create_type_node);
    test_runner.add_test("frontend", "create_storage_spec", test_create_storage_spec);
    test_runner.add_test("frontend", "create_var_decl", test_create_var_decl);
    test_runner.add_test("frontend", "create_var_init", test_create_var_init);
    test_runner.add_test("frontend", "create_array_decl", test_create_array_decl);
    test_runner.add_test("frontend", "create_param_list", test_create_param_list);
    test_runner.add_test("frontend", "create_parameter", test_create_parameter);
}
