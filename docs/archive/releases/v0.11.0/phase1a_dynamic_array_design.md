# Phase 1a: 動的配列（Vector）の実装

**作成日**: 2025年10月27日  
**対象**: v0.11.0 Phase 1aの基礎機能  
**優先度**: 🔴 最優先（すべてのコンテナの基礎）

---

## 📋 概要

動的配列（Vector）は、サイズが自動的に拡張される配列です。Cb言語では `T[]` というサイズなし配列構文で表現し、C++の `std::vector` と同等の機能を提供します。

```cb
// サイズなし配列 = 動的配列（Vector）
int[] arr;              // 空の動的配列
arr.push(10);           // 要素追加
arr.push(20);
arr.push(30);

int x = arr[0];         // インデックスアクセス
int len = arr.length(); // 長さ取得
arr.pop();              // 末尾削除

// 範囲for文
for (int val : arr) {
    print_int(val);
}
```

---

## 🎯 動的配列の構文

### 1. 宣言と初期化

```cb
// 空の動的配列
int[] arr;

// 初期値付き
int[] arr = {1, 2, 3, 4, 5};

// ジェネリック型
struct Point { int x; int y; }
Point[] points;

// ポインタ
int[]* ptr_arr;
```

### 2. 要素の追加・削除

```cb
int[] arr;

// 末尾に追加（O(1) amortized）
arr.push(10);
arr.push(20);

// 末尾から削除（O(1)）
int val = arr.pop();  // 20を返す

// 先頭に追加（O(n)）
arr.push_front(5);

// 先頭から削除（O(n)）
int val = arr.pop_front();

// 指定位置に挿入（O(n)）
arr.insert(2, 15);  // インデックス2に15を挿入

// 指定位置を削除（O(n)）
arr.remove(2);  // インデックス2を削除

// 全削除
arr.clear();
```

### 3. アクセスと検索

```cb
int[] arr = {10, 20, 30, 40, 50};

// インデックスアクセス
int x = arr[0];     // 10
arr[1] = 25;        // 変更

// 境界チェック付きアクセス
int x = arr.at(10); // エラー: out of bounds

// 先頭・末尾アクセス
int first = arr.front();  // 10
int last = arr.back();    // 50

// 検索
bool found = arr.contains(30);  // true
int idx = arr.index_of(30);     // 2 (見つからない場合は-1)
```

### 4. サイズとキャパシティ

```cb
int[] arr = {1, 2, 3};

// サイズ取得
int len = arr.length();    // 3
int sz = arr.size();       // 3（lengthのエイリアス）

// 空チェック
bool empty = arr.is_empty();  // false

// キャパシティ
int cap = arr.capacity();  // 内部バッファサイズ

// キャパシティを予約
arr.reserve(100);  // 100要素分を事前確保

// サイズ変更
arr.resize(10);    // サイズを10に変更（不足分は0で初期化）
```

### 5. イテレーション

```cb
int[] arr = {1, 2, 3, 4, 5};

// 範囲for文
for (int val : arr) {
    print_int(val);
}

// インデックス付きループ
for (int i = 0; i < arr.length(); i = i + 1) {
    print_int(arr[i]);
}

// forEach（関数ポインタ）
void print_value(int x) {
    print_int(x);
}
arr.for_each(print_value);
```

---

## 🔧 内部実装の設計

### 動的配列の内部構造

```cb
// コンパイラが自動生成する内部構造
struct __DynamicArray_int {
    int* data;        // 実データへのポインタ
    int length;       // 現在の要素数
    int capacity;     // 確保済みキャパシティ
}

// ユーザーコード
int[] arr;

// ↓コンパイラが変換↓

__DynamicArray_int arr;
arr.data = nullptr;
arr.length = 0;
arr.capacity = 0;
```

### メモリ管理

```cb
// push()の擬似実装
void __DynamicArray_int_push(__DynamicArray_int* self, int value) {
    // キャパシティチェック
    if (self->length >= self->capacity) {
        // 1.5倍または2倍に拡張
        int new_capacity = self->capacity == 0 ? 4 : self->capacity * 2;
        
        // 新しいメモリを確保
        int* new_data = new int[new_capacity];
        
        // 既存データをコピー
        for (int i = 0; i < self->length; i = i + 1) {
            new_data[i] = self->data[i];
        }
        
        // 古いメモリを解放
        if (self->data != nullptr) {
            delete[] self->data;
        }
        
        // 新しいバッファに切り替え
        self->data = new_data;
        self->capacity = new_capacity;
    }
    
    // 要素を追加
    self->data[self->length] = value;
    self->length = self->length + 1;
}
```

### デストラクタ（自動解放）

