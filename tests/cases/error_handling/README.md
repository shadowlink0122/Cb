# エラーハンドリング テストスイート

## 概要

v0.13.1で実装された`try`式 / `checked`式 / `?`演算子と`RuntimeError`列挙型の統合挙動を検証します。ポインタ参照・配列アクセス・算術演算が失敗した際に`Result<T, RuntimeError>`へ包まれるか、そして`?`で適切に伝播するかを重点的にテストします。

## 基本構文

```cb
Result<int, RuntimeError> safe_divide(int lhs, int rhs) {
    return try (lhs / rhs);
}

Result<int, RuntimeError> fetch(int[] values, int index) {
    return checked values[index];
}

Result<int, RuntimeError> compute(int* ptr) {
    int value = (try *ptr)?;  // 失敗時はErrをそのまま伝播
    return try (value / 2);
}
```

## テストファイル

### 1. `basic.cb`
- `try`式でのポインタデリファレンス
- `?`演算子による早期リターン
- `checked`式による境界チェック

**実行方法**
```bash
./main tests/cases/error_handling/basic.cb
```

### 2. `runtime_error_enum.cb`
- `RuntimeError`列挙型のバリアント生成と`match`式での判定

**実行方法**
```bash
./main tests/cases/error_handling/runtime_error_enum.cb
```

### 3. `try_checked.cb`
- ゼロ除算と配列範囲外アクセスを`Result`で受け取るユーティリティ
- `Ok`/`Err`の検証ヘルパーによる期待値チェック

**実行方法**
```bash
./main tests/cases/error_handling/try_checked.cb
```

## 全テスト実行

```bash
for file in tests/cases/error_handling/*.cb; do
    ./main "$file" || break
done
```

## 実装ステータス

- [x] テストケース作成
- [x] Integration test作成
- [x] ドキュメント更新
