#include "simple_event_loop.h"
#include "../../../common/debug.h"
#include "../../../common/debug_messages.h"
#include "../core/interpreter.h"
#include <iostream>

namespace cb {

// 関数内にyield文があるかどうかを再帰的にチェック
static bool has_yield_statement(const ASTNode *node) {
    if (!node) {
        return false;
    }

    // yield文を発見
    if (node->node_type == ASTNodeType::AST_YIELD_STMT) {
        return true;
    }

    // 子ノードを再帰的にチェック
    if (node->left && has_yield_statement(node->left.get())) {
        return true;
    }
    if (node->right && has_yield_statement(node->right.get())) {
        return true;
    }
    if (node->body && has_yield_statement(node->body.get())) {
        return true;
    }

    // statements配列をチェック
    for (const auto &stmt : node->statements) {
        if (has_yield_statement(stmt.get())) {
            return true;
        }
    }

    return false;
}

SimpleEventLoop::SimpleEventLoop(Interpreter &interpreter)
    : interpreter_(interpreter) {}

int SimpleEventLoop::register_task(AsyncTask task) {
    int task_id = next_task_id_++;
    task.task_id = task_id;

    // v0.13.0 Phase 2.0: yield文がない場合は自動yieldを有効化
    // 各ステートメント実行後に自動的にyieldして協調的マルチタスクを実現
    if (task.function_node && !has_yield_statement(task.function_node)) {
        task.auto_yield = true;
        if (debug_mode) {
            std::cerr << "[SIMPLE_EVENT_LOOP] Auto-yield enabled for "
                      << task.function_name << " (no explicit yield found)"
                      << std::endl;
        }
    }

    tasks_[task_id] = task;
    task_queue_.push_back(task_id);

    debug_msg(DebugMsgId::ASYNC_TASK_REGISTER, task.function_name.c_str(),
              task_id);

    return task_id;
}

void SimpleEventLoop::run() {
    if (task_queue_.empty()) {
        debug_msg(DebugMsgId::EVENT_LOOP_EMPTY_QUEUE);
        return;
    }

    debug_msg(DebugMsgId::EVENT_LOOP_START,
              static_cast<int>(task_queue_.size()));

    // 全タスクが完了するまでラウンドロビン実行
    while (!task_queue_.empty()) {
        int task_id = task_queue_.front();
        task_queue_.pop_front();

        bool should_continue = execute_one_step(task_id);

        if (should_continue) {
            // タスクがまだ完了していない場合、キューの最後に追加
            task_queue_.push_back(task_id);
        } else {
            // タスク完了
            debug_msg(DebugMsgId::EVENT_LOOP_TASK_COMPLETE, task_id);
        }
    }

    debug_msg(DebugMsgId::EVENT_LOOP_COMPLETE);
}

// v0.12.0: イベントループを1サイクル実行（1タスクを1ステップだけ）
// async関数呼び出し時にバックグラウンドでタスクを少しずつ実行するために使用
// 協調的マルチタスク: mainとバックグラウンドタスクが交互に実行される
void SimpleEventLoop::run_one_cycle() {
    if (task_queue_.empty()) {
        return;
    }

    if (debug_mode) {
        std::cerr << "[SIMPLE_EVENT_LOOP] run_one_cycle: processing 1 task"
                  << std::endl;
    }

    // キューの先頭タスクを1ステップだけ実行
    int task_id = task_queue_.front();
    task_queue_.pop_front();

    bool should_continue = execute_one_step(task_id);

    if (should_continue) {
        // タスクがまだ完了していない場合、キューの最後に追加
        task_queue_.push_back(task_id);
    } else {
        // タスク完了
        debug_msg(DebugMsgId::EVENT_LOOP_TASK_COMPLETE, task_id);
    }
}

bool SimpleEventLoop::execute_one_step(int task_id) {
    auto it = tasks_.find(task_id);
    if (it == tasks_.end()) {
        return false;
    }

    AsyncTask &task = it->second;

    if (task.is_executed) {
        return false;
    }

    // 初回実行時: タスクスコープを初期化
    if (!task.is_started) {
        initialize_task_scope(task);
        task.is_started = true;
    }

    // v0.12.0: auto_yieldモードを設定（forループなどで自動yieldするため）
    bool prev_auto_yield_mode = interpreter_.is_in_auto_yield_mode();
    if (task.auto_yield) {
        interpreter_.set_auto_yield_mode(true);
    }

    // タスクスコープに切り替え
    interpreter_.push_scope();
    interpreter_.current_scope() = *task.task_scope;

    debug_msg(DebugMsgId::EVENT_LOOP_EXECUTE, task_id,
              task.current_statement_index);

    try {
        // トップレベルのステートメントを1つ実行
        const ASTNode *body = task.function_node->body.get();

        if (body->node_type == ASTNodeType::AST_STMT_LIST) {
            if (task.current_statement_index < body->statements.size()) {
                const ASTNode *stmt =
                    body->statements[task.current_statement_index].get();

                // ステートメントを実行
                interpreter_.execute_statement(stmt);

                // 次のステートメントへ進む
                task.current_statement_index++;

                // スコープを保存
                *task.task_scope = interpreter_.current_scope();
                interpreter_.pop_scope();

                // auto_yieldモードを元に戻す
                interpreter_.set_auto_yield_mode(prev_auto_yield_mode);

                // まだステートメントが残っている
                if (task.current_statement_index < body->statements.size()) {
                    // v0.13.0 Phase 2.0:
                    // auto_yieldが有効な場合、毎ステートメント後にyield
                    if (task.auto_yield) {
                        if (debug_mode) {
                            std::cerr
                                << "[SIMPLE_EVENT_LOOP] Auto-yield after stmt "
                                << (task.current_statement_index - 1)
                                << " in task " << task_id << std::endl;
                        }
                    }
                    return true; // 次のステートメントがあるので継続
                } else {
                    // 全ステートメント実行完了
                    task.is_executed = true;

                    // Future.is_readyをtrueに設定
                    if (task.future_var) {
                        auto ready_it =
                            task.future_var->struct_members.find("is_ready");
                        if (ready_it != task.future_var->struct_members.end()) {
                            ready_it->second.value = 1;
                        }
                    }

                    return false;
                }
            } else {
                // 既に全ステートメント実行済み
                task.is_executed = true;
                interpreter_.set_auto_yield_mode(prev_auto_yield_mode);
                interpreter_.pop_scope();
                return false;
            }
        } else {
            // STMT_LISTでない場合（通常あり得ない）
            interpreter_.execute_statement(body);
            task.is_executed = true;

            if (task.future_var) {
                auto ready_it =
                    task.future_var->struct_members.find("is_ready");
                if (ready_it != task.future_var->struct_members.end()) {
                    ready_it->second.value = 1;
                }
            }

            interpreter_.set_auto_yield_mode(prev_auto_yield_mode);
            interpreter_.pop_scope();
            return false;
        }
    } catch (const YieldException &e) {
        // yieldで中断
        debug_msg(DebugMsgId::EVENT_LOOP_TASK_YIELD, task_id);

        // スコープを保存
        *task.task_scope = interpreter_.current_scope();
        interpreter_.set_auto_yield_mode(prev_auto_yield_mode);
        interpreter_.pop_scope();

        // v0.13.0 Phase 2.0:
        // - ループ内の自動yield (e.is_from_loop == true):
        //   同じステートメント(forループ)を継続するため、statement_indexはそのまま
        // - 明示的なyield文 (e.is_from_loop == false):
        //   yield後のコードを実行するため、次のステートメントに進む
        if (!e.is_from_loop) {
            task.current_statement_index++;
        }

        return true; // キューに戻す
    } catch (const ReturnException &e) {
        // return文で完了
        task.is_executed = true;

        // Future.valueに値を設定
        if (task.future_var) {
            if (debug_mode) {
                std::cerr << "[SIMPLE_EVENT_LOOP] Setting return value "
                          << e.value << " to Future" << std::endl;
            }

            auto value_it = task.future_var->struct_members.find("value");
            if (value_it != task.future_var->struct_members.end()) {
                if (e.type == TYPE_STRING) {
                    value_it->second.type = TYPE_STRING;
                    value_it->second.str_value = e.str_value;
                } else if (e.type == TYPE_FLOAT || e.type == TYPE_DOUBLE ||
                           e.type == TYPE_QUAD) {
                    value_it->second.type = e.type;
                    value_it->second.double_value = e.double_value;
                } else if (e.is_struct) {
                    value_it->second = e.struct_value;
                } else {
                    value_it->second.type = TYPE_INT;
                    value_it->second.value = e.value;
                }
                value_it->second.is_assigned = true;
            }

            // is_readyをtrueに設定
            auto ready_it = task.future_var->struct_members.find("is_ready");
            if (ready_it != task.future_var->struct_members.end()) {
                ready_it->second.value = 1;

                if (debug_mode) {
                    std::cerr << "[SIMPLE_EVENT_LOOP] Set is_ready=true"
                              << std::endl;
                }
            }
        } else {
            if (debug_mode) {
                std::cerr
                    << "[SIMPLE_EVENT_LOOP] Warning: task.future_var is null!"
                    << std::endl;
            }
        }

        interpreter_.set_auto_yield_mode(prev_auto_yield_mode);
        interpreter_.pop_scope();
        return false;
    } catch (...) {
        // その他の例外
        interpreter_.set_auto_yield_mode(prev_auto_yield_mode);
        interpreter_.pop_scope();
        throw;
    }
}

void SimpleEventLoop::initialize_task_scope(AsyncTask &task) {
    task.task_scope = std::make_shared<Scope>();

    // 引数をタスクスコープに設定
    const ASTNode *func = task.function_node;
    for (size_t i = 0; i < task.args.size() && i < func->parameters.size();
         i++) {
        const auto &param = func->parameters[i];
        task.task_scope->variables[param->name] = task.args[i];
    }
}

bool SimpleEventLoop::is_empty() const { return task_queue_.empty(); }

bool SimpleEventLoop::has_tasks() const { return !tasks_.empty(); }

size_t SimpleEventLoop::task_count() const { return tasks_.size(); }

AsyncTask *SimpleEventLoop::get_task(int task_id) {
    auto it = tasks_.find(task_id);
    if (it != tasks_.end()) {
        return &it->second;
    }
    return nullptr;
}

} // namespace cb
