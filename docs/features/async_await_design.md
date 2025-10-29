# Cb言語 非同期処理設計

**バージョン**: v0.11.0 Complete  
**優先度**: 高  
**ステータス**: 設計中

---

## 📋 目次

1. [概要](#概要)
2. [設計方針](#設計方針)
3. [構文定義](#構文定義)
4. [アーキテクチャ](#アーキテクチャ)
5. [実装詳細](#実装詳細)
6. [使用例](#使用例)
7. [実装スケジュール](#実装スケジュール)

---

## 概要

Cb言語にJavaScript/TypeScript/Rustスタイルの**非同期処理（async/await）**を導入します。シングルスレッドのEvent Loopベースで実装し、I/O処理やタイマーを効率的に扱えるようにします。

---

## 🎯 目標

### ✅ 達成すべきこと
1. **async/await構文**: 非同期関数の定義と待機
2. **Future<T>型**: 非同期処理の結果を表現
3. **Event Loop**: タスクのスケジューリングと実行
4. **組み込み関数**: `sleep()`, `timeout()`, `spawn()`など
5. **エラーハンドリング**: async関数でのResult<T, E>統合

---

## 📝 設計方針

### 1. 参考にする言語

| 言語 | 採用する要素 |
|-----|------------|
| **JavaScript/TypeScript** | async/await構文、Promise風のFuture<T> |
| **Rust** | Result<T, E>との統合、明示的なエラー処理 |
| **Python** | asyncioのEvent Loop概念 |

### 2. 基本原則

- **シングルスレッド**: 複雑な並行制御を避ける
- **協調的マルチタスク**: yield点（await）で制御を渡す
- **明示的**: async関数は明示的にマーク
- **型安全**: Future<T>で型を保証

---

## 🔤 構文定義

### BNF拡張

```bnf
# 非同期関数定義
async_function_declaration ::= "async" type_specifier IDENTIFIER "(" parameter_list? ")" block_statement

# await式
await_expression ::= "await" postfix_expression

# Future型
future_type ::= "Future" "<" type_specifier ">"

# 例
async int fetch_data() {
    await sleep(1000);
    return 42;
}

void main() {
    Future<int> future = spawn(fetch_data);
    int result = await future;
    println("Result: ", result);
}
```

### キーワード追加

```cpp
// src/common/token.h

enum TokenType {
    // ... 既存のトークン ...
    
    // 非同期処理
    TOK_ASYNC,      // "async"
    TOK_AWAIT,      // "await"
    TOK_FUTURE,     // "Future"
    TOK_SPAWN,      // "spawn"
};
```

---

## 🏗️ アーキテクチャ

### 全体構成図

```
┌─────────────────────────────────────────┐
│         Cb Program (User Code)          │
│  async int fetch() { await sleep(1); }  │
└───────────────┬─────────────────────────┘
                │
                ▼
┌─────────────────────────────────────────┐
│         Interpreter (Frontend)          │
│  - Parser: async/await構文解析          │
│  - AST: AST_ASYNC_FUNCTION, AST_AWAIT   │
└───────────────┬─────────────────────────┘
                │
                ▼
┌─────────────────────────────────────────┐
│       Event Loop (Core System)          │
│  - Task Queue: Vector<Task>             │
│  - Scheduler: run_until_complete()      │
│  - Timer: sleep(), timeout()            │
└───────────────┬─────────────────────────┘
                │
                ▼
┌─────────────────────────────────────────┐
│        Future<T> (Runtime Type)         │
│  - State: Pending | Ready(T)            │
│  - Callbacks: on_complete()             │
└─────────────────────────────────────────┘
```

---

## 🔧 実装詳細

### 1. Future<T>型

#### 構造体定義

```cpp
// src/backend/interpreter/core/future.h

enum class FutureState {
    PENDING,    // 処理中
    READY       // 完了
};

template<typename T>
struct Future {
    FutureState state;
    T value;                                      // READY時の値
    std::vector<std::function<void(T)>> callbacks; // 完了時のコールバック
    
    bool is_ready() const { return state == FutureState::READY; }
    
    void set_ready(T val) {
        state = FutureState::READY;
        value = val;
        // コールバック実行
        for (auto& cb : callbacks) {
            cb(value);
        }
    }
    
    void on_complete(std::function<void(T)> callback) {
        if (is_ready()) {
            callback(value);
        } else {
            callbacks.push_back(callback);
        }
    }
};
```

#### インタプリタでの表現

```cpp
// src/backend/interpreter/core/value.h

struct Value {
    ValueType type;
    union {
        // ... 既存の型 ...
        Future<Value>* future_value;  // Future<T>
    };
};

// TYPE_FUTURE型を追加
enum ValueType {
    // ... 既存 ...
    TYPE_FUTURE,
};
```

---

### 2. Event Loop

#### Task構造体

```cpp
// src/backend/interpreter/core/event_loop.h

struct Task {
    int id;
    std::function<Value()> function;  // 実行する関数
    Future<Value>* result_future;     // 結果を格納するFuture
    bool is_suspended;                // await中かどうか
    Value* awaiting_future;           // 待機中のFuture
};

class EventLoop {
private:
    std::vector<Task> task_queue_;
    int next_task_id_;
    bool running_;
    
public:
    EventLoop() : next_task_id_(0), running_(false) {}
    
    // タスクを追加
    int spawn_task(std::function<Value()> func);
    
    // メインループ
    void run_until_complete();
    
    // タスクを1ステップ実行
    void step();
    
    // awaitポイントで中断
    void suspend_current_task(Future<Value>* future);
    
    // タスク再開
    void resume_task(int task_id);
};
```

#### メインループ実装

```cpp
// src/backend/interpreter/core/event_loop.cpp

void EventLoop::run_until_complete() {
    running_ = true;
    
    while (!task_queue_.empty() && running_) {
        step();
    }
    
    running_ = false;
}

void EventLoop::step() {
    if (task_queue_.empty()) return;
    
    Task& task = task_queue_.front();
    
    if (task.is_suspended) {
        // await中のタスクをチェック
        if (task.awaiting_future->is_ready()) {
            task.is_suspended = false;
            task.awaiting_future = nullptr;
            // タスク再開（次のループで実行）
        }
    } else {
        // タスク実行
        try {
            Value result = task.function();
            task.result_future->set_ready(result);
            
            // 完了したタスクを削除
            task_queue_.erase(task_queue_.begin());
        } catch (...) {
            // エラーハンドリング
            task_queue_.erase(task_queue_.begin());
        }
    }
}

void EventLoop::suspend_current_task(Future<Value>* future) {
    if (!task_queue_.empty()) {
        Task& task = task_queue_.front();
        task.is_suspended = true;
        task.awaiting_future = future;
    }
}
```

---

### 3. async/await構文

#### Parser拡張

```cpp
// src/frontend/parser/declaration_parser.cpp

ASTNode* Parser::parse_async_function_declaration() {
    consume(TOK_ASYNC, "Expected 'async'");
    
    // 通常の関数宣言を解析
    ASTNode* func_node = parse_function_declaration();
    func_node->node_type = AST_ASYNC_FUNCTION;
    func_node->is_async = true;
    
    // 戻り値型をFuture<T>に変換
    // async int func() → Future<int> func()
    
    return func_node;
}

ASTNode* Parser::parse_await_expression() {
    consume(TOK_AWAIT, "Expected 'await'");
    
    ASTNode* await_node = create_node(AST_AWAIT_EXPR);
    await_node->left = parse_postfix_expression();
    
    return await_node;
}
```

#### AST定義

```cpp
// src/common/ast.h

enum ASTNodeType {
    // ... 既存 ...
    AST_ASYNC_FUNCTION,     // async関数定義
    AST_AWAIT_EXPR,         // await式
    AST_FUTURE_TYPE,        // Future<T>型
};

struct ASTNode {
    // ... 既存 ...
    bool is_async;          // async関数フラグ
};
```

---

### 4. Evaluator拡張

#### async関数の実行

```cpp
// src/backend/interpreter/evaluator/evaluator.cpp

Value Interpreter::evaluate_async_function_call(ASTNode* node) {
    // 1. 新しいFuture<T>を作成
    Future<Value>* future = new Future<Value>();
    future->state = FutureState::PENDING;
    
    // 2. タスクとして登録
    int task_id = event_loop_.spawn_task([=]() {
        return evaluate_function_body(node);
    });
    
    // 3. Future<T>を返す
    Value future_val;
    future_val.type = TYPE_FUTURE;
    future_val.future_value = future;
    
    return future_val;
}

Value Interpreter::evaluate_await_expression(ASTNode* node) {
    // 1. await対象の式を評価
    Value future_val = evaluate(node->left);
    
    if (future_val.type != TYPE_FUTURE) {
        throw_error("await expects a Future<T>");
    }
    
    Future<Value>* future = future_val.future_value;
    
    // 2. Futureが完了していない場合は中断
    if (!future->is_ready()) {
        event_loop_.suspend_current_task(future);
        // ここで制御がEvent Loopに戻る
    }
    
    // 3. Futureが完了していれば値を返す
    return future->value;
}
```

---

### 5. 組み込み関数

#### sleep関数

```cpp
// src/backend/interpreter/stdlib/async_functions.cpp

Future<void> sleep(int milliseconds) {
    Future<void>* future = new Future<void>();
    
    // タイマー登録（実装は簡易版）
    std::thread([=]() {
        std::this_thread::sleep_for(std::chrono::milliseconds(milliseconds));
        future->set_ready();
    }).detach();
    
    return *future;
}
```

#### spawn関数

```cpp
Future<T> spawn(std::function<T()> func) {
    Future<T>* future = new Future<T>();
    
    int task_id = event_loop_.spawn_task([=]() {
        T result = func();
        future->set_ready(result);
        return result;
    });
    
    return *future;
}
```

---

## 💡 使用例

### Example 1: 基本的なsleep

```cb
async void delayed_print() {
    println("Starting...");
    await sleep(1000);  // 1秒待機
    println("Done!");
}

void main() {
    Future<void> f = spawn(delayed_print);
    await f;
}
```

### Example 2: 非同期計算

```cb
async int fetch_data(int id) {
    await sleep(500);
    return id * 2;
}

async void main() {
    Future<int> f1 = spawn(fetch_data(10));
    Future<int> f2 = spawn(fetch_data(20));
    
    int result1 = await f1;
    int result2 = await f2;
    
    println("Results: ", result1, ", ", result2);
}
```

### Example 3: エラーハンドリング

```cb
async Result<int, string> safe_divide(int a, int b) {
    await sleep(100);
    
    if (b == 0) {
        return Result<int, string>::Err("Division by zero");
    }
    
    return Result<int, string>::Ok(a / b);
}

async void main() {
    Future<Result<int, string>> f = spawn(safe_divide(10, 2));
    Result<int, string> result = await f;
    
    match (result) {
        Ok(value) => println("Result: ", value),
        Err(error) => println("Error: ", error),
    }
}
```

---

## 📊 実装スケジュール

### Week 4 Day 2: Event Loop基盤（2日）
- [ ] Task構造体定義
- [ ] EventLoop class実装
- [ ] spawn_task()実装
- [ ] run_until_complete()実装
- [ ] 基本テスト

### Week 4 Day 3: Future<T>実装（1日）
- [ ] Future構造体定義
- [ ] TYPE_FUTURE型追加
- [ ] Future状態管理
- [ ] コールバック機構

### Week 4 Day 4: async/await構文（2日）
- [ ] TOK_ASYNC, TOK_AWAIT追加
- [ ] parse_async_function_declaration()
- [ ] parse_await_expression()
- [ ] AST拡張

### Week 4 Day 5: Evaluator実装（2日）
- [ ] evaluate_async_function_call()
- [ ] evaluate_await_expression()
- [ ] タスク中断・再開ロジック

### Week 5 Day 1: 組み込み関数（1日）
- [ ] sleep()実装
- [ ] spawn()実装
- [ ] timeout()実装

### Week 5 Day 2: テストと統合（1日）
- [ ] 非同期処理テストスイート
- [ ] 既存テストの互換性確認
- [ ] ドキュメント更新

**総見積もり**: 9日（約2週間）

---

## 🎯 成功基準

1. **構文解析**: async/await構文が正しく解析される
2. **実行**: 非同期関数が正常に実行される
3. **中断・再開**: awaitでタスクが中断・再開される
4. **複数タスク**: 複数の非同期タスクが並行実行される
5. **エラーハンドリング**: async関数でResult<T, E>が使える
6. **パフォーマンス**: オーバーヘッドが最小限

---

## 🚀 次のステップ

1. **Event Loop実装**: タスクキューとスケジューラ
2. **Future<T>実装**: 型システムと状態管理
3. **Parser拡張**: async/await構文解析
4. **Evaluator拡張**: 非同期実行ロジック
5. **テスト**: 包括的なテストスイート

この実装により、Cb言語は**最先端の非同期処理機能**を持つモダンな言語になります。

---

## 📝 技術的課題

### 課題1: タスク中断・再開
- **問題**: C++でコルーチンを実装する必要がある
- **解決策**: 状態機械（State Machine）で簡易的に実装

### 課題2: メモリ管理
- **問題**: Futureオブジェクトのライフサイム管理
- **解決策**: 参照カウント（shared_ptr）使用

### 課題3: 型推論
- **問題**: async関数の戻り値型がFuture<T>になる
- **解決策**: Parserで自動的に型を変換

---

**作成日**: 2025年10月29日  
**作成者**: v0.11.0実装チーム
