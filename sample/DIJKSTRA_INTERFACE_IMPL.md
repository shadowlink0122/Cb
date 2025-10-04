# ダイクストラ法 - Interface/Impl実装

## 📋 概要

`sample/dijkstra_interface.cb` は、Interface/Implパターンを使って優先度キューを実装したダイクストラ法です。

## 🎯 Interface/Implパターンの活用

### Interface定義

```cb
interface PriorityQueue {
    int getSize();
    bool checkEmpty();
    int getCapacity();
}
```

このinterfaceは優先度キューの**読み取り専用操作**を定義しています。

### Implementation

```cb
impl PriorityQueue for PriorityQueueData {
    int getSize() {
        PriorityQueueData pq = self;
        return pq.queue_size;
    }
    
    bool checkEmpty() {
        PriorityQueueData pq = self;
        return pq.queue_size == 0;
    }
    
    int getCapacity() {
        PriorityQueueData pq = self;
        return pq.capacity;
    }
}
```

## 🏗️ データ構造

### PriorityQueueData構造体

```cb
struct PriorityQueueData {
    int[8] node_ids;      // ノードIDの配列
    int[8] priorities;    // 優先度の配列
    int queue_size;       // 現在のサイズ
    int capacity;         // 最大容量
}
```

**設計上の注意点**:
- 構造体の配列要素へのメンバーアクセス制限により、平坦な配列を使用
- `node_ids`と`priorities`を別々の配列で管理

## 🔧 API設計

### Interface経由の操作

| メソッド | 説明 | 戻り値 |
|---------|------|--------|
| `getSize()` | キューのサイズを取得 | int |
| `checkEmpty()` | キューが空かチェック | bool |
| `getCapacity()` | キューの容量を取得 | int |

### 直接操作（ラッパー関数）

| 関数 | 説明 | 引数 |
|------|------|------|
| `pq_push(int, int)` | 要素を追加 | node_id, priority |
| `pq_pop()` | 最小要素を取り出し | なし |
| `pq_isEmpty()` | キューが空かチェック（interface経由） | なし |
| `pq_size()` | サイズ取得（interface経由） | なし |

## 💡 実装の特徴

### 1. Interface/Implの分離

```cb
// Interface経由でアクセス
bool pq_isEmpty() {
    PriorityQueue pq_interface = pq_data;
    return pq_interface.checkEmpty();
}
```

- データ構造の実装詳細を隠蔽
- interfaceを通じた抽象化されたアクセス
- 将来的な実装の変更が容易

### 2. 挿入ソートによる優先度管理

```cb
void pq_push(int node_id, int priority) {
    // 末尾に追加
    int insert_pos = pq_data.queue_size;
    pq_data.node_ids[insert_pos] = node_id;
    pq_data.priorities[insert_pos] = priority;
    pq_data.queue_size = pq_data.queue_size + 1;
    
    // 挿入ソートで適切な位置に移動
    int i = insert_pos;
    while (i > 0) {
        int prev = i - 1;
        if (pq_data.priorities[i] < pq_data.priorities[prev]) {
            // スワップ処理
            ...
        }
    }
}
```

### 3. 最小要素の効率的な取り出し

```cb
int pq_pop() {
    if (pq_data.queue_size == 0) {
        return -1;
    }
    
    int min_node = pq_data.node_ids[0];
    
    // 要素を前に詰める
    for (int i = 0; i < new_size; i = i + 1) {
        pq_data.node_ids[i] = pq_data.node_ids[i + 1];
        pq_data.priorities[i] = pq_data.priorities[i + 1];
    }
    
    pq_data.queue_size = new_size;
    return min_node;
}
```

## 📊 実行結果

```
=== Dijkstra's Algorithm Results ===
Starting from node: 0

Shortest distances:
Node 0: distance = 0
Node 1: distance = 4
Node 2: distance = 13
Node 3: distance = 15
Node 4: distance = 12
Node 5: distance = 8
Node 6: distance = 7

Shortest paths:
Path from 0 to 1 (distance: 4):
  0 -> 1
Path from 0 to 2 (distance: 13):
  0 -> 1 -> 2
...
```

## 🎓 設計パターンの利点

