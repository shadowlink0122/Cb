# ラムダ式の即座実行機能 - 実装レポート

## 概要

v0.10.0で追加されたラムダ式に、**即座実行（Immediate Invocation）**機能を追加しました。これにより、ラムダ式を定義した直後に引数を渡して実行できるようになりました。

## 実装内容

### 1. 構文サポート

以下の形式をサポート：

```cb
// 基本的な即座実行
int result = int func(int a, int b) {
    return a * b;
}(10, 20);  // 200

// 式の中での使用
int value = int func(int x) {
    return x * x;
}(7) + 10;  // 59

// 複雑な本体を持つラムダの即座実行
int result = int func(int a, int b) {
    int sum = a + b;
    int product = a * b;
    return sum + product;
}(5, 3);  // 23
```

### 2. コード変更

#### 2.1 AST定義の拡張

**ファイル**: `src/common/ast.h`

- `is_lambda_call`フラグを追加：ラムダの即座実行を識別

```cpp
bool is_lambda_call = false;  // 無名関数の即座実行呼び出しかどうか
```

#### 2.2 パーサーの拡張

**ファイル**: `src/frontend/recursive_parser/parsers/primary_expression_parser.cpp`

`parseLambda()`関数の末尾に、ラムダ定義直後の`(`をチェックして関数呼び出しノードを作成する処理を追加：

```cpp
// ラムダの直接実行をサポート: int func(int x){return x;}(10) 形式
ASTNode *result = lambda;
while (parser_->check(TokenType::TOK_LPAREN)) {
    parser_->advance(); // consume '('
    
    // ラムダ即座実行ノードを作成
    ASTNode *call_node = new ASTNode(ASTNodeType::AST_FUNC_CALL);
    call_node->left = std::unique_ptr<ASTNode>(result);
    call_node->is_lambda_call = true;
    
    // 引数リストの解析
    if (!parser_->check(TokenType::TOK_RPAREN)) {
        do {
            ASTNode *arg = parser_->parseExpression();
            call_node->arguments.push_back(std::unique_ptr<ASTNode>(arg));
        } while (parser_->match(TokenType::TOK_COMMA));
    }
    
    parser_->consume(TokenType::TOK_RPAREN,
                     "Expected ')' after lambda call arguments");
    
    result = call_node; // 次のチェーンのために更新
}
```

**特徴**:
- チェーン呼び出しもサポート（`func()()()`形式）
- 通常の関数呼び出しチェーンと同じパターン

#### 2.3 インタプリタの拡張

**ファイル**: `src/backend/interpreter/evaluator/functions/call_impl.cpp`

`evaluate_function_call_impl()`の先頭に、`is_lambda_call`フラグをチェックする処理を追加：

```cpp
// ラムダの即座実行をチェック: int func(int x){return x;}(10) 形式
if (node->is_lambda_call && node->left) {
    const ASTNode *lambda_node = node->left.get();
    
    if (lambda_node->node_type == ASTNodeType::AST_LAMBDA_EXPR) {
        // 新しいスコープを作成してラムダを実行
        interpreter_.push_scope();
        
        // パラメータをバインド
        for (size_t i = 0; i < lambda_node->parameters.size(); ++i) {
            const ASTNode *param = lambda_node->parameters[i].get();
            int64_t arg_value = evaluate_expression(node->arguments[i].get());
            
            Variable var;
            var.type = param->type_info;
            var.value = arg_value;
            var.is_const = param->is_const;
            
            interpreter_.current_scope().variables[param->name] = var;
        }
        
        // ラムダ本体を実行
        int64_t result = 0;
        if (lambda_node->lambda_body) {
            try {
                for (const auto &stmt : lambda_node->lambda_body->statements) {
                    interpreter_.execute_statement(stmt.get());
                }
            } catch (const ReturnException &e) {
                result = e.value;
            }
        }
        
        // スコープをクリーンアップ
        interpreter_.pop_scope();
        
        return result;
    }
}
```

**処理フロー**:
1. ラムダノードが`node->left`にあることを確認
2. 新しいスコープを作成
3. 引数を評価してパラメータにバインド
4. ラムダ本体を実行
5. return文からの例外をキャッチして戻り値を取得
6. スコープをクリーンアップ

### 3. テストファイルの整理

#### 3.1 ファイル移動

integration/直下に散らばっていたラムダテストファイルを`lambda/cases/`に移動：

```bash
lambda_test.cb           → lambda/cases/comprehensive.cb
lambda_simple_test.cb    → lambda/cases/simple.cb
lambda_assign_test.cb    → lambda/cases/assignment.cb
lambda_call_test.cb      → lambda/cases/function_call.cb
lambda_compound_test.cb  → lambda/cases/compound_body.cb
lambda_debug_test.cb     → lambda/cases/debug.cb
```

#### 3.2 新規テストファイル

1. **`immediate_invocation.cb`** - 即座実行の基本テスト
   - 基本的な即座実行
   - 単一パラメータの即座実行
   - 複雑な式での即座実行

2. **`chain_invocation.cb`** - チェーン/式での使用テスト
   - 式の中での即座実行
   - 複数のラムダの連続実行
   - ネストしたラムダの即座実行

### 4. HPPテストスイートの更新

**ファイル**: `tests/integration/lambda/lambda_tests.hpp`

2つの新しいテストケースを追加：

