#include "event_loop.h"

#include <algorithm>
#include <thread>

namespace cb {

void EventLoop::schedule_task(std::function<void()> task) {
    task_queue_.push(std::make_unique<ImmediateTask>(std::move(task)));
}

void EventLoop::schedule_delayed_task(std::function<void()> task,
                                      std::chrono::milliseconds delay) {
    timer_queue_.push_back(std::make_unique<TimerTask>(std::move(task), delay));
}

void EventLoop::run() {
    is_running_ = true;

    while (is_running_ && has_pending_tasks()) {
        // タイマーキューから準備完了したタスクを処理
        process_timers();

        // タスクキューから1つのタスクを実行
        if (!task_queue_.empty()) {
            auto task = std::move(task_queue_.front());
            task_queue_.pop();

            if (task->is_ready()) {
                task->execute();
            }
        }

        // タスクがない場合は短時間スリープ（CPUを無駄に使わない）
        if (task_queue_.empty() && !timer_queue_.empty()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
    }

    is_running_ = false;
}

// v0.12.0 Phase 8: 条件が満たされるまでイベントループを実行
void EventLoop::run_until(const std::function<bool()> &condition) {
    is_running_ = true;

    while (is_running_ && !condition()) {
        // タイマーキューから準備完了したタスクを処理
        process_timers();

        // タスクキューから1つのタスクを実行
        if (!task_queue_.empty()) {
            auto task = std::move(task_queue_.front());
            task_queue_.pop();

            if (task->is_ready()) {
                task->execute();
            }
        } else if (timer_queue_.empty()) {
            // タスクキューもタイマーキューも空の場合、条件をチェックして終了
            if (condition()) {
                break;
            }
            // 短時間スリープしてから再チェック
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        } else {
            // タイマーだけある場合は短時間スリープ
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
    }

    is_running_ = false;
}

void EventLoop::stop() { is_running_ = false; }

bool EventLoop::has_pending_tasks() const {
    return !task_queue_.empty() || !timer_queue_.empty();
}

void EventLoop::process_timers() {
    // 準備完了したタイマーを検索してタスクキューに移動
    auto it = timer_queue_.begin();
    while (it != timer_queue_.end()) {
        if ((*it)->is_ready()) {
            // タイマーが準備完了 → タスクキューに追加
            task_queue_.push(std::move(*it));
            it = timer_queue_.erase(it);
        } else {
            ++it;
        }
    }
}

} // namespace cb
