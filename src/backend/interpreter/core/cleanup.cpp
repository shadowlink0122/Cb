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
void Interpreter::push_scope() {
    variable_manager_->push_scope();
    push_defer_scope();
}

void Interpreter::pop_scope() {
    // deferを実行
    pop_defer_scope();

    // 配列参照のコピーバック処理
    // 関数終了時に、参照変数のデータベクトルを元の配列にコピーバック
    for (auto &var_pair : current_scope().variables) {
        Variable &var = var_pair.second;
        if (var.is_reference && var.is_array) {
            // 元の配列へのポインタを取得
            Variable *original_array = reinterpret_cast<Variable *>(var.value);
            if (original_array) {

                // データベクトルをコピーバック
                if (var.is_multidimensional) {
                    original_array->multidim_array_values =
                        var.multidim_array_values;
                    original_array->multidim_array_float_values =
                        var.multidim_array_float_values;
                    original_array->multidim_array_double_values =
                        var.multidim_array_double_values;
                    original_array->multidim_array_quad_values =
                        var.multidim_array_quad_values;
                    original_array->multidim_array_strings =
                        var.multidim_array_strings;
                } else {
                    original_array->array_values = var.array_values;
                    original_array->array_float_values = var.array_float_values;
                    original_array->array_double_values =
                        var.array_double_values;
                    original_array->array_quad_values = var.array_quad_values;
                    original_array->array_strings = var.array_strings;
                }
            }
        }
    }

    variable_manager_->pop_scope();
}

Scope &Interpreter::current_scope() {
    return variable_manager_->current_scope();
}

// ========================================================================
// Defer管理
// ========================================================================
void Interpreter::push_defer_scope() {
    defer_stacks_.push_back(std::vector<const ASTNode *>());
}

void Interpreter::pop_defer_scope() {
    if (defer_stacks_.empty()) {
        return;
    }

    // LIFO順でdeferを実行するため、コピーを作成
    // (execute_statement内でスコープが変更される可能性があるため)
    std::vector<const ASTNode *> defers_to_execute = defer_stacks_.back();
    defer_stacks_.pop_back();

    for (auto it = defers_to_execute.rbegin(); it != defers_to_execute.rend();
         ++it) {
        try {
            execute_statement(*it);
        } catch (...) {
            // deferの実行中のエラーは無視して次のdeferを実行
            // (Goの仕様と同様)
        }
    }
}

void Interpreter::add_defer(const ASTNode *stmt) {
    if (!defer_stacks_.empty()) {
        defer_stacks_.back().push_back(stmt);
    }
}

void Interpreter::execute_defers() { pop_defer_scope(); }

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
