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
    if (debug_mode) {
        debug_print("push_scope: destructor_stacks_ size before: %zu\n",
                    destructor_stacks_.size());
    }

    variable_manager_->push_scope();
    push_defer_scope();
    // v0.10.0: デストラクタスタックも追加
    destructor_stacks_.push_back(
        std::vector<std::pair<std::string, std::string>>());

    if (debug_mode) {
        debug_print("push_scope: destructor_stacks_ size after: %zu\n",
                    destructor_stacks_.size());
    }
}

void Interpreter::push_scope(const std::string &scope_id) {
    if (debug_mode) {
        debug_print(
            "push_scope(scope_id='%s'): destructor_stacks_ size before: %zu\n",
            scope_id.c_str(), destructor_stacks_.size());
    }

    variable_manager_->push_scope();
    push_defer_scope();
    // v0.10.0: デストラクタスタックも追加
    destructor_stacks_.push_back(
        std::vector<std::pair<std::string, std::string>>());

    // TODO:
    // スコープIDの設定（variable_manager内部のスコープスタックにアクセスが必要）
    // 現在は未実装

    if (debug_mode) {
        debug_print(
            "push_scope(scope_id='%s'): destructor_stacks_ size after: %zu\n",
            scope_id.c_str(), destructor_stacks_.size());
    }
}

