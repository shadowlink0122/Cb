#include "simple_event_loop.h"
#include "../../../common/debug.h"
#include "../../../common/debug_messages.h"
#include "../core/interpreter.h"
#include <iostream>

// プラットフォーム固有のヘッダー (sleep_task用)
#ifdef _WIN32
#include <windows.h> // GetSystemTimeAsFileTime()
#else
#include <sys/time.h> // gettimeofday()
#endif

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
    }

    debug_msg(DebugMsgId::EVENT_LOOP_REGISTER_TASK, task_id,
              static_cast<int>(task.internal_future.struct_members.size()));

    // Store task before potentially being moved
    debug_msg(DebugMsgId::EVENT_LOOP_STORE_TASK, task_id);

    tasks_[task_id] = task;

    task_queue_.push_back(task_id);

    debug_msg(DebugMsgId::ASYNC_TASK_REGISTER, task.function_name.c_str(),
              task_id);

    return task_id;
}

void SimpleEventLoop::run() {
    if (task_queue_.empty()) {
        return;
    }

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
            debug_msg(DebugMsgId::EVENT_LOOP_TASK_COMPLETED, task_id);
        }
    }
}

// v0.12.0: イベントループを1サイクル実行（1タスクを1ステップだけ）
// async関数呼び出し時にバックグラウンドでタスクを少しずつ実行するために使用
// 協調的マルチタスク: mainとバックグラウンドタスクが交互に実行される
void SimpleEventLoop::run_one_cycle() {
    if (task_queue_.empty()) {
        return;
    }

    debug_msg(DebugMsgId::EVENT_LOOP_RUN_ONE_CYCLE, 1);

    // キューの先頭タスクを1ステップだけ実行
    int task_id = task_queue_.front();
    task_queue_.pop_front();

    // v0.13.0: 現在実行中のタスクはスキップ（再帰実行を防ぐ）
    if (task_id == current_executing_task_id_) {
        debug_msg(DebugMsgId::EVENT_LOOP_SKIP_EXECUTING, task_id);
        // キューに戻す
        task_queue_.push_back(task_id);
        return;
    }

    bool should_continue = execute_one_step(task_id);

    if (should_continue) {
        // タスクがまだ完了していない場合、キューの最後に追加
        task_queue_.push_back(task_id);
    } else {
        // タスク完了
        debug_msg(DebugMsgId::EVENT_LOOP_TASK_COMPLETED, task_id);
    }
}

