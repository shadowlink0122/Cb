#pragma once

#include <deque>
#include <map>
#include <memory>

// 前方宣言
class Interpreter;
struct AsyncTask;

namespace cb {

// v0.12.0: SimpleEventLoop
// async関数をラウンドロビン方式で実行する
// バックグラウンドタスクはメインプログラム終了時に自動的に破棄される
class SimpleEventLoop {
  public:
    SimpleEventLoop(Interpreter &interpreter);
    ~SimpleEventLoop() = default;

    // タスクを登録してキューに追加
    // 戻り値: タスクID
    int register_task(AsyncTask task);

    // イベントループを実行（全タスク完了まで）
    void run();

    // イベントループを1サイクル実行（全タスクを1ステップずつ）
    // v0.12.0: async関数呼び出し時にバックグラウンド実行を実現
    void run_one_cycle();

    // 特定のタスクが完了するまで実行（await用）
    // 指定されたタスクを優先的に実行し、完了するまでブロック
    void run_until_complete(int task_id);

    // タスクキューが空かどうか
    bool is_empty() const;

    // 登録されているタスクがあるかどうか
    bool has_tasks() const;

    // 登録されているタスク数
    size_t task_count() const;

    // タスクIDからタスクを取得
    AsyncTask *get_task(int task_id);

    // v0.12.0: タスクをsleep状態にする（非同期sleep用）
    void sleep_task(int task_id, int64_t duration_ms);

  private:
    // 1タスクを1ステートメント実行
    // 戻り値: true = タスクを継続, false = タスク完了
    bool execute_one_step(int task_id);

    // タスクスコープの初期化
    void initialize_task_scope(AsyncTask &task);

    // タスク完了時の後処理（selfの同期など）
    void finalize_task_if_needed(int task_id);
    void sync_async_self_receiver(AsyncTask &task);

    Interpreter &interpreter_;
    std::deque<int> task_queue_;     // 実行待ちタスクID
    std::map<int, AsyncTask> tasks_; // タスクID -> AsyncTask
    int next_task_id_ = 1;           // 次のタスクID
    int current_executing_task_id_ =
        -1; // 現在execute_one_step中のタスクID (-1=なし)
};

} // namespace cb
