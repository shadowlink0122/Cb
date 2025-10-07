#include "static_variable_manager.h"
#include "../../../common/ast.h"
#include "../../../common/debug_messages.h"
#include "../core/interpreter.h"

StaticVariableManager::StaticVariableManager(Interpreter *interpreter)
    : interpreter_(interpreter) {}

// ========================================================================
// Static変数管理
// ========================================================================

Variable *
StaticVariableManager::find_static_variable(const std::string &name) {
    std::string static_key =
        interpreter_->get_current_function_name() + "::" + name;
    auto it = static_variables_.find(static_key);
    if (it != static_variables_.end()) {
        return &it->second;
    }
    return nullptr;
}

void StaticVariableManager::create_static_variable(const std::string &name,
                                                    const ASTNode *node) {
    Variable var;
    var.type = node->type_info;
    var.is_const = node->is_const;
    var.is_array = false;
    var.is_assigned = false;
    var.is_multidimensional = false;

    // デフォルト値を設定
    if (var.type == TYPE_STRING) {
        var.str_value = "";
    } else {
        var.value = 0;
    }

    // 初期化式があれば評価して設定
    if (node->init_expr) {
        if (var.type == TYPE_STRING &&
            node->init_expr->node_type == ASTNodeType::AST_STRING_LITERAL) {
            var.str_value = node->init_expr->str_value;
        } else {
            var.value = interpreter_->evaluate(node->init_expr.get());
        }
        var.is_assigned = true;
    }

    // static変数をユニークな名前で保存（関数名+変数名）
    std::string static_key =
        interpreter_->get_current_function_name() + "::" + name;
    static_variables_[static_key] = var;
}

// ========================================================================
// Impl Static変数管理
// ========================================================================

std::string StaticVariableManager::get_impl_static_namespace() const {
    if (!current_impl_context_.is_active) {
        return "";
    }
    return "impl::" + current_impl_context_.interface_name +
           "::" + current_impl_context_.struct_type_name + "::";
}

void StaticVariableManager::enter_impl_context(
    const std::string &interface_name, const std::string &struct_type_name) {
    current_impl_context_.interface_name = interface_name;
    current_impl_context_.struct_type_name = struct_type_name;
    current_impl_context_.is_active = true;
}

void StaticVariableManager::exit_impl_context() {
    current_impl_context_.is_active = false;
    current_impl_context_.interface_name = "";
    current_impl_context_.struct_type_name = "";
}

Variable *
StaticVariableManager::find_impl_static_variable(const std::string &name) {
    std::string ns = get_impl_static_namespace();
    if (ns.empty()) {
        return nullptr;
    }

    std::string full_name = ns + name;
    auto it = impl_static_variables_.find(full_name);
    if (it != impl_static_variables_.end()) {
        return &it->second;
    }
    return nullptr;
}

void StaticVariableManager::create_impl_static_variable(const std::string &name,
                                                        const ASTNode *node) {
    if (!current_impl_context_.is_active) {
        interpreter_->throw_runtime_error_with_location(
            "impl static variable '" + name +
                "' can only be declared inside impl block",
            node);
        return;
    }

    Variable var;
    var.type = node->type_info;
    var.is_const = node->is_const;
    var.is_array = false;
    var.is_assigned = false;
    var.is_multidimensional = false;
    var.is_unsigned = node->is_unsigned;

    // デフォルト値を設定
    if (var.type == TYPE_STRING) {
        var.str_value = "";
    } else if (var.type == TYPE_FLOAT) {
        var.float_value = 0.0f;
    } else if (var.type == TYPE_DOUBLE) {
        var.double_value = 0.0;
    } else {
        var.value = 0;
    }

    // 初期化式があれば評価して設定
    if (node->init_expr) {
        if (var.type == TYPE_STRING &&
            node->init_expr->node_type == ASTNodeType::AST_STRING_LITERAL) {
            var.str_value = node->init_expr->str_value;
        } else if (var.type == TYPE_FLOAT || var.type == TYPE_DOUBLE) {
            TypedValue result =
                interpreter_->evaluate_typed(node->init_expr.get());
            if (var.type == TYPE_FLOAT) {
                var.float_value = static_cast<float>(result.double_value);
            } else {
                var.double_value = result.double_value;
            }
        } else {
            var.value = interpreter_->evaluate(node->init_expr.get());
        }
        var.is_assigned = true;
    }

    // impl static変数を名前空間付きで保存
    std::string full_name = get_impl_static_namespace() + name;
    impl_static_variables_[full_name] = var;

    debug_msg(DebugMsgId::PARSE_VAR_DECL, name.c_str(),
              "impl_static_variable_created");
}
