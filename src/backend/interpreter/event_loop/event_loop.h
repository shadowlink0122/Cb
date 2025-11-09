#ifndef EVENT_LOOP_H
#define EVENT_LOOP_H

#include <chrono>
#include <functional>
#include <memory>
#include <queue>
#include <vector>

namespace cb {

// タスクの基底クラス
class Task {
  public:
    virtual ~Task() = default;
    virtual void execute() = 0;
    virtual bool is_ready() const = 0;
};

// 通常のタスク
class ImmediateTask : public Task {
  public:
    explicit ImmediateTask(std::function<void()> func)
        : func_(std::move(func)) {}

    void execute() override { func_(); }
    bool is_ready() const override { return true; }

  private:
    std::function<void()> func_;
};

// タイマータスク
class TimerTask : public Task {
  public:
    TimerTask(std::function<void()> func, std::chrono::milliseconds delay)
        : func_(std::move(func)),
          execute_at_(std::chrono::steady_clock::now() + delay) {}

    void execute() override { func_(); }

    bool is_ready() const override {
        return std::chrono::steady_clock::now() >= execute_at_;
    }

    std::chrono::steady_clock::time_point get_execute_at() const {
        return execute_at_;
    }

  private:
    std::function<void()> func_;
    std::chrono::steady_clock::time_point execute_at_;
};

// Event Loopクラス
class EventLoop {
  public:
    EventLoop() = default;
    ~EventLoop() = default;

    // コピー/ムーブ禁止
    EventLoop(const EventLoop &) = delete;
    EventLoop &operator=(const EventLoop &) = delete;
    EventLoop(EventLoop &&) = delete;
    EventLoop &operator=(EventLoop &&) = delete;

    // タスクをスケジュール
    void schedule_task(std::function<void()> task);

    // 遅延実行タスクをスケジュール
    void schedule_delayed_task(std::function<void()> task,
                               std::chrono::milliseconds delay);

    // イベントループを実行
    void run();

    // v0.12.0 Phase 8: 条件が満たされるまでイベントループを実行
    void run_until(const std::function<bool()> &condition);

    // イベントループを停止
    void stop();

    // 実行中かどうか
    bool is_running() const { return is_running_; }

    // タスクキューが空かどうか
    bool has_pending_tasks() const;

  private:
    std::queue<std::unique_ptr<Task>> task_queue_;
    std::vector<std::unique_ptr<TimerTask>> timer_queue_;
    bool is_running_ = false;

    // タイマーキューから準備完了したタスクを処理
    void process_timers();
};

} // namespace cb

#endif // EVENT_LOOP_H
