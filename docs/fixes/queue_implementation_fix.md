# Queue<T> 実装の問題修正レポート

## 日付
2025年10月30日

## 問題の概要
Queue<T>のジェネリック実装において、2つの異なるアプローチで問題が発生：
1. **リンクリスト実装**: 無限ループとバスエラー
2. **循環バッファ実装**: memcpyによるデータコピー失敗

## 根本原因

### 1. リンクリスト実装の問題
**症状**: `new_node->data = value`でバスエラー、無限ループ

**原因**:
- ジェネリック構造体ポインタ`QueueNode<T>*`のメンバーアクセスが未サポート
- `malloc()`で確保したメモリに対する直接メンバーアクセスが機能しない
- 構造体ポインタの`->`演算子がジェネリック型で正しく動作しない

**コード例** (問題のあるコード):
```rust
QueueNode<T>* new_node = (QueueNode<T>*)node_mem;
new_node->next = nullptr;    // ❌ セグフォルト
new_node->data = value;      // ❌ バスエラー
```

### 2. 循環バッファ実装の問題
**症状**: `memcpy()`で値がコピーされず、常に0が返される

**原因**:
- **アドレス演算子`&`が正しく動作していない**
- `&value`がアドレスではなく値そのものを返す
- `&result`が0を返す
- ローカル変数のアドレス取得に問題がある

**検証結果**:
```bash
Original value: 42
Address of value: 42  # ❌ 本来はアドレス（例: 0x7fff...）が返るべき
```

**問題のあるコード**:
```rust
void* src = &value;      // ❌ アドレスが正しく取得できない
memcpy(dest, src, size); // ❌ 誤ったアドレスからコピー
```

## 解決策

### 循環バッファ実装の修正
**アプローチ**: Vectorと同じ`array_get`/`array_set`組み込み関数を使用

**修正前**:
```rust
void enqueue(T value) {
    int offset = self.rear * sizeof(T);
    void* dest = (void*)((long)self.data + offset);
    void* src = &value;  // ❌ アドレス取得が失敗
    memcpy(dest, src, sizeof(T));
}
```

**修正後**:
```rust
void enqueue(T value) {
    // array_setを使用（組み込み関数、正しく動作）
    array_set(self.data, self.rear, value);  // ✅
    
    self.rear = (self.rear + 1) % self.capacity;
    self.length = self.length + 1;
}
```

**dequeueの修正**:
```rust
T dequeue() {
    if (self.length <= 0) {
        return array_get(self.data, 0);
    }
    
    // array_getを使用
    T result = array_get(self.data, self.front);  // ✅
    
    self.front = (self.front + 1) % self.capacity;
    self.length = self.length - 1;
    
    return result;
}
```

**resizeの修正**:
```rust
void resize(int new_capacity) {
    int element_size = sizeof(T);
    void* new_data = malloc(new_capacity * element_size);
    
    // 循環バッファを線形配列に変換
    int i = 0;
    while (i < self.length) {
        int src_index = (self.front + i) % self.capacity;
        T value = array_get(self.data, src_index);  // ✅
        array_set(new_data, i, value);              // ✅
        i = i + 1;
    }
    
    free(self.data);
    self.data = new_data;
    self.capacity = new_capacity;
    self.front = 0;
    self.rear = self.length;
}
```

## テスト結果

### 修正前
```
❌ stdlib tests: 6/27 failed (Queue関連テスト)
❌ bus error / segmentation fault
❌ 無限ループ
```

### 修正後
```
✅ stdlib tests: 27/27 passed
✅ integration tests: 3,525/3,525 passed
✅ unit tests: 30/30 passed
✅ Queue<int>, Queue<long>, Queue<short>, Queue<string> 全て動作
```

## 実装の特徴

### 現在の実装（循環バッファ）
- ✅ 固定サイズ（初期化時に容量指定）
- ✅ リサイズ可能
- ✅ O(1) enqueue/dequeue
- ✅ ジェネリック型対応（int, string, struct, etc）
- ✅ メモリ効率的

### リンクリスト実装（将来対応）
- ⏳ 動的サイズ（容量制限なし）
- ⏳ ジェネリック構造体ポインタメンバーアクセスのサポート待ち
- ⏳ v0.11.1以降で対応予定

## 使用例

```rust
import "stdlib/collections/queue.cb";

// Queue<int>
Queue<int> int_queue;
int_queue.init(10);
int_queue.enqueue(42);
int val = int_queue.dequeue();

// Queue<string>
Queue<string> str_queue;
str_queue.init(5);
str_queue.enqueue("Hello");
string msg = str_queue.dequeue();

// リサイズ
if (int_queue.size() == int_queue.get_capacity()) {
    int_queue.resize(int_queue.get_capacity() * 2);
}
```

## 学んだこと

1. **アドレス演算子の制限**
   - ローカル変数の`&`が正しく動作しない
   - `memcpy()`で`&value`を使用する場合は注意が必要
   - 組み込み関数`array_get`/`array_set`を使用すべき

2. **ジェネリック構造体ポインタの制限**
   - `malloc()`後のジェネリック構造体メンバーアクセスが未サポート
   - `QueueNode<T>* node; node->data`が機能しない
   - リンクリストは現時点では実装困難

3. **Vectorの実装パターンが正解**
   - `array_get`/`array_set`を使用
   - ポインタ演算やアドレス取得を避ける
   - インタプリタの組み込み関数に依存する

## 今後の課題

1. **v0.11.1で対応予定**:
   - アドレス演算子`&`の修正
   - ジェネリック構造体ポインタメンバーアクセスのサポート
   - リンクリストベースのQueue<T>実装

2. **ドキュメント更新**:
   - Queue<T>が循環バッファ実装であることを明記
   - リサイズの必要性について説明
   - 使用例とベストプラクティスを追加

## まとめ

- ✅ **問題解決**: `array_get`/`array_set`を使用することで循環バッファ実装が完全動作
- ✅ **全テスト成功**: 3,582テスト全て成功
- ✅ **安定版実装**: Queue<T>は本番環境で使用可能
- ⏳ **リンクリスト実装**: v0.11.1以降で対応予定

---
**Status**: ✅ 完了 (2025年10月30日)
