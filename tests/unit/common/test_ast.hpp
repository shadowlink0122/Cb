#pragma once
#include "../framework/test_framework.hpp"
#include "../../../src/common/ast.h"
#include <memory>

inline void test_ast_node_creation() {
    ASTNode node(ASTNodeType::AST_NUMBER);
    ASSERT_EQ(static_cast<int>(ASTNodeType::AST_NUMBER), static_cast<int>(node.node_type));
}

inline void test_ast_node_number_literal() {
    ASTNode node(ASTNodeType::AST_NUMBER);
    node.int_value = 42;
    ASSERT_EQ(42, node.int_value);
}

inline void test_ast_node_string_literal() {
    ASTNode node(ASTNodeType::AST_STRING_LITERAL);
    node.str_value = "Hello World";
    ASSERT_STREQ("Hello World", node.str_value.c_str());
}

inline void test_ast_node_variable() {
    ASTNode node(ASTNodeType::AST_VARIABLE);
    node.name = "test_variable";
    ASSERT_STREQ("test_variable", node.name.c_str());
}

inline void test_ast_node_type_info() {
    ASTNode node(ASTNodeType::AST_TYPE_SPEC);
    node.type_info = TYPE_INT;
    ASSERT_EQ(TYPE_INT, node.type_info);
}

inline void test_ast_node_storage_class() {
    ASTNode node(ASTNodeType::AST_STORAGE_SPEC);
    node.is_static = true;
    node.is_const = false;
    ASSERT_TRUE(node.is_static);
    ASSERT_FALSE(node.is_const);
}

inline void test_ast_node_binary_op() {
    ASTNode node(ASTNodeType::AST_BINARY_OP);
    node.op = "+";
    
    // 左辺
    auto left = std::make_unique<ASTNode>(ASTNodeType::AST_NUMBER);
    left->int_value = 10;
    
    // 右辺
    auto right = std::make_unique<ASTNode>(ASTNodeType::AST_NUMBER);
    right->int_value = 20;
    
    node.left = std::move(left);
    node.right = std::move(right);
    
    ASSERT_STREQ("+", node.op.c_str());
    ASSERT_NOT_NULL(node.left.get());
    ASSERT_NOT_NULL(node.right.get());
    ASSERT_EQ(10, node.left->int_value);
    ASSERT_EQ(20, node.right->int_value);
}

inline void test_ast_node_function_declaration() {
    ASTNode node(ASTNodeType::AST_FUNC_DECL);
    node.name = "test_function";
    node.type_info = TYPE_INT;
    node.is_static = false;
    node.is_const = false;
    
    ASSERT_STREQ("test_function", node.name.c_str());
    ASSERT_EQ(TYPE_INT, node.type_info);
    ASSERT_FALSE(node.is_static);
    ASSERT_FALSE(node.is_const);
}

inline void test_ast_node_parameter() {
    // ASTNodeのparameter機能をテスト
    ASTNode node(ASTNodeType::AST_PARAM_DECL);
    node.name = "param_name";
    node.type_info = TYPE_STRING;
    
    ASSERT_STREQ("param_name", node.name.c_str());
    ASSERT_EQ(TYPE_STRING, node.type_info);
}

inline void test_ast_node_children() {
    ASTNode parent(ASTNodeType::AST_STMT_LIST);
    
    auto child1 = std::make_unique<ASTNode>(ASTNodeType::AST_NUMBER);
    child1->int_value = 1;
    
    auto child2 = std::make_unique<ASTNode>(ASTNodeType::AST_NUMBER);
    child2->int_value = 2;
    
    parent.children.push_back(std::move(child1));
    parent.children.push_back(std::move(child2));
    
    ASSERT_EQ(2, parent.children.size());
    ASSERT_EQ(1, parent.children[0]->int_value);
    ASSERT_EQ(2, parent.children[1]->int_value);
}

inline void register_ast_tests() {
    RUN_TEST("ast_node_creation", test_ast_node_creation);
    RUN_TEST("ast_node_number_literal", test_ast_node_number_literal);
    RUN_TEST("ast_node_string_literal", test_ast_node_string_literal);
    RUN_TEST("ast_node_variable", test_ast_node_variable);
    RUN_TEST("ast_node_type_info", test_ast_node_type_info);
    RUN_TEST("ast_node_storage_class", test_ast_node_storage_class);
    RUN_TEST("ast_node_binary_op", test_ast_node_binary_op);
    RUN_TEST("ast_node_function_declaration", test_ast_node_function_declaration);
    RUN_TEST("ast_node_parameter", test_ast_node_parameter);
    RUN_TEST("ast_node_children", test_ast_node_children);
}