bool SimpleEventLoop::execute_one_step(int task_id) {
    // 現在実行中のタスクを記録（RAII パターンで自動リセット）
    struct ExecutingTaskGuard {
        int &loop_ref;
        Interpreter &interpreter;
        int prev_loop;
        int prev_interpreter;
        ExecutingTaskGuard(int &lr, Interpreter &interp, int new_val)
            : loop_ref(lr), interpreter(interp), prev_loop(lr),
              prev_interpreter(interp.get_current_executing_task_id()) {
            lr = new_val;
            interp.set_current_executing_task_id(new_val);
        }
        ~ExecutingTaskGuard() {
            loop_ref = prev_loop;
            interpreter.set_current_executing_task_id(prev_interpreter);
        }
    } guard(current_executing_task_id_, interpreter_, task_id);

    auto it = tasks_.find(task_id);
    if (it == tasks_.end()) {
        return false;
    }

    AsyncTask &task = it->second;

    if (task.is_executed) {
        return false;
    }

    // v0.13.0: 待機中のタスクはスキップ
    if (task.is_waiting) {
        // 待機中のタスクが完了したかチェック
        auto waited_it = tasks_.find(task.waiting_for_task_id);
        if (waited_it != tasks_.end() && waited_it->second.is_executed) {
            // 待機タスクが完了したので、待機状態を解除
            task.is_waiting = false;
            task.waiting_for_task_id = -1;
            debug_msg(DebugMsgId::EVENT_LOOP_TASK_RESUME, task_id);
        } else {
            // まだ待機中
            return true; // キューに戻す
        }
    }

    // v0.12.0: sleep中のタスクをチェック
    if (task.is_sleeping) {
        // 現在時刻を取得
        int64_t current_time_ms;
#ifdef _WIN32
        FILETIME ft;
        GetSystemTimeAsFileTime(&ft);
        ULARGE_INTEGER uli;
        uli.LowPart = ft.dwLowDateTime;
        uli.HighPart = ft.dwHighDateTime;
        current_time_ms = (uli.QuadPart / 10000) - 11644473600000LL;
#else
        struct timeval tv;
        gettimeofday(&tv, nullptr);
        current_time_ms = static_cast<int64_t>(tv.tv_sec) * 1000 +
                          static_cast<int64_t>(tv.tv_usec) / 1000;
#endif

        if (current_time_ms < task.wake_up_time_ms) {
            // まだsleep中
            debug_msg(DebugMsgId::SLEEP_TASK_SLEEPING, task_id,
                      task.wake_up_time_ms - current_time_ms);
            return true; // 継続（まだsleep中）
        } else {
            // sleep完了
            task.is_sleeping = false;
            debug_msg(DebugMsgId::SLEEP_TASK_WOKE_UP, task_id);

            // sleep専用タスク（function_nodeがnullptr）の場合、即座に完了
            if (task.function_node == nullptr) {
                task.is_executed = true;

                // Future.is_readyをtrueに設定
                if (task.use_internal_future) {
                    auto ready_it =
                        task.internal_future.struct_members.find("is_ready");
                    if (ready_it != task.internal_future.struct_members.end()) {
                        ready_it->second.value = 1;
                    }
                } else if (task.future_var) {
                    auto ready_it =
                        task.future_var->struct_members.find("is_ready");
                    if (ready_it != task.future_var->struct_members.end()) {
                        ready_it->second.value = 1;
                    }
                }

                return false; // タスク完了
            }
        }
    }

    // v0.12.0: auto_yieldモードを設定（forループなどで自動yieldするため）
    bool prev_auto_yield_mode = interpreter_.is_in_auto_yield_mode();
    if (task.auto_yield) {
        interpreter_.set_auto_yield_mode(true);
    }

    // v0.13.2 FIX: タスクスコープに切り替え
    // 初回実行時はタスクスコープを初期化
    if (!task.is_started) {
        initialize_task_scope(task);
        task.is_started = true;
    }

    // sleep専用タスク（function_nodeがnullptr）の場合、スコープ操作は不要
    if (task.function_node == nullptr) {
        // sleep専用タスクはsleep完了後に即座に完了するため、ここには到達しない
        task.is_executed = true;
        return false;
    }

    // スコープスタックのサイズを記録
    size_t scope_stack_size_before = interpreter_.get_scope_stack().size();

    // 新しいスコープをpushする前に、タスクスコープの内容を一時保存
    Scope task_scope_copy = *task.task_scope;

    // 新しいスコープをpush
    interpreter_.push_scope();

    // pushされたスコープにタスクスコープの変数をコピー
    // (上書きではなく、変数だけをコピー)
    for (auto &var_pair : task_scope_copy.variables) {
        interpreter_.current_scope().variables[var_pair.first] =
            var_pair.second;
    }

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

                // タスクスコープを保存
                *task.task_scope = interpreter_.current_scope();

                // スコープをpopして元のサイズに戻す
                while (interpreter_.get_scope_stack().size() >
                       scope_stack_size_before) {
                    interpreter_.pop_scope();
                }

                // auto_yieldモードを元に戻す
                interpreter_.set_auto_yield_mode(prev_auto_yield_mode);

                // まだステートメントが残っている
                if (task.current_statement_index < body->statements.size()) {
                    // v0.13.0 Phase 2.0:
                    // auto_yieldが有効な場合、毎ステートメント後にyield
                    if (task.auto_yield) {
                        // Auto-yield after statement
                    }
                    return true; // 次のステートメントがあるので継続
                } else {
                    // 全ステートメント実行完了
                    task.is_executed = true;

                    // Future.is_readyをtrueに設定
                    if (task.use_internal_future) {
                        auto ready_it =
                            task.internal_future.struct_members.find(
                                "is_ready");
                        if (ready_it !=
                            task.internal_future.struct_members.end()) {
                            ready_it->second.value = 1;
                        }
                    } else if (task.future_var) {
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
                while (interpreter_.get_scope_stack().size() >
                       scope_stack_size_before) {
                    interpreter_.pop_scope();
                }
                return false;
            }
        } else {
            // STMT_LISTでない場合（通常あり得ない）
            interpreter_.execute_statement(body);
            task.is_executed = true;

            if (task.use_internal_future) {
                auto ready_it =
                    task.internal_future.struct_members.find("is_ready");
                if (ready_it != task.internal_future.struct_members.end()) {
                    ready_it->second.value = 1;
                }
            } else if (task.future_var) {
                auto ready_it =
                    task.future_var->struct_members.find("is_ready");
                if (ready_it != task.future_var->struct_members.end()) {
                    ready_it->second.value = 1;
                }
            }

            interpreter_.set_auto_yield_mode(prev_auto_yield_mode);
            while (interpreter_.get_scope_stack().size() >
                   scope_stack_size_before) {
                interpreter_.pop_scope();
            }
            return false;
        }
    } catch (const YieldException &e) {
        // yieldで中断

        // タスクスコープを保存
        *task.task_scope = interpreter_.current_scope();

        // スコープをpopして元のサイズに戻す
        interpreter_.set_auto_yield_mode(prev_auto_yield_mode);
        while (interpreter_.get_scope_stack().size() >
               scope_stack_size_before) {
            interpreter_.pop_scope();
        }

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
        task.has_return_value = true;
        task.return_type = e.type;

        // タスクに戻り値を保存
        if (e.is_struct) {
            task.return_is_struct = true;
            task.return_struct_value = e.struct_value;
        } else if (e.type == TYPE_STRING) {
            task.return_string_value = e.str_value;
        } else if (e.type == TYPE_FLOAT || e.type == TYPE_DOUBLE ||
                   e.type == TYPE_QUAD) {
            task.return_double_value = e.double_value;
        } else {
            task.return_value = e.value;
        }

        // Future.valueに値を設定
        if (task.use_internal_future) {
            debug_msg(DebugMsgId::EVENT_LOOP_SET_VALUE,
                      static_cast<int>(e.type));

            auto value_it = task.internal_future.struct_members.find("value");
            if (value_it != task.internal_future.struct_members.end()) {
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
            auto ready_it =
                task.internal_future.struct_members.find("is_ready");
            if (ready_it != task.internal_future.struct_members.end()) {
                ready_it->second.value = 1;
                debug_msg(DebugMsgId::EVENT_LOOP_TASK_COMPLETED, task_id);
            }
        } else if (task.future_var) {
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
                debug_msg(DebugMsgId::EVENT_LOOP_TASK_COMPLETED, task_id);
            }
        }

        interpreter_.set_auto_yield_mode(prev_auto_yield_mode);
        while (interpreter_.get_scope_stack().size() >
               scope_stack_size_before) {
            interpreter_.pop_scope();
        }
        return false;
    } catch (...) {
        // その他の例外
        interpreter_.set_auto_yield_mode(prev_auto_yield_mode);
        while (interpreter_.get_scope_stack().size() >
               scope_stack_size_before) {
            interpreter_.pop_scope();
        }
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
        debug_msg(DebugMsgId::EVENT_LOOP_GET_TASK, task_id, "found");
        return &it->second;
    }
    debug_msg(DebugMsgId::EVENT_LOOP_GET_TASK, task_id, "not found");
    return nullptr;
}

