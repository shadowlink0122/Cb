#include "cleanup.h"
#include "interpreter.h"
#include "managers/types/interfaces.h"
#include "managers/variables/manager.h"

// ========================================================================
// デストラクタは interpreter.cpp に残しています
// （unique_ptrの完全な型定義が必要なため）
// ========================================================================

// ========================================================================
// スコープ管理
// ========================================================================
void Interpreter::push_scope() { variable_manager_->push_scope(); }

void Interpreter::pop_scope() { variable_manager_->pop_scope(); }

Scope &Interpreter::current_scope() {
    return variable_manager_->current_scope();
}

// ========================================================================
// 一時変数管理
// ========================================================================
void Interpreter::add_temp_variable(const std::string &name,
                                    const Variable &var) {
    interface_operations_->add_temp_variable(name, var);
}

void Interpreter::remove_temp_variable(const std::string &name) {
    interface_operations_->remove_temp_variable(name);
}

void Interpreter::clear_temp_variables() {
    interface_operations_->clear_temp_variables();
}