### 1. 抽象化
- interfaceがデータ構造の実装詳細を隠蔽
- クライアントコードは実装に依存しない

### 2. 拡張性
- 異なる優先度キューの実装に容易に切り替え可能
- 例: ヒープベース、リストベースなど

### 3. テスト容易性
- interfaceをモック化してテスト可能
- 単体テストが書きやすい

### 4. 保守性
- 実装の変更がinterfaceの利用者に影響しない
- コードの変更範囲が限定的

## 🔍 Cb言語の制約と対応

### 制約1: 配列要素の構造体メンバーアクセス

❌ **使えない記法**:
```cb
struct Element {
    int id;
    int value;
}

Element[8] array;
array[0].id = 1;  // エラー!
```

✅ **対応策**:
```cb
struct Data {
    int[8] ids;
    int[8] values;
}

Data data;
data.ids[0] = 1;  // OK
```

### 制約2: Interfaceでの変更操作

現在のinterface実装では、`self`を通じた変更が制限されるため：
- **読み取り専用操作**をinterfaceに定義
- **変更操作**はラッパー関数で実装

## 📈 パフォーマンス

### 時間計算量

| 操作 | 計算量 | 説明 |
|------|--------|------|
| `push` | O(n) | 挿入ソート |
| `pop` | O(n) | 配列のシフト |
| `isEmpty` | O(1) | サイズチェック |
| `getSize` | O(1) | メンバー取得 |

### 全体のアルゴリズム
- **時間計算量**: O((V+E) log V)
- **空間計算量**: O(V²) (隣接行列) + O(E) (エッジリスト)

## 🎨 アーキテクチャ図

```
┌─────────────────────────────────────┐
│         Dijkstra Algorithm          │
└────────────┬────────────────────────┘
             │ uses
             ▼
┌─────────────────────────────────────┐
│   PriorityQueue Interface (読み取り) │
│   - getSize()                       │
│   - checkEmpty()                    │
│   - getCapacity()                   │
└────────────┬────────────────────────┘
             │ implements
             ▼
┌─────────────────────────────────────┐
│   PriorityQueueData (実装)          │
│   - int[8] node_ids                 │
│   - int[8] priorities               │
│   - int queue_size                  │
│   - int capacity                    │
└─────────────────────────────────────┘
             ▲
             │ wrapper functions
             │
┌─────────────────────────────────────┐
│   Helper Functions (変更操作)       │
│   - pq_push()                       │
│   - pq_pop()                        │
│   - pq_isEmpty()                    │
│   - pq_size()                       │
└─────────────────────────────────────┘
```

## 🔄 実装の比較

### dijkstra_struct.cb vs dijkstra_interface.cb

| 項目 | struct版 | interface版 |
|------|----------|-------------|
| 設計パターン | 手続き型 | Interface/Impl |
| 優先度キュー | 直接実装 | Interface経由 |
| 抽象化レベル | 低 | 高 |
| 拡張性 | 中 | 高 |
| コード量 | 339行 | 約400行 |
| 学習価値 | 基本的 | 高度 |

## 🎯 学習ポイント

1. **Interface設計**: 抽象的なAPIの定義方法
2. **Impl実装**: Interfaceの具体的な実装
3. **デザインパターン**: 実装と抽象化の分離
4. **制約への対応**: 言語機能の制限を回避する設計
5. **ラッパーパターン**: 複雑な操作の簡潔なAPI提供

## 📝 使用例

```bash
# プログラムの実行
./main sample/dijkstra_interface.cb

# 出力:
# - Interface/Implパターンの説明
# - グラフ構造の表示
# - アルゴリズムの実行過程
# - 最短距離の結果
# - 最短経路の表示
# - 実装詳細の説明
```

## 🎉 まとめ

このinterface/impl実装は：

✅ **教育的価値**: デザインパターンの実践的な学習
✅ **拡張性**: 異なる実装への切り替えが容易
✅ **保守性**: 実装詳細の隠蔽による変更容易性
✅ **実用性**: 実際のアルゴリズムでの活用例

Cb言語のinterface/impl機能を使った、実践的なデータ構造とアルゴリズムの実装例です！
