# PR: Dynamic Memory Management for stdlib Collections

## 概要
stdlib/collectionsのVector/Queueを動的メモリ管理に移行しました。new/deleteによるメモリ確保・解放、自動デストラクタによるRAIIを実装。

## 主な変更

### 1. Queue.cbの完全実装
- 循環バッファ + new/delete による動的メモリ管理
- `init()`: new int[capacity]で動的配列確保
- `resize()`: 新配列確保→データコピー→旧配列解放
- `enqueue()/dequeue()`: 循環バッファのインデックス管理
- デストラクタ `~self()`: スコープ終了時に自動メモリ解放

### 2. Vector.cbの確認
- 既に動的メモリ管理実装済み
- resize(), memcpy(), 自動デストラクタが動作中

### 3. ドキュメント更新
- README.md: 動的メモリ管理セクション追加
- release_notes/v0.11.0.md: Week 4 Day 3セクション追加

## テスト結果
- ✅ Vector: 全テスト成功
- ✅ Queue: 全テスト成功（基本操作、リサイズ、循環バッファ、デストラクタ）
- ✅ Integration tests: **3,463/3,463 (100%)**
- ✅ メモリリークなし（デストラクタ出力で確認）

## 技術的詳細

### 実装の制約
- Cbの構造体は値渡しのため、メソッド内のself変更が呼び出し元に反映されない
- push()/enqueue()内での自動リサイズは不可能
- 呼び出し側で明示的にresize()を呼び出す必要がある

### コード例
```cb
void main() {
    Queue q;
    q.init(5);       // capacity=5
    q.enqueue(100);
    q.enqueue(200);
    
    q.resize(10);    // 手動リサイズ
    
    // 自動デストラクタでメモリ解放
}
```

## 変更ファイル
- `stdlib/collections/queue.cb` (+215行): 動的メモリQueue実装
- `README.md`: 動的メモリ管理セクション追加
- `release_notes/v0.11.0.md`: Week 4 Day 3ドキュメント

## 次のステップ
async/await実装の基盤が整いました。Event Loop、Future<T>、async/await構文の実装に進みます。

## コミット
- `c912a44`: feat(stdlib): Implement dynamic memory management for Queue collection
