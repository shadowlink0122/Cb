# async/await Phase 5実装レポート
# 日付: 2025-11-07
# バージョン: v0.12.0

## 実装概要

Phase 5では、async/awaitの真の並行処理に向けた基盤を実装しました。

### 実装した機能

#### 1. AsyncTask構造体の定義 (`interpreter.h`)

```cpp
struct AsyncTask {
    int task_id;                          // 一意なタスクID
    const ASTNode *function_node;         // 実行する関数のASTノード
    std::string function_name;            // 関数名
    std::vector<int64_t> args;            // 引数リスト（簡易版）
    Variable *future_var;                 // 紐づくFuture変数へのポインタ
    bool is_executed = false;             // 実行済みフラグ
};
```

**目的**: 非同期タスクの実行情報を保存し、後で取得・実行できるようにする。

#### 2. Interpreterクラスの拡張

**追加メンバ変数**:
```cpp
std::map<int, AsyncTask> async_tasks_;           // task_id -> AsyncTask
std::map<std::string, int> future_to_task_;      // Future変数名 -> task_id
```

**追加メソッド**:
- `register_async_task(int task_id, const AsyncTask &task)`: タスク登録
- `get_async_task(int task_id)`: タスク取得
- `associate_future_with_task(const std::string &future_var_name, int task_id)`: Future-Task関連付け
- `get_task_id_for_future(const std::string &future_var_name)`: FutureからタスクID取得

#### 3. async関数呼び出し時の処理 (`call_impl.cpp`)

```cpp
// タスクIDを生成
int task_id = interpreter_.get_async_task_counter();
interpreter_.increment_async_task_counter();

// Future構造体を作成
Variable future_var;
future_var.struct_members["value"] = value_member;
future_var.struct_members["is_ready"] = ready_member;
future_var.struct_members["task_id"] = task_id_member;  // NEW!

// AsyncTask情報を登録
AsyncTask task(task_id, func, node->name, evaluated_args, future_ptr);
interpreter_.register_async_task(task_id, task);

// 関数本体を即座に実行（Phase 5準備版）
interpreter_.execute_statement(func->body.get());
```

**重要な変更点**:
- Futureに`task_id`メンバーを追加
- AsyncTask情報をInterpreterに登録
- 現段階では即座に実行（is_ready=true）

#### 4. await式評価の準備 (`binary_unary.cpp`)

```cpp
if (!is_ready) {
    // TODO: 実際の実装では、Event Loopと統合して
    // タスクを実行する必要があります
    throw std::runtime_error(
        "[Phase 5 準備完了] await on unready Future: "
        "Event Loop統合が必要です");
}
```

**枠組み**: is_ready=falseの場合の処理準備（将来の実装用）

## テスト結果

### 既存テスト

```
========================================
  Test Results
========================================
Total tests: 15
Passed: 15
Failed: 0

All tests PASSED! ✓
```

**結論**: 後方互換性を完全に維持しています。

### 新規テストファイル

1. **`tests/integration/async_deferred_execution_test.cb`**
   - 遅延実行のテストケース（準備版）
   - is_readyフラグの確認
   - task_id の存在確認

2. **`tests/integration/async_simple_deferred_test.cb`**
   - シンプルな動作確認テスト

**注**: asyncキーワードを使った関数の変数代入に課題があるため、
現時点では手動Future構築のテストのみが動作します。

## 現在の動作

### async関数の流れ

1. **関数呼び出し時**:
   ```cb
   Future<int> f = async_function();  // ← async関数呼び出し
   ```
   - task_id生成
   - Future構造体作成（value, is_ready, task_id）
   - AsyncTask登録
   - 関数本体を即座に実行
   - is_ready=trueに設定

2. **await時**:
   ```cb
   int result = await f;  // ← await式
   ```
   - is_readyチェック
   - true → valueを取得して返す
   - false → エラー（Phase 6で実装予定）

### データフロー図

```
async関数呼び出し
    ↓
task_id生成 (async_task_counter_++)
    ↓
Future構造体作成
  - value: 初期値
  - is_ready: true (現段階)
  - task_id: 生成されたID
    ↓
AsyncTask登録
  - Interpreterのasync_tasks_に保存
  - future_to_task_でFutureと関連付け
    ↓
関数本体実行 (execute_statement)
    ↓
結果をFutureに格納
    ↓
Futureを返す (ポインタとして)
```

## 既知の問題

### 1. async関数の変数代入エラー

**問題**:
```cb
Future<int> f = async_function();  // エラー
```

**エラーメッセージ**:
```
Expected struct return but got numeric value
```

**原因**: 
- `evaluate_function_call_impl`はint64_t（ポインタ）を返す
- 変数宣言時に構造体として扱おうとして型不一致が発生

**回避策**（現在のサンプル）:
```cb
// 手動でFutureを構築
Future<int> f;
f.value = 42;
f.is_ready = true;
// await f;  ← これは動作する
```

**解決方針**（Phase 6）:
- TypedValueベースの返却メカニズムの実装
- または構造体代入処理の改善

### 2. 真の遅延実行が未実装

**現状**: async関数は即座に実行される（is_ready=true）

**将来の実装**:
- Event Loopへのタスク登録
- is_ready=falseでFutureを返す
- await時に初めて実行

## 次のステップ（Phase 6予定）

### 優先度1: asyncキーワード関数の修正

**目標**: `Future<int> f = async_function()`が動作するようにする

