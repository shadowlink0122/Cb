# ポインタシステム 将来実装予定機能

**最終更新**: 2025年10月5日  
**バージョン**: v0.9.0

---

## 📋 v0.9.0 実装状況サマリー

### ✅ 完全実装済み

- **ポインタ宣言と初期化**: `int* ptr = &value;`
- **デリファレンス**: `*ptr` による値の取得・変更
- **アドレス演算子**: `&variable` でアドレス取得
- **ポインタ演算**: `ptr++`, `ptr--`, `ptr + n`, `ptr - n`
- **16進数アドレス表示**: `0x[hex]` 形式
- **構造体ポインタ**: `(*ptr).member` および `ptr->member`
- **Interfaceポインタ**: `(*shape_ptr).area()` でポリモーフィック呼び出し
- **ポインタ配列**: `int* ptrs[10];`
- **ネストしたポインタアクセス**: `(*(*p).nested).value`
- **アドレス演算**: `&ptr` でポインタ変数自身のアドレス取得

### 🚧 未実装（将来予定）

以下の機能はv0.9.0時点では未実装であり、将来のバージョンで実装予定です。

---

## 🔮 将来実装予定機能

### 1. impl内でのstatic変数

implブロック内でのstatic変数宣言をサポート予定。

#### 基本構文
```c++
struct Counter {
    int value;
};

impl Helper for Counter {
    static int shared_counter = 0;  // すべてのCounterインスタンスで共有
    
    void increment() {
        self.value++;
        shared_counter++;
    }
    
    int get_total() {
        return shared_counter;
    }
};

int main() {
    Counter c1 = {value: 0};
    Counter c2 = {value: 0};
    
    c1.increment();  // shared_counter = 1
    c2.increment();  // shared_counter = 2
    
    println("Total:", c1.get_total());  // 2
    return 0;
}
```

#### 特徴
- **impl単位での共有**: 同じ`impl Interface for Struct`内のメソッドで共有
- **型ごとに独立**: `impl I for A`と`impl I for B`は異なるstatic変数を持つ
- **永続性**: プログラム実行中ずっと保持される
- **初期化**: 最初のアクセス時に一度だけ初期化

#### 名前空間設計
```
impl::InterfaceName::StructTypeName::variable_name
例: impl::Helper::Counter::shared_counter
```

---

### 2. 動的メモリ管理（new/delete）

ヒープメモリの動的確保・解放機能。

#### 基本構文
```c++
// 動的メモリ確保
Point* p = new Point;
p->x = 10;
p->y = 20;

// メモリ解放
delete p;

// 配列の動的確保
int* arr = new int[10];
arr[0] = 100;
delete[] arr;
```

#### 特徴
- **RAIIベース**: スマートポインタと組み合わせた自動管理
- **null安全**: `delete nullptr;` は安全に動作
- **自動null化**: 解放後は自動的にnullptrを設定

#### 使用例
```c++
struct Node {
    int value;
    Node* next;
};

Node* create_list(int[5] values) {
    Node* head = new Node;
    head->value = values[0];
    head->next = nullptr;
    
    Node* current = head;
    for (int i = 1; i < 5; i++) {
        current->next = new Node;
        current = current->next;
        current->value = values[i];
        current->next = nullptr;
    }
    
    return head;
}

void free_list(Node* head) {
    while (head != nullptr) {
        Node* temp = head;
        head = head->next;
        delete temp;
    }
}
```

---

### 3. スマートポインタ

自動メモリ管理のためのスマートポインタ。

#### unique_ptr
```c++
// 所有権を持つポインタ（排他的所有）
unique_ptr<Point> p = make_unique<Point>();
p->x = 10;
p->y = 20;

// 所有権の移動
unique_ptr<Point> p2 = move(p);
// p はnullptrになる
```

#### shared_ptr
```c++
// 参照カウント式の共有ポインタ
shared_ptr<Point> p1 = make_shared<Point>();
shared_ptr<Point> p2 = p1;  // 参照カウント +1

// すべての参照がスコープを抜けたら自動削除
```

#### weak_ptr
```c++
// 循環参照を防ぐ弱参照
shared_ptr<Node> parent = make_shared<Node>();
weak_ptr<Node> child_ref = parent;  // 参照カウントは増えない
```

---

### 4. void* 汎用ポインタ

型安全性を犠牲にした汎用ポインタ。

```c++
void* generic_ptr = &some_value;

// キャストして使用
int* int_ptr = (int*)generic_ptr;
```

**注意**: 型安全性の観点から慎重な導入を検討中。

---

### 5. 関数ポインタ

関数へのポインタとコールバック機能。

```c++
// 関数ポインタ型の定義
typedef int (*BinaryOp)(int, int);

int add(int a, int b) {
    return a + b;
}

int multiply(int a, int b) {
    return a * b;
}

int apply(BinaryOp op, int x, int y) {
    return op(x, y);
}

int main() {
    println("Add:", apply(add, 5, 3));         // 8
    println("Multiply:", apply(multiply, 5, 3)); // 15
    return 0;
}
```

---

### 6. ポインタの高度な演算

#### アドレス差分計算
```c++
int[10] arr;
int* p1 = &arr[0];
int* p2 = &arr[5];

int distance = p2 - p1;  // 5 (要素数での差分)
```

#### ポインタ比較
```c++
if (ptr1 < ptr2) {
    println("ptr1 is before ptr2");
}
```

---

### 7. const ポインタ

ポインタ自体やポイント先の不変性を保証。

```c++
// ポイント先が定数
const int* ptr1 = &value;
// *ptr1 = 10;  // エラー: 値を変更できない
ptr1 = &other;  // OK: ポインタ自体は変更可能

// ポインタ自体が定数
int* const ptr2 = &value;
*ptr2 = 10;     // OK: 値は変更可能
// ptr2 = &other;  // エラー: ポインタを変更できない

// 両方が定数
const int* const ptr3 = &value;
// *ptr3 = 10;     // エラー
// ptr3 = &other;  // エラー
```

---

### 8. 多重ポインタ（ポインタのポインタ）

```c++
int value = 42;
int* ptr = &value;
int** pptr = &ptr;

println("Value:", **pptr);  // 42

**pptr = 100;
println("Modified:", value);  // 100
```

---

### 9. nullptr リテラル

明示的なnullポインタリテラル。

```c++
int* ptr = nullptr;

if (ptr == nullptr) {
    println("Pointer is null");
}
```

**現状**: v0.9.0では数値0を使用してnullを表現。

---

## 📅 実装ロードマップ

### v0.10.0（予定）
- **impl内でのstatic変数** - Interface/Implシステムの完成度向上
- 多次元配列へのポインタ - ポインタシステムの完全性向上
- 動的メモリ管理（`new`/`delete`）の基本実装

### v0.11.0（予定）
- スマートポインタ（`unique_ptr`, `shared_ptr`）
- 構造体配列メンバーの関数戻り値代入
- `nullptr` リテラル

### v1.0.0に向けて
- 関数ポインタ
- const ポインタ
- 多重ポインタの完全サポート
- void* 汎用ポインタ（慎重に検討）
- 配列要素へのポインタ構文糖衣 `ptr[n]`

---

## 🔗 関連ドキュメント

- [完全仕様書](spec.md) - 現在実装済みの全機能
- [BNF文法定義](BNF.md) - 言語の文法構造
- [アーカイブ](archive/) - 過去の実装計画

---

**Cb言語 v0.9.0 - ポインタシステム完全実装版**  
将来実装予定機能リスト
