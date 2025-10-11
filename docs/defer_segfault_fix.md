# defer文のセグメンテーションフォルト修正

## 問題

`test_defer_loop.cb`と`test_defer_break.cb`でセグメンテーションフォルト（exit code 139）が発生していました。

## 原因

forループとwhileループでスコープが作成されていなかったため、ループ終了時にdeferスタックが正しく処理されていませんでした。deferスタックとスコープが同期していないため、メモリ破壊が発生していました。

## 修正内容

### 1. control_flow_executor.cpp - forループにスコープ追加

```cpp
void ControlFlowExecutor::execute_for_statement(const ASTNode *node) {
    debug_msg(DebugMsgId::INTERPRETER_FOR_STMT_START, "");

    // forループ用のスコープを作成（初期化変数とdefer用）
    interpreter_->push_scope();

    try {
        // ... ループ本体 ...
    } catch (const BreakException &e) {
        // break文でループ脱出
    }

    // forループのスコープを終了（deferを実行）
    interpreter_->pop_scope();
}
```

### 2. control_flow_executor.cpp - whileループにスコープ追加

```cpp
void ControlFlowExecutor::execute_while_statement(const ASTNode *node) {
    debug_msg(DebugMsgId::INTERPRETER_WHILE_STMT_START, "");

    // whileループ用のスコープを作成（defer用）
    interpreter_->push_scope();

    try {
        // ... ループ本体 ...
    } catch (const BreakException &e) {
        // break文でループ脱出
        debug_msg(DebugMsgId::INTERPRETER_WHILE_BREAK, "");
    }

    // whileループのスコープを終了（deferを実行）
    interpreter_->pop_scope();

    debug_msg(DebugMsgId::INTERPRETER_WHILE_STMT_END, "");
}
```

## 修正後の動作

### test_defer_loop.cb
```
Loop test:
0
1
2
Done
defer
```
✅ **PASSED** - ループ終了後にdeferが正しく実行される

### test_defer_break.cb
```
Break test:
0
1
2
Done
defer
```
✅ **PASSED** - break後にdeferが正しく実行される

## テスト結果

### 手動実行テスト
全8テストケースが成功：
- test_defer_basic.cb ✅
- test_defer_println.cb ✅
- test_defer_two.cb ✅
- test_defer_mixed.cb ✅
- test_defer_after.cb ✅
- test_defer_scope.cb ✅
- test_defer_loop.cb ✅
- test_defer_break.cb ✅

### インテグレーションテスト
`make integration-test`の結果：
```
[integration-test] Running Defer Statement Tests...
[integration-test] ✅ PASS: Defer Statement Tests (79 tests)
```

全テストケース成功、79個のアサーション全てパス ✅

## 新規作成ファイル

### tests/integration/defer/test_defer.hpp
- 8つのテストケースを含む包括的なインテグレーションテスト
- LIFO順序、スコープ管理、ループとの統合をテスト
- 期待される出力と実際の出力を詳細に検証

### tests/integration/main.cpp（修正）
- `#include "defer/test_defer.hpp"` を追加
- `test_integration_defer`をテストスイートに追加

## 技術的詳細

### スコープとdeferの関係
1. `push_scope()`が呼ばれると、`push_defer_scope()`も呼ばれる
2. `pop_scope()`が呼ばれると、`pop_defer_scope()`が先に実行される
3. `pop_defer_scope()`内でLIFO順にdefer文が実行される

### ループスコープの必要性
- forループの初期化変数はループスコープ内で宣言される
- ループ終了時にdeferを実行する必要がある
- break/continue時も適切にdeferが実行される

### メモリ安全性
- defer実行前にベクトルのコピーを作成（イテレータ無効化を防ぐ）
- スコープの階層構造を正しく維持
- 例外発生時もdeferが正しく実行される

## 結論

forループとwhileループにスコープを追加することで、deferスタックとスコープが正しく同期し、セグメンテーションフォルトが解決されました。全8テストケース、79のアサーションが成功し、defer文の実装が完了しました。
