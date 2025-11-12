# async/await テストスイート

## 概要

このディレクトリには、Cb言語のasync/await機能（v0.12.0+）とResult型統合（v0.13.0+）のテストが含まれています。

## バージョン履歴

### v0.13.0 (2025-11-09)
- ✅ async関数からResult<T,E>型を返す機能
- ✅ awaitでResult型を受け取り、enum情報を完全保持
- ✅ パターンマッチングとの完全統合
- ✅ エラーハンドリングパターンのベストプラクティス

### v0.12.0 (2024-2025)
- Phase 1: 基本的なasync/await構文
- Phase 2: 協調的マルチタスキング（yield）
- Phase 2.0: 自動yield（ループ公平性）
- ビルトインFuture<T>型

## 基本構文

### async関数の定義と呼び出し

```cb
// 基本的なasync関数
async Future<int> fetch_data(int id) {
    return id * 10;
}

void main() {
    Future<int> f = fetch_data(42);
    int result = await f;
    println("Result: {result}");  // Result: 420
}
```

### async + Result型（v0.13.0）

```cb
// エラーハンドリング付きasync関数
async Future<Result<int, string>> divide(int a, int b) {
    if (b == 0) {
        return Result<int, string>::Err("Division by zero");
    }
    return Result<int, string>::Ok(a / b);
}

void main() {
    Future<Result<int, string>> f = divide(10, 2);
    Result<int, string> r = await f;
    
    match (r) {
        Ok(value) => {
            println("Success: {value}");
        }
        Err(msg) => {
            println("Error: {msg}");
        }
    }
}
```

## テストファイル

### v0.13.0 テスト

#### 1. comprehensive_async_result.cb
**包括的なasync + Result統合テスト**

**テスト内容**:
- 基本的なasync Result（成功/エラーケース）
- Variant access
- ネストされたResult型
- 文字列Result型
- チェーン操作（エラー伝搬）
- 早期リターン
- 複数の連続await操作

**実行方法**:
```bash
./main tests/cases/async/comprehensive_async_result.cb
```

**期待される出力**:
```
=== Comprehensive Async/Await + Result Test ===
--- Test 1: Basic async Result (success) ---
Test 1.1: divide_async(10, 2) = 5 - PASSED
...
=== All Tests Passed ===
```

---

#### 2. test_async_result_integration.cb
**async Result基本統合テスト**

**テスト内容**:
- 基本的なasync Result機能
- パターンマッチング
- 成功/エラーケース

**実行方法**:
```bash
./main tests/cases/async/test_async_result_integration.cb
```

---

#### 3. test_async_result_*.cb
その他のResult型テストファイル:
- `test_async_result_basic.cb` - 基本機能
- `test_async_result_debug.cb` - デバッグ検証
- `test_async_result_minimal.cb` - 最小限のテスト
- `test_async_result_simple.cb` - シンプルなテスト
- `test_async_simple_result.cb` - シンプルなResult
- `test_async_variant_check.cb` - Variant確認

---

### Phase 2.0 テスト（v0.12.0）

#### 4. phase2_for_loop_fairness.cb
**forループ内の公平性テスト**

**テスト内容**:
- forループ内での自動yield
- 複数タスクの公平な実行

**実行方法**:
```bash
./main tests/cases/async/phase2_for_loop_fairness.cb
```

---

#### 5. phase2_while_loop_fairness.cb
**whileループ内の公平性テスト**

---

#### 6. phase2_auto_yield_test.cb
**自動yield機能テスト**

**テスト内容**:
- 各ステートメント後の自動yield
- タスクのインターリーブ実行

---

#### 7. phase2_yield_test.cb
**手動yield機能テスト**

---

### Phase 1 & Phase 2 基本テスト

#### 8. phase1_syntax_test.cb
**基本的なasync/await構文テスト**

#### 9. phase1_multiple_async.cb
**複数のasync関数テスト**

#### 10. phase2_builtin_future_test.cb
**ビルトインFuture<T>型テスト**

#### 11. phase2_direct_await_test.cb
**直接await呼び出しテスト**

