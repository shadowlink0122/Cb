# Cb言語 async/await Phase 2 設計ドキュメント

**バージョン**: v0.13.0 (予定)  
**作成日**: 2025年11月7日  
**ステータス**: 設計フェーズ

---

## Phase 1での学び

Phase 1の実装を通じて、以下の課題が明確になりました：

### ❌ Phase 1で試みて失敗したアプローチ

1. **ステートメント単位実行の限界**:
   - トップレベルのステートメントしか追跡できない
   - ループ内でyieldを使うと無限ループになる
   - 関数呼び出しをまたぐyieldができない
   - ネストした制御構造の状態管理が複雑

2. **ASTベースの実行位置追跡**:
   - 実行位置の保存・復元が困難
   - ループカウンタ、変数のスコープ管理が複雑
   - スタックフレームの完全な保存が必要

3. **await時の実行管理**:
   - 全てのpendingタスクを実行しようとして無限再帰
   - タスク間の依存関係を管理できない

### ✅ Phase 1で成功したこと

- async関数の定義と呼び出し
- Future<T>構造体の生成
- await式の評価（is_readyチェック）
- デバッグトレース（DebugMsgId使用）
- 基本的なテストケース

---

## Phase 2の新しいアプローチ

Phase 1の経験を踏まえて、**実装可能な最小限の機能**から始めます。

### 設計方針

1. **段階的な実装**:
   - まずはトップレベルのyieldのみサポート
   - ループ内yield、関数呼び出しyieldは後回し

2. **シンプルなイベントループ**:
   - タスクキューで管理
   - 1タスクずつ順次実行
   - yieldで次のタスクに切り替え

3. **制限を明示**:
   - サポートする機能としない機能を明確に文書化
   - 将来の拡張パスを残す

---

## Phase 2.0: トップレベルyieldのサポート

### 目標

関数のトップレベルでのみyieldをサポートする最小限の実装。

### サポートする構文

```cb
async Future<void> task1() {
    println("Step 1");
    yield;  // ✅ OK: トップレベル
    println("Step 2");
    yield;  // ✅ OK: トップレベル
    println("Step 3");
}

async Future<void> task2() {
    println("Task 2 - Step 1");
    yield;
    println("Task 2 - Step 2");
}

void main() {
    Future<void> f1 = task1();  // タスクを登録
    Future<void> f2 = task2();  // タスクを登録
    
    run_event_loop();  // イベントループで全タスク実行
}
```

### サポートしない構文（Phase 2.0）

```cb
// ❌ NG: ループ内のyield
async Future<void> bad_task() {
    for (int i = 0; i < 5; i = i + 1) {
        println("Step {i}");
        yield;  // エラー: ループ内yieldは未サポート
    }
}

// ❌ NG: if文内のyield
async Future<void> bad_task2(bool flag) {
    if (flag) {
        yield;  // エラー: if文内yieldは未サポート
    }
}

// ❌ NG: 関数呼び出しをまたぐyield
async Future<void> helper() {
    yield;  // これ自体はOK
}

async Future<void> bad_task3() {
    await helper();  // エラー: async関数からのasync呼び出しは未サポート
}
```

---

## データ構造

### AsyncTask（簡易版）

```cpp
struct AsyncTask {
    int task_id;
    const ASTNode* function_node;
    std::string function_name;
    std::vector<Variable> args;
    Variable* future_var;
    
    // Phase 2.0: トップレベル実行状態のみ
    bool is_executed = false;
    bool is_started = false;
    size_t current_statement_index = 0;  // トップレベルのステートメント番号
    
    // タスク専用スコープ
    std::shared_ptr<Scope> task_scope;
};
```

### EventLoop（最小限）

```cpp
class SimpleEventLoop {
    std::deque<int> task_queue;  // 実行待ちタスクID
    std::map<int, AsyncTask> tasks;
    
public:
    // タスクを登録してキューに追加
    int register_task(AsyncTask task);
    
    // イベントループ実行（全タスク完了まで）
    void run();
    
    // 1タスクを1ステートメント実行
    bool execute_one_step(int task_id);
};
```

