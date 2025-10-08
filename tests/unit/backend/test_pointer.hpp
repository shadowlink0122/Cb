#pragma once
#include "../framework/test_framework.hpp"
#include "../../../src/backend/interpreter/core/interpreter.h"
#include "../../../src/backend/interpreter/managers/types/manager.h"
#include "../../../src/backend/interpreter/services/variable_access_service.h"

inline void test_struct_pointer_member_metadata() {
    Interpreter interpreter(false);

    StructDefinition node_def("Node");
    node_def.add_member("value", TYPE_INT);
    node_def.add_member("next",
                        TYPE_POINTER,
                        "",
                        true,
                        1,
                        "Node",
                        TYPE_STRUCT);

    interpreter.register_struct_definition("Node", node_def);
    interpreter.push_scope();

    ASTNode node_decl(ASTNodeType::AST_VAR_DECL);
    node_decl.name = "head";
    node_decl.type_info = TYPE_STRUCT;
    node_decl.type_name = "Node";

    interpreter.execute_statement(&node_decl);

    Variable *head_var = interpreter.find_variable("head");
    ASSERT_NOT_NULL(head_var);

    auto member_it = head_var->struct_members.find("next");
    ASSERT_TRUE(member_it != head_var->struct_members.end());

    const Variable &next_member = member_it->second;
    ASSERT_TRUE(next_member.is_pointer);
    ASSERT_EQ(1, next_member.pointer_depth);
    ASSERT_STREQ("Node", next_member.pointer_base_type_name.c_str());
    ASSERT_TRUE(next_member.pointer_base_type == TYPE_STRUCT);

    interpreter.pop_scope();
}

inline void test_struct_double_pointer_metadata() {
    Interpreter interpreter(false);

    StructDefinition node_def("Node");
    node_def.add_member("value", TYPE_INT);
    node_def.add_member("next",
                        TYPE_POINTER,
                        "",
                        true,
                        1,
                        "Node",
                        TYPE_STRUCT);
    node_def.add_member("parent",
                        TYPE_POINTER,
                        "",
                        true,
                        2,
                        "Node",
                        TYPE_STRUCT);

    interpreter.register_struct_definition("Node", node_def);
    interpreter.push_scope();

    ASTNode node_decl(ASTNodeType::AST_VAR_DECL);
    node_decl.name = "root";
    node_decl.type_info = TYPE_STRUCT;
    node_decl.type_name = "Node";

    interpreter.execute_statement(&node_decl);

    Variable *root_var = interpreter.find_variable("root");
    ASSERT_NOT_NULL(root_var);

    auto parent_it = root_var->struct_members.find("parent");
    ASSERT_TRUE(parent_it != root_var->struct_members.end());

    const Variable &parent_member = parent_it->second;
    ASSERT_TRUE(parent_member.is_pointer);
    ASSERT_EQ(2, parent_member.pointer_depth);
    ASSERT_STREQ("Node", parent_member.pointer_base_type_name.c_str());
    ASSERT_TRUE(parent_member.pointer_base_type == TYPE_STRUCT);

    interpreter.pop_scope();
}

inline void test_struct_primitive_pointer_metadata() {
    Interpreter interpreter(false);

    StructDefinition buffer_def("Buffer");
    buffer_def.add_member("length", TYPE_INT);
    buffer_def.add_member("data",
                          TYPE_POINTER,
                          "",
                          true,
                          1,
                          "int",
                          TYPE_INT);

    interpreter.register_struct_definition("Buffer", buffer_def);
    interpreter.push_scope();

    ASTNode buffer_decl(ASTNodeType::AST_VAR_DECL);
    buffer_decl.name = "buf";
    buffer_decl.type_info = TYPE_STRUCT;
    buffer_decl.type_name = "Buffer";

    interpreter.execute_statement(&buffer_decl);

    Variable *buffer_var = interpreter.find_variable("buf");
    ASSERT_NOT_NULL(buffer_var);

    auto data_it = buffer_var->struct_members.find("data");
    ASSERT_TRUE(data_it != buffer_var->struct_members.end());

    const Variable &data_member = data_it->second;
    ASSERT_TRUE(data_member.is_pointer);
    ASSERT_EQ(1, data_member.pointer_depth);
    ASSERT_STREQ("int", data_member.pointer_base_type_name.c_str());
    ASSERT_EQ(TYPE_INT, data_member.pointer_base_type);

    interpreter.pop_scope();
}

