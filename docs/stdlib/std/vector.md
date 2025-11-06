# Vector&lt;T&gt; - ジェネリック双方向リンクリスト

## 概要

`Vector<T>`は任意の型`T`に対応した動的配列（双方向リンクリスト実装）です。要素の追加・削除・アクセス・ソートをサポートし、自動メモリ管理を提供します。

## インポート

```cb
import stdlib.std.vector;
```

## 型定義

```cb
export struct Vector<T> {
    void* front;      // 先頭ノードへのポインタ
    void* back;       // 末尾ノードへのポインタ
    long length;      // 要素数
};
```

## インターフェース

```cb
export interface VectorOps<T> {
    // 要素の追加
    void push_back(T value);
    void push_front(T value);
    
    // 要素の削除
    void pop_back();
    void pop_front();
    void delete_at(long index);
    
    // 要素のアクセス
    T at(long index);
    long find(T value);
    
    // ソート
    void sort(void* compare_fn);
    void smaller();
    void greater();
    
    // ユーティリティ
    long get_length();
    bool is_empty();
    void clear();
}
```

## メソッド

### 要素の追加

#### `void push_back(T value)`

末尾に要素を追加します。

- **時間計算量**: O(1)
- **例**:
```cb
Vector<int> vec;
vec.push_back(10);
vec.push_back(20);
vec.push_back(30);
// vec = [10, 20, 30]
```

#### `void push_front(T value)`

先頭に要素を追加します。

- **時間計算量**: O(1)
- **例**:
```cb
Vector<int> vec;
vec.push_front(10);
vec.push_front(20);
// vec = [20, 10]
```

### 要素の削除

#### `void pop_back()`

末尾の要素を削除します。

- **時間計算量**: O(1)
- **前提条件**: `!is_empty()`
- **例**:
```cb
Vector<int> vec;
vec.push_back(10);
vec.push_back(20);
vec.pop_back();
// vec = [10]
```

#### `void pop_front()`

先頭の要素を削除します。

- **時間計算量**: O(1)
- **前提条件**: `!is_empty()`
- **例**:
```cb
Vector<int> vec;
vec.push_back(10);
vec.push_back(20);
vec.pop_front();
// vec = [20]
```

#### `void delete_at(long index)`

指定インデックスの要素を削除します。

- **時間計算量**: O(n)
- **前提条件**: `0 <= index < get_length()`
- **例**:
```cb
Vector<int> vec;
vec.push_back(10);
vec.push_back(20);
vec.push_back(30);
vec.delete_at(1);
// vec = [10, 30]
```

### 要素のアクセス

#### `T at(long index)`

指定インデックスの要素を取得します。

- **時間計算量**: O(n)
- **前提条件**: `0 <= index < get_length()`
- **例**:
```cb
Vector<int> vec;
vec.push_back(10);
vec.push_back(20);
int value = vec.at(1);  // 20
```

#### `long find(T value)`

要素を線形探索し、最初に見つかったインデックスを返します。

- **時間計算量**: O(n)
- **戻り値**: 見つかった場合はインデックス、見つからない場合は-1
- **例**:
```cb
Vector<int> vec;
vec.push_back(10);
vec.push_back(20);
vec.push_back(30);

long idx = vec.find(20);  // 1
long not_found = vec.find(99);  // -1
```

### ソート

#### `void sort(void* compare_fn)`

カスタム比較関数でソートします。

- **時間計算量**: O(n log n)
- **アルゴリズム**: ボトムアップマージソート（安定ソート）
- **compare_fn**: 比較関数ポインタ `int (*)(T, T)`
  - `nullptr`: デフォルト昇順
  - 戻り値: 負（第1引数 < 第2引数）、0（等しい）、正（第1引数 > 第2引数）
- **例**:
```cb
// 昇順ソート（デフォルト）
Vector<int> vec;
vec.push_back(30);
vec.push_back(10);
vec.push_back(20);
vec.sort(nullptr);
// vec = [10, 20, 30]

// カスタム比較関数で降順
int greater(int a, int b) {
    return b - a;  // 降順
}

vec.sort(&greater);
// vec = [30, 20, 10]
```

#### `void smaller()`

昇順（小さい順）にソートします。

- **時間計算量**: O(n log n)
- **例**:
```cb
Vector<int> vec;
vec.push_back(30);
vec.push_back(10);
vec.push_back(20);
vec.smaller();
// vec = [10, 20, 30]
```

#### `void greater()`

降順（大きい順）にソートします。

- **時間計算量**: O(n log n)
- **例**:
```cb
Vector<int> vec;
vec.push_back(10);
vec.push_back(30);
vec.push_back(20);
vec.greater();
// vec = [30, 20, 10]
```

### ユーティリティ

#### `long get_length()`

要素数を返します。

- **時間計算量**: O(1)
- **例**:
```cb
Vector<int> vec;
vec.push_back(10);
vec.push_back(20);
long len = vec.get_length();  // 2
```

#### `bool is_empty()`

ベクターが空かどうかを返します。

- **時間計算量**: O(1)
- **例**:
```cb
Vector<int> vec;
if (vec.is_empty()) {
    println("Vector is empty");
}
```

#### `void clear()`

全ての要素を削除します。

- **時間計算量**: O(n)
- **例**:
```cb
Vector<int> vec;
vec.push_back(10);
vec.push_back(20);
vec.clear();
// vec = []
```

## コンストラクタとデストラクタ

### コンストラクタ

```cb
Vector<int> vec;  // 自動的に初期化（front=nullptr, back=nullptr, length=0）
```

### デストラクタ

スコープ終了時に自動的に全てのノードのメモリを解放します。