---

## 実行フロー

### タスク登録

```cpp
// async関数呼び出し時
Future<void> f = task1();

// 内部処理:
// 1. AsyncTaskを作成
// 2. タスクIDを生成
// 3. EventLoopに登録
// 4. Future{is_ready=false, task_id=X} を返す
```

### イベントループ実行

```cpp
void SimpleEventLoop::run() {
    while (!task_queue.empty()) {
        int task_id = task_queue.front();
        task_queue.pop_front();
        
        bool should_continue = execute_one_step(task_id);
        
        if (should_continue) {
            // タスクがまだ完了していない場合、キューに戻す
            task_queue.push_back(task_id);
        }
    }
}

bool SimpleEventLoop::execute_one_step(int task_id) {
    AsyncTask& task = tasks[task_id];
    
    if (!task.is_started) {
        // 初回実行: スコープを初期化
        task.task_scope = std::make_shared<Scope>();
        // 引数を設定
        for (size_t i = 0; i < task.args.size(); i++) {
            task.task_scope->variables[param_name] = task.args[i];
        }
        task.is_started = true;
    }
    
    // タスクスコープに切り替え
    interpreter.push_scope();
    interpreter.current_scope() = *task.task_scope;
    
    try {
        // トップレベルのステートメントを1つ実行
        const ASTNode* body = task.function_node->body.get();
        if (body->node_type == AST_STMT_LIST) {
            if (task.current_statement_index < body->statements.size()) {
                const ASTNode* stmt = body->statements[task.current_statement_index].get();
                
                interpreter.execute_statement(stmt);
                
                task.current_statement_index++;
                
                // まだステートメントが残っている
                return true;
            } else {
                // 全ステートメント実行完了
                task.is_executed = true;
                task.future_var->struct_members["is_ready"].value = 1;
                return false;
            }
        }
    } catch (const YieldException&) {
        // yieldで中断: 次のステートメントへ進まない
        // スコープを保存
        *task.task_scope = interpreter.current_scope();
        interpreter.pop_scope();
        return true;  // キューに戻す
    } catch (const ReturnException& e) {
        // return文で完了
        task.is_executed = true;
        // Future.valueに値を設定
        task.future_var->struct_members["value"] = /* e.value */;
        task.future_var->struct_members["is_ready"].value = 1;
        interpreter.pop_scope();
        return false;
    }
    
    // スコープを保存して戻す
    *task.task_scope = interpreter.current_scope();
    interpreter.pop_scope();
    return true;
}
```

### yield文の処理

```cpp
// execute_statement() 内
case AST_YIELD_STMT:
    debug_msg(DebugMsgId::ASYNC_YIELD_CONTROL);
    throw YieldException();
    break;
```

---

## 使用例と実行結果

### テストケース

```cb
async Future<void> task1() {
    println("Task 1: Step 1");
    yield;
    println("Task 1: Step 2");
    yield;
    println("Task 1: Step 3");
}

async Future<void> task2() {
    println("Task 2: Step 1");
    yield;
    println("Task 2: Step 2");
}

void main() {
    println("=== Starting Event Loop ===");
    
    Future<void> f1 = task1();
    Future<void> f2 = task2();
    
    run_event_loop();
    
    println("=== Event Loop Complete ===");
}
```

### 期待される出力

```
=== Starting Event Loop ===
Task 1: Step 1
Task 2: Step 1
Task 1: Step 2
Task 2: Step 2
Task 1: Step 3
=== Event Loop Complete ===
```

### デバッグ出力 (`--debug`)

