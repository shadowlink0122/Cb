# Vector<T> 双方向リンクリスト実装

## 実装日
2025年11月3日

## 概要
Vector<T>を配列ベースから双方向リンクリストに再設計し、O(1)でのfront/back操作を実現しました。

## 背景

### 問題点
- 配列ベースのVectorは`push_front()`がO(n)（全要素をシフト）
- `pop_front()`もO(n)
- 動的リサイズ時のメモリコピーのオーバーヘッド

### 解決策
双方向リンクリストを採用し、以下を実現:
- O(1): `push_front()`, `push_back()`, `pop_front()`, `pop_back()`
- O(n): `at(index)`, `find(value)`, `delete_at(index)`
- 動的なメモリ管理（ノード単位）

## データ構造

### 旧実装（配列ベース）
```cb
export struct Vector<T> {
    int capacity;     // 確保容量
    int length;       // 要素数
    void* data;       // T[]配列
};
```

### 新実装（双方向リンクリスト）
```cb
export struct Vector<T> {
    void* front;      // 先頭ノード
    void* back;       // 末尾ノード
    long length;      // 要素数
};
```

### ノードのメモリレイアウト
```
[prev (sizeof(void*))][next (sizeof(void*))][data (sizeof(T))]
```

- **prev**: 前のノードへのポインタ（offset 0）
- **next**: 次のノードへのポインタ（offset sizeof(void*)）
- **data**: 実際のデータ（offset sizeof(void*) * 2）

## API変更

### 削除されたメソッド
- `init()`: コンストラクタに統合
- `reserve()`: 配列ベース特有の操作
- `get_capacity()`: capacity概念の削除

### 追加されたメソッド
- `push_front(T value)`: 先頭に要素を追加（O(1)）
- `pop_front()`: 先頭要素を削除（O(1)）
- `delete_at(long index)`: インデックス指定削除（O(n)）
- `find(T value)`: 線形探索（O(n)）
- `sort()`: バブルソート（O(n²)）
- `sort_with(void* compare_fn)`: カスタム比較関数でソート

### 変更されたメソッド
- `push()` → `push_back()`
- `pop()` → `pop_back()`
- `get()` → `at()`

## 実装詳細

### push_back（末尾追加）
```cb
void push_back(T value) {
    int ptr_size = sizeof(void*);
    int data_size = sizeof(T);
    int node_size = ptr_size + ptr_size + data_size;
    void* new_node = malloc(node_size);
    
    // prevポインタを設定 (offset 0)
    void** prev_ptr = new_node;
    *prev_ptr = self.back;
    
    // nextポインタを設定 (offset ptr_size)
    void** next_ptr = new_node + ptr_size;
    *next_ptr = nullptr;
    
    // データを設定 (offset ptr_size * 2)
    void* data_ptr = new_node + ptr_size + ptr_size;
    array_set(data_ptr, 0, value);
    
    if (self.back != nullptr) {
        // 既存の末尾ノードのnextを更新
        void** old_back_next = self.back + ptr_size;
        *old_back_next = new_node;
    }
    
    self.back = new_node;
    
    if (self.front == nullptr) {
        self.front = new_node;
    }
    
    self.length = self.length + 1;
}
```

### at（インデックスアクセス）
```cb
T at(long index) {
    if (index < 0 || index >= self.length || self.front == nullptr) {
        T default_value;
        return default_value;
    }
    
    int ptr_size = sizeof(void*);
    void* current = self.front;
    long i = 0;
    while (i < index) {
        void** next_ptr = current + ptr_size;
        current = *next_ptr;
        i = i + 1;
    }
    
    void* data_ptr = current + ptr_size + ptr_size;
    return array_get(data_ptr, 0);
}
```