```cb
void example() {
    Vector<int> vec;
    vec.push_back(10);
    // スコープ終了時、自動的に全メモリ解放
}
```

## サポートされる型

### プリミティブ型

- `Vector<int>`
- `Vector<long>`
- `Vector<float>`
- `Vector<double>`
- `Vector<string>`
- `Vector<bool>`

### 構造体

```cb
struct Point {
    int x;
    int y;
};

Vector<Point> points;
Point p;
p.x = 10;
p.y = 20;
points.push_back(p);
```

### ネストしたジェネリクス

```cb
Vector<Vector<int>> matrix;
Vector<int> row;
row.push_back(1);
row.push_back(2);
matrix.push_back(row);
```

## パフォーマンス特性

| 操作 | 時間計算量 | 備考 |
|------|-----------|------|
| push_back | O(1) | 末尾追加 |
| push_front | O(1) | 先頭追加 |
| pop_back | O(1) | 末尾削除 |
| pop_front | O(1) | 先頭削除 |
| at | O(n) | インデックスアクセス |
| delete_at | O(n) | 特定位置の削除 |
| find | O(n) | 線形探索 |
| sort | O(n log n) | マージソート |
| smaller/greater | O(n log n) | マージソート |
| get_length | O(1) | 要素数取得 |
| is_empty | O(1) | 空判定 |
| clear | O(n) | 全削除 |

### 空間計算量

- **メモリ使用量**: O(n) - 各要素に追加で2つのポインタ（前後）
- **ノードサイズ**: `2 * sizeof(void*) + sizeof(T)`

## 使用例

### 基本的な使い方

```cb
import stdlib.std.vector;

void main() {
    Vector<int> vec;
    
    // 要素の追加
    vec.push_back(10);
    vec.push_back(20);
    vec.push_back(30);
    
    // 要素のアクセス
    println("Length: {vec.get_length()}");  // 3
    println("First: {vec.at(0)}");          // 10
    println("Last: {vec.at(2)}");           // 30
    
    // 要素の検索
    long idx = vec.find(20);
    if (idx != -1) {
        println("Found 20 at index {idx}");
    }
    
    // ソート
    vec.push_back(5);
    vec.smaller();  // 昇順: [5, 10, 20, 30]
    
    // 要素の削除
    vec.pop_back();
    println("After pop: {vec.get_length()}");  // 3
}
```

### 構造体とVector

```cb
import stdlib.std.vector;

struct Person {
    string name;
    int age;
};

void main() {
    Vector<Person> people;
    
    Person alice;
    alice.name = "Alice";
    alice.age = 30;
    people.push_back(alice);
    
    Person bob;
    bob.name = "Bob";
    bob.age = 25;
    people.push_back(bob);
    
    // アクセス
    Person first = people.at(0);
    println("{first.name} is {first.age} years old");
}
```

### カスタムソート

```cb
import stdlib.std.vector;

struct Person {
    string name;
    int age;
};

// 年齢で降順ソート
int compare_by_age_desc(Person a, Person b) {
    return b.age - a.age;
}

void main() {
    Vector<Person> people;
    // ... peopleに要素を追加 ...
    
    people.sort(&compare_by_age_desc);
}
```

### イテレーション

```cb
import stdlib.std.vector;

void print_all(Vector<int> vec) {
    long i = 0;
    while (i < vec.get_length()) {
        int value = vec.at(i);
        println("vec[{i}] = {value}");
        i = i + 1;
    }
}

void main() {
    Vector<int> vec;
    vec.push_back(10);
    vec.push_back(20);
    vec.push_back(30);
    
    print_all(vec);
}
```

## ベストプラクティス

### 1. 事前にサイズがわかっている場合

残念ながら現在のVectorは容量の事前確保をサポートしていませんが、将来のバージョンで`reserve()`メソッドを追加予定です。

### 2. 頻繁な挿入/削除

- 末尾/先頭の操作: O(1)で非常に効率的
- 中間の操作: O(n)なので、頻繁な場合は別のデータ構造を検討

### 3. 頻繁なアクセス

`at()`はO(n)なので、頻繁なランダムアクセスが必要な場合は配列の使用を検討してください。

### 4. メモリ管理

デストラクタが自動的にメモリを解放するため、手動でメモリ管理する必要はありません。

## 制限事項

1. **ランダムアクセス**: O(n)（配列はO(1)）
2. **キャパシティ管理**: 現在未サポート
3. **イテレータ**: 現在未サポート（for-eachループ未対応）

## トラブルシューティング

### セグメンテーションフォルト

```cb
Vector<int> vec;
int value = vec.at(0);  // ❌ 空のvectorへのアクセス
```

**解決策**: アクセス前に`is_empty()`または`get_length()`でチェック

```cb
if (!vec.is_empty()) {
    int value = vec.at(0);  // ✅ 安全
}
```

### ソート後の不正な動作

```cb
Vector<int> vec;
vec.push_back(30);
vec.push_back(10);
vec.push_back(20);
vec.smaller();
// vec内部のポインタ構造が更新されます
```

ソート後もvectorは正常に動作します。内部実装がポインタを適切に管理します。

## 関連項目

- [Queue](./queue.md) - FIFO キュー
- [Map](./map.md) - ハッシュマップ
- [stdlib README](./README.md) - 標準ライブラリ概要

## バージョン履歴

- **v0.11.0**: 初版リリース
  - O(n log n)マージソート実装
  - `smaller()`/`greater()`メソッド追加
  - カスタム比較関数サポート
  - 自動メモリ管理

## 今後の予定

- **v0.12.0**:
  - `reserve(capacity)` - 容量事前確保
  - `shrink_to_fit()` - 余分なメモリ解放
  - `insert_at(index, value)` - 任意位置への挿入
  - イテレータサポート
