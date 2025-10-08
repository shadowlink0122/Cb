#include "struct_assignment_manager.h"
#include "../../../common/ast.h"
#include "../../../common/debug.h"
#include "../../../common/debug_messages.h"
#include "core/interpreter.h"
#include "evaluator/expression_evaluator.h"
#include "managers/type_manager.h"
#include "managers/variable_manager.h"
#include <functional>
#include <map>

StructAssignmentManager::StructAssignmentManager(Interpreter *interpreter)
    : interpreter_(interpreter) {}

void StructAssignmentManager::assign_struct_literal(
    const std::string &var_name, const ASTNode *literal_node) {
    // TODO: 実装は次のセッションで移植
    // 現在は何もしない（テスト用）
    throw std::runtime_error("StructAssignmentManager::assign_struct_literal not implemented yet");
}

void StructAssignmentManager::assign_struct_member(
    const std::string &var_name, const std::string &member_name, long value) {
    // TODO: 実装は次のセッションで移植
    throw std::runtime_error("StructAssignmentManager::assign_struct_member(int) not implemented yet");
}

void StructAssignmentManager::assign_struct_member(
    const std::string &var_name, const std::string &member_name,
    const std::string &str_value) {
    // TODO: 実装は次のセッションで移植
    throw std::runtime_error("StructAssignmentManager::assign_struct_member(string) not implemented yet");
}

void StructAssignmentManager::assign_struct_member(
    const std::string &var_name, const std::string &member_name,
    const Variable &value_var) {
    // TODO: 実装は次のセッションで移植
    throw std::runtime_error("StructAssignmentManager::assign_struct_member(var) not implemented yet");
}

void StructAssignmentManager::assign_struct_member_struct(
    const std::string &var_name, const std::string &member_name,
    const Variable &src_struct) {
    // TODO: 実装は次のセッションで移植
    throw std::runtime_error("StructAssignmentManager::assign_struct_member_struct not implemented yet");
}

void StructAssignmentManager::assign_struct_member_array_element(
    const std::string &var_name, const std::string &member_name, int index,
    long value) {
    // TODO: 実装は次のセッションで移植
    throw std::runtime_error("StructAssignmentManager::assign_struct_member_array_element(int) not implemented yet");
}

void StructAssignmentManager::assign_struct_member_array_element(
    const std::string &var_name, const std::string &member_name, int index,
    const Variable &value_var) {
    // TODO: 実装は次のセッションで移植
    throw std::runtime_error("StructAssignmentManager::assign_struct_member_array_element(var) not implemented yet");
}

void StructAssignmentManager::assign_struct_member_array_literal(
    const std::string &var_name, const std::string &member_name,
    const std::vector<long> &values) {
    // TODO: 実装は次のセッションで移植
    throw std::runtime_error("StructAssignmentManager::assign_struct_member_array_literal not implemented yet");
}