### sort（バブルソート）
```cb
void sort() {
    if (self.length <= 1) {
        return;
    }
    
    int ptr_size = sizeof(void*);
    bool swapped = true;
    while (swapped) {
        swapped = false;
        void* current = self.front;
        
        while (current != nullptr) {
            void** next_ptr = current + ptr_size;
            void* next_node = *next_ptr;
            
            if (next_node == nullptr) {
                break;
            }
            
            void* current_data_ptr = current + ptr_size + ptr_size;
            void* next_data_ptr = next_node + ptr_size + ptr_size;
            T current_data = array_get(current_data_ptr, 0);
            T next_data = array_get(next_data_ptr, 0);
            
            if (current_data > next_data) {
                array_set(current_data_ptr, 0, next_data);
                array_set(next_data_ptr, 0, current_data);
                swapped = true;
            }
            
            current = next_node;
        }
    }
}
```

### デストラクタ
```cb
~self() {
    int ptr_size = sizeof(void*);
    void* current = self.front;
    while (current != nullptr) {
        void** next_ptr = current + ptr_size;
        void* next_node = *next_ptr;
        free(current);
        current = next_node;
    }
}
```

## ポータビリティ

### sizeof(void*)の使用
```cb
int ptr_size = sizeof(void*);
```

- 32ビット: `sizeof(void*) = 4`
- 64ビット: `sizeof(void*) = 8`

すべてのオフセット計算に`sizeof(void*)`を使用することで、プラットフォーム非依存を実現。

## パフォーマンス特性

| 操作 | 旧実装（配列） | 新実装（リンクリスト） |
|-----|--------------|-------------------|
| push_back | O(1)* | O(1) |
| push_front | O(n) | O(1) |
| pop_back | O(1) | O(1) |
| pop_front | O(n) | O(1) |
| at(index) | O(1) | O(n) |
| find(value) | O(n) | O(n) |
| delete_at | O(n) | O(n) |
| sort | - | O(n²) |

*リサイズ時はO(n)

## テスト結果

### 新規テスト
`test_new_vector.cb` - 7テストケース
- ✅ push_back/push_front
- ✅ pop_back/pop_front
- ✅ find
- ✅ delete_at
- ✅ sort
- ✅ clear

### 既存テスト更新
- `test_vector_import.cb`: API変更対応
- `test_selective_import.cb`: API変更
- `test_advanced_selective.cb`: API変更
- `test_generic_containers.cb`: capacity削除、API変更
- `test_vector.hpp`: 期待値更新

### テスト結果
- ✅ 新規テスト: 7/7成功
- ✅ stdlib-test: 27/27成功
- ✅ すべてのVector操作が正常動作

## 使用例

```cb
import stdlib.std.vector;

void main() {
    Vector<int> vec;
    
    // O(1)操作
    vec.push_back(10);   // [10]
    vec.push_front(5);   // [5, 10]
    vec.push_back(20);   // [5, 10, 20]
    
    println("Length: ", vec.get_length());  // 3
    
    // O(n)操作
    int value = vec.at(1);  // 10
    println("At index 1: ", value);
    
    long index = vec.find(20);  // 2
    println("Find 20: ", index);
    
    // ソート
    vec.push_back(3);
    vec.sort();  // [3, 5, 10, 20]
    
    // 削除
    vec.pop_front();     // [5, 10, 20]
    vec.delete_at(1);    // [5, 20]
    
    // スコープ終了時に自動的にメモリ解放
}
```

## 今後の改善

### 可能な最適化
1. **インデックスキャッシュ**: 最後にアクセスしたノードを記憶
2. **双方向探索**: 後ろからもアクセス可能
3. **効率的なソート**: クイックソートやマージソート
4. **イテレータ**: ノードを直接走査

### 追加機能候補
1. **reverse()**: リストを反転
2. **insert(index, value)**: 任意位置に挿入
3. **splice()**: 部分リストの切り出し
4. **merge()**: ソート済みリストのマージ

## 関連コミット
- `0b9d88b`: Refactor Vector to doubly-linked list with O(1) operations

## 参考資料
- `stdlib/collections/vector.cb`: 実装ファイル
- `test_new_vector.cb`: 検証テスト
- `docs/features/vector_queue_generic_complete.md`: ジェネリックコレクションの概要
