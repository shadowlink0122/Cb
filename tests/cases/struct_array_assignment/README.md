# Struct Array Assignment Tests

構造体配列への代入機能のテストケース集です。

## バグ修正の背景

Week 3 Day 1のTaskQueue実装中に発見された構造体配列代入バグの修正を検証します。

### 修正前の問題

```cb
Task[10] tasks;
Task t = {42, 5, 1, nullptr};
tasks[0] = t;  // ❌ 代入が失敗していた
println(tasks[0].task_id);  // 0が出力される（期待値: 42）
```

### 修正後

構造体配列への全ての代入パターンが正しく動作します。

## テストケース

### test_basic.cb
基本的な構造体変数の配列要素への代入をテストします。

```cb
Task t = {42, 5};
tasks[0] = t;  // ✅ 正しく代入される
```

### test_literal.cb
構造体リテラルの直接代入をテストします。

```cb
tasks[0] = {100, 10};  // ✅ リテラルの直接代入
```

### test_loop.cb
ループ内での連続的な代入をテストします。

```cb
for (int i = 0; i < 10; i++) {
    Task t = {i, i * 2};
    tasks[i] = t;  // ✅ ループ内での代入
}
```

### test_element_copy.cb
配列要素間のコピーをテストします。

```cb
dst[0] = src[0];  // ✅ 配列要素間のコピー
```

### test_function_return.cb
関数戻り値の代入をテストします。

```cb
tasks[0] = task_create(999, 88);  // ✅ 関数戻り値の代入
```

### test_comprehensive.cb
全てのパターンを網羅的にテストします。

## 実行方法

```bash
# 個別テスト実行
./main tests/cases/struct_array_assignment/test_basic.cb
./main tests/cases/struct_array_assignment/test_literal.cb
./main tests/cases/struct_array_assignment/test_loop.cb
./main tests/cases/struct_array_assignment/test_element_copy.cb
./main tests/cases/struct_array_assignment/test_function_return.cb
./main tests/cases/struct_array_assignment/test_comprehensive.cb

# Integration test実行
make integration-test
```

## 期待される結果

全てのテストが以下の条件を満たすこと：

1. 終了コード: 0
2. 全てのアサーションが成功
3. 各テストの"PASSED"メッセージが出力される

## 関連ドキュメント

- [実装レポート](../../../docs/features/week3_day1_taskqueue_report.md)
- [技術詳細](../../../docs/todo/struct_array_assignment_technical.md)
- [バグ修正コミット](git log --oneline --grep="構造体配列代入")

## 影響

この修正により以下が可能になりました：

- Phase 1: Task[100]による固定配列実装
- コード量の50%削減（並列配列から固定配列へ）
- Phase 2: Vector<Task, A>による動的配列への道