void Interpreter::pop_scope() {
    if (debug_mode) {
        debug_print("pop_scope: destructor_stacks_ size before: %zu, "
                    "is_calling_destructor: %d, scope_stack_size: %zu\n",
                    destructor_stacks_.size(), is_calling_destructor_,
                    scope_stack.size());
    }

    // v0.10.0: デストラクタをLIFO順で呼び出す（最後に作成された変数から破棄）
    if (!destructor_stacks_.empty()) {
        if (!is_calling_destructor_) {
            // 通常のスコープ終了時：デストラクタを呼び出す
            // まず、このスコープのデストラクタリストのコピーを取得
            std::vector<std::pair<std::string, std::string>> destroy_list =
                destructor_stacks_.back();

            // スタックから削除（デストラクタ呼び出し前に削除して、
            // デストラクタ内でのpush/pop_scopeの影響を受けないようにする）
            destructor_stacks_.pop_back();

            if (debug_mode && !destroy_list.empty()) {
                debug_print("pop_scope: calling %zu destructors\n",
                            destroy_list.size());
            }

            for (auto it = destroy_list.rbegin(); it != destroy_list.rend();
                 ++it) {
                const std::string &var_name = it->first;
                const std::string &struct_type_name = it->second;

                if (debug_mode) {
                    debug_print("Destroying variable %s of type %s\n",
                                var_name.c_str(), struct_type_name.c_str());
                }

                // デストラクタを呼び出す
                call_destructor(var_name, struct_type_name);
            }
        } else {
            // デストラクタ呼び出し中：スタックだけpop（デストラクタは呼ばない）
            destructor_stacks_.pop_back();
        }

        if (debug_mode) {
            debug_print("pop_scope: destructor_stacks_ size after pop: %zu\n",
                        destructor_stacks_.size());
        }
    } else {
        if (debug_mode) {
            debug_print("pop_scope: WARNING - destructor_stacks_ is empty!\n");
        }
    }

    // deferを実行
    pop_defer_scope();

    // 配列参照のコピーバック処理
    // 関数終了時に、参照変数のデータベクトルを元の配列にコピーバック
    // NOTE: current_scope()はvariable_manager_->pop_scope()の前に呼ぶ必要がある
    Scope &scope_to_pop = current_scope();
    for (auto &var_pair : scope_to_pop.variables) {
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

// ========================================================================
// デストラクタスコープ管理（変数スコープは作成しない）
// ========================================================================
void Interpreter::push_destructor_scope() {
    if (debug_mode) {
        debug_print(
            "push_destructor_scope: destructor_stacks_ size before: %zu\n",
            destructor_stacks_.size());
    }

    // 変数スコープは作成しない（variable_manager_->push_scope()を呼ばない）
    // deferスコープとデストラクタスタックのみ作成
    push_defer_scope();
    destructor_stacks_.push_back(
        std::vector<std::pair<std::string, std::string>>());

    if (debug_mode) {
        debug_print(
            "push_destructor_scope: destructor_stacks_ size after: %zu\n",
            destructor_stacks_.size());
    }
}

void Interpreter::pop_destructor_scope() {
    if (debug_mode) {
        debug_print(
            "pop_destructor_scope: destructor_stacks_ size before: %zu, "
            "is_calling_destructor: %d\n",
            destructor_stacks_.size(), is_calling_destructor_);
    }

    // デストラクタをLIFO順で呼び出す（最後に作成された変数から破棄）
    if (!destructor_stacks_.empty()) {
        if (!is_calling_destructor_) {
            // 通常のスコープ終了時：デストラクタを呼び出す
            std::vector<std::pair<std::string, std::string>> destroy_list =
                destructor_stacks_.back();

            destructor_stacks_.pop_back();

            if (debug_mode && !destroy_list.empty()) {
                debug_print("pop_destructor_scope: calling %zu destructors\n",
                            destroy_list.size());
            }

            for (auto it = destroy_list.rbegin(); it != destroy_list.rend();
                 ++it) {
                const std::string &var_name = it->first;
                const std::string &struct_type_name = it->second;

                if (debug_mode) {
                    debug_print("Destroying variable %s of type %s\n",
                                var_name.c_str(), struct_type_name.c_str());
                }

                call_destructor(var_name, struct_type_name);
            }
        } else {
            // デストラクタ呼び出し中：スタックだけpop（デストラクタは呼ばない）
            destructor_stacks_.pop_back();
        }

        if (debug_mode) {
            debug_print("pop_destructor_scope: destructor_stacks_ size after "
                        "pop: %zu\n",
                        destructor_stacks_.size());
        }
    }

    // deferを実行
    pop_defer_scope();

    // 変数スコープはpopしない（variable_manager_->pop_scope()を呼ばない）
}

Scope &Interpreter::current_scope() {
    return variable_manager_->current_scope();
}

// ========================================================================
// Defer管理
// ========================================================================
void Interpreter::push_defer_scope() {
    if (debug_mode) {
        debug_print("push_defer_scope: defer_stacks_ size before: %zu\n",
                    defer_stacks_.size());
    }
    defer_stacks_.push_back(std::vector<const ASTNode *>());
    if (debug_mode) {
        debug_print("push_defer_scope: defer_stacks_ size after: %zu\n",
                    defer_stacks_.size());
    }
}

void Interpreter::pop_defer_scope() {
    if (debug_mode) {
        debug_print("pop_defer_scope: defer_stacks_ size before: %zu\n",
                    defer_stacks_.size());
    }

    if (defer_stacks_.empty()) {
        if (debug_mode) {
            debug_print("pop_defer_scope: defer_stacks_ is empty!\n");
        }
        return;
    }

    // LIFO順でdeferを実行するため、コピーを作成
    // (execute_statement内でスコープが変更される可能性があるため)
    std::vector<const ASTNode *> defers_to_execute = defer_stacks_.back();
    defer_stacks_.pop_back();

    if (debug_mode) {
        debug_print("pop_defer_scope: executing %zu defers\n",
                    defers_to_execute.size());
    }

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
        if (debug_mode) {
            debug_print("add_defer: added defer to stack (stack size: %zu, "
                        "defer count: %zu)\n",
                        defer_stacks_.size(), defer_stacks_.back().size());
        }
    } else {
        if (debug_mode) {
            debug_print("add_defer: WARNING - defer_stacks_ is empty!\n");
        }
    }
}

void Interpreter::execute_defers() { pop_defer_scope(); }

// return文実行前のクリーンアップ処理
// deferとデストラクタを実行するが、変数スコープはpopしない
void Interpreter::execute_pre_return_cleanup() {
    // 1. defer実行（LIFO順）
    if (!defer_stacks_.empty() && !defer_stacks_.back().empty()) {
        std::vector<const ASTNode *> defers = defer_stacks_.back();
        defer_stacks_.pop_back();
        for (auto it = defers.rbegin(); it != defers.rend(); ++it) {
            execute_statement(*it);
        }
    }

    // 2. デストラクタ実行（LIFO順）
    if (!destructor_stacks_.empty() && !destructor_stacks_.back().empty()) {
        const auto &destroy_list = destructor_stacks_.back();
        for (auto it = destroy_list.rbegin(); it != destroy_list.rend(); ++it) {
            const std::string &var_name = it->first;
            const std::string &struct_type_name = it->second;
            call_destructor(var_name, struct_type_name);
        }
        destructor_stacks_.pop_back();
    }
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
