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

void Interpreter::pop_scope() {
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
