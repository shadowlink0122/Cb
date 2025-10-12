# Rvalue Reference (T&&) Tests

このディレクトリには T&& (右辺値参照) のテストケースが含まれています。

## v0.10.0 実装状況

### ✅ 動作するテスト
- `syntax_parse.cb` - T&& 構文のパース
- `type_restriction.cb` - 型制限（構造体のみ）

### ❌ 動作しないテスト（v0.10.1以降で修正予定）
- `member_access.cb` - メンバーアクセス
- `member_assignment.cb` - メンバー代入
- `aliasing.cb` - エイリアスセマンティクス

## テスト実行方法

```bash
# 個別テスト
./main tests/cases/rvalue_reference/syntax_parse.cb

# 全テスト（統合テスト実装後）
make test
```

## 既知の制限事項

現在の実装では、T&& 変数は**構文的にのみサポート**されています。
参照セマンティクス（エイリアス動作）は未実装です。

詳細は `/docs/todo/v0.10.0_rvalue_reference_status.md` を参照してください。
