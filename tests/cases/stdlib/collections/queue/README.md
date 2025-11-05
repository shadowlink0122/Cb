# Queue Tests

このフォルダには、`stdlib/collections/queue.cb`の実装をテストするファイルが含まれています。

## テストファイル

### test_queue_comprehensive.cb
- **目的**: Queue<T>の全機能の包括的なテスト
- **内容**:
  - enqueue() - 要素のキューへの追加
  - dequeue() - 要素のキューからの取り出し
  - front() - 先頭要素の参照
  - size() - キューのサイズ取得
  - is_empty() - 空判定
  - FIFO（First-In-First-Out）動作の検証

### test_queue_string.cb
- **目的**: Queue<String>の文字列操作テスト
- **内容**: 文字列型キューの基本操作

### test_queue_single_enqueue.cb
- **目的**: 単一要素の操作テスト
- **内容**: エッジケースと最小操作の検証

### test_queue_import.cb
- **目的**: インポート機構のテスト
- **内容**: `import "stdlib/collections/queue.cb"`の動作確認

## パフォーマンス特性

- **enqueue()**: O(1)
- **dequeue()**: O(1)
- **front()**: O(1)
- **size()**: O(1)

## データ構造

Queueは内部的に動的配列またはリンクリストで実装されています：
- FIFO（先入れ先出し）保証
- 効率的なメモリ管理
- ジェネリック型サポート（Queue<T>）

## テストの実行

```bash
# 包括的テスト
./main tests/cases/stdlib/collections/queue/test_queue_comprehensive.cb

# 文字列型テスト
./main tests/cases/stdlib/collections/queue/test_queue_string.cb

# 基本操作テスト
./main tests/cases/stdlib/collections/queue/test_queue_single_enqueue.cb
```

## 新しいテストの追加

このフォルダに新しいテストファイルを追加する際は：
1. ファイル名: `test_queue_<feature>.cb`
2. 先頭にコメントでテストの目的を記述
3. このREADMEに説明を追加
