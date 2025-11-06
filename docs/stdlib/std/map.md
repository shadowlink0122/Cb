# Map&lt;K, V&gt; - ジェネリックハッシュマップ

## 概要

`Map<K, V>`はキー・値のペアを格納するハッシュマップです。高速な検索・挿入・削除をサポートします。

## インポート

```cb
import stdlib.std.map;
```

## 基本的な使い方

```cb
Map<string, int> ages;
ages.insert("Alice", 30);
ages.insert("Bob", 25);

bool has_alice = ages.contains("Alice");  // true
int alice_age = ages.get("Alice", 0);     // 30
bool removed = ages.try_remove("Bob");    // true
int map_size = ages.size();               // 1
```

## メソッド

| メソッド | 説明 | 時間計算量 |
|---------|------|-----------|
| `insert(K key, V value)` | キー・値ペアを挿入 | O(1) 平均 |
| `get(K key, V default)` | キーに対応する値を取得 | O(1) 平均 |
| `contains(K key)` | キーが存在するか | O(1) 平均 |
| `try_remove(K key)` | キーを削除 | O(1) 平均 |
| `size()` | 要素数を返す | O(1) |
| `is_empty()` | 空かどうか | O(1) |
| `clear()` | 全要素を削除 | O(n) |

## サポートされるキー型

- `int`, `long`, `string`
- カスタムハッシュ関数が必要: 構造体

## 使用例

### 基本例

```cb
import stdlib.std.map;

void main() {
    Map<string, int> word_count;
    word_count.insert("apple", 5);
    word_count.insert("banana", 3);
    word_count.insert("cherry", 7);
    
    if (word_count.contains("apple")) {
        int count = word_count.get("apple", 0);
        println("apple count: {count}");
    }
}
```

### カスタム構造体

```cb
import stdlib.std.map;

struct User {
    string name;
    int age;
};

void main() {
    Map<int, User> users;
    
    User alice;
    alice.name = "Alice";
    alice.age = 30;
    users.insert(1, alice);
    
    User bob;
    bob.name = "Bob";
    bob.age = 25;
    users.insert(2, bob);
    
    if (users.contains(1)) {
        User user = users.get(1, alice);  // Default value if not found
        println("User 1: {user.name}, {user.age}");
    }
}
```

## パフォーマンス

- **時間計算量**: 平均O(1)、最悪O(n)
- **空間計算量**: O(n)
- **ハッシュ関数**: 単純な剰余ハッシュ（将来改善予定）

## 制限事項

1. **ハッシュ衝突**: チェイニングで解決
2. **リハッシュ**: 現在未サポート（固定サイズバケット）

## 関連項目

- [Vector](./vector.md)
- [Queue](./queue.md)
