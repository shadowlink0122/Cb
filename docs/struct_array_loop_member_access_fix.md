# 構造体配列ループのメンバーアクセスエラー修正

## 問題

`./main tests/cases/struct/struct_array_loop.cb`を実行すると、ループ内での構造体配列要素のメンバーアクセス時に`(member access error)`が発生していました。

### エラーの再現

```cb
struct Point {
    int x;
    int y;
};

int main() {
    Point[3] points;
    
    // ループで初期化
    for (int i = 0; i < 3; i++) {
        points[i].x = i * 10;  // 書き込みは成功
        points[i].y = i * 20;
    }
    
    // ループで読み取り
    for (int i = 0; i < 3; i++) {
        println(points[i].x);  // (member access error) が発生
        println(points[i].y);  // (member access error) が発生
    }
    
    return 0;
}
```

## 原因

forループにスコープを追加した際（`push_scope()`/`pop_scope()`）、ループ内で作成された構造体配列要素の変数（`points[0]`, `points[1]`, `points[2]`）がループスコープに属するようになりました。

ループ終了時に`pop_scope()`が呼ばれると、これらの変数がスコープから削除され、ループ外からアクセスできなくなっていました。

### 問題の詳細

1. **forループにスコープを追加**（defer対応のため）
   ```cpp
   interpreter_->push_scope();  // ループ開始時
   // ... ループ本体 ...
   interpreter_->pop_scope();   // ループ終了時
   ```

2. **構造体配列要素の変数がループスコープに作成される**
   - `points[i].x`にアクセスすると、`points[i]`が存在しない場合に`create_struct_variable("points[i]", "Point")`が呼ばれる
   - これは**現在のスコープ**（＝ループスコープ）に変数を作成する

3. **ループ終了時にスコープがクリア**
   - `pop_scope()`でループスコープが削除される
   - `points[1]`, `points[2]`などの変数も削除される

4. **ループ外からアクセスできない**
   - ループ外から`points[1].x`にアクセスしようとすると、変数が見つからない
   - `(member access error)`が発生

## 解決策

forループとwhileループに**deferスコープのみ**を追加し、**変数スコープは追加しない**ようにしました。

### 修正内容

**control_flow_executor.cpp**

#### forループ
```cpp
// FOR文の実行
void ControlFlowExecutor::execute_for_statement(const ASTNode *node) {
    debug_msg(DebugMsgId::INTERPRETER_FOR_STMT_START, "");

    // forループ用のdeferスコープのみを作成（変数スコープは作成しない）
    // これにより、deferはループ終了時に実行されるが、
    // ループ内で作成された変数は親スコープに属する
    interpreter_->push_defer_scope();

    try {
        if (node->init_expr) {
            debug_msg(DebugMsgId::INTERPRETER_FOR_INIT_EXEC, "");
            interpreter_->execute_statement(node->init_expr.get());
        }

        // ... ループ本体 ...

    } catch (const BreakException &e) {
        // break文でループ脱出
    }

    // forループのdeferスコープを終了（deferを実行）
    interpreter_->pop_defer_scope();
}
```

#### whileループ
```cpp
// WHILE文の実行
void ControlFlowExecutor::execute_while_statement(const ASTNode *node) {
    debug_msg(DebugMsgId::INTERPRETER_WHILE_STMT_START, "");

    // whileループ用のdeferスコープのみを作成（変数スコープは作成しない）
    interpreter_->push_defer_scope();

    try {
        // ... ループ本体 ...
    } catch (const BreakException &e) {
        // break文でループ脱出
        debug_msg(DebugMsgId::INTERPRETER_WHILE_BREAK, "");
    }

    // whileループのdeferスコープを終了（deferを実行）
    interpreter_->pop_defer_scope();

    debug_msg(DebugMsgId::INTERPRETER_WHILE_STMT_END, "");
}
```

### 修正の効果

1. **deferスコープと変数スコープの分離**
   - `push_defer_scope()`/`pop_defer_scope()`は独立して呼び出される
   - 変数は親スコープ（通常はmain関数スコープ）に属する

2. **構造体配列要素の変数が永続化**
   - ループ内で作成された`points[i]`は親スコープに属する
   - ループ終了後もアクセス可能

3. **deferは正しく動作**
   - ループ内の`defer`文はループ終了時に実行される
   - LIFO順での実行も保証される

## テスト結果

### 構造体配列ループテスト

**test_struct_array_loop.cb**
```
=== Struct Array Loop Test ===
Processing 3 points
Point:
0
0
0
Point:
1
10
20
Point:
2
20
40
=== Test completed ===
```
✅ **PASSED** - 全てのメンバーアクセスが成功

### deferテスト

全てのdeferテストも引き続き成功：

```
=== test_defer_basic.cb ===
Start
Middle
1
2
✅ PASSED

=== test_defer_loop.cb ===
Loop test:
0
1
2
Done
defer
✅ PASSED

=== test_defer_break.cb ===
Break test:
0
1
2
Done
defer
✅ PASSED
```

### インテグレーションテスト

```
[integration-test] Running Defer Statement Tests...
[integration-test] ✅ PASS: Defer Statement Tests (79 tests)
```

全79個のアサーションがパス ✅

## まとめ

forループとwhileループのスコープ管理を修正し、deferスコープと変数スコープを分離することで：

1. ✅ 構造体配列要素のメンバーアクセスエラーを解決
2. ✅ deferの正しい動作を維持
3. ✅ 全てのテストケースが成功

この修正により、ループ内で作成された構造体配列要素の変数が親スコープに属するため、ループ終了後もアクセス可能になりました。同時に、deferスコープは独立して管理されるため、ループ終了時にdeferが正しく実行されます。
