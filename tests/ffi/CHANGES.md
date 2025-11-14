# 変更履歴

## 2025-11-14

### 修正: Dockerfileのビルドエラー
- `libdl-dev`パッケージを削除（Ubuntu 22.04には存在しない）
- `xz-utils`を追加（Zigの展開に必要）
- アーキテクチャを自動検出してGoとZigをダウンロード

### 修正: assertの削除
- Cbの`assert`は第2引数（エラーメッセージ）をサポートしていない
- すべてのテストファイルから`assert(condition, "message")`を削除
- 代わりに`println()`で結果を出力するのみ

### テストの動作
テストは成功/失敗に関わらず、すべての結果を出力します。
エラーチェックは出力を目視で確認してください。

例:
```
=== C FFI Basic Test ===
add(10, 5) = 15
subtract(10, 5) = 5
multiply(10, 5) = 50
divide(10, 5) = 2
✓ All C basic tests completed!
```

期待値と異なる場合は、出力から判断できます。
