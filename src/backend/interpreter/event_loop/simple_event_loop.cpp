#include "simple_event_loop.h"
#include "../../../common/debug.h"
#include "../../../common/debug_messages.h"
#include "../core/interpreter.h"
#include <iostream>

#ifdef _WIN32
#include <windows.h>
#else
#include <sys/time.h>
#endif

namespace cb {

// 現在時刻をミリ秒で取得（エポックからの経過時間）
static int64_t get_current_time_ms() {
#ifdef _WIN32
    FILETIME ft;
    GetSystemTimeAsFileTime(&ft);
    ULARGE_INTEGER ull;
    ull.LowPart = ft.dwLowDateTime;
    ull.HighPart = ft.dwHighDateTime;
    // 100ns単位からms単位に変換し、UNIXエポック(1970/1/1)からの時間に調整
    return static_cast<int64_t>((ull.QuadPart - 116444736000000000ULL) / 10000);
#else
    struct timeval tv;
    gettimeofday(&tv, nullptr);
    return static_cast<int64_t>(tv.tv_sec) * 1000 + tv.tv_usec / 1000;
#endif
}

static bool has_yield_statement(const ASTNode *node) {
    // Note: Yield statement support is not yet implemented
    // This function is reserved for future use
    return false;
}

SimpleEventLoop::SimpleEventLoop(Interpreter &interpreter)
    : interpreter_(interpreter) {}

int SimpleEventLoop::register_task(AsyncTask task) {
    int task_id = next_task_id_++;
    task.task_id = task_id;

    if (task.function_node && !has_yield_statement(task.function_node)) {
        task.auto_yield = true;
        if (debug_mode) {
            std::cerr << "[SIMPLE_EVENT_LOOP] Auto-yield enabled for "
                      << task.function_name << std::endl;
        }
    }

    tasks_[task_id] = task;
    task_queue_.push_back(task_id);

    if (debug_mode) {
        std::cerr << "[SIMPLE_EVENT_LOOP] Registered task " << task_id << ": "
                  << task.function_name << std::endl;
    }

    return task_id;
}

void SimpleEventLoop::run_one_cycle() {
    if (task_queue_.empty())
        return;

    if (debug_mode) {
        std::cerr << "[SIMPLE_EVENT_LOOP] run_one_cycle: processing 1 task"
                  << std::endl;
    }

    int task_id = task_queue_.front();
    task_queue_.pop_front();

    bool should_continue = execute_one_step(task_id);

    if (should_continue) {
        task_queue_.push_back(task_id);
    } else {
        if (debug_mode) {
            std::cerr << "[SIMPLE_EVENT_LOOP] Task " << task_id << " completed"
                      << std::endl;
        }
    }
}

void SimpleEventLoop::run_until_complete(int task_id) {
    if (debug_mode) {
        std::cerr << "[SimpleEventLoop] Waiting for task " << task_id
                  << std::endl;
    }

    while (true) {
        auto it = tasks_.find(task_id);
        if (it == tasks_.end() || it->second.is_executed)
            break;

        bool should_continue = execute_one_step(task_id);
        if (!should_continue)
            break;

        size_t other_tasks = task_queue_.size();
        for (size_t i = 0; i < other_tasks && i < 3; ++i) {
            if (task_queue_.empty())
                break;

            int other_id = task_queue_.front();
            if (other_id == task_id) {
                task_queue_.pop_front();
                task_queue_.push_back(other_id);
                continue;
            }

            task_queue_.pop_front();
            bool other_continue = execute_one_step(other_id);
            if (other_continue) {
                task_queue_.push_back(other_id);
            }
        }
    }

    if (debug_mode) {
        std::cerr << "[SimpleEventLoop] Task " << task_id << " completed"
                  << std::endl;
    }
}

bool SimpleEventLoop::execute_one_step(int task_id) {
    auto it = tasks_.find(task_id);
    if (it == tasks_.end())
        return false;

    AsyncTask &task = it->second;
    if (task.is_executed)
        return false;

    // v0.12.0: sleep中のタスクは現在時刻をチェック
    if (task.is_sleeping) {
        // 現在時刻を取得
        int64_t current_time = get_current_time_ms();

        if (current_time < task.wake_up_time_ms) {
            // まだ起床時刻に達していない
            if (debug_mode) {
                std::cerr << "[SIMPLE_EVENT_LOOP] Task " << task_id
                          << " still sleeping (wake up in "
                          << (task.wake_up_time_ms - current_time) << "ms)"
                          << std::endl;
            }
            return true; // タスクは継続中（sleep中）
        }

        // 起床時刻に達した
        if (debug_mode) {
            std::cerr << "[SIMPLE_EVENT_LOOP] Task " << task_id
                      << " woke up from sleep" << std::endl;
        }
        task.is_sleeping = false;

        // sleep専用タスク（function_nodeがない場合）は完了とする
        if (task.function_node == nullptr) {
            task.is_executed = true;
            if (debug_mode) {
                std::cerr << "[SIMPLE_EVENT_LOOP] Sleep task " << task_id
                          << " completed" << std::endl;
            }
            return false; // タスク完了
        }
    }

    if (!task.is_started) {
        initialize_task_scope(task);
        task.is_started = true;
    }

    // v0.12.0: EventLoop実行深度を管理（再入防止）
    interpreter_.enter_event_loop();
    interpreter_.set_current_task_id(task_id); // 現在のタスクIDを設定

    interpreter_.push_scope();
    interpreter_.current_scope() = *task.task_scope;

    if (debug_mode) {
        std::cerr << "[SIMPLE_EVENT_LOOP] Executing task " << task_id
                  << " statement " << task.current_statement_index << std::endl;
    }

    try {
        const ASTNode *body = task.function_node->body.get();

        if (body->node_type == ASTNodeType::AST_STMT_LIST) {
            if (task.current_statement_index < body->statements.size()) {
                const ASTNode *stmt =
                    body->statements[task.current_statement_index].get();

                interpreter_.execute_statement(stmt);
                task.current_statement_index++;

                *task.task_scope = interpreter_.current_scope();
                interpreter_.pop_scope();
                interpreter_.set_current_task_id(-1); // タスクIDをリセット
                interpreter_.exit_event_loop(); // EventLoop深度を減らす

                if (task.current_statement_index < body->statements.size()) {
                    return true;
                } else {
                    task.is_executed = true;
                    return false;
                }
            } else {
                task.is_executed = true;
                interpreter_.pop_scope();
                interpreter_.set_current_task_id(-1);
                interpreter_.exit_event_loop(); // EventLoop深度を減らす
                return false;
            }
        } else {
            interpreter_.execute_statement(body);
            task.is_executed = true;
            interpreter_.pop_scope();
            interpreter_.set_current_task_id(-1);
            interpreter_.exit_event_loop(); // EventLoop深度を減らす
            return false;
        }
    } catch (const ReturnException &e) {
        task.is_executed = true;
        task.has_return_value = true;
        task.return_value = e.value;
        task.return_string_value = e.str_value;
        task.return_type = e.type;

        interpreter_.pop_scope();
        interpreter_.set_current_task_id(-1);
        interpreter_.exit_event_loop(); // EventLoop深度を減らす
        return false;
    } catch (...) {
        interpreter_.pop_scope();
        interpreter_.set_current_task_id(-1);
        interpreter_.exit_event_loop(); // EventLoop深度を減らす
        throw;
    }
}

void SimpleEventLoop::initialize_task_scope(AsyncTask &task) {
    task.task_scope = std::make_shared<Scope>();

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

void SimpleEventLoop::sleep_task(int task_id, int64_t duration_ms) {
    auto it = tasks_.find(task_id);
    if (it == tasks_.end())
        return;

    AsyncTask &task = it->second;
    task.is_sleeping = true;
    task.wake_up_time_ms = get_current_time_ms() + duration_ms;

    if (debug_mode) {
        std::cerr << "[SIMPLE_EVENT_LOOP] Task " << task_id << " sleeping for "
                  << duration_ms << "ms" << std::endl;
    }
}

} // namespace cb