**アプローチ**:
1. evaluate_function_call_implの返却値をTypedValueに変更を検討
2. または変数宣言時の構造体処理を改善
3. 既存の構造体返却パターン（例：メソッド戻り値）を調査

### 優先度2: Event Loopとの統合

**目標**: 真の非同期実行を実現

**実装内容**:
```cpp
// call_impl.cppで
auto& event_loop = interpreter_.get_event_loop();
event_loop.schedule_task([task_id, &interpreter_]() {
    AsyncTask* task = interpreter_.get_async_task(task_id);
    if (task && !task->is_executed) {
        // 関数を実行
        // 結果をFutureに格納
        task->future_var->struct_members["is_ready"].value = 1;
        task->is_executed = true;
    }
});
```

### 優先度3: 遅延実行の実装

**目標**: is_ready=falseでFutureを返し、await時に実行

**変更箇所**:
1. `call_impl.cpp`: is_ready=falseに設定
2. `binary_unary.cpp`: await時にタスク実行

```cpp
// binary_unary.cpp (await評価)
if (!is_ready) {
    int task_id = static_cast<int>(future_var.struct_members["task_id"].value);
    AsyncTask* task = get_async_task(task_id);
    if (task) {
        // タスクを実行
        execute_async_task(task);
        // is_readyを更新
        future_var.struct_members["is_ready"].value = 1;
    }
}
```

### 優先度4: 引数の保存と復元

**課題**: 現在の`std::vector<int64_t> args`では限定的

**改善案**:
```cpp
struct AsyncTaskArgument {
    TypeInfo type;
    int64_t int_value;
    std::string str_value;
    double double_value;
    Variable struct_value;
};

struct AsyncTask {
    // ...
    std::vector<AsyncTaskArgument> args;  // 型付き引数リスト
};
```

### 優先度5: マルチスレッド対応

**目標**: 複数のasyncタスクを並行実行

**要件**:
- スレッドセーフなタスクキュー
- ワーカースレッドプール
- 同期プリミティブ（mutex, condition_variable）

## アーキテクチャ図

```
┌─────────────────────────────────────────────┐
│           Cb プログラム                      │
│                                             │
│  async int compute() { return 42; }         │
│  Future<int> f = compute();                 │
│  int result = await f;                      │
└─────────────────┬───────────────────────────┘
                  │
                  ↓
┌─────────────────────────────────────────────┐
│          Parser/AST                         │
│  - is_async_function フラグ                 │
│  - async関数定義の保存                      │
└─────────────────┬───────────────────────────┘
                  │
                  ↓
┌─────────────────────────────────────────────┐
│       Interpreter (実行エンジン)            │
│                                             │
│  ┌─────────────────────────────────┐       │
│  │  async_tasks_: map<id, Task>    │       │
│  │  future_to_task_: map<name, id> │       │
│  │  async_task_counter_: int       │       │
│  └─────────────────────────────────┘       │
│                                             │
│  ┌─────────────────────────────────┐       │
│  │    Event Loop (未統合)          │       │
│  │  - task_queue_                  │       │
│  │  - timer_queue_                 │       │
│  └─────────────────────────────────┘       │
└─────────────────┬───────────────────────────┘
                  │
                  ↓
┌─────────────────────────────────────────────┐
│     関数呼び出し評価 (call_impl.cpp)        │
│                                             │
│  async関数の場合:                           │
│  1. task_id生成                             │
│  2. Future構造体作成                        │
│  3. AsyncTask登録                           │
│  4. 関数本体実行 (Phase 5)                  │
│  5. Futureポインタを返す                    │
└─────────────────┬───────────────────────────┘
                  │
                  ↓
┌─────────────────────────────────────────────┐
│      await式評価 (binary_unary.cpp)         │
│                                             │
│  1. Futureからis_readyチェック              │
│  2. true → valueを取得                      │
│  3. false → タスク実行 (Phase 6予定)       │
└─────────────────────────────────────────────┘
```

## コードレビューポイント

### 良い設計

1. **モジュール化**: AsyncTask構造体による情報のカプセル化
2. **拡張性**: Event Loopとの統合を想定した設計
3. **後方互換性**: 既存テストがすべてパス
4. **段階的実装**: Phase 5で基盤、Phase 6で完全実装

### 改善が必要な点

1. **構造体返却**: asyncキーワード関数の返却メカニズム
2. **引数の型**: `std::vector<int64_t>`では限定的
3. **エラーハンドリング**: 例外処理の統一
4. **ドキュメント**: 内部APIのコメント追加

## 実装統計

### 変更ファイル

- `src/backend/interpreter/core/interpreter.h`: +50行
- `src/backend/interpreter/evaluator/functions/call_impl.cpp`: +80行
- `src/backend/interpreter/evaluator/operators/binary_unary.cpp`: +10行
- `tests/integration/async_deferred_execution_test.cb`: +165行（新規）
- `tests/integration/async_simple_deferred_test.cb`: +25行（新規）

**合計**: 約330行の追加

### コミット情報

- コミットID: c4aa88d
- ブランチ: feature/async_await
- 日付: 2025-11-07

## 結論

Phase 5では、async/awaitの真の並行処理に向けた重要な基盤を構築しました。
AsyncTask管理システムにより、将来のEvent Loop統合と遅延実行が実現可能になりました。

**現状**: ✅ インフラ準備完了、既存機能の互換性維持
**次回**: ⏭️ asyncキーワード関数の修正とEvent Loop統合

## 参考資料

- docs/features/async_await_design.md
- src/backend/interpreter/event_loop/event_loop.h
- stdlib/std/future.cb
