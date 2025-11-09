# Phase 1 協調的マルチタスク実装の実現可能性評価

**評価日**: 2025年11月7日  
**評価者**: GitHub Copilot

---

## 🎯 Phase 1の目標（再確認）

設計ドキュメントによると、Phase 1は以下を目指しています：

```cb
// Phase 1: 即座実行モデル
async Future<int> fetch_data(int id) {
    println("Fetching {id}...");
    return id * 100;
}

void main() {
    Future<int> f1 = fetch_data(1);  // ← 呼び出し時に完了まで実行
    Future<int> f2 = fetch_data(2);  // ← 呼び出し時に完了まで実行
    
    int r1 = await f1;  // ← 既に完了済みなので値を取得するだけ
    int r2 = await f2;
    
    println("Results: {r1}, {r2}");
}
```

**Phase 1の特徴**:
- ✅ async関数は呼び出し時に完了まで実行（順次実行）
- ✅ yieldなし
- ✅ 並行実行なし
- ✅ デバッグトレースあり

---

## ✅ 実現可能性の評価: **YES（実装可能）**

### 理由

Phase 1は**非常にシンプル**な設計であり、現在の実装に必要な変更は最小限です。

---

## 📋 必要な実装タスク

### 1. async関数の即座実行 ⚡ 簡単

**現状**: 
- async関数呼び出し時に`AsyncTask`を作成して、後で実行しようとしている
- 複雑な`execute_task_one_step()`やステートメント管理がある

**Phase 1での変更**:
```cpp
// async関数が呼ばれたとき
TypedValue call_async_function(FunctionDef* func, const std::vector<Variable>& args) {
    // 1. 新しいスコープで関数を実行
    push_scope();
    
    // 2. 引数を設定
    for (size_t i = 0; i < args.size(); i++) {
        current_scope().variables[func->parameters[i]->name] = args[i];
    }
    
    // 3. 関数本体を実行（通常の関数呼び出しと同じ）
    TypedValue return_value;
    try {
        execute_statement(func->body.get());
        return_value.value = 0;  // return文なし
    } catch (const ReturnException& e) {
        return_value = e.value;  // return文あり
    }
    
    pop_scope();
    
    // 4. Future構造体を作成
    Variable future_var = create_future_struct(func->return_type, return_value);
    future_var.struct_members["is_ready"].value = 1;  // 完了済み
    
    return convert_to_typed_value(future_var);
}
```

**作業量**: 50-100行程度の新規コード

---

### 2. awaitの簡易評価 ⚡ 簡単

**現状**: 
- `evaluate_await()`が複雑なタスク管理をしようとしている
- 無限再帰の問題がある

**Phase 1での変更**:
```cpp
TypedValue evaluate_await(const ASTNode* node) {
    // 1. オペランドを評価（Future構造体を取得）
    TypedValue future_value = evaluate_expression(node->left.get());
    
    // 2. Future構造体から値を取得
    Variable* future_var = get_variable_from_typed_value(future_value);
    
    // 3. is_readyをチェック（Phase 1では常にtrue）
    auto ready_it = future_var->struct_members.find("is_ready");
    if (ready_it->second.value == 0) {
        // Phase 1では発生しないはず
        throw std::runtime_error("Future is not ready (should not happen in Phase 1)");
    }
    
    // 4. valueメンバーを取得して返す
    auto value_it = future_var->struct_members.find("value");
    return convert_to_typed_value(value_it->second);
}
```

**作業量**: 30-50行程度の新規コード

---

### 3. デバッグトレース追加 ⚡ 簡単

**追加するトレース**:

```cpp
// async関数の開始
if (debug_mode) {
    std::cerr << "[ASYNC] Entering async function: " << func->name << "(";
    for (size_t i = 0; i < args.size(); i++) {
        if (i > 0) std::cerr << ", ";
        std::cerr << func->parameters[i]->name << "=" << args[i].value;
    }
    std::cerr << ")" << std::endl;
}

// async関数の終了
if (debug_mode) {
    std::cerr << "[ASYNC] Returning from async function: " << func->name
              << " -> Future{is_ready=true}" << std::endl;
}

// awaitの評価
if (debug_mode) {
    std::cerr << "[AWAIT] Awaiting Future (already ready)" << std::endl;
    std::cerr << "[AWAIT] Extracted value: " << result.value << std::endl;
}
```

**作業量**: 10-20行程度の追加

---

### 4. 不要なコードの削除 🧹 重要

**削除すべきもの**:
- `AsyncTask` の複雑なフィールド（`current_statement_index`, `is_suspended`など）
- `execute_task_one_step()` 関数
- `execute_tasks_concurrently()` 関数
- `YieldException` クラス
- yield文の処理コード

**理由**: Phase 1ではこれらは全て不要。コードを複雑にしているだけ。