```cb
// 動的配列のデストラクタ（コンパイラ自動生成）
void __DynamicArray_int_destructor(__DynamicArray_int* self) {
    if (self->data != nullptr) {
        // 各要素のデストラクタを呼び出し（必要な場合）
        for (int i = 0; i < self->length; i = i + 1) {
            // 要素型がデストラクタを持つ場合のみ
            __call_destructor(&self->data[i]);
        }
        
        // メモリ解放
        delete[] self->data;
        self->data = nullptr;
        self->length = 0;
        self->capacity = 0;
    }
}

// スコープを抜けると自動的に呼び出される
void example() {
    int[] arr;
    arr.push(10);
    arr.push(20);
}  // ここで自動的にデストラクタが呼ばれる
```

---

## 📊 C++バックエンド実装

### 1. AST拡張

```cpp
// src/common/ast.h

// 動的配列型を表すフラグ
struct ParsedTypeInfo {
    std::string base_type;
    bool is_dynamic_array;  // 新規追加
    bool is_static_array;   // 既存の配列と区別
    int array_size;         // static配列のサイズ
    
    // ...
};

// 動的配列の操作を表すノード
enum ASTNodeType {
    // ...
    AST_ARRAY_PUSH,         // arr.push(x)
    AST_ARRAY_POP,          // arr.pop()
    AST_ARRAY_INDEX,        // arr[i]
    AST_ARRAY_LENGTH,       // arr.length()
    // ...
};
```

### 2. パーサー拡張

```cpp
// src/frontend/recursive_parser/parsers/type_parser.cpp

ParsedTypeInfo TypeParser::parse_type() {
    ParsedTypeInfo type;
    type.base_type = current_token().value;
    advance();
    
    // 配列のチェック
    if (current_token().type == TOK_LBRACKET) {
        advance();
        
        if (current_token().type == TOK_RBRACKET) {
            // サイズなし = 動的配列
            type.is_dynamic_array = true;
            type.is_static_array = false;
            advance();
        } else {
            // サイズあり = 静的配列
            type.is_static_array = true;
            type.is_dynamic_array = false;
            type.array_size = parse_constant_expression();
            expect(TOK_RBRACKET);
        }
    }
    
    return type;
}
```

### 3. メソッド呼び出しの解析

```cpp
// src/frontend/recursive_parser/parsers/primary_expression_parser.cpp

ASTNode* PrimaryExpressionParser::parse_method_call(ASTNode* object) {
    std::string method_name = current_token().value;
    advance();
    
    // 動的配列のメソッド
    if (object->type_info.is_dynamic_array) {
        if (method_name == "push") {
            return parse_array_push(object);
        } else if (method_name == "pop") {
            return parse_array_pop(object);
        } else if (method_name == "length" || method_name == "size") {
            return parse_array_length(object);
        }
        // ... その他のメソッド
    }
    
    // 通常の構造体メソッド
    return parse_struct_method_call(object, method_name);
}
```

### 4. インタプリタ実装

```cpp
// src/backend/interpreter/evaluator/core/evaluator.cpp

int64_t Evaluator::evaluate_array_push(const ASTNode* node) {
    // 配列オブジェクトを取得
    DynamicArrayValue* arr = get_dynamic_array(node->left);
    
    // 追加する値を評価
    int64_t value = evaluate_expression(node->right);
    
    // キャパシティチェックと拡張
    if (arr->length >= arr->capacity) {
        resize_dynamic_array(arr);
    }
    
    // 要素を追加
    arr->data[arr->length] = value;
    arr->length++;
    
    return 0;  // pushは値を返さない
}

void Evaluator::resize_dynamic_array(DynamicArrayValue* arr) {
    int new_capacity = arr->capacity == 0 ? 4 : arr->capacity * 2;
    
    // 新しいバッファを確保
    int64_t* new_data = new int64_t[new_capacity];
    
    // 既存データをコピー
    std::memcpy(new_data, arr->data, arr->length * sizeof(int64_t));
    
    // 古いバッファを解放
    delete[] arr->data;
    
    // 新しいバッファに切り替え
    arr->data = new_data;
    arr->capacity = new_capacity;
}
```

---

## 🧪 テスト計画

### 基本機能テスト（15個）

1. **test_dynamic_array_basic.cb** - 基本的な宣言と追加
   ```cb
   int[] arr;
   arr.push(10);
   arr.push(20);
   assert(arr.length() == 2);
   assert(arr[0] == 10);
   ```

2. **test_dynamic_array_pop.cb** - pop操作
   ```cb
   int[] arr = {1, 2, 3};
   int x = arr.pop();
   assert(x == 3);
   assert(arr.length() == 2);
   ```

3. **test_dynamic_array_resize.cb** - 自動リサイズ
   ```cb
   int[] arr;
   for (int i = 0; i < 100; i = i + 1) {
       arr.push(i);
   }
   assert(arr.length() == 100);
   ```

4. **test_dynamic_array_clear.cb** - 全削除
   ```cb
   int[] arr = {1, 2, 3};
   arr.clear();
   assert(arr.is_empty());
   ```