```
[ASYNC_FUNCTION_CALL] Registering async task: task1 (task_id=1)
[ASYNC_FUNCTION_CALL] Registering async task: task2 (task_id=2)
[EVENT_LOOP_START] Starting event loop with 2 tasks
[EVENT_LOOP_EXECUTE] Executing task 1, statement 0
Task 1: Step 1
[ASYNC_YIELD_CONTROL] Task 1 yielded
[EVENT_LOOP_EXECUTE] Executing task 2, statement 0
Task 2: Step 1
[ASYNC_YIELD_CONTROL] Task 2 yielded
[EVENT_LOOP_EXECUTE] Executing task 1, statement 1
Task 1: Step 2
[ASYNC_YIELD_CONTROL] Task 1 yielded
[EVENT_LOOP_EXECUTE] Executing task 2, statement 1
Task 2: Step 2
[EVENT_LOOP_EXECUTE] Executing task 1, statement 2
Task 1: Step 3
[ASYNC_TASK_COMPLETE] Task 1 completed
[ASYNC_TASK_COMPLETE] Task 2 completed
[EVENT_LOOP_COMPLETE] Event loop finished
```

---

## 実装タスク

### Phase 2.0.1: YieldExceptionの復活

- [ ] `YieldException`クラスを再追加
- [ ] `AST_YIELD_STMT`の処理を実装
- [ ] デバッグメッセージ `ASYNC_YIELD_CONTROL` を追加

### Phase 2.0.2: SimpleEventLoop実装

- [ ] `SimpleEventLoop`クラスを作成
- [ ] タスク登録機能 `register_task()`
- [ ] イベントループ `run()`
- [ ] 1ステップ実行 `execute_one_step()`

### Phase 2.0.3: async関数の登録モード

- [ ] async関数呼び出し時、即座実行ではなく登録モードに変更
- [ ] Future{is_ready=false}を返す
- [ ] task_idをFutureに設定

### Phase 2.0.4: run_event_loop()組み込み関数

- [ ] `run_event_loop()`組み込み関数を実装
- [ ] 全タスクを完了まで実行

### Phase 2.0.5: テストケース

- [ ] トップレベルyieldのテスト
- [ ] 複数タスクの交互実行テスト
- [ ] デバッグ出力の検証

---

## Phase 2.1以降の計画（将来）

### Phase 2.1: await時の自動実行

```cb
void main() {
    Future<int> f = fetch_data(1);
    
    // await時にイベントループを自動実行
    int result = await f;  // 内部でrun_event_loop()を呼ぶ
}
```

### Phase 2.2: ループ内yield（要バイトコードVM）

```cb
async Future<void> task() {
    for (int i = 0; i < 5; i = i + 1) {
        println("Step {i}");
        yield;  // ループ状態を保存・復元
    }
}
```

### Phase 2.3: async関数からasync呼び出し

```cb
async Future<int> helper() {
    yield;
    return 42;
}

async Future<int> main_task() {
    int result = await helper();  // async関数内でawait
    return result + 10;
}
```

---

## 制限事項と注意点

### Phase 2.0の制限

1. **トップレベルのみ**:
   - yield文は関数のトップレベルでのみ使用可能
   - ループ、if文、関数呼び出し内では使用不可

2. **明示的なrun_event_loop()**:
   - ユーザーが手動でイベントループを開始する必要がある
   - await時の自動実行は未サポート

3. **エラーハンドリング**:
   - yield使用時のエラーメッセージを明確にする
   - 制限事項を実行時に検出してエラー

### 実装上の注意

1. **デバッグメッセージ**:
   - `GENERIC_DEBUG`を使わない
   - 専用の`DebugMsgId`を定義する（例：`ASYNC_YIELD_CONTROL`, `EVENT_LOOP_START`）

2. **スコープ管理**:
   - タスク切り替え時にスコープを正しく保存・復元
   - グローバル変数への影響を最小限に

3. **テスト**:
   - 各機能ごとに単体テストを作成
   - デバッグモードでの動作確認

---

## まとめ

Phase 2.0では、**最小限のyieldサポート**を実装します：

- ✅ トップレベルのyield文
- ✅ SimpleEventLoop
- ✅ 明示的な`run_event_loop()`
- ✅ 複数タスクの協調的実行

Phase 1の失敗を踏まえて、実装可能な範囲に絞り、段階的に機能を拡張していきます。

次のステップ：
1. YieldExceptionの復活
2. SimpleEventLoopの実装
3. テストケースの作成
