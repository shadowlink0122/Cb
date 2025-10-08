#include "struct_assignment_manager.h"
#include "core/interpreter.h"
#include "core/ast.h"
#include "common/debug.h"

StructAssignmentManager::StructAssignmentManager(Interpreter *interpreter)
    : interpreter_(interpreter) {}

void StructAssignmentManager::assign_struct_literal(
    const std::string &var_name, const ASTNode *literal_node) {
    // 実装は後で追加
    throw std::runtime_error("Not implemented yet: assign_struct_literal");
}

void StructAssignmentManager::assign_struct_member(
    const std::string &var_name, const std::string &member_name, long value) {
    // 実装は後で追加
    throw std::runtime_error("Not implemented yet: assign_struct_member(int)");
}

void StructAssignmentManager::assign_struct_member(
    const std::string &var_name, const std::string &member_name,
    const std::string &str_value) {
    // 実装は後で追加
    throw std::runtime_error(
        "Not implemented yet: assign_struct_member(string)");
}

void StructAssignmentManager::assign_struct_member(
    const std::string &var_name, const std::string &member_name,
    const Variable &value_var) {
    // 実装は後で追加
    throw std::runtime_error("Not implemented yet: assign_struct_member(var)");
}

void StructAssignmentManager::assign_struct_member_struct(
    const std::string &var_name, const std::string &member_name,
    const Variable &src_struct) {
    // 実装は後で追加
    throw std::runtime_error("Not implemented yet: assign_struct_member_struct");
}

void StructAssignmentManager::assign_struct_member_array_element(
    const std::string &var_name, const std::string &member_name, int index,
    long value) {
    // 実装は後で追加
    throw std::runtime_error(
        "Not implemented yet: assign_struct_member_array_element(int)");
}

void StructAssignmentManager::assign_struct_member_array_element(
    const std::string &var_name, const std::string &member_name, int index,
    const Variable &value_var) {
    // 実装は後で追加
    throw std::runtime_error(
        "Not implemented yet: assign_struct_member_array_element(var)");
}

void StructAssignmentManager::assign_struct_member_array_literal(
    const std::string &var_name, const std::string &member_name,
    const std::vector<long> &values) {
    // 実装は後で追加
    throw std::runtime_error(
        "Not implemented yet: assign_struct_member_array_literal");
}
