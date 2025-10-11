# デフォルトメンバー機能テスト

このディレクトリには、Cb言語v0.10.0のデフォルトメンバー機能のテストが含まれています。

## テストファイル一覧

### メインテストスイート
- **`test_suite.cb`** - 包括的なテストスイート（期待値チェック付き）
  - 整数型、文字列型、bool型のデフォルトメンバー
  - 複数メンバーを持つ構造体
  - 配列内の構造体
  - ポインタアクセス
  - 暗黙的参照

### 個別機能テスト
- `test_default_member_basic.cb` - 基本的な暗黙的代入と参照
- `test_default_member_implicit_assign.cb` - 暗黙的代入のテスト
- `test_implicit_read.cb` - 暗黙的参照のテスト
- `test_default_all_types.cb` - 全型のテスト
- `test_default_typedef_enum.cb` - typedef/enumのテスト
- `test_default_array_pointer.cb` - 配列とポインタのテスト
- `test_default_impl.cb` - interface実装のテスト
- `test_bool_fix.cb` - bool型の修正確認

### エラーテスト
- `test_default_member_error_multiple.cb` - 複数デフォルトメンバーエラー

### ヘルパー
- `test_helpers.cb` - テストヘルパー関数（Cb言語用）
- `test_runner.hpp` - C++テストフレームワーク（将来の拡張用）

## テストの実行方法

### 全テストの実行
```bash
./main tests/cases/default_member/test_suite.cb
```

### 個別テストの実行
```bash
./main tests/cases/default_member/test_default_member_basic.cb
```

### 全テストの一括実行
```bash
for test in tests/cases/default_member/test_*.cb; do
    echo "=== $test ==="
    ./main "$test" 2>&1 | head -20
    echo ""
done
```

## テスト結果の見方

テストは以下の形式で結果を表示します:

```
✓ テスト名  # 成功
✗ テスト名  # 失敗
```

`test_suite.cb`は包括的なテストで、各機能を体系的にテストします。

## 期待される動作

### 暗黙的代入 (Implicit Assignment)
```cb
struct IntBox {
    default int value;
};

IntBox ib;
ib = 100;  // ib.value = 100 と同等
```

### 暗黙的参照 (Implicit Reference)
```cb
struct StringBox {
    default string value;
};

StringBox sb;
sb.value = "Hello";
println(sb);  // "Hello" が出力される
```

### ポインタアクセス
```cb
IntBox ib;
ib.value = 42;
IntBox* ptr = &ib;
println(ptr->value);  // 42
```

## テスト対象機能

### ✅ 完全サポート
- [x] int型のデフォルトメンバー
- [x] string型のデフォルトメンバー
- [x] bool型のデフォルトメンバー
- [x] double型のデフォルトメンバー
- [x] typedef型のデフォルトメンバー
- [x] 複数メンバーを持つ構造体
- [x] ポインタ経由のアクセス（int, string）
- [x] 配列内の構造体（明示的アクセス）
- [x] interface実装経由のアクセス
- [x] 暗黙的代入 (s = "value")
- [x] 暗黙的参照 (println(s))

### ⚠️ 部分的サポート
- [ ] 配列要素への暗黙的代入 (arr[0] = 100) - 未サポート
- [ ] union型のデフォルトメンバー - 暗黙的代入が動作しない
- [ ] enum型のデフォルトメンバー - 暗黙的代入が動作しない

### ❌ 既知の制限
- float型の構造体初期化 (構造体初期化全般の問題)

## トラブルシューティング

### テストが失敗する場合
1. 最新のビルドを確認: `make clean && make`
2. デバッグモードで実行: `./main --debug test_file.cb`
3. TEST_REPORT.mdで既知の問題を確認

### 新しいテストの追加
1. `test_helpers.cb`のアサーション関数を使用
2. 期待値と実際の値を明示的に比較
3. 失敗時に分かりやすいメッセージを出力

## 関連ドキュメント

### 📚 機能の詳細ドキュメント
- **[docs/features/default_member.md](../../../docs/features/default_member.md)** - 完全な機能仕様、実装の詳細、テスト結果

### その他
- `../../docs/spec.md` - 言語仕様書
- `../../docs/todo/v0.10.0_implementation_plan.md` - 実装計画