#### 12. test_future_basic.cb
**Future<T>型の基本テスト**

---

### インターフェースとasync

#### 13. test_interface_basic.cb
**インターフェースの基本テスト**

#### 14. test_interface_concurrent.cb
**インターフェースの協調実行テスト**

#### 15. test_interface_self.cb
**インターフェースのself使用テスト**

---

### タスクキューとイベントループ

#### 16. test_task_queue_basic.cb
**タスクキューの基本テスト**

#### 17. test_task_queue_comprehensive.cb
**タスクキューの包括的テスト**

---

### その他のテスト

- `test_await_simple.cb` - シンプルなawaitテスト
- `test_async_loop.cb` - ループ内のasync
- `test_async_nested.cb` - ネストされたasync
- `test_async_recursive.cb` - 再帰的async
- `simple_async_loop_test.cb` - シンプルなループテスト
- `simple_auto_yield_test.cb` - シンプルな自動yield

---

## 全テスト実行

### 個別実行
```bash
# 個別のテストファイルを実行
./main tests/cases/async/comprehensive_async_result.cb
```

### カテゴリ別実行
```bash
# v0.13.0 Result型テストのみ
for file in tests/cases/async/test_async_result*.cb tests/cases/async/comprehensive_async_result.cb; do
    echo "Running $file..."
    ./main "$file"
done

# Phase 2.0テストのみ
for file in tests/cases/async/phase2*.cb; do
    echo "Running $file..."
    ./main "$file"
done
```

### 統合テスト
```bash
# すべてのテストを実行（Integration test含む）
make integration-test
```

---

## サンプルコード

### sample/async/async_result_error_handling.cb
**async + Result型のエラーハンドリングパターン集**

以下のパターンを含む実践的なサンプル:
1. 基本的なエラーハンドリング
2. エラー伝搬（早期リターン）
3. 複数のasync操作
4. フォールバックハンドリング
5. エラー収集
6. ネストされたResult処理
7. Variantチェック

**実行方法**:
```bash
./main sample/async/async_result_error_handling.cb
```

---

## 実装ステータス

### v0.13.0
- [x] async Future<Result<T,E>> 構文
- [x] awaitでのResult型保持
- [x] パターンマッチング統合
- [x] Variant access
- [x] チェーン操作
- [x] 包括的テスト作成
- [x] Integration test追加
- [x] サンプルコード作成
- [ ] ネストされたResult matchサポート（将来の改善）

### v0.12.0
- [x] Phase 1: 基本async/await
- [x] Phase 2: 協調的マルチタスキング
- [x] Phase 2.0: 自動yield
- [x] ビルトインFuture<T>型
- [x] インターフェースサポート

---

## 既知の制限事項

### v0.13.0
1. **ネストされたResult matchの制限**: 
   ```cb
   match (outer_result) {
       Ok(inner) => {
           match (inner) {  // まだサポートされていない
               Ok(value) => { ... }
           }
       }
   }
   ```
   回避策: inner Resultの値を直接処理

2. **配列初期化構文の制限**:
   ```cb
   int arr[3] = {1, 2, 3};  // サポートされていない
   ```
   回避策: 個別に値を設定

---

## トラブルシューティング

### テストが失敗する

**問題**: `await expression did not return struct`

**原因**: awaitの戻り値型が正しく処理されていない

**解決**: v0.13.0に更新済み。最新版をビルドしてください。

---

### enum情報が失われる

**問題**: `Result variant = ""`

**原因**: async関数からの返却でenum情報が失われている

**解決**: v0.13.0で修正済み。変数宣言時のenum情報保持を改善。

---

## 参照

- [async/await 設計ドキュメント](../../../docs/features/async_await_design.md)
- [Result型ドキュメント](../../../docs/features/result_type.md)
- [パターンマッチングドキュメント](../../../docs/features/pattern_matching.md)
- [サンプルコード](../../../sample/async/)
- [Integration Test Guide](../../../docs/testing/integration_test_guide.md)

---

**最終更新**: 2025-11-09  
**バージョン**: v0.13.0  
**ステータス**: ✅ 完成
