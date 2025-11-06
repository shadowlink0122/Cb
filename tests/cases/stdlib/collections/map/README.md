# Map Tests

このフォルダには、`stdlib/collections/map.cb`の実装をテストするファイルが含まれています。

## テストファイル

### test_basic.cb
- **目的**: Map<K, V>の基本機能のテスト
- **内容**:
  - insert() - キーと値のペアの挿入
  - get() - キーによる値の取得
  - contains() - キーの存在確認
  - remove() - キーと値のペアの削除
  - size() - マップのサイズ取得
  - 基本的なエッジケース

### test_stress.cb
- **目的**: AVLツリーバランシングと大規模データのストレステスト
- **内容**:
  - **Test 1**: 1000要素の挿入とツリーの高さ検証（O(log n)性能保証）
  - **Test 2**: 200回の検索操作（100回の成功、100回の失敗）
  - **Test 3**: 500要素の削除とサイズ管理の検証
  - **Test 4**: String型キーでの動作確認
- **パフォーマンス指標**:
  - 1000要素で高さ10（理論的最適値: log₂(1000)≈9.97）
  - すべての操作がO(log n)で動作

## AVLツリーバランシング

`stdlib/collections/map.cb`は、自己平衡二分探索木（AVLツリー）として実装されています：
- すべての挿入・検索・削除操作がO(log n)
- 4つの回転ケース（LL, LR, RR, RL）を実装
- Cbインタプリタのバグ回避のため、回転ロジックはインライン化

## テストの実行

```bash
# 基本テストの実行
./main tests/cases/stdlib/collections/map/test_basic.cb

# ストレステストの実行（推奨）
./main tests/cases/stdlib/collections/map/test_stress.cb
```

## 新しいテストの追加

このフォルダに新しいテストファイルを追加する際は：
1. ファイル名: `test_<feature>.cb`
2. 先頭にコメントでテストの目的を記述
3. このREADMEに説明を追加