**作業量**: 削除するだけなので簡単（200-300行程度の削除）

---

## 🔧 実装の流れ

### Step 1: クリーンアップ（1-2時間）
1. 既存の複雑なAsyncTask管理コードを削除
2. YieldExceptionとyield文の処理を削除
3. execute_task_one_step()とexecute_tasks_concurrently()を削除

### Step 2: Phase 1実装（2-3時間）
1. `call_async_function()` を実装（即座実行）
2. `evaluate_await()` を簡易版に書き換え
3. デバッグトレースを追加

### Step 3: テスト（1時間）
1. シンプルなテストケースを作成
2. デバッグモードで動作確認
3. エラーケースのテスト

**総作業時間見積もり**: 4-6時間

---

## ✨ Phase 1実装後のメリット

### 1. シンプルで理解しやすい
- コード量が大幅に削減される
- 複雑な状態管理がない
- デバッグが容易

### 2. 確実に動作する
- 通常の関数呼び出しとほぼ同じ
- エッジケースが少ない
- 無限再帰やスタックオーバーフローのリスクなし

### 3. async/await構文に慣れることができる
- ユーザーが文法を学べる
- 将来のPhase 2への移行がスムーズ

### 4. 段階的拡張が可能
- Phase 2でyieldを追加
- Phase 3でイベントループを追加
- 各段階で動作するバージョンが存在する

---

## 🚧 Phase 1の制限（再確認）

### ❌ できないこと

1. **真の並行実行**
   ```cb
   // これは順次実行される
   Future<int> f1 = fetch_data(1);  // ← ここで完了まで実行
   Future<int> f2 = fetch_data(2);  // ← ここで完了まで実行
   
   // 出力:
   // Fetching 1...
   // Data 1 ready
   // Fetching 2...
   // Data 2 ready
   ```

2. **yield（中断・再開）**
   ```cb
   async Future<void> task() {
       println("Step 1");
       yield;  // ← エラー: yieldは使えない
       println("Step 2");
   }
   ```

3. **非同期I/O**
   ```cb
   async Future<string> read_file(string path) {
       // Phase 1ではブロッキング読み込み
       return read_file_blocking(path);
   }
   ```

### ✅ できること

1. **async/await構文の使用**
   ```cb
   async Future<int> compute() {
       return 42;
   }
   
   void main() {
       Future<int> f = compute();
       int result = await f;
       println("Result: {result}");
   }
   ```

2. **Future型の利用**
   ```cb
   struct Future<T> {
       T value;
       bool is_ready;
   };
   ```

3. **デバッグトレース**
   ```bash
   ./main --debug test.cb
   
   # 出力:
   # [ASYNC] Entering async function: compute()
   # [ASYNC] Returning -> Future{is_ready=true}
   # [AWAIT] Awaiting Future (already ready)
   # [AWAIT] Extracted value: 42
   ```

---

## 📊 結論

### ✅ Phase 1は実装可能です

**理由**:
1. **技術的難易度**: 低い（通常の関数呼び出しとほぼ同じ）
2. **必要な作業量**: 少ない（4-6時間程度）
3. **リスク**: 低い（シンプルな実装、エッジケースが少ない）

### 🎯 推奨アクション

1. **既存の複雑なコードを削除**
   - AsyncTaskの不要なフィールド
   - execute_task_one_step()
   - YieldException関連

2. **Phase 1を実装**
   - call_async_function()（即座実行）
   - evaluate_await()（簡易版）
   - デバッグトレース

3. **テストして動作確認**
   - シンプルなテストケース
   - デバッグモードでトレース確認

4. **Phase 2の設計を詳細化**
   - yieldの実装方法（バイトコードVM vs CPS変換）
   - イベントループの設計

---

## 🤔 Phase 2への道筋

Phase 1が動作した後、Phase 2（真の協調的マルチタスク）への移行は大きな変更が必要です：

### Phase 2で必要なこと

1. **バイトコードVM** または **CPS変換**
   - 任意の位置での中断・再開
   - プログラムカウンタによる実行位置管理

2. **イベントループ**
   - 複数タスクのスケジューリング
   - ラウンドロビン実行

3. **ループ状態の保存**
   - イテレーション回数
   - ループ変数

**Phase 2の作業時間見積もり**: 20-40時間（大規模なリファクタリング）

---

## 💡 最終評価

**Phase 1の協調的マルチタスク実装は実現可能です。**

ただし、「協調的マルチタスク」という言葉の定義によります：

- **Phase 1**: 「async/await構文を使える」という意味では実現可能 ✅
- **真の協調的マルチタスク**: yieldによる中断・再開、複数タスクのインターリーブ実行はPhase 2以降 ⏭️

Phase 1は**async/awaitの基礎を固める**ための重要なステップです。
