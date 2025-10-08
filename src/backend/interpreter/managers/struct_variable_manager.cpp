#include "managers/struct_variable_manager.h"
#include "../../../common/ast.h"
#include "../../../common/debug.h"
#include "core/interpreter.h"
#include "managers/type_manager.h"
#include "managers/variable_manager.h"

StructVariableManager::StructVariableManager(Interpreter *interpreter)
    : interpreter_(interpreter) {}

void StructVariableManager::create_struct_variable(
    const std::string &var_name, const std::string &struct_type_name) {
    // Implementation moved from interpreter.cpp
    // Delegate to interpreter for now - will be implemented in next step
    interpreter_->create_struct_variable(var_name, struct_type_name);
}

void StructVariableManager::create_struct_member_variables_recursively(
    const std::string &base_path, const std::string &struct_type_name,
    Variable &parent_var) {
    // Implementation moved from interpreter.cpp
    // Delegate to interpreter for now - will be implemented in next step
    interpreter_->create_struct_member_variables_recursively(
        base_path, struct_type_name, parent_var);
}
