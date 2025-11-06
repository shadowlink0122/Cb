# Memory Error Tests

このディレクトリには、メモリ管理に関するエラーケースのテストが含まれています。
各テストファイルは意図的にエラーを引き起こし、適切にエラーハンドリングされることを確認します。

## エラーテストファイル一覧

### 1. double_delete.cb
**二重削除エラー**
- 同じポインタを2回deleteすると発生
- 実行時エラーが期待される
- 理由: 既に解放されたメモリの二重解放

```cb
int* ptr = new int;
delete ptr;
delete ptr;  // ERROR: 二重削除
```

### 2. use_after_delete.cb
**delete後の使用エラー（Use-After-Free）**
- deleteした後のポインタにアクセス
- 実行時エラーが期待される
- 理由: 解放済みメモリへのアクセス

```cb
Node* node = new Node;
delete node;
int val = node->value;  // ERROR: delete後のアクセス
```

### 3. delete_uninitialized.cb
**未初期化ポインタの削除エラー**
- 初期化されていないポインタをdelete
- 実行時エラーが期待される
- 理由: 無効なアドレスの解放

```cb
int* ptr;  // 未初期化
delete ptr;  // ERROR: 未初期化ポインタのdelete
```

### 4. memory_leak_detection.cb
**メモリリーク検出**
- deleteせずにポインタを上書き
- 警告またはリーク検出が期待される
- 理由: 割り当てたメモリが解放されない

```cb
int* ptr = new int;
ptr = 0;  // LEAK: 元のメモリが解放されない
```

### 5. dangling_pointer_return.cb
**ダングリングポインタ返却エラー**
- ローカル変数のアドレスを関数から返す
- 実行時エラーが期待される
- 理由: スコープ外の変数へのアクセス

```cb
int* func() {
    int x = 42;
    return &x;  // ERROR: ローカル変数のアドレス
}
```

### 6. invalid_pointer_arithmetic.cb
**無効なポインタ演算エラー**
- new/deleteで管理されていないポインタをdelete
- 実行時エラーが期待される
- 理由: 不正なメモリアドレスの解放

```cb
int* ptr = 0x12345678;  // 任意のアドレス
delete ptr;  // ERROR: newで割り当てられていない
```

## テスト実行方法

### 個別実行（エラーが期待される）
```bash
./main tests/cases/memory/errors/double_delete.cb
# Expected: Error or crash

./main tests/cases/memory/errors/use_after_delete.cb
# Expected: Error or crash

./main tests/cases/memory/errors/delete_uninitialized.cb
# Expected: Error or crash
```

### Integration Test経由
```bash
make test
# Integration testフレームワークが各エラーを検証
```

## 期待される動作

| テストファイル | 期待される結果 | 終了コード |
|-------------|-------------|----------|
| double_delete.cb | 実行時エラー | != 0 |
| use_after_delete.cb | 実行時エラー | != 0 |
| delete_uninitialized.cb | 実行時エラー | != 0 |
| memory_leak_detection.cb | 正常終了（リーク警告） | 0 |
| dangling_pointer_return.cb | 実行時エラー | != 0 |
| invalid_pointer_arithmetic.cb | 実行時エラー | != 0 |

## エラー検出の仕組み

現在のCb実装では、以下の方法でエラーを検出します:

1. **二重削除**: OSレベルで検出（segfault）
2. **Use-After-Free**: OSレベルで検出（segfault）
3. **未初期化ポインタ**: OSレベルで検出（segfault）
4. **メモリリーク**: 将来的にメモリトラッカーで検出予定
5. **ダングリングポインタ**: OSレベルで検出（segfault）
6. **無効ポインタ**: OSレベルで検出（segfault）

## 今後の改善

将来的に以下の機能を追加予定:

- [ ] メモリトラッキング（割り当て/解放の記録）
- [ ] ダングリングポインタ検出（delete時にポインタをnullに設定）
- [ ] メモリリーク検出（プログラム終了時のチェック）
- [ ] デバッグモードでの詳細なエラーメッセージ
- [ ] Valgrindスタイルのメモリプロファイリング

## 注意事項

⚠️ これらのテストは意図的にエラーを引き起こします。
⚠️ 実際のコードではこのようなパターンを避けてください。
⚠️ 一部のテストはsegmentation faultを引き起こす可能性があります。

## ベストプラクティス

メモリ管理のベストプラクティス:

1. **deleteしたポインタは使用しない**
   ```cb
   delete ptr;
   ptr = 0;  // 明示的にnullに設定
   ```

2. **new/deleteはペアで使用**
   ```cb
   int* ptr = new int;
   // ... use ptr ...
   delete ptr;  // 必ずdeleteする
   ```

3. **nullポインタのチェック**
   ```cb
   if (ptr != 0) {
       delete ptr;
   }
   ```

4. **スマートポインタの使用（将来実装予定）**
   ```cb
   // 将来: RAII スタイルの自動メモリ管理
   UniquePtr<int> ptr = make_unique<int>();
   // スコープ終了時に自動delete
   ```

---

**実装状況**: エラー検出機能は基本的なOSレベルの検出のみ
**テストカバレッジ**: 6つの主要なエラーパターン
**次のステップ**: メモリトラッキング機能の実装
