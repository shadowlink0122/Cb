# Cbインタプリタのバグ: ジェネリックメソッドの戻り値をポインタメンバーに代入するとクラッシュ

## 日付
2025年11月4日

## バグの概要
ジェネリック構造体のメソッドが、同じジェネリック構造体のポインタを返し、それを構造体のポインタメンバーに代入すると、Cbインタプリタがセグメンテーションフォルトを起こします。

## 最小再現コード

```cb
struct MapNode<K, V> {
    K key;
    V value;
    MapNode<K, V>* left;   // ポインタメンバー
    MapNode<K, V>* right;  // ポインタメンバー
};

export interface MapOps<K, V> {
    MapNode<K, V>* create_node(K key, V value);
    MapNode<K, V>* insert_to_node(MapNode<K, V>* node, K key, V value);
}

export struct Map<K, V> {
    MapNode<K, V>* root;
};

impl MapOps<K, V> for Map<K, V> {
    private MapNode<K, V>* create_node(K key, V value) {
        int node_size = sizeof(MapNode<K, V>);
        void* mem = malloc(node_size);
        MapNode<K, V>* new_node = (MapNode<K, V>*)mem;
        
        new_node->key = key;
        new_node->value = value;
        new_node->left = nullptr;
        new_node->right = nullptr;
        
        return new_node;
    }
    
    private MapNode<K, V>* insert_to_node(MapNode<K, V>* node, K key, V value) {
        if (node == nullptr) {
            return self.create_node(key, value);
        }
        
        if (key < node->key) {
            // ここでクラッシュ！
            node->left = self.insert_to_node(node->left, key, value);
        } else {
            node->right = self.insert_to_node(node->right, key, value);
        }
        
        return node;
    }
    
    void insert(K key, V value) {
        // これもクラッシュの可能性
        self.root = self.insert_to_node(self.root, key, value);
    }
}

int main() {
    Map<int, int> map;
    map.insert(50, 5000);  // 最初の挿入は成功
    map.insert(30, 3000);  // 2回目でクラッシュ
    return 0;
}
```

## 実行結果

```
=== Test 1 insertion with struct ===
Map created, root =  0

1. Insert 50
zsh: segmentation fault  ./main test_struct_map_single.cb
```

または

```
zsh: bus error  ./main test_struct_map_single.cb
```

## 動作するパターン

以下のパターンでは**動作します**：

### 1. 戻り値を変数に受けない
```cb
void insert_to_node(MapNode<K, V>* node, K key, V value) {
    if (node == nullptr) {
        node = self.create_node(key, value);  // これは動作しない（ローカル変数）
        return;
    }
    // 戻り値なしで処理
}
```

### 2. イテレーティブアルゴリズムを使用
```cb
void insert_to_node_iterative(MapNode<K, V>* node, K key, V value) {
    MapNode<K, V>* current = node;
    MapNode<K, V>** parent_link = nullptr;
    
    while (current != nullptr) {
        if (key < current->key) {
            parent_link = &(current->left);
            current = current->left;
        } else {
            parent_link = &(current->right);
            current = current->right;
        }
    }
    
    if (parent_link != nullptr) {
        *parent_link = self.create_node(key, value);  // 直接ポインタ操作
    }
}
```

### 3. longで管理して最後にキャスト
```cb
struct MapNode<K, V> {
    K key;
    V value;
    long left;   // void*の代わりにlongで管理
    long right;
};

private MapNode<K, V>* insert_to_node(MapNode<K, V>* node, K key, V value) {
    if (node == nullptr) {
        return self.create_node(key, value);
    }
    
    if (key < node->key) {
        node->left = (long)self.insert_to_node((MapNode<K, V>*)node->left, key, value);
    }
    
    return node;
}
```

## 問題の詳細

### クラッシュが発生する条件
1. ジェネリック構造体 `T<K, V>` が定義されている
2. その構造体がポインタメンバー `T<K, V>* member` を持つ
3. ジェネリックメソッドが `T<K, V>*` を返す
4. そのメソッドが再帰的に自分自身を呼び出す
5. 戻り値をポインタメンバーに代入する: `node->member = self.method(...)`

### 推測される原因
- ジェネリック型のインスタンス化時のメタデータ管理の問題
- 再帰呼び出しのスタックフレーム管理の問題
- ポインタメンバーへの代入時の型解決の問題
- メモリアライメントの問題

## 影響範囲

この問題により、以下のデータ構造が実装できません：
- 二分探索木（Binary Search Tree）
- AVL木
- 赤黒木
- リンクリスト（ジェネリック版）
- グラフ構造（隣接リスト）
- その他の再帰的データ構造

## 推奨される回避策

現時点では、以下の回避策を使用してください：

1. **イテレーティブアルゴリズムを使用**（推奨）
   - 再帰の代わりにループを使用
   - ポインタのポインタ（`T**`）を使って親ノードを追跡
   
2. **longで管理**
   - ポインタを `long` 型で保存
   - 必要に応じてキャスト
   - 型安全性は失われるが動作する

3. **非ジェネリック版を実装**
   - 特定の型（`int`, `string`）に対してのみ実装
   - ジェネリクスを使わない

## 修正が必要な箇所（インタプリタ開発者向け）

以下のファイルを確認する必要があります：

1. `/src/backend/interpreter/evaluator/functions/call_impl.cpp`
   - ジェネリックメソッドの呼び出し処理
   - 戻り値の処理

2. `/src/backend/interpreter/executors/assignments/member_assignment.cpp`
   - 構造体メンバーへの代入処理
   - ポインタメンバーへの代入

3. `/src/backend/interpreter/evaluator/functions/generic_instantiation.cpp`
   - ジェネリック型のインスタンス化
   - 型パラメータの解決

4. `/src/backend/interpreter/managers/structs/operations.cpp`
   - 構造体操作
   - ポインタメンバーの管理

## テストケース

以下のテストケースを追加すべきです：

1. `tests/integration/generics/test_generic_recursive_struct.cb`
   - ジェネリック再帰構造のテスト
   
2. `tests/integration/generics/test_generic_pointer_member_assignment.cb`
   - ジェネリックポインタメンバー代入のテスト

## 関連する問題

- 以前のバグレポート: void*戻り値の問題（一部解決済み）
- メモリアライメントの問題（解決済み）

## 優先度

**高** - 多くの基本的なデータ構造が実装できないため、標準ライブラリの拡張に大きな影響があります。

## ステータス

**未解決** - 現在はイテレーティブアルゴリズムで回避中