// v0.12.0: 特定のタスクが完了するまで実行（await用）
void SimpleEventLoop::run_until_complete(int task_id) {
    auto it = tasks_.find(task_id);
    if (it == tasks_.end()) {
        debug_msg(DebugMsgId::EVENT_LOOP_RUN_UNTIL_COMPLETE, task_id,
                  "not found");
        return;
    }

    if (it->second.is_executed) {
        // 既に完了している
        debug_msg(DebugMsgId::EVENT_LOOP_RUN_UNTIL_COMPLETE, task_id,
                  "already completed");
        return;
    }

    debug_msg(DebugMsgId::EVENT_LOOP_RUN_UNTIL_COMPLETE, task_id, "waiting");

    // ターゲットタスクが完了するまで run_one_cycle を繰り返し呼び出す
    // これにより、すべてのタスクが平等にラウンドロビンで実行される
    while (true) {
        auto target_it = tasks_.find(task_id);
        if (target_it == tasks_.end() || target_it->second.is_executed) {
            debug_msg(DebugMsgId::EVENT_LOOP_RUN_UNTIL_COMPLETE, task_id,
                      "completed");
            break;
        }

        // キューが空で、ターゲットタスクが未完了の場合
        if (task_queue_.empty()) {
            break;
        }

        // 1サイクル実行
        run_one_cycle();
    }
}

// v0.12.0: タスクをsleep状態にする
void SimpleEventLoop::sleep_task(int task_id, int64_t duration_ms) {
    auto it = tasks_.find(task_id);
    if (it == tasks_.end()) {
        return;
    }

    AsyncTask &task = it->second;

    // 現在時刻を取得してwake_up_timeを設定
    // now()関数と同じ実装
    int64_t current_time_ms;
#ifdef _WIN32
    FILETIME ft;
    GetSystemTimeAsFileTime(&ft);
    ULARGE_INTEGER uli;
    uli.LowPart = ft.dwLowDateTime;
    uli.HighPart = ft.dwHighDateTime;
    current_time_ms = (uli.QuadPart / 10000) - 11644473600000LL;
#else
    struct timeval tv;
    gettimeofday(&tv, nullptr);
    current_time_ms = static_cast<int64_t>(tv.tv_sec) * 1000 +
                      static_cast<int64_t>(tv.tv_usec) / 1000;
#endif

    task.is_sleeping = true;
    task.wake_up_time_ms = current_time_ms + duration_ms;

    debug_msg(DebugMsgId::SLEEP_TASK_REGISTER, task_id, duration_ms,
              task.wake_up_time_ms);
}

} // namespace cb