```cpp
// テスト4: ラムダの即座実行（immediate invocation）
run_cb_test_with_output_and_time("./lambda/cases/immediate_invocation.cb", 
    [](const std::string& output, int exit_code) {
        INTEGRATION_ASSERT_EQ(0, exit_code, "Lambda immediate invocation should succeed");
        INTEGRATION_ASSERT_CONTAINS(output, "200", "Should print 200 (10 * 20)");
        INTEGRATION_ASSERT_CONTAINS(output, "49", "Should print 49 (7 * 7)");
        INTEGRATION_ASSERT_CONTAINS(output, "23", "Should print 23 ((5+3)+(5*3))");
        INTEGRATION_ASSERT_CONTAINS(output, "All tests passed!", "Should complete all tests");
    }, execution_time);

// テスト5: ラムダのチェーン呼び出し
run_cb_test_with_output_and_time("./lambda/cases/chain_invocation.cb", 
    [](const std::string& output, int exit_code) {
        INTEGRATION_ASSERT_EQ(0, exit_code, "Lambda chain invocation should succeed");
        INTEGRATION_ASSERT_CONTAINS(output, "60", "Should print 60 ((10+20)*2)");
        INTEGRATION_ASSERT_CONTAINS(output, "20", "Should print 20 ((5*2)+10)");
        INTEGRATION_ASSERT_CONTAINS(output, "21", "Should print 21 (7*3)");
        INTEGRATION_ASSERT_CONTAINS(output, "All tests passed!", "Should complete all tests");
    }, execution_time);
```

## テスト結果

### 統合テスト

```
=== Test Summary ===
Total:  2799
Passed: 2799
Failed: 0

🎉 ALL TESTS PASSED! 🎉
```

### ラムダテスト詳細

```
[integration-test] Running Lambda Function Tests...
[integration-test] Running lambda function tests...
[integration-test] [PASS] basic lambda function (basic.cb)
[integration-test] [PASS] lambda with multiple parameters (multiple_params.cb)
[integration-test] [PASS] lambda with void return (void_return.cb)
[integration-test] [PASS] lambda immediate invocation (immediate_invocation.cb)
[integration-test] [PASS] lambda chain invocation (chain_invocation.cb)
[integration-test] Lambda function tests completed
[integration-test] ✅ PASS: Lambda Function Tests (21 tests)
[integration-test] Average: 9.37 ms (8 measured tests)
```

**テストカバレッジ**:
- ラムダ関数の基本機能: 3ファイル
- ラムダの即座実行: 2ファイル
- 合計5テストケース、21アサーション

## 使用例

### 例1: 簡単な計算

```cb
int result = int func(int a, int b) {
    return a * b;
}(10, 20);

println(result);  // 200
```

### 例2: 式の中で使用

```cb
int value = int func(int x) {
    return x * 2;
}(15) + int func(int y) {
    return y * 3;
}(10);

println(value);  // 60 (30 + 30)
```

### 例3: 複雑な処理

```cb
int result = int func(int a, int b) {
    int sum = a + b;
    int product = a * b;
    return sum + product;
}(5, 3);

println(result);  // 23 ((5+3)+(5*3))
```

## メソッドチェインとの関連

この機能は、**メソッドチェイン**の一環として実装されています：

```cb
// 将来的にサポート予定（関数ポインタを返す場合）
int result = getOperation('+')(5, 3);  // チェーン呼び出し
```

現在の実装では：
- ラムダ式の即座実行はサポート ✅
- 通常の関数のチェーン呼び出しはサポート ✅
- 関数ポインタを返すラムダは今後の拡張で対応予定

## 技術的な注意点

### 1. スコープ管理

ラムダの即座実行では、以下のスコープ管理が行われます：
- 新しいスコープを作成（`push_scope()`）
- パラメータをローカル変数として登録
- 実行後にスコープをクリーンアップ（`pop_scope()`）

### 2. 戻り値の処理

- `ReturnException`をキャッチして戻り値を取得
- ラムダ本体で`return`文がない場合、デフォルト値（0）を返す

### 3. チェーン呼び出し

パーサーは`while`ループで連続する`(`をチェックするため、以下のような形式も自動的にサポート：

```cb
// 理論上可能（関数ポインタを返す場合）
result = func()()(10, 20);
```

## まとめ

### 実装完了した機能

✅ ラムダ式の定義直後に引数を渡して実行
✅ 式の中での即座実行
✅ チェーン呼び出しの構文サポート
✅ 包括的なテストカバレッジ（21アサーション）
✅ 全テスト合格（2,799テスト）

### 今後の拡張

- 関数ポインタを返すラムダのチェーン呼び出し
- クロージャ（外部変数のキャプチャ）
- 型推論（パラメータ型の省略）

## 関連ファイル

- `src/common/ast.h` - AST定義
- `src/frontend/recursive_parser/parsers/primary_expression_parser.cpp` - パーサー
- `src/backend/interpreter/evaluator/functions/call_impl.cpp` - インタプリタ
- `tests/integration/lambda/cases/immediate_invocation.cb` - テスト
- `tests/integration/lambda/cases/chain_invocation.cb` - テスト
- `tests/integration/lambda/lambda_tests.hpp` - HPPテストスイート

---

実装日: 2025年10月12日
バージョン: v0.10.0
機能: ラムダ式の即座実行（Immediate Lambda Invocation）
