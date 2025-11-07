#pragma once

#include <deque>
#include <map>
#include <memory>

// 前方宣言
class Interpreter;
struct AsyncTask;

namespace cb {

// v0.13.0 Phase 2.0: SimpleEventLoop
// トップレベルyieldのみをサポートする最小限のイベントループ
//
// 制限事項:
// - トップレベルのyield文のみサポート
// - ループ内、if文内のyieldは未サポート
// - async関数からのasync呼び出しは未サポート
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

    // タスクキューが空かどうか
    bool is_empty() const;

    // 登録されているタスクがあるかどうか
    bool has_tasks() const;

    // 登録されているタスク数
    size_t task_count() const;

    // タスクIDからタスクを取得
    AsyncTask *get_task(int task_id);

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