inline void test_struct_private_member_metadata() {
    Interpreter interpreter(false);

    StructDefinition secure_def("Secure");
    secure_def.add_member("id", TYPE_INT, "", false, 0, "", TYPE_UNKNOWN, true);
    secure_def.add_member("name", TYPE_STRING);

    interpreter.register_struct_definition("Secure", secure_def);
    interpreter.push_scope();

    ASTNode secure_decl(ASTNodeType::AST_VAR_DECL);
    secure_decl.name = "config";
    secure_decl.type_info = TYPE_STRUCT;
    secure_decl.type_name = "Secure";

    interpreter.execute_statement(&secure_decl);

    Variable *config_var = interpreter.find_variable("config");
    ASSERT_NOT_NULL(config_var);

    auto secret_it = config_var->struct_members.find("id");
    ASSERT_TRUE(secret_it != config_var->struct_members.end());
    ASSERT_TRUE(secret_it->second.is_private_member);

    auto name_it = config_var->struct_members.find("name");
    ASSERT_TRUE(name_it != config_var->struct_members.end());
    ASSERT_FALSE(name_it->second.is_private_member);

    Variable *direct_secret = interpreter.find_variable("config.id");
    ASSERT_NOT_NULL(direct_secret);
    ASSERT_TRUE(direct_secret->is_private_member);

    Variable *direct_name = interpreter.find_variable("config.name");
    ASSERT_NOT_NULL(direct_name);
    ASSERT_FALSE(direct_name->is_private_member);

    interpreter.pop_scope();
}

inline void test_struct_private_member_chain_access() {
    Interpreter interpreter(false);

    StructDefinition secure_def("Secure");
    secure_def.add_member("secret", TYPE_INT, "", false, 0, "", TYPE_UNKNOWN, true);

    interpreter.register_struct_definition("Secure", secure_def);
    interpreter.push_scope();

    ASTNode secure_decl(ASTNodeType::AST_VAR_DECL);
    secure_decl.name = "secure";
    secure_decl.type_info = TYPE_STRUCT;
    secure_decl.type_name = "Secure";

    interpreter.execute_statement(&secure_decl);

    VariableAccessService access_service(&interpreter);

    Variable *member = access_service.find_struct_member_safe("secure", "secret", "unit-test");
    ASSERT_NOT_NULL(member);
    ASSERT_TRUE(member->is_private_member);

    Variable *member_cached = access_service.find_struct_member_safe("secure", "secret", "unit-test");
    ASSERT_TRUE(member == member_cached);

    interpreter.pop_scope();
}

inline void test_union_detection_for_pointer_base() {
    Interpreter interpreter(false);
    TypeManager *type_manager = interpreter.get_type_manager();

    UnionDefinition union_def("Result");
    union_def.add_allowed_type(TYPE_INT);
    type_manager->register_union_typedef("Result", union_def);

    Variable pointer_var;
    pointer_var.is_pointer = true;
    pointer_var.pointer_depth = 1;
    pointer_var.pointer_base_type_name = "Result";

    ASSERT_TRUE(type_manager->is_union_type(pointer_var));
}

inline void register_pointer_tests() {
    RUN_TEST("struct_pointer_member_metadata", test_struct_pointer_member_metadata);
    RUN_TEST("struct_double_pointer_metadata", test_struct_double_pointer_metadata);
    RUN_TEST("struct_primitive_pointer_metadata", test_struct_primitive_pointer_metadata);
    RUN_TEST("struct_private_member_metadata", test_struct_private_member_metadata);
    RUN_TEST("struct_private_member_chain_access", test_struct_private_member_chain_access);
    RUN_TEST("union_detection_for_pointer_base", test_union_detection_for_pointer_base);
}