5. **test_dynamic_array_insert.cb** - 挿入操作
   ```cb
   int[] arr = {1, 3, 4};
   arr.insert(1, 2);
   assert(arr[1] == 2);
   ```

6. **test_dynamic_array_remove.cb** - 削除操作
   ```cb
   int[] arr = {1, 2, 3, 4};
   arr.remove(2);
   assert(arr[2] == 4);
   ```

7. **test_dynamic_array_contains.cb** - 検索
   ```cb
   int[] arr = {10, 20, 30};
   assert(arr.contains(20));
   assert(!arr.contains(40));
   ```

8. **test_dynamic_array_for_each.cb** - forEach
   ```cb
   int[] arr = {1, 2, 3};
   for (int x : arr) {
       print_int(x);
   }
   ```

9. **test_dynamic_array_struct.cb** - 構造体の動的配列
   ```cb
   struct Point { int x; int y; }
   Point[] points;
   Point p;
   p.x = 10;
   p.y = 20;
   points.push(p);
   ```

10. **test_dynamic_array_nested.cb** - ネストした動的配列
    ```cb
    int[][] matrix;
    int[] row = {1, 2, 3};
    matrix.push(row);
    ```

11. **test_dynamic_array_generic.cb** - ジェネリック型
    ```cb
    struct Box<T> { T value; }
    Box<int>[] boxes;
    ```

12. **test_dynamic_array_reserve.cb** - reserve操作
    ```cb
    int[] arr;
    arr.reserve(1000);
    assert(arr.capacity() >= 1000);
    ```

13. **test_dynamic_array_resize_value.cb** - resize操作
    ```cb
    int[] arr = {1, 2, 3};
    arr.resize(10);
    assert(arr.length() == 10);
    ```

14. **test_dynamic_array_front_back.cb** - front/back
    ```cb
    int[] arr = {10, 20, 30};
    assert(arr.front() == 10);
    assert(arr.back() == 30);
    ```

15. **test_dynamic_array_destructor.cb** - デストラクタ
    ```cb
    void test() {
        int[] arr;
        for (int i = 0; i < 1000; i = i + 1) {
            arr.push(i);
        }
    }  // メモリが自動解放される
    test();
    ```

---

## 📁 実装するメソッド一覧

### 基本操作
- `void push(T value)` - 末尾に追加
- `T pop()` - 末尾を削除して返す
- `void clear()` - 全削除

### アクセス
- `T operator[](int index)` - インデックスアクセス
- `T at(int index)` - 境界チェック付きアクセス
- `T front()` - 先頭要素
- `T back()` - 末尾要素

### サイズ
- `int length()` - 要素数
- `int size()` - 要素数（lengthのエイリアス）
- `bool is_empty()` - 空チェック
- `int capacity()` - キャパシティ

### 検索
- `bool contains(T value)` - 要素の存在チェック
- `int index_of(T value)` - インデックス検索

### 挿入・削除
- `void insert(int index, T value)` - 指定位置に挿入
- `void remove(int index)` - 指定位置を削除
- `void push_front(T value)` - 先頭に追加
- `T pop_front()` - 先頭を削除

### メモリ管理
- `void reserve(int capacity)` - キャパシティ予約
- `void resize(int new_size)` - サイズ変更

---

## 📅 実装スケジュール

### Week 1: 基本機能（5日）

**Day 1: パーサー拡張**
- [ ] 動的配列の型解析（`T[]`）
- [ ] メソッド呼び出しの解析

**Day 2: 基本メソッド実装**
- [ ] push, pop, length
- [ ] インデックスアクセス

**Day 3: メモリ管理**
- [ ] 自動リサイズ
- [ ] デストラクタ

**Day 4-5: テストとデバッグ**
- [ ] 15個のテスト作成
- [ ] メモリリークチェック

---

## 🎯 Phase 1bへの接続

動的配列が実装できれば、Queue<T>は以下のように実装できます：

```cb
// stdlib/queue.cb

struct Queue<T> {
    T[] items;      // 動的配列を使用
    int front_idx;  // 先頭インデックス
    
    void enqueue(T item) {
        items.push(item);
    }
    
    T dequeue() {
        if (front_idx >= items.length()) {
            // エラー: 空のキュー
            return T();  // デフォルト値
        }
        T item = items[front_idx];
        front_idx = front_idx + 1;
        
        // 定期的にメモリを解放
        if (front_idx > items.length() / 2) {
            compact();
        }
        
        return item;
    }
    
    void compact() {
        T[] new_items;
        for (int i = front_idx; i < items.length(); i = i + 1) {
            new_items.push(items[i]);
        }
        items = new_items;
        front_idx = 0;
    }
}
```

---

**作成者**: GitHub Copilot  
**レビュアー**: shadowlink0122  
**最終更新**: 2025年10月27日  
**次のフェーズ**: Phase 1b Queue<T>実装（動的配列完了後）
