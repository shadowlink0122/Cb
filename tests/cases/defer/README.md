# defer文のテストケース

defer文は、スコープ終了時に実行される文を登録する機能です。
複数のdefer文がある場合、登録された逆順（LIFO: Last In First Out）で実行されます。

## テストケース一覧

### 基本機能

#### test_defer_basic.cb ✅
- **概要**: defer文の基本動作
- **テスト内容**: 
  - 単純なdefer文の実行
  - 通常の文との混在
  - LIFO順での実行確認
- **期待出力**: `Start`, `Middle`, `1`, `2` (LIFO順)

#### test_defer_println.cb ✅
- **概要**: println関数とdeferの組み合わせ
- **テスト内容**:
  - defer内でのprintln実行
  - 単一のdeferとprintlnの混在
- **期待出力**: `1`, `2`, `3`

#### test_defer_two.cb ✅
- **概要**: 2つのdefer文のLIFO実行
- **テスト内容**:
  - 2つのdefer文が逆順で実行されることを確認
  - 登録順: 2→1
  - 実行順: 1→2
- **期待出力**: `1`, `2`

#### test_defer_mixed.cb ✅
- **概要**: 通常の文とdeferの混在
- **テスト内容**:
  - defer前後のprintln
  - 複数のdeferと通常の文の実行順序
- **期待出力**: `Start`, `1`, `2`

#### test_defer_after.cb ✅
- **概要**: defer後の文の実行
- **テスト内容**:
  - defer文の後に通常の文が実行される
  - defer文は最後に実行される
- **期待出力**: `Start`, `Middle`, `1`, `2`

### スコープ管理

#### test_defer_scope.cb ✅
- **概要**: スコープごとのdefer実行
- **テスト内容**:
  - ブロックスコープ内のdefer
  - ネストしたスコープでの動作
  - 各スコープ終了時にdeferが実行されることを確認
- **期待出力**: 
  - 各ブロックの開始・終了メッセージ
  - 各スコープ終了時にdefer実行
  - ネストしたスコープの正しい実行順序

### 制御フロー

#### test_defer_loop.cb ✅
- **概要**: ループとdeferの相互作用
- **テスト内容**:
  - forループ終了時にdeferが実行される
  - ループスコープでのdefer管理
- **期待出力**: `Loop test:`, `0`, `1`, `2`, `Done`, `defer`

#### test_defer_break.cb ✅
- **概要**: break文とdeferの相互作用
- **テスト内容**:
  - forループでのbreak時にdeferが実行されること
  - breakによる早期脱出でもdeferが正しく動作すること
- **期待出力**: `Break test:`, `0`, `1`, `2`, `Done`, `defer` (3,4は出力されない)

## テスト実行方法

### 個別実行
```bash
./main tests/cases/defer/test_defer_basic.cb
./main tests/cases/defer/test_defer_scope.cb
# ... その他のテスト
```

### 一括実行（手動）
```bash
# すべてのdeferテストを実行
for test in test_defer_basic.cb test_defer_println.cb test_defer_two.cb test_defer_mixed.cb test_defer_after.cb test_defer_scope.cb test_defer_loop.cb test_defer_break.cb; do
    echo "=== $test ==="
    ./main tests/cases/defer/$test && echo "✅ PASSED" || echo "❌ FAILED"
    echo ""
done
```

### インテグレーションテスト
```bash
# 全体のインテグレーションテストを実行
make integration-test

# deferテストの結果のみ表示
make integration-test 2>&1 | grep -A 20 "Defer Statement Tests"
```

## テスト結果

**全8テストケース成功** ✅

### インテグレーションテスト結果
```
[integration-test] Running Defer Statement Tests...
[integration-test] Running defer tests...
[integration-test] [PASS] Basic defer with LIFO order (test_defer_basic.cb)
[integration-test] [PASS] Simple defer with println (test_defer_println.cb)
[integration-test] [PASS] Two defer statements in LIFO order (test_defer_two.cb)
[integration-test] [PASS] Mixed defer and regular statements (test_defer_mixed.cb)
[integration-test] [PASS] Defer after regular statements (test_defer_after.cb)
[integration-test] [PASS] Nested scope with defer (test_defer_scope.cb)
[integration-test] [PASS] Defer with for loop (test_defer_loop.cb)
[integration-test] [PASS] Defer with break statement (test_defer_break.cb)
[integration-test] Defer tests completed successfully
[integration-test] ✅ PASS: Defer Statement Tests (79 tests)
```

**79個のアサーション全てパス** ✅

## 期待される動作

### LIFO順での実行
```cb
defer println("1");
defer println("2");
defer println("3");
// 実行順: 3 → 2 → 1
```

### スコープ終了時の実行
```cb
{
    defer println("inner");
}  // ここで"inner"が実行される
```

### return時の実行
```cb
int func() {
    defer println("cleanup");
    return 42;  // returnの前に"cleanup"が実行される
}
```

## 実装の詳細

### スコープ管理
- 各スコープごとにdeferスタックを管理
- `push_scope()` 時に新しいdeferスコープを作成
- `pop_scope()` 時にdeferを実行してからスコープを破棄

### 実行タイミング
1. 通常のスコープ終了時
2. return文実行時
3. break/continue実行時（ループスコープ）
4. 例外発生時（将来実装予定）

### メモリ管理
- deferスタックはASTノードへのポインタを保持
- スコープをまたぐ場合はコピーを作成
- イテレータ無効化を防ぐための実装

## 実装状況

### 完了した機能 ✅
- ✅ 基本的なdefer文の実行
- ✅ LIFO（Last In First Out）順での実行
- ✅ スコープごとのdefer管理
- ✅ ブロックスコープとの統合
- ✅ return文実行時のdefer
- ✅ forループとの統合（ループスコープでdefer）
- ✅ whileループとの統合
- ✅ break/continue時のdefer実行
- ✅ 複数のdeferの連鎖

### 技術的詳細
- **スコープ管理**: `push_scope()`時に`push_defer_scope()`を呼び出し
- **実行タイミング**: `pop_scope()`時に`pop_defer_scope()`でLIFO実行
- **メモリ安全性**: defer実行前にベクトルのコピーを作成（イテレータ無効化防止）
- **例外処理**: defer実行中のエラーは無視して次のdeferを実行（Go仕様準拠）

## 今後の拡張（将来の検討事項）

- [ ] defer内でのreturn禁止の実装（コンパイル時エラー）
- [ ] defer実行中のエラーハンドリング強化
- [ ] パフォーマンス最適化（大量のdefer時）
- [ ] デバッグ機能（deferスタックのトレース）
- [ ] 関数ポインタを使ったdefer
- [ ] defer内での複合文（ブロック）のサポート
