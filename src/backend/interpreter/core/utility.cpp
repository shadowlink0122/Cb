#include "utility.h"
#include "../../../common/ast.h"
#include "core/error_handler.h"
#include "interpreter.h"
#include "managers/arrays/manager.h"
#include "managers/types/manager.h"
#include "managers/variables/manager.h"
#include "managers/variables/static.h"
#include <string>
#include <vector>

// ========================================================================
// 型解決ユーティリティ
// ========================================================================
std::string Interpreter::resolve_typedef(const std::string &type_name) {
    return type_manager_->resolve_typedef(type_name);
}

TypeInfo Interpreter::resolve_type_alias(TypeInfo base_type,
                                         const std::string &type_name) {
    std::string resolved_type = type_manager_->resolve_typedef(type_name);
    if (resolved_type != type_name) {
        return type_manager_->string_to_type_info(resolved_type);
    }
    return base_type;
}

TypeInfo Interpreter::string_to_type_info(const std::string &type_str) {
    return type_manager_->string_to_type_info(type_str);
}

void Interpreter::check_type_range(TypeInfo type, int64_t value,
                                   const std::string &name, bool is_unsigned) {
    type_manager_->check_type_range(type, value, name, is_unsigned);
}

// ========================================================================
// 配列操作ヘルパー
// ========================================================================
std::string Interpreter::extract_array_name(const ASTNode *node) {
    return variable_manager_->extract_array_name(node);
}

std::vector<int64_t> Interpreter::extract_array_indices(const ASTNode *node) {
    return variable_manager_->extract_array_indices(node);
}

std::string Interpreter::extract_array_element_name(const ASTNode *node) {
    // 配列要素名を生成 (例: arr[0] -> "arr[0]")
    std::string array_name = extract_array_name(node);
    std::vector<int64_t> indices = extract_array_indices(node);

    std::string element_name = array_name;
    for (int64_t index : indices) {
        element_name += "[" + std::to_string(index) + "]";
    }

    return element_name;
}

int64_t Interpreter::getMultidimensionalArrayElement(
    const Variable &var, const std::vector<int64_t> &indices) {
    return array_manager_->getMultidimensionalArrayElement(var, indices);
}

// ========================================================================
// 変数検索ヘルパー
// ========================================================================
Variable *Interpreter::find_variable(const std::string &name) {
    return variable_manager_->find_variable(name);
}

std::string
Interpreter::find_variable_name_by_address(const Variable *target_var) {
    if (!target_var) {
        return "";
    }

    // 現在のスコープスタックから検索
    // 全スコープを逆順に検索（最新のスコープから）
    if (!scope_stack.empty()) {
        for (auto it = scope_stack.rbegin(); it != scope_stack.rend(); ++it) {
            for (const auto &[name, var] : it->variables) {
                if (&var == target_var) {
                    return name;
                }
            }
        }
    }

    // グローバルスコープも検索
    for (const auto &[name, var] : global_scope.variables) {
        if (&var == target_var) {
            return name;
        }
    }

    return "";
}

std::string Interpreter::find_variable_name(const Variable *target_var) {
    if (!target_var)
        return "";

    // VariableManagerから変数名を取得
    return variable_manager_->find_variable_name(target_var);
}

// ========================================================================
// Static変数管理
// ========================================================================
Variable *Interpreter::find_static_variable(const std::string &name) {
    return static_variable_manager_->find_static_variable(name);
}

void Interpreter::create_static_variable(const std::string &name,
                                         const ASTNode *node) {
    static_variable_manager_->create_static_variable(name, node);
}

Variable *Interpreter::find_impl_static_variable(const std::string &name) {
    return static_variable_manager_->find_impl_static_variable(name);
}

void Interpreter::create_impl_static_variable(const std::string &name,
                                              const ASTNode *node) {
    static_variable_manager_->create_impl_static_variable(name, node);
}

void Interpreter::enter_impl_context(const std::string &interface_name,
                                     const std::string &struct_type_name) {
    static_variable_manager_->enter_impl_context(interface_name,
                                                 struct_type_name);
}

void Interpreter::exit_impl_context() {
    static_variable_manager_->exit_impl_context();
}

std::string Interpreter::get_impl_static_namespace() const {
    return static_variable_manager_->get_impl_static_namespace();
}

// ========================================================================
// エラー報告ヘルパー
// ========================================================================
void Interpreter::throw_runtime_error_with_location(const std::string &message,
                                                    const ASTNode *node) {
    print_error_with_ast_location(message, node);
    throw std::runtime_error(message);
}

void Interpreter::print_error_at_node(const std::string &message,
                                      const ASTNode *node) {
    print_error_with_ast_location(message, node);
}
