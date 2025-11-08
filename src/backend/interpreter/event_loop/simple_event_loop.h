#pragma once

#include "async_task.h"
#include <deque>
#include <map>
#include <memory>

// 前方宣言
class Interpreter;

namespace cb {

// v0.12.0: シンプルなイベントループ
// async関数をラウンドロビン方式で実行する
// バックグラウンドタスクはメインプログラム終了時に自動的に破棄される
class SimpleEventLoop {
  public:
    SimpleEventLoop(Interpreter &interpreter);
    ~SimpleEventLoop() = default;

    // タスクを登録してキューに追加
    // 戻り値: タスクID
    int register_task(AsyncTask task);

    // イベントループを1サイクル実行（1タスクを1ステップだけ）
    // awaitなしの場合に、バックグラウンドでタスクを少しずつ実行
    void run_one_cycle();

    // 特定のタスクが完了するまで実行（await用）
    // 指定されたタスクを優先的に実行し、完了するまでブロック
    void run_until_complete(int task_id);

    // タスクキューが空かどうか
    bool is_empty() const;

    // 登録されているタスクがあるかどうか
    bool has_tasks() const;

    // タスク数を取得
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

    Interpreter &interpreter_;
    std::deque<int> task_queue_;     // 実行待ちタスクID
    std::map<int, AsyncTask> tasks_; // タスクID -> AsyncTask
    int next_task_id_ = 1;           // 次のタスクID
};

} // namespace cb
