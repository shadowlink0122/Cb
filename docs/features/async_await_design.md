# Cb言語 async/await 設計ドキュメント# Cb言語 非同期処理設計



**バージョン**: v0.12.0  **バージョン**: v0.11.0 Complete  

**作成日**: 2025年11月7日  **優先度**: 高  

**ステータス**: Phase 1 実装中**ステータス**: 設計中



------



## 概要## 📋 目次



Cb言語の**協調的マルチタスク**システムは、TypeScriptのAPI設計とRust Tokioの内部アーキテクチャを参考にしています。1. [概要](#概要)

2. [設計方針](#設計方針)

### 参考実装3. [構文定義](#構文定義)

4. [アーキテクチャ](#アーキテクチャ)

#### TypeScript (Promise/async/await)5. [実装詳細](#実装詳細)

```typescript6. [使用例](#使用例)

async function fetchData(id: number): Promise<number> {7. [実装スケジュール](#実装スケジュール)

    console.log(`Fetching ${id}...`);

    await delay(1000);---

    return id * 100;

}## 概要



async function main() {Cb言語にJavaScript/TypeScript/Rustスタイルの**非同期処理（async/await）**を導入します。シングルスレッドのEvent Loopベースで実装し、I/O処理やタイマーを効率的に扱えるようにします。

    const f1 = fetchData(1);  // 非同期で開始

    const f2 = fetchData(2);  // 非同期で開始---

    

    const r1 = await f1;  // 完了を待つ## 🎯 目標

    const r2 = await f2;  // 完了を待つ

    ### ✅ 達成すべきこと

    console.log(r1, r2);1. **async/await構文**: 非同期関数の定義と待機

}2. **Future<T>型**: 非同期処理の結果を表現

```3. **Event Loop**: タスクのスケジューリングと実行

4. **組み込み関数**: `sleep()`, `timeout()`, `spawn()`など

#### Rust (Future/async/await)5. **エラーハンドリング**: async関数でのResult<T, E>統合

```rust

async fn fetch_data(id: i32) -> i32 {---

    println!("Fetching {}...", id);

    tokio::time::sleep(Duration::from_secs(1)).await;## 📝 設計方針

    id * 100

}### 1. 参考にする言語



#[tokio::main]| 言語 | 採用する要素 |

async fn main() {|-----|------------|

    let f1 = fetch_data(1);  // Futureを作成（遅延評価）| **JavaScript/TypeScript** | async/await構文、Promise風のFuture<T> |

    let f2 = fetch_data(2);  // Futureを作成| **Rust** | Result<T, E>との統合、明示的なエラー処理 |

    | **Python** | asyncioのEvent Loop概念 |

    let (r1, r2) = tokio::join!(f1, f2);  // 並行実行

    ### 2. 基本原則

    println!("{} {}", r1, r2);

}- **シングルスレッド**: 複雑な並行制御を避ける

```- **協調的マルチタスク**: yield点（await）で制御を渡す

- **明示的**: async関数は明示的にマーク

---- **型安全**: Future<T>で型を保証



## Phase 1: 基本的なasync/await (v0.12.0)---



### 設計方針## 🔤 構文定義



- **実行モデル**: **即座実行（Eager Execution）**### BNF拡張

  - async関数は呼び出し時に即座に完了まで実行される

  - yieldによる中断はサポートしない```bnf

  - 並行実行ではなく、順次実行# 非同期関数定義

async_function_declaration ::= "async" type_specifier IDENTIFIER "(" parameter_list? ")" block_statement

- **Future型**: 結果を保持するコンテナ

  ```cb# await式

  struct Future<T> {await_expression ::= "await" postfix_expression

      T value;        // 結果値

      bool is_ready;  // 完了フラグ（Phase 1では常にtrue）# Future型

  };future_type ::= "Future" "<" type_specifier ">"

  ```

# 例

- **デバッグ**: `--debug`フラグで実行トレースasync int fetch_data() {

  ```    await sleep(1000);

  [ASYNC] Entering async function: fetch_data(id=1)    return 42;

  [ASYNC] Returning from async function: fetch_data -> Future{is_ready=true}}

  [AWAIT] Awaiting Future (already ready)

  [AWAIT] Extracted value: 100void main() {

  ```    Future<int> future = spawn(fetch_data);

    int result = await future;

### 使用例    println("Result: ", result);

}

```cb```

struct Future<T> {

    T value;### キーワード追加

    bool is_ready;

};```cpp

// src/common/token.h

async Future<int> fetch_data(int id) {

    println("Fetching data {id}...");enum TokenType {

    int result = id * 100;    // ... 既存のトークン ...

    println("Data {id} ready: {result}");    

    return result;    // 非同期処理

}    TOK_ASYNC,      // "async"

    TOK_AWAIT,      // "await"

async Future<void> process_data(int id) {    TOK_FUTURE,     // "Future"

    println("Processing {id}...");    TOK_SPAWN,      // "spawn"

    int value = id + 10;};

    println("Processed: {value}");```

}

---

void main() {

    // async関数を呼び出す（即座に完了まで実行）## 🏗️ アーキテクチャ

    Future<int> f1 = fetch_data(1);

    Future<int> f2 = fetch_data(2);### 全体構成図

    Future<void> f3 = process_data(3);

    ```

    // awaitで値を取得（Phase 1では既に完了している）┌─────────────────────────────────────────┐

    int result1 = await f1;│         Cb Program (User Code)          │

    int result2 = await f2;│  async int fetch() { await sleep(1); }  │

    await f3;└───────────────┬─────────────────────────┘

                    │

    println("Results: {result1}, {result2}");                ▼

}┌─────────────────────────────────────────┐

```│         Interpreter (Frontend)          │

│  - Parser: async/await構文解析          │

### 実行出力│  - AST: AST_ASYNC_FUNCTION, AST_AWAIT   │

└───────────────┬─────────────────────────┘

```                │

Fetching data 1...                ▼

Data 1 ready: 100┌─────────────────────────────────────────┐

Fetching data 2...│       Event Loop (Core System)          │

Data 2 ready: 200│  - Task Queue: Vector<Task>             │

Processing 3...│  - Scheduler: run_until_complete()      │

Processed: 13│  - Timer: sleep(), timeout()            │

Results: 100, 200└───────────────┬─────────────────────────┘

```                │

                ▼

### デバッグ出力 (`./main --debug test.cb`)┌─────────────────────────────────────────┐

│        Future<T> (Runtime Type)         │

```│  - State: Pending | Ready(T)            │

[ASYNC] Entering async function: fetch_data(id=1)│  - Callbacks: on_complete()             │

Fetching data 1...└─────────────────────────────────────────┘

Data 1 ready: 100```

[ASYNC] Returning from async function: fetch_data -> Future{value=100, is_ready=true}

[ASYNC] Entering async function: fetch_data(id=2)---

Fetching data 2...

Data 2 ready: 200## 🔧 実装詳細

[ASYNC] Returning from async function: fetch_data -> Future{value=200, is_ready=true}

[ASYNC] Entering async function: process_data(id=3)### 1. Future<T>型

Processing 3...

Processed: 13#### 構造体定義

[ASYNC] Returning from async function: process_data -> Future{value=0, is_ready=true}

[AWAIT] Awaiting Future (already ready)```cpp

[AWAIT] Extracted value: 100// src/backend/interpreter/core/future.h

[AWAIT] Awaiting Future (already ready)

[AWAIT] Extracted value: 200enum class FutureState {

[AWAIT] Awaiting Future (already ready)    PENDING,    // 処理中

Results: 100, 200    READY       // 完了

```};



### Phase 1 の特徴template<typename T>

struct Future {

#### 利点    FutureState state;

1. **シンプルで確実**: 複雑な状態管理が不要    T value;                                      // READY時の値

2. **デバッグしやすい**: トレースが容易    std::vector<std::function<void(T)>> callbacks; // 完了時のコールバック

3. **文法の習得**: async/await構文に慣れることができる    

4. **段階的拡張**: Phase 2への基盤となる    bool is_ready() const { return state == FutureState::READY; }

    

#### 制限    void set_ready(T val) {

1. **並行実行なし**: 順次実行のみ        state = FutureState::READY;

2. **yieldなし**: 中断・再開不可        value = val;

3. **真の非同期I/Oなし**: ブロッキング実行        // コールバック実行

        for (auto& cb : callbacks) {

---            cb(value);

        }

## Phase 2: yieldによる協調的マルチタスク (v0.13.0以降)    }

    

### 設計方針    void on_complete(std::function<void(T)> callback) {

        if (is_ready()) {

Phase 2では、真の協調的マルチタスクを実装します。            callback(value);

        } else {

- **実行モデル**: **遅延評価（Lazy Execution）** + **明示的yield**            callbacks.push_back(callback);

  - async関数は呼び出し時に即座に実行されない        }

  - `await`または`yield`で実行が進む    }

  - 複数のタスクをインターリーブ実行};

```

- **yield**: 明示的な実行権の譲渡

  ```cb#### インタプリタでの表現

  async Future<void> task() {

      println("Step 1");```cpp

      yield;  // 他のタスクに制御を譲る// src/backend/interpreter/core/value.h

      println("Step 2");

      yield;struct Value {

      println("Step 3");    ValueType type;

  }    union {

  ```        // ... 既存の型 ...

        Future<Value>* future_value;  // Future<T>

- **実装方法**: **バイトコードVM** または **CPS変換**    };

  - ASTベースでは任意の位置での中断が困難};

  - プログラムカウンタ（PC）による実行位置管理が必要

// TYPE_FUTURE型を追加

### 使用例（Phase 2）enum ValueType {

    // ... 既存 ...

```cb    TYPE_FUTURE,

async Future<void> task1() {};

    for (int i = 0; i < 3; i = i + 1) {```

        println("Task1: step {i}");

        yield;  // 他のタスクに制御を譲る---

    }

}### 2. Event Loop



async Future<void> task2() {#### Task構造体

    for (int i = 0; i < 3; i = i + 1) {

        println("Task2: step {i}");```cpp

        yield;// src/backend/interpreter/core/event_loop.h

    }

}struct Task {

    int id;

void main() {    std::function<Value()> function;  // 実行する関数

    Future<void> f1 = task1();  // タスクを作成（まだ実行されない）    Future<Value>* result_future;     // 結果を格納するFuture

    Future<void> f2 = task2();  // タスクを作成    bool is_suspended;                // await中かどうか

        Value* awaiting_future;           // 待機中のFuture

    // イベントループで並行実行};

    run_event_loop([f1, f2]);

    class EventLoop {

    // または個別にawaitprivate:

    await f1;  // この間にf2も少しずつ実行される    std::vector<Task> task_queue_;

    await f2;    int next_task_id_;

}    bool running_;

```    

public:

### 期待される出力（Phase 2）    EventLoop() : next_task_id_(0), running_(false) {}

    

```    // タスクを追加

Task1: step 0    int spawn_task(std::function<Value()> func);

Task2: step 0    

Task1: step 1    // メインループ

Task2: step 1    void run_until_complete();

Task1: step 2    

Task2: step 2    // タスクを1ステップ実行

```    void step();

    

---    // awaitポイントで中断

    void suspend_current_task(Future<Value>* future);

## デバッグガイド    

    // タスク再開

### デバッグフラグの使用    void resume_task(int task_id);

};

```bash```

# Phase 1のデバッグ

./main --debug async_test.cb#### メインループ実装



# 出力例```cpp

[ASYNC] Entering async function: fetch_data(id=1)// src/backend/interpreter/core/event_loop.cpp

Fetching data 1...

[ASYNC] Returning from async function: fetch_datavoid EventLoop::run_until_complete() {

[AWAIT] Awaiting Future (already ready)    running_ = true;

[AWAIT] Extracted value: 100    

```    while (!task_queue_.empty() && running_) {

        step();

### デバッグ出力の種類    }

    

| プレフィックス | 意味 | 内容 |    running_ = false;

|--------------|------|------|}

| `[ASYNC]` | async関数の実行 | 関数の開始・終了、引数、戻り値 |

| `[AWAIT]` | await式の評価 | Futureの状態、値の取得 |void EventLoop::step() {

| `[YIELD]` | yield文の実行（Phase 2） | タスクの中断 |    if (task_queue_.empty()) return;

| `[RESUME]` | タスクの再開（Phase 2） | 実行位置、ラウンド番号 |    

    Task& task = task_queue_.front();

---    

    if (task.is_suspended) {

## まとめ        // await中のタスクをチェック

        if (task.awaiting_future->is_ready()) {

### Phase 1 (v0.12.0)            task.is_suspended = false;

- ✅ 基本的なasync/await構文            task.awaiting_future = nullptr;

- ✅ 即座実行モデル            // タスク再開（次のループで実行）

- ✅ デバッグトレース        }

- ❌ 並行実行（順次実行のみ）    } else {

- ❌ yield（中断・再開）        // タスク実行

        try {

### Phase 2 (v0.13.0以降)            Value result = task.function();

- ✅ yieldによる明示的な制御権譲渡            task.result_future->set_ready(result);

- ✅ ループ内でのyield            

- ✅ 真の協調的マルチタスク            // 完了したタスクを削除

- ✅ イベントループベースの実行            task_queue_.erase(task_queue_.begin());

- 実装方法: バイトコードVMまたはCPS変換        } catch (...) {

            // エラーハンドリング

### TypeScript/Rustとの比較            task_queue_.erase(task_queue_.begin());

        }

| 機能 | TypeScript | Rust (Tokio) | Cb Phase 1 | Cb Phase 2 |    }

|------|-----------|--------------|-----------|-----------|}

| async/await | ✅ | ✅ | ✅ | ✅ |

| Promise/Future | ✅ | ✅ | ✅ | ✅ |void EventLoop::suspend_current_task(Future<Value>* future) {

| 遅延評価 | ✅ | ✅ | ❌ | ✅ |    if (!task_queue_.empty()) {

| 並行実行 | ✅ | ✅ | ❌ | ✅ |        Task& task = task_queue_.front();

| yield | ❌ | ❌* | ❌ | ✅ |        task.is_suspended = true;

| イベントループ | ✅ (暗黙) | ✅ (tokio::spawn) | ❌ | ✅ |        task.awaiting_future = future;

    }

*Rustにはgeneratorとしてyieldがあるが、async/awaitとは別機能}

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

---

## ✅ 実装完了レポート（v0.12.0）

### Phase 2.0 完全実装（2025年1月）

#### ✅ 実装された機能

1. **協調的マルチタスク**
   - `yield`文によるタスク中断・再開
   - SimpleEventLoopによるタスクスケジューリング
   - awaitでの自動Event Loop実行

2. **自動yield機能**
   - yield文が存在しない関数で各ステートメント後に自動yield
   - `has_yield_statement()`による検出
   - AsyncTask構造体の`auto_yield`フラグ

3. **ビルトインFuture<T>型**
   - ParserとInterpreterの両方でビルトイン型として登録
   - ユーザーが`struct Future<T>`を定義する必要がなくなった
   - Option<T>、Result<T,E>と同じくジェネリクスビルトイン型

#### 実装詳細

**Future<T>ビルトイン化**:
- `RecursiveParser::initialize_builtin_types()`でstruct定義登録
- `Interpreter::register_builtin_struct_future()`でInterpreter側も登録
- `sync_struct_definitions_from_parser()`で同期

**構造**:
```cb
// ユーザーコードで定義不要（ビルトイン）
struct Future<T> {
    T value;
    bool is_ready;
}
```

**使用例**:
```cb
// Phase 2.0の完全な使用例
async Future<int> task1() {
    println("Task1: Statement 1");
    yield;
    println("Task1: Statement 2");
    return 100;
}

void main() {
    Future<int> f = task1();
    int result = await f;  // SimpleEventLoop自動実行
    println("Result: {result}");
}
```

#### テスト結果

- ✅ `test_future_basic.cb`: ビルトインFuture<T>の基本動作
- ✅ `phase1_syntax_test.cb`: async/await構文
- ✅ `phase1_multiple_async.cb`: 複数async関数
- ✅ `phase2_yield_test.cb`: yield文による協調的マルチタスク
- ✅ `phase2_auto_yield_test.cb`: 自動yield機能
- ✅ `phase2_builtin_future_test.cb`: ビルトインFuture<T>（定義なし）

**成果**:
- すべての既存テストが引き続き動作
- Future<T>定義の削除により、すべてのテストファイルが簡潔に
- ユーザー体験の大幅な向上

---

