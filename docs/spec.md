# Cb言語 完全仕様書 v0.11.0

**最終更新**: 2025年10月28日  
**バージョン**: v0.11.0 - Generics, String Interpolation & Destructors

## 目次

1. [言語概要](#言語概要)
2. [型システム](#型システム)
3. [変数と宣言](#変数と宣言)
4. [演算子](#演算子)
5. [制御構造](#制御構造)
6. [関数](#関数)
7. [ジェネリクス](#ジェネリクス)
8. [配列](#配列)
9. [構造体](#構造体)
10. [Union型](#union型)
11. [Interface/Implシステム](#interfaceimplシステム)
12. [デストラクタとRAII](#デストラクタとraii)
13. [文字列補間](#文字列補間)
14. [ポインタと参照](#ポインタと参照)
15. [モジュールシステム](#モジュールシステム)
16. [入出力](#入出力)
17. [エラーハンドリング](#エラーハンドリング)
18. [メモリ管理](#メモリ管理)

---

## 言語概要

### 設計思想

Cb（シーフラット）は、C++の表現力とTypeScriptの型安全性を融合した、モダンな静的型付けプログラミング言語です。

**主要な設計原則**:
- **ゼロコスト抽象化**: ランタイムオーバーヘッドを最小化
- **型安全性**: コンパイル時の厳密な型チェック
- **明示的なメモリ管理**: ガベージコレクションなし、RAIIベース
- **実用性重視**: 学習コストを抑えつつ、実用的な機能を提供

### 影響を受けた言語

| 言語 | 採用した要素 |
|------|------------|
| C/C++ | 基本構文、制御構造、ポインタシステム |
| TypeScript | Union型、Interface、typedef |
| Rust | Interface/Implパターン、所有権の概念 |
| Go | シンプルな配列構文、モジュールシステム |

---

## 型システム

### 基本型

#### 整数型

| 型 | サイズ | 範囲 | 説明 |
|---|-------|------|------|
| `tiny` | 8bit | -128 ~ 127 | 最小整数型 |
| `short` | 16bit | -32,768 ~ 32,767 | 短整数 |
| `int` | 32bit | -2,147,483,648 ~ 2,147,483,647 | 標準整数 |
| `long` | 64bit | -9,223,372,036,854,775,808 ~ 9,223,372,036,854,775,807 | 長整数 |

#### 符号なし整数型

すべての整数型に`unsigned`修飾子を適用可能:

```c++
unsigned tiny ut;    // 0 ~ 255
unsigned short us;   // 0 ~ 65,535
unsigned int ui;     // 0 ~ 4,294,967,295
unsigned long ul;    // 0 ~ 18,446,744,073,709,551,615
```

**特徴**:
- 実行時に負値が代入されると自動的に0にクランプ
- 警告メッセージを出力

#### 文字型

```c++
char c = 'A';        // ASCII文字（0-255）
char newline = '\n'; // エスケープシーケンス対応
```

**サポートするエスケープシーケンス**:
- `\n` - 改行
- `\t` - タブ
- `\\` - バックスラッシュ
- `\'` - シングルクォート
- `\"` - ダブルクォート

#### 文字列型

```c++
string s = "Hello, Cb!";
string japanese = "こんにちは";  // UTF-8対応
```

**文字列の内部表現**:
- すべての文字列はnull終端文字(`\0`)で終了
- メモリ上では「文字列の内容 + `\0`」として格納
- 文字列リテラルは自動的にnull終端文字が付加される

**文字列の長さ**:
```c++
// 文字列の長さを取得する例
int strlen(string str) {
    int len = 0;
    // \0が見つかるまでループ
    while (str[len] != '\0') {
        len = len + 1;
    }
    return len;
}

void main() {
    string s = "Hello";
    int len = strlen(s);
    println(len);  // 5
}
```

**注意事項**:
- 文字列操作時は常にnull終端文字を意識する必要がある
- 配列として文字列を扱う場合、最後の要素は`\0`である
- 文字列のコピー時はnull終端文字も含めてコピーする必要がある

#### ブール型

```c++
bool flag = true;
bool done = false;
```

#### 浮動小数点数型 ✅

| 型 | サイズ | 精度 | 説明 |
|---|-------|------|------|
| `float` | 32bit | 約7桁 | 単精度浮動小数点数 |
| `double` | 64bit | 約15桁 | 倍精度浮動小数点数 |

```c++
float f = 3.14f;           // float型リテラル
double d = 2.71828;        // double型リテラル
float e = 1.23e-4f;        // 指数表記
double pi = 3.141592653589793;

// 配列
float[5] farr = [1.1, 2.2, 3.3, 4.4, 5.5];
double[3] darr = [1.0, 2.0, 3.0];

// 演算
float result = f * 2.0f;
double sum = d + pi;
```

**特徴**:
- IEEE 754準拠の浮動小数点演算
- 四則演算、比較演算対応
- 複合代入演算子対応（`+=`, `-=`, `*=`, `/=`）
- 構造体メンバーとして使用可能
- 配列要素として使用可能

### 型修飾子

#### const修飾子

```c++
const int MAX_SIZE = 100;
const string MESSAGE = "Hello";

// 配列サイズに使用可能
int[MAX_SIZE] buffer;
```

#### static修飾子

```c++
void counter() {
    static int count = 0;  // 関数呼び出し間で値を保持
    count++;
    println("Count:", count);
}
```

### typedef（型エイリアス）

```c++
// 基本型のエイリアス
typedef int Integer;
typedef string Text;

// 配列型のエイリアス
typedef int[5] IntArray5;
typedef int[3][3] Matrix3x3;

// 再帰的typedef
typedef int ID;
typedef ID UserID;
typedef UserID AdminID;  // 各レベルで独立した型
```

---

## 変数と宣言

### 基本的な宣言

```c++
int x;              // 宣言のみ
int y = 10;         // 初期化付き宣言
```

### 複数変数の同時宣言

```c++
int a, b, c;                    // 同じ型の複数変数
int x = 1, y = 2, z = 3;        // 初期化付き
string name, title, message;    // 文字列の複数宣言
```

### 配列の複数宣言

```c++
int[5] arr1, arr2;                      // 同じサイズの配列
string[3] names = ["Alice", "Bob"];     // 初期化付き
```

### 配列操作組み込み関数

#### array_get() - 配列要素の安全な取得

実行時に配列境界チェックを行い、範囲外アクセスを防ぎます。

**構文**:
```cb
T array_get<T>(T[] array, int index, int array_size)
```

**使用例**:
```cb
int[5] arr = [10, 20, 30, 40, 50];

// 安全な配列アクセス
int value = array_get(arr, 2, 5);  // 30
println(value);

// 範囲外アクセス（実行時エラー）
// int bad = array_get(arr, 10, 5);  // Error: Array index out of bounds
```

**特徴**:
- コンパイル時ではなく実行時にチェック
- 範囲外アクセスでプログラムを停止
- デバッグモードで詳細なエラーメッセージを表示

#### array_set() - 配列要素の安全な設定

実行時に配列境界チェックを行い、安全に値を設定します。

**構文**:
```cb
void array_set<T>(T[] array, int index, T value, int array_size)
```

**使用例**:
```cb
int[5] arr = [1, 2, 3, 4, 5];

// 安全な配列への代入
array_set(arr, 2, 100, 5);  // arr[2] = 100
println(arr[2]);  // 100

// 範囲外への代入（実行時エラー）
// array_set(arr, 10, 999, 5);  // Error: Array index out of bounds
```

**用途**:
- 動的に計算されるインデックスでの安全なアクセス
- デバッグモードでの境界チェック
- 複雑な配列操作での安全性確保

### スコープ

```c++
int global_var = 100;  // グローバルスコープ

int main() {
    int local_var = 10;  // ローカルスコープ
    
    if (true) {
        int block_var = 5;  // ブロックスコープ
        println(local_var);  // アクセス可能
    }
    // println(block_var);  // エラー: スコープ外
    
    return 0;
}
```

---

## 演算子

### 優先順位表

| 優先度 | 演算子 | 説明 | 結合性 |
|-------|--------|------|--------|
| 1 | `()` `[]` `.` `->` | 関数呼び出し、配列アクセス、メンバアクセス | 左→右 |
| 2 | `++` `--` (前置) `&` `*` | 前置演算子、アドレス、デリファレンス | 右→左 |
| 3 | `*` `/` `%` | 乗算、除算、剰余 | 左→右 |
| 4 | `+` `-` | 加算、減算 | 左→右 |
| 5 | `<<` `>>` | ビットシフト | 左→右 |
| 6 | `<` `<=` `>` `>=` | 比較演算子 | 左→右 |
| 7 | `==` `!=` | 等価演算子 | 左→右 |
| 8 | `&` | ビットAND | 左→右 |
| 9 | `^` | ビットXOR | 左→右 |
| 10 | `|` | ビットOR | 左→右 |
| 11 | `&&` | 論理AND | 左→右 |
| 12 | `||` | 論理OR | 左→右 |
| 13 | `?:` | 三項演算子 | 右→左 |
| 14 | `=` `+=` `-=` `*=` `/=` `%=` `&=` `|=` `^=` `<<=` `>>=` | 代入演算子 | 右→左 |

### 算術演算子

```c++
int a = 10, b = 3;

int sum = a + b;      // 13
int diff = a - b;     // 7
int prod = a * b;     // 30
int quot = a / b;     // 3
int rem = a % b;      // 1
```

### 比較演算子

```c++
int x = 5, y = 10;

bool eq = (x == y);   // false
bool ne = (x != y);   // true
bool lt = (x < y);    // true
bool le = (x <= y);   // true
bool gt = (x > y);    // false
bool ge = (x >= y);   // false
```

### 論理演算子

```c++
bool a = true, b = false;

bool and_result = a && b;  // false
bool or_result = a || b;   // true
bool not_result = !a;      // false
```

### ビット演算子

```c++
int a = 0b1100;  // 12
int b = 0b1010;  // 10

int and_bit = a & b;  // 0b1000 = 8
int or_bit = a | b;   // 0b1110 = 14
int xor_bit = a ^ b;  // 0b0110 = 6
int not_bit = ~a;     // ビット反転
int lshift = a << 2;  // 0b110000 = 48
int rshift = a >> 2;  // 0b0011 = 3
```

### 複合代入演算子（全10種）

#### 算術複合代入

```c++
int x = 10;
x += 5;   // x = x + 5  → 15
x -= 3;   // x = x - 3  → 12
x *= 2;   // x = x * 2  → 24
x /= 4;   // x = x / 4  → 6
x %= 5;   // x = x % 5  → 1
```

#### ビット演算複合代入

```c++
int flags = 0b1100;
flags &= 0b1010;  // flags = flags & 0b1010 → 0b1000
flags |= 0b0011;  // flags = flags | 0b0011 → 0b1011
flags ^= 0b0101;  // flags = flags ^ 0b0101 → 0b1110
```

#### シフト演算複合代入

```c++
int value = 4;
value <<= 2;  // value = value << 2 → 16
value >>= 3;  // value = value >> 3 → 2
```

#### 配列要素への複合代入

```c++
int[5] arr = [10, 20, 30, 40, 50];

arr[0] += 5;           // arr[0] = 15
arr[1] *= 2;           // arr[1] = 40
arr[2] -= arr[0];      // arr[2] = 15
arr[i*2+1] += (x+y);   // 複雑な式も対応
```

### インクリメント/デクリメント演算子

#### 前置演算子

```c++
int x = 5;
int y = ++x;  // x = 6, y = 6（先にインクリメント）
int z = --x;  // x = 5, z = 5（先にデクリメント）
```

#### 後置演算子（文として）

```c++
int count = 10;
count++;  // count = 11
count--;  // count = 10
```

**注意**: 後置演算子は文としてのみ使用可能（式の一部としては未対応）

### アドレス演算子とデリファレンス

```c++
int value = 42;
int* ptr = &value;     // アドレス取得

int x = *ptr;          // デリファレンス（値の取得）
*ptr = 100;            // デリファレンスして代入
```

### 三項演算子

```c++
int max = (a > b) ? a : b;
string status = (score >= 60) ? "Pass" : "Fail";
```

---

## 制御構造

### if文

```c++
if (condition) {
    // 処理
}

if (condition) {
    // 処理1
} else {
    // 処理2
}

if (condition1) {
    // 処理1
} else if (condition2) {
    // 処理2
} else {
    // 処理3
}
```

### ブロックなし単文

```c++
if (x > 0)
    println("Positive");
    
if (flag)
    x++;
else
    x--;
```

### for文

```c++
for (int i = 0; i < 10; i++) {
    println(i);
}

// 無限ループ
for (;;) {
    // 処理
    if (condition) break;
}
```

### while文

```c++
while (condition) {
    // 処理
}

int count = 0;
while (count < 10) {
    println(count);
    count++;
}
```

### break文

```c++
for (int i = 0; i < 100; i++) {
    if (i > 10) {
        break;  // ループを抜ける
    }
    println(i);
}
```

### continue文

```c++
for (int i = 0; i < 10; i++) {
    if (i % 2 == 0) {
        continue;  // 偶数をスキップ
    }
    println(i);  // 奇数のみ出力
}
```

### match文（パターンマッチング）

v0.11.0でEnum専用のパターンマッチングが実装されました。

**基本構文:**

```cb
match (expression) {
    Pattern1 => statement,
    Pattern2 => { block },
    Pattern3 => statement,
}
```

**Option<T>のマッチング:**

```cb
enum Option<T> {
    Some(T),
    None
};

int main() {
    Option<int> opt = Option<int>::Some(42);
    
    match (opt) {
        Some(value) => {
            println("Value: ", value);
        },
        None => {
            println("No value");
        }
    }
    
    return 0;
}
```

**Result<T, E>のマッチング:**

```cb
enum Result<T, E> {
    Ok(T),
    Err(E)
};

Result<int, string> divide(int a, int b) {
    if (b == 0) {
        return Result<int, string>::Err("Division by zero");
    }
    return Result<int, string>::Ok(a / b);
}

int main() {
    match (divide(10, 2)) {
        Ok(value) => println("Result: ", value),
        Err(error) => println("Error: ", error),
    }
    
    return 0;
}
```

**ワイルドカードパターン:**

```cb
enum Status {
    Ready(int),
    Running(int),
    Stopped(int),
    Done,
    Failed
};

int main() {
    Status s = Status::Running(50);
    
    match (s) {
        Ready(value) => println("Ready: ", value),
        Running(_) => println("Running (value discarded)"),
        _ => println("Other status"),
    }
    
    return 0;
}
```

**機能:**
- Enum variantのマッチング
- 関連値の抽出（destructuring）
- ワイルドカード（`_`）バインディング
- 変数、関数呼び出し、Enum構築式のサポート

---

## 関数

### 基本的な関数定義

```c++
int add(int a, int b) {
    return a + b;
}

void greet(string name) {
    println("Hello,", name);
}

int main() {
    int result = add(5, 3);
    greet("Alice");
    return 0;
}
```

### 再帰関数

```c++
int factorial(int n) {
    if (n <= 1) {
        return 1;
    }
    return n * factorial(n - 1);
}

int fibonacci(int n) {
    if (n <= 1) {
        return n;
    }
    return fibonacci(n - 1) + fibonacci(n - 2);
}
```

### 配列を引数に取る関数

```c++
void print_array(int[5] arr) {
    for (int i = 0; i < 5; i++) {
        println(arr[i]);
    }
}

int sum_array(int[10] values) {
    int total = 0;
    for (int i = 0; i < 10; i++) {
        total += values[i];
    }
    return total;
}
```

### 配列を返す関数（typedef使用）

```c++
typedef int[5] IntArray5;

IntArray5 create_sequence() {
    IntArray5 result;
    for (int i = 0; i < 5; i++) {
        result[i] = i + 1;
    }
    return result;
}

int main() {
    IntArray5 seq = create_sequence();
    // seq = [1, 2, 3, 4, 5]
    return 0;
}
```

### 多次元配列を返す関数

```c++
typedef int[2][2] Matrix2x2;

Matrix2x2 create_identity() {
    Matrix2x2 m;
    m[0][0] = 1; m[0][1] = 0;
    m[1][0] = 0; m[1][1] = 1;
    return m;
}
```

---

## ジェネリクス

### ジェネリック構造体

#### 基本的な定義

```cb
struct Box<T> {
    T value;
};

struct Pair<K, V> {
    K key;
    V value;
};
```

### 組み込みジェネリック型

#### Option<T> 型

`Option<T>`は値が存在するか存在しないかを表現する型です。Rustの`Option`型に相当します。

**定義**:
```cb
enum Option<T> {
    Some(T),
    None
};
```

**使用例**:
```cb
Option<int> find_value(int[10] arr, int target) {
    for (int i = 0; i < 10; i++) {
        if (arr[i] == target) {
            return Option<int>::Some(i);
        }
    }
    return Option<int>::None;
}

int main() {
    int[10] numbers = [1, 2, 3, 4, 5, 6, 7, 8, 9, 10];
    Option<int> result = find_value(numbers, 5);
    
    // variantフィールドで判定
    if (result.variant == "Some") {
        println("Found at index: {result.value}");
    } else {
        println("Not found");
    }
    
    return 0;
}
```

**match文での使用**:
```cb
match (result) {
    Some(index) => println("Found at: {index}"),
    None => println("Not found")
}
```

#### Result<T, E> 型

`Result<T, E>`は成功値または失敗値を表現する型です。エラーハンドリングに使用します。

**定義**:
```cb
enum Result<T, E> {
    Ok(T),
    Err(E)
};
```

**使用例**:
```cb
Result<int, string> divide(int a, int b) {
    if (b == 0) {
        return Result<int, string>::Err("Division by zero");
    }
    return Result<int, string>::Ok(a / b);
}

int main() {
    Result<int, string> result = divide(10, 2);
    
    match (result) {
        Ok(value) => println("Result: {value}"),
        Err(error) => println("Error: {error}")
    }
    
    return 0;
}
```

**内部フィールド**:
- `variant`: 文字列型。"Some"/"None"または"Ok"/"Err"
- `value`: T型。Someまたはokの場合の値
- `error`: E型。Errの場合のエラー値（Result専用）

#### 使用例

```cb
int main() {
    // 型パラメータを指定してインスタンス化
    Box<int> int_box;
    int_box.value = 42;
    
    Box<string> str_box;
    str_box.value = "Hello";
    
    Pair<string, int> age_pair;
    age_pair.key = "Alice";
    age_pair.value = 30;
    
    return 0;
}
```

#### ネストされたジェネリクス

```cb
struct Vec<T> {
    T data[10];
    int size;
};

int main() {
    Vec<int> numbers;
    Vec<Vec<int>> matrix;  // Vec<Vec<int>> のネスト
    
    return 0;
}
```

### ジェネリック関数

#### 基本的な定義

```cb
T identity<T>(T value) {
    return value;
}

T max<T>(T a, T b) {
    return a > b ? a : b;
}
```

#### 使用例

```cb
int main() {
    int x = identity<int>(42);
    long y = identity<long>(100);
    
    int m = max<int>(10, 20);        // 20
    double d = max<double>(3.14, 2.71);  // 3.14
    
    return 0;
}
```

#### 型パラメータを関数本体で使用

```cb
T duplicate<T>(T value) {
    T copy = value;  // 型パラメータを変数の型として使用
    return copy;
}

void swap<T>(T* a, T* b) {
    T temp = *a;
    *a = *b;
    *b = temp;
}
```

#### ジェネリック構造体を返す関数

```cb
struct Box<T> {
    T value;
};

Box<T> make_box<T>(T val) {
    Box<T> box;
    box.value = val;
    return box;
}

int main() {
    Box<int> int_box = make_box<int>(42);
    Box<string> str_box = make_box<string>("Hello");
    
    return 0;
}
```

### ジェネリック Enum

#### 基本的な定義

```cb
enum Option<T> {
    Some(T),
    None
};

enum Result<T, E> {
    Ok(T),
    Err(E)
};
```

#### 使用例

```cb
int main() {
    Option<int> some_val = Option<int>::Some(42);
    println(some_val.variant);  // "Some"
    println(some_val.value);    // 42
    
    Option<int> none_val = Option<int>::None;
    println(none_val.variant);  // "None"
    
    Result<int, string> ok = Result<int, string>::Ok(100);
    Result<int, string> err = Result<int, string>::Err("Error occurred");
    
    return 0;
}
```

### インターフェース境界（v0.11.0で実装済み）

ジェネリック型パラメータにインターフェース制約を指定できます：

```cb
// Allocatorインターフェースを実装する型のみ受け入れる
struct Vector<T, A: Allocator> {
    T* data;
    int size;
    A allocator;
}

impl Vector<T, A: Allocator> {
    void init() {
        self.data = self.allocator.allocate(sizeof(T) * 10);
        self.size = 0;
    }
    
    ~self() {
        if (self.data != nullptr) {
            self.allocator.deallocate(self.data);
        }
    }
}

// 使用例
int main() {
    Vector<int, SystemAllocator> vec;
    SystemAllocator alloc;
    vec.allocator = alloc;
    vec.init();
    
    return 0;
}
```

#### 複数のインターフェース境界

```cb
// 複数のインターフェースを要求（+で結合）
struct Container<T, A: Allocator + Clone> {
    T* data;
    A allocator;
}

// 複数の型パラメータにそれぞれ境界を指定
struct MultiContainer<K: Clone + Debug, V, A: Allocator + Clone> {
    K* keys;
    V* values;
    A allocator;
}
```

#### 型チェック

インターフェース境界に違反する型を使用するとコンパイルエラーになります：

```cb
struct NotAnAllocator {
    int x;
}

int main() {
    // エラー: NotAnAllocatorはAllocatorインターフェースを実装していない
    Vector<int, NotAnAllocator> vec;  // コンパイルエラー
    
    return 0;
}
```

---

## 配列

### 静的配列の宣言と初期化

```c++
// 宣言のみ
int[5] arr1;

// 配列リテラルで初期化
int[5] arr2 = [1, 2, 3, 4, 5];

// 部分初期化（残りは0）
int[10] arr3 = [1, 2, 3];  // [1, 2, 3, 0, 0, 0, 0, 0, 0, 0]

// 文字列配列
string[3] names = ["Alice", "Bob", "Charlie"];
```

### 配列要素へのアクセス

```c++
int[5] arr = [10, 20, 30, 40, 50];

int first = arr[0];     // 10
int last = arr[4];      // 50

arr[2] = 100;           // 要素の変更
arr[0] += 5;            // 複合代入
```

### 多次元配列

```c++
// 2次元配列
int[3][3] matrix = [
    [1, 2, 3],
    [4, 5, 6],
    [7, 8, 9]
];

int element = matrix[1][2];  // 6

// 3次元配列
int[2][3][4] cube;
cube[0][1][2] = 42;
```

### 配列とループ

```c++
int[10] numbers;

// 初期化
for (int i = 0; i < 10; i++) {
    numbers[i] = i * i;
}

// 合計計算
int sum = 0;
for (int i = 0; i < 10; i++) {
    sum += numbers[i];
}
```

### const配列

```c++
const int[5] PRIMES = [2, 3, 5, 7, 11];
// PRIMES[0] = 1;  // エラー: const配列は変更不可
```

---

## 構造体

### 基本的な構造体定義

```c++
struct Point {
    int x;
    int y;
};

struct Rectangle {
    int width;
    int height;
    string name;
};
```

### 構造体の初期化

#### 名前付きフィールド初期化

```c++
Point p1 = {x: 10, y: 20};

Rectangle rect = {
    width: 100,
    height: 50,
    name: "Sample"
};
```

#### 位置指定初期化

```c++
Point p2 = {30, 40};
Rectangle rect2 = {200, 100, "Large"};
```

#### 末尾カンマ対応

```c++
Point p3 = {
    x: 15,
    y: 25,  // 末尾カンマOK
};
```

### 構造体メンバーへのアクセス

```c++
Point p = {x: 5, y: 10};

int x_val = p.x;      // メンバーアクセス
p.y = 20;             // メンバーの変更
p.x += 5;             // 複合代入
```

### 構造体配列

```c++
Point[3] points = [
    {x: 0, y: 0},
    {x: 10, y: 10},
    {x: 20, y: 20}
];

// 配列要素のメンバーアクセス
points[0].x = 5;
int y_value = points[1].y;
```

### 配列メンバーを持つ構造体

```c++
struct Data {
    int[5] values;
    string name;
};

Data d = {
    values: [1, 2, 3, 4, 5],
    name: "Sample"
};

d.values[0] = 10;
```

### 構造体を関数引数・戻り値に使う

```c++
struct Circle {
    int radius;
    int x;
    int y;
};

int calculate_area(Circle c) {
    return c.radius * c.radius * 3;  // 簡易的な面積計算
}

Circle create_circle(int r, int cx, int cy) {
    Circle c = {radius: r, x: cx, y: cy};
    return c;
}
```

### ネストした構造体 ✅

構造体のメンバーに別の構造体を含めることができます。

#### 基本的なネスト

```c++
struct Point {
    int x;
    int y;
};

struct Circle {
    Point center;
    int radius;
};

// 初期化
Circle c = {
    center: {x: 10, y: 20},
    radius: 5
};

// メンバーアクセス
int cx = c.center.x;  // 10
int cy = c.center.y;  // 20

// メンバーの変更
c.center.x = 30;
c.radius = 10;
```

#### 多階層ネスト

3階層以上のネストも可能です。

```c++
struct Position {
    int x;
    int y;
    int z;
};

struct Orientation {
    int yaw;
    int pitch;
    int roll;
};

struct Transform {
    Position position;
    Orientation orientation;
};

struct GameObject {
    string name;
    Transform transform;
};

// 初期化
GameObject obj = {
    name: "Player",
    transform: {
        position: {x: 0, y: 0, z: 0},
        orientation: {yaw: 0, pitch: 0, roll: 0}
    }
};

// 多階層アクセス
int player_x = obj.transform.position.x;
obj.transform.orientation.yaw = 90;
```

#### ネストした構造体の配列

```c++
struct Address {
    string street;
    string city;
};

struct Person {
    string name;
    Address address;
};

Person[3] people = [
    {name: "Alice", address: {street: "123 Main", city: "Tokyo"}},
    {name: "Bob", address: {street: "456 Oak", city: "Osaka"}},
    {name: "Charlie", address: {street: "789 Pine", city: "Kyoto"}}
];

// アクセス
string alice_city = people[0].address.city;  // "Tokyo"
```

#### ネストした構造体とポインタ

```c++
Circle c = {center: {x: 10, y: 20}, radius: 5};
Circle* ptr = &c;

// ポインタ経由でネストしたメンバーにアクセス
int x = (*ptr).center.x;  // 10
ptr->center.y = 30;        // アロー演算子でもアクセス可能

// ネストした構造体へのポインタ
Point* centerPtr = &(ptr->center);
centerPtr->x = 50;
```

---

## コンストラクタとデストラクタ ✅ (v0.10.0)

### コンストラクタの基本

構造体に対してコンストラクタ（初期化関数）を定義できます。コンストラクタは`impl`ブロック内で`self()`として定義します。

```c++
struct Point {
    int x;
    int y;
};

impl Point {
    // デフォルトコンストラクタ
    self() {
        self.x = 0;
        self.y = 0;
        println("Point created at origin");
    }
}

void main() {
    Point p;  // デフォルトコンストラクタが自動的に呼ばれる
    // 出力: "Point created at origin"
    println(p.x, p.y);  // 0 0
}
```

### 引数付きコンストラクタ

コンストラクタは引数を取ることができます。

```c++
struct Point {
    int x;
    int y;
};

impl Point {
    // デフォルトコンストラクタ
    self() {
        self.x = 0;
        self.y = 0;
    }
    
    // 引数付きコンストラクタ
    self(int px, int py) {
        self.x = px;
        self.y = py;
    }
}

void main() {
    Point p1;           // デフォルトコンストラクタ
    Point p2(10, 20);   // 引数付きコンストラクタ
}
```

### デフォルト引数

コンストラクタのパラメータにデフォルト値を指定できます。

```c++
struct Rectangle {
    int x;
    int y;
    int width;
    int height;
};

impl Rectangle {
    // デフォルト引数付きコンストラクタ
    self(int w = 100, int h = 100) {
        self.x = 0;
        self.y = 0;
        self.width = w;
        self.height = h;
    }
    
    self(int px, int py, int w = 50, int h = 50) {
        self.x = px;
        self.y = py;
        self.width = w;
        self.height = h;
    }
}

void main() {
    Rectangle r1;           // w=100, h=100
    Rectangle r2(200);      // w=200, h=100
    Rectangle r3(200, 150); // w=200, h=150
    
    Rectangle r4(10, 20);     // px=10, py=20, w=50, h=50
    Rectangle r5(10, 20, 80); // px=10, py=20, w=80, h=50
}
```

### コピーコンストラクタ

既存のオブジェクトからコピーを作成するコンストラクタです。`const`参照を引数に取ります。

```c++
struct Point {
    int x;
    int y;
};

impl Point {
    self() {
        self.x = 0;
        self.y = 0;
    }
    
    // コピーコンストラクタ
    self(const Point& other) {
        self.x = other.x;
        self.y = other.y;
        println("Copy constructor called");
    }
}

void main() {
    Point p1(10, 20);
    Point p2 = p1;  // コピーコンストラクタが呼ばれる
}
```

### ムーブコンストラクタ

右辺値参照（`&&`）を使用して、リソースの所有権を移動します。ムーブ後、元のオブジェクトは無効な状態になります。

```c++
struct Buffer {
    int size;
    int* data;  // 実際のデータへのポインタ
    bool owns_data;
};

impl Buffer {
    self(int s) {
        self.size = s;
        // メモリ確保（簡略化）
        self.owns_data = true;
    }
    
    // コピーコンストラクタ（重い操作）
    self(const Buffer& other) {
        self.size = other.size;
        // ディープコピー
        self.owns_data = true;
        println("Deep copy (expensive)");
    }
    
    // ムーブコンストラクタ（軽い操作）
    self(Buffer&& other) {
        self.size = other.size;
        self.data = other.data;
        self.owns_data = other.owns_data;
        
        // 元のオブジェクトを無効化
        other.size = 0;
        other.data = nullptr;
        other.owns_data = false;
        println("Move (fast)");
    }
}

void main() {
    Buffer b1(100);
    Buffer b2 = b1;           // コピー: "Deep copy (expensive)"
    Buffer b3 = move(b1);     // ムーブ: "Move (fast)"
    // b1はもう使用できない
}
```

**ムーブの制約**:
- `const`修飾されたオブジェクトはムーブできません
- `const * const`ポインタはムーブ不可能
- ムーブ後の元のオブジェクトにアクセスすると未定義動作

### デストラクタ

オブジェクトがスコープを抜ける際に自動的に呼ばれる関数です。`~self()`として定義します。

```c++
struct Resource {
    int id;
    bool allocated;
};

impl Resource {
    self(int resource_id) {
        self.id = resource_id;
        self.allocated = true;
        println("Resource", resource_id, "allocated");
    }
    
    ~self() {
        if (self.allocated) {
            println("Resource", self.id, "freed");
            self.allocated = false;
        }
    }
}

void main() {
    {
        Resource r(42);
        // 何か処理...
    }  // スコープを抜ける → デストラクタが自動呼び出し
    // 出力: "Resource 42 freed"
}
```

### プライベート関数の使用

`impl`ブロック内でプライベート関数を定義し、コンストラクタから呼び出すことができます。

```c++
struct Circle {
    int x;
    int y;
    int radius;
    double area;
};

impl Circle {
    // プライベート関数
    self.calculateArea() {
        // πr²を計算（簡略化）
        self.area = 3.14159 * self.radius * self.radius;
    }
    
    // コンストラクタでプライベート関数を使用
    self(int cx, int cy, int r) {
        self.x = cx;
        self.y = cy;
        self.radius = r;
        self.calculateArea();  // プライベート関数呼び出し
    }
}

void main() {
    Circle c(10, 20, 5);
    println("Area:", c.area);  // 78.53975
}
```

### コンストラクタのオーバーロード

複数の異なるシグネチャのコンストラクタを定義できます。

```c++
struct Vector3D {
    int x;
    int y;
    int z;
};

impl Vector3D {
    // デフォルトコンストラクタ
    self() {
        self.x = 0;
        self.y = 0;
        self.z = 0;
    }
    
    // 1つの値で全要素を初期化
    self(int value) {
        self.x = value;
        self.y = value;
        self.z = value;
    }
    
    // 3つの値で初期化
    self(int vx, int vy, int vz) {
        self.x = vx;
        self.y = vy;
        self.z = vz;
    }
    
    // コピーコンストラクタ
    self(const Vector3D& other) {
        self.x = other.x;
        self.y = other.y;
        self.z = other.z;
    }
}

void main() {
    Vector3D v1;             // (0, 0, 0)
    Vector3D v2(5);          // (5, 5, 5)
    Vector3D v3(1, 2, 3);    // (1, 2, 3)
    Vector3D v4 = v3;        // コピー
}
```

### スコープとクリーンアップのタイミング ✅ (v0.10.0)

**スコープの終了とは**:
1. **通常のブロック終了**: `}` に到達したとき
2. **return文の実行**: return文は実行前にスコープ終了として扱われる

#### return文での自動クリーンアップ

return文を実行する直前に、以下の順序でクリーンアップが実行されます:

1. **defer文の実行**（LIFO順）
2. **ローカル変数のデストラクタ実行**（LIFO順）
3. **return値の評価とコピー/ムーブ**

```c++
struct Resource {
    int id;
    
    self(int i) {
        self.id = i;
        println("Resource", self.id, "created");
    }
    
    ~self() {
        println("Resource", self.id, "destroyed");
    }
}

Resource create_resource() {
    Resource r(1);
    defer println("Defer statement");
    
    println("Before return");
    return r;  // ✅ ここでdefer→デストラクタ→returnの順に実行
}

void main() {
    println("=== Start ===");
    Resource result = create_resource();
    println("=== After create ===");
}

// 出力:
// === Start ===
// Resource 1 created
// Before return
// Defer statement          ← return前に実行
// Resource 1 destroyed     ← return前に実行
// === After create ===
// Resource 1 destroyed     ← main終了時
```

#### 複数のreturn経路

どのreturn文でも、同じクリーンアップルールが適用されます:

```c++
void process(bool condition) {
    Resource r1(1);
    defer println("Defer 1");
    
    if (condition) {
        Resource r2(2);
        defer println("Defer 2");
        return;  // ✅ Defer 2 → r2デストラクタ → Defer 1 → r1デストラクタ
    }
    
    Resource r3(3);
    defer println("Defer 3");
    return;  // ✅ Defer 3 → r3デストラクタ → Defer 1 → r1デストラクタ
}
```

### 重要な注意事項

1. **自動呼び出し**: 構造体変数を宣言すると、適切なコンストラクタが自動的に呼ばれます
2. **デストラクタの自動呼び出し**: スコープを抜けると自動的にデストラクタが呼ばれます
3. **return前のクリーンアップ**: return文実行前にdefer/デストラクタが実行される (v0.10.0)
4. **`self`キーワード**: コンストラクタ/デストラクタ内では`self`で現在のオブジェクトを参照
5. **オーバーロード**: 引数の型と数で適切なコンストラクタが選択されます
6. **implブロック**: コンストラクタとデストラクタは`impl StructName {}`で定義（インターフェース不要）
7. **参照の区別**:
   - `&`: 通常の参照（コピーコンストラクタ用）
   - `&&`: 右辺値参照（ムーブコンストラクタ専用）

---

## Union型

### Union型の基本

TypeScript風のUnion型システムを完全サポート。

```c++
// リテラル値Union
typedef Status = 200 | 404 | 500;

// 基本型Union
typedef NumValue = int | long;
typedef StringOrInt = string | int;

// 混合Union
typedef Mixed = 42 | int | string;
```

### Union型の使用

```c++
Status code = 200;    // OK
// Status invalid = 301;  // エラー: 許可されていない値

StringOrInt value = 10;      // int値
value = "Hello";             // string値に変更可能
```

### カスタム型Union

```c++
typedef UserID = int;
typedef ProductID = string;
typedef ID = UserID | ProductID;

UserID uid = 123;
ID general_id = uid;  // OK
```

### 構造体Union

```c++
struct User {
    int id;
    string name;
};

struct Product {
    string code;
    int price;
};

typedef Entity = User | Product;

User alice = {id: 1, name: "Alice"};
Entity entity = alice;  // OK
```

### 配列Union

```c++
typedef ArrayUnion = int[5] | string[3];

int[5] numbers = [1, 2, 3, 4, 5];
ArrayUnion arr = numbers;  // OK
```

### Union型への複合代入

```c++
typedef Uni = int | string;

Uni value = 10;
value += 5;   // int型として扱われる → 15

value = "Hello";  // 型変更
// value += " World";  // string連結は未実装
```

---

## enum型 ✅

### enum型の基本

C/C++風の列挙型を完全サポート。

```c++
enum Color {
    RED = 0,
    GREEN = 1,
    BLUE = 2
};

enum Status {
    OK = 200,
    NOT_FOUND = 404,
    SERVER_ERROR = 500
};
```

### enum値のアクセス

```c++
Color c = Color::RED;
Status s = Status::OK;

println("Color:", c);     // 0
println("Status:", s);    // 200
```

### 自動値割り当て

明示的な値を指定しない場合、自動的に連番が割り当てられます。

```c++
enum Day {
    MONDAY,     // 0
    TUESDAY,    // 1
    WEDNESDAY,  // 2
    THURSDAY,   // 3
    FRIDAY,     // 4
    SATURDAY,   // 5
    SUNDAY      // 6
};

Day today = Day::WEDNESDAY;
println("Day:", today);  // 2
```

### typedef enum

typedef構文でenum型を定義することも可能。

```c++
typedef enum Color {
    RED = 0xFF0000,
    GREEN = 0x00FF00,
    BLUE = 0x0000FF
} Color;

Color myColor = Color::RED;
```

### enum値の比較

```c++
enum Status {
    IDLE,
    RUNNING,
    DONE
};

Status current = Status::RUNNING;

if (current == Status::RUNNING) {
    println("System is running");
}

if (current != Status::IDLE) {
    println("System is not idle");
}
```

### switch文での使用（将来実装）

```c++
// 将来実装予定
switch (status) {
    case Status::OK:
        println("Success");
        break;
    case Status::NOT_FOUND:
        println("Not found");
        break;
    default:
        println("Other status");
}
```

---

## Interface/Implシステム

### Interfaceの定義

```c++
interface Drawable {
    void draw();
    int getSize();
};

interface Printable {
    string toString();
};
```

### 基本型へのImpl

```c++
typedef MyInt = int;

impl Printable for MyInt {
    string toString() {
        return "MyInt value";
    }
};
```

### 配列型へのImpl

```c++
typedef IntArray = int[5];

impl Printable for IntArray {
    string toString() {
        return "IntArray[5]";
    }
};
```

### 構造体へのImpl

```c++
struct Point {
    int x;
    int y;
};

impl Drawable for Point {
    void draw() {
        println("Point at (", self.x, ",", self.y, ")");
    }
    
    int getSize() {
        return 2;  // x, yの2要素
    }
};
```

### Interfaceを使ったポリモーフィズム

```c++
typedef MyInt = int;
typedef IntArray = int[5];

impl Printable for MyInt {
    string toString() { return "MyInt"; }
};

impl Printable for IntArray {
    string toString() { return "IntArray"; }
};

int main() {
    MyInt mi = 42;
    IntArray arr = [1, 2, 3, 4, 5];
    
    // Interface型変数で抽象化
    Printable p1 = mi;
    Printable p2 = arr;
    
    // 統一的なメソッド呼び出し
    println(p1.toString());  // "MyInt"
    println(p2.toString());  // "IntArray"
    
    return 0;
}
```

### implブロック内でのポインタ操作

```c++
struct Container {
    int[10] values;
};

impl Printable for Container {
    string toString() {
        // implブロック内でもポインタ使用可能
        int* ptr = &self.values[0];
        int sum = 0;
        
        for (int i = 0; i < 10; i++) {
            sum += *ptr;
            ptr++;
        }
        
        return "Sum: " + sum;
    }
};
```

### 再帰的Typedef独立性

各typedef層で独立したImpl定義が可能:

```c++
typedef int INT;
typedef INT INT2;
typedef INT2 INT3;

// INT3にのみPrintableを実装
impl Printable for INT3 {
    string toString() {
        return "INT3 implementation";
    }
};

int main() {
    int original = 100;   // Printableなし
    INT int1 = 200;       // Printableなし
    INT2 int2 = 300;      // Printableなし
    INT3 int3 = 400;      // Printableあり
    
    Printable p = int3;   // OK
    // Printable p2 = int2; // エラー
    
    return 0;
}
```

### impl内Static変数 🆕 (v0.9.0)

implブロック内でstatic変数を宣言することで、同じimpl定義内のすべてのメソッドで共有される状態を管理できます。

#### 基本構文

```c++
interface Counter {
    int increment();
    int get_count();
};

struct Point {
    int x;
    int y;
};

impl Counter for Point {
    static int shared_counter = 0;  // impl全体で共有されるstatic変数
    
    int increment() {
        shared_counter = shared_counter + 1;
        return shared_counter;
    }
    
    int get_count() {
        return shared_counter;
    }
};
```

#### スコープと独立性

- **impl単位での共有**: 同じ`impl Interface for Struct`内のメソッドで共有
- **型ごとに独立**: `impl I for A`と`impl I for B`は異なるstatic変数を持つ

```c++
interface Shape {
    int register_instance();
    int get_count();
};

struct Circle {
    int radius;
};

struct Rectangle {
    int width;
    int height;
};

impl Shape for Circle {
    static int instance_count = 0;
    
    int register_instance() {
        instance_count++;
        return instance_count;
    }
    
    int get_count() {
        return instance_count;
    }
};

impl Shape for Rectangle {
    static int instance_count = 0;  // Circleとは独立した変数
    
    int register_instance() {
        instance_count++;
        return instance_count;
    }
    
    int get_count() {
        return instance_count;
    }
};

int main() {
    Circle c1 = {radius: 5};
    Circle c2 = {radius: 10};
    Rectangle r1 = {width: 3, height: 4};
    Rectangle r2 = {width: 5, height: 6};
    
    Shape s1 = c1;
    Shape s2 = c2;
    Shape s3 = r1;
    Shape s4 = r2;
    
    println(s1.register_instance());  // 1 (Circle用カウンター)
    println(s2.register_instance());  // 2 (Circle用カウンター)
    println(s3.register_instance());  // 1 (Rectangle用カウンター、Circleとは独立)
    println(s4.register_instance());  // 2 (Rectangle用カウンター)
    
    return 0;
}
```

#### 名前空間設計

impl static変数は以下の名前空間で管理されます:

```
impl::InterfaceName::StructTypeName::variable_name

例:
impl::Counter::Point::shared_counter
impl::Shape::Circle::instance_count
impl::Shape::Rectangle::instance_count  // ← Circleとは別のstatic変数
```

#### 特徴

1. **永続性**: プログラム実行中ずっと保持される
2. **const修飾子**: `static const int MAX = 100;` のような定数定義が可能
3. **初期化式**: `static int counter = 0;` のような初期化式をサポート
4. **アクセス制限**: implメソッド内からのみアクセス可能

#### ユースケース

**インスタンスカウンター**:
```c++
impl Tracker for Stats {
    static int instance_count = 0;
    
    void register_instance() {
        instance_count++;
    }
};
```

**共有設定値**:
```c++
impl Config for Settings {
    static const int MAX_VALUE = 100;
    static int access_count = 0;
    
    int get_max() {
        access_count++;
        return MAX_VALUE;
    }
};
```

**デバッグ統計**:
```c++
impl Debugger for Tracer {
    static int total_calls = 0;
    static long sum = 0;
    
    void record(int value) {
        total_calls++;
        sum = sum + value;
    }
};
```

---

## デストラクタとRAII

### デストラクタの基本

デストラクタは、構造体がスコープを抜けるときに自動的に呼び出される特別なメソッドです。リソースの自動解放を実現します。

#### 基本構文

```cb
struct Resource {
    int id;
}

impl Resource {
    ~self() {
        println("Resource {id} destroyed");
    }
}
```

#### 実行タイミング

```cb
int main() {
    {
        Resource r;
        r.id = 1;
        println("Resource created");
        // スコープ終了時に自動的にデストラクタが呼ばれる
    }
    println("After scope");
    
    return 0;
}

// 出力:
// Resource created
// Resource 1 destroyed
// After scope
```

### LIFO順序

複数の変数がある場合、デストラクタは宣言の逆順（LIFO: Last In, First Out）で実行されます。

```cb
struct Item {
    int id;
}

impl Item {
    ~self() {
        println("Item {id} destroyed");
    }
}

int main() {
    Item a;
    a.id = 1;
    Item b;
    b.id = 2;
    Item c;
    c.id = 3;
    
    return 0;
}

// 出力:
// Item 3 destroyed
// Item 2 destroyed
// Item 1 destroyed
```

### ジェネリック構造体のデストラクタ

ジェネリック構造体でもデストラクタを定義できます。

```cb
struct Vector<T, A: Allocator> {
    T* data;
    int size;
    int capacity;
    A allocator;
}

impl Vector<T, A: Allocator> {
    ~self() {
        println("Vector deinit - cleaning up {size} elements");
        if (data != NULL) {
            allocator.free(data);
        }
    }
}

int main() {
    Vector<int, SystemAllocator> vec;
    vec.size = 10;
    // スコープ終了時にデストラクタが自動実行される
    return 0;
}
```

### deferとの組み合わせ

デストラクタとdefer文は組み合わせて使用できます。実行順序はLIFO（後入れ先出し）です。

```cb
struct Resource {
    int id;
};

impl Resource {
    fn deinit() {
        println("Resource {id} destroyed");
    }
};

int main() {
    Resource r;
    r.id = 1;
    
    defer println("Defer 1");
    defer println("Defer 2");
    
    return 0;
}

// 出力:
// Defer 2
// Defer 1
// Resource 1 destroyed
```

### break/continueとの統合

break文やcontinue文でループから抜ける場合でも、デストラクタは正しく実行されます。

```cb
struct Item {
    int id;
};

impl Item {
    fn deinit() {
        println("Item {id} destroyed");
    }
};

int main() {
    for (int i = 0; i < 5; i++) {
        Item item;
        item.id = i;
        
        if (i == 2) {
            break;  // break前に item.deinit() が実行される
        }
    }
    
    return 0;
}

// 出力:
// Item 0 destroyed
// Item 1 destroyed
// Item 2 destroyed
```

### 制限事項

1. **スコープベースの呼び出しのみ**: デストラクタはスコープ終了時にのみ自動呼び出しされます
2. **ヒープオブジェクトは手動管理**: ポインタで管理されるヒープ上のオブジェクトは手動で解放が必要
3. **明示的呼び出し不可**: デストラクタを直接呼び出すことはできません

---

## 文字列補間

### 基本的な使い方

文字列リテラル内で`{}`を使用して式を埋め込むことができます。

```cb
int x = 42;
string name = "Alice";
string message = "Hello, {name}! The answer is {x}";
// "Hello, Alice! The answer is 42"
```

### 式の埋め込み

任意の式を埋め込むことができます。

```cb
int a = 10;
int b = 20;
println("Sum: {a + b}");        // "Sum: 30"
println("Product: {a * b}");    // "Product: 200"
println("Comparison: {a < b}"); // "Comparison: true"
```

### フォーマット指定子

#### 整数フォーマット

```cb
int num = 255;

println("{num:d}");   // "255" (10進数)
println("{num:x}");   // "ff" (16進数小文字)
println("{num:X}");   // "FF" (16進数大文字)
println("{num:o}");   // "377" (8進数)
println("{num:b}");   // "11111111" (2進数)
```

#### 幅指定

```cb
int value = 42;

println("{value:5d}");   // "   42" (右詰め、幅5)
println("{value:05d}");  // "00042" (ゼロ埋め、幅5)
```

#### 浮動小数点フォーマット

```cb
double pi = 3.14159265;

println("{pi:f}");       // "3.141593" (デフォルト精度)
println("{pi:.2f}");     // "3.14" (小数点以下2桁)
println("{pi:8.3f}");    // "   3.142" (幅8、精度3)
println("{pi:e}");       // "3.141593e+00" (指数表記)
```

### 複数の補間

```cb
string first = "John";
string last = "Doe";
int age = 30;

string profile = "{first} {last} is {age} years old";
// "John Doe is 30 years old"
```

### 構造体メンバーアクセス

```cb
struct Point {
    int x;
    int y;
};

Point p;
p.x = 10;
p.y = 20;

println("Point at ({p.x}, {p.y})");
// "Point at (10, 20)"
```

### エスケープ

`{}`を文字として出力する場合は、`{{`と`}}`を使用します。

```cb
println("Use {{}} for interpolation");
// "Use {} for interpolation"
```

### 制限事項

1. **フォーマット指定子は数値型のみ**: 文字列型には適用できません
2. **ネストした補間は未対応**: `"{"{x}"}"` のような記述はできません
3. **実行時評価**: 式は実行時に評価されるため、静的な文字列結合よりオーバーヘッドがあります

---

## ポインタと参照

### ポインタの基本

#### ポインタの宣言と初期化

```c++
int value = 42;
int* ptr;          // ポインタ宣言

ptr = &value;      // アドレス取得
int* ptr2 = &value; // 宣言時初期化 ✅
```

#### デリファレンス（値の取得・変更）

```c++
int value = 10;
int* ptr = &value;

int x = *ptr;      // デリファレンスして値取得: x = 10
*ptr = 20;         // デリファレンスして値変更: value = 20

println("value =", value);  // 20
println("*ptr =", *ptr);    // 20
```

### ポインタ演算

#### 加算・減算

```c++
int[5] arr = [10, 20, 30, 40, 50];
int* ptr = &arr[0];

ptr = ptr + 1;     // 次の要素を指す
int val = *ptr;    // 20

ptr = ptr + 2;     // さらに2つ先
val = *ptr;        // 40

ptr = ptr - 1;     // 1つ戻る
val = *ptr;        // 30
```

#### インクリメント・デクリメント

```c++
int[5] numbers = [1, 2, 3, 4, 5];
int* p = &numbers[0];

p++;               // 次の要素へ
println(*p);       // 2

p--;               // 前の要素へ
println(*p);       // 1
```

### ポインタと配列

```c++
int[5] arr = [10, 20, 30, 40, 50];
int* ptr = &arr[0];

// ポインタを使った配列走査
for (int i = 0; i < 5; i++) {
    println("arr[", i, "] =", *ptr);
    ptr++;
}
```

### ポインタと関数

```c++
void modify_value(int* ptr) {
    *ptr = 100;
}

int main() {
    int value = 10;
    modify_value(&value);
    println("value =", value);  // 100
    return 0;
}
```

### 構造体ポインタ

```c++
struct Point {
    int x;
    int y;
};

int main() {
    Point p = {x: 10, y: 20};
    Point* ptr = &p;
    
    // デリファレンス構文
    (*ptr).x = 30;
    (*ptr).y = 40;
    
    println("p.x =", p.x);  // 30
    println("p.y =", p.y);  // 40
    
    return 0;
}
```

### アドレスの表示

ポインタ値は16進数形式で表示:

```c++
int value = 42;
int* ptr = &value;

println("ptr =", ptr);      // 0x7fff5fbff8ac (例)
println("&value =", &value); // 0x7fff5fbff8ac (例)
println("&ptr =", &ptr);    // 0x7fff5fbff8b0 (例)
```

### ポインタの配列

```c++
int a = 10, b = 20, c = 30;
int* ptrs[3];

ptrs[0] = &a;
ptrs[1] = &b;
ptrs[2] = &c;

for (int i = 0; i < 3; i++) {
    println("*ptrs[", i, "] =", *ptrs[i]);
}
```

### Interfaceポインタ

```c++
interface Shape {
    int area();
};

struct Rectangle {
    int width;
    int height;
};

impl Shape for Rectangle {
    int area() {
        return self.width * self.height;
    }
};

int main() {
    Rectangle rect = {width: 10, height: 5};
    Shape* shape_ptr = &rect;
    
    // Interface経由でメソッド呼び出し
    int a = (*shape_ptr).area();  // 50
    println("Area:", a);
    
    return 0;
}
```

### 参照型 ✅

参照型（`T&`）を使用すると、引数を参照渡しで関数に渡すことができます。

```c++
void increment(int& ref) {
    ref++;
}

void modify_value(int& ref) {
    ref = 100;
}

void main() {
    int value = 10;
    println(value);     // 10
    increment(value);   // valueが直接変更される
    println(value);     // 11
    modify_value(value);
    println(value);     // 100
}
```

#### 構造体参照型

構造体も参照型として渡すことができます。

```c++
struct Point {
    int x;
    int y;
};

void move_point(Point& p, int dx, int dy) {
    p.x = p.x + dx;
    p.y = p.y + dy;
}

void main() {
    Point p;
    p.x = 10;
    p.y = 20;
    println(p.x);      // 10
    println(p.y);      // 20
    move_point(p, 5, 15);
    println(p.x);      // 15
    println(p.y);      // 35
}
```

**配列の参照渡し** ✅ (v0.9.2):
- 関数に配列を渡す際、自動的に参照として渡される（C/C++と同様の動作）
- 関数内での配列要素の変更が呼び出し元に反映される

```cb
void modify(int[3] arr) {
    arr[0] = 100;  // 呼び出し元の配列が変更される
}

void main() {
    int[3] nums = [1, 2, 3];
    modify(nums);
    println(nums[0]);  // 100
}
```

**制限事項**:
- 配列参照**型**（`int[N]&`形式の明示的型宣言）は現在サポートされていません（v0.10.0で実装予定）
- 参照のポインタ（`int&*`）はサポートされていません

**右辺値参照（ムーブセマンティクス）** ✅ (v0.10.0):
- `&&`は右辺値参照（rvalue reference）として使用され、ムーブセマンティクスを実現します
- ムーブコンストラクタやムーブ代入演算子で使用されます
- `const`修飾されたオブジェクトや`const * const`ポインタはムーブできません

```c++
struct Buffer {
    int size;
    int* data;
};

impl Buffer {
    // ムーブコンストラクタ
    self(Buffer&& other) {
        self.size = other.size;
        self.data = other.data;
        // 元のオブジェクトを無効化
        other.size = 0;
        other.data = nullptr;
    }
}

void main() {
    Buffer b1(100);
    Buffer b2 = move(b1);  // ムーブ
}
```

### 関数ポインタ ✅

関数へのポインタを取得し、関数を変数として扱うことができます。

```c++
int add(int a, int b) {
    return a + b;
}

int subtract(int a, int b) {
    return a - b;
}

void main() {
    // 関数ポインタの宣言と初期化
    int* op = &add;
    
    // 呼び出し（2つの形式）
    int result1 = op(5, 3);      // 暗黙的呼び出し
    int result2 = (*op)(5, 3);   // 明示的デリファレンス
    println(result1);  // 8
    println(result2);  // 8
    
    // 関数ポインタの再代入
    op = &subtract;
    println(op(10, 3));  // 7
    
    // アドレス表示
    println(op);  // 0x... (16進数)
}
```

#### コールバック関数

```c++
int apply(int* callback, int x, int y) {
    return callback(x, y);
}

void main() {
    int result = apply(&add, 10, 5);
    println(result);  // 15
}
```

#### 関数ポインタを返す関数

```c++
int* get_operation(int code) {
    if (code == 1) {
        return &add;
    }
    return &subtract;
}

void main() {
    int* op = get_operation(1);
    println(op(8, 3));  // 11
    
    // チェーン呼び出し
    int result = get_operation(2)(10, 4);
    println(result);  // 6
}
```

### アロー演算子 ✅

構造体ポインタのメンバーアクセスを簡潔に記述できます。

```c++
struct Point {
    int x;
    int y;
};

void main() {
    Point p;
    p.x = 10;
    p.y = 20;
    Point* ptr = &p;
    
    // アロー演算子
    ptr->x = 30;
    ptr->y = 40;
    
    println(ptr->x);  // 30
    println(ptr->y);  // 40
    
    // (*ptr).x と ptr->x は同等
}
```

### Const Pointer Safety（v0.9.1）✅

**const変数のアドレスを非constポインタに代入することを防ぐ安全機能**です。constの制約を迂回してデータを変更する危険なコードを検出します。

#### 検出される違反パターン

##### 1. const変数のアドレス → 非constポインタ

**エラーになるコード**:
```c++
const int x = 42;
int* ptr;
ptr = &x;  // ❌ Error: Cannot assign address of const variable to non-const pointer
```

**正しいコード**:
```c++
const int x = 42;
const int* ptr = &x;  // ✅ OK
*ptr;  // 値の読み取りは可能
// *ptr = 100;  // エラー: const経由で変更不可
```

##### 2. const T*のアドレス → T**（ダブルポインタ）

**エラーになるコード**:
```c++
const int* ptr1 = &x;
int** ptr2 = &ptr1;  // ❌ Error: Cannot assign address of pointer to const
```

**正しいコード**:
```c++
const int* ptr1 = &x;
const int** ptr2 = &ptr1;  // ✅ OK
```

##### 3. T* constのアドレス → T**

**エラーになるコード**:
```c++
int* const ptr1 = &x;
int** ptr2 = &ptr1;  // ❌ Error: Cannot assign address of const pointer
```

**正しいコード**:
```c++
int* const ptr1 = &x;
int* const* ptr2 = &ptr1;  // ✅ OK
```

#### エラーメッセージ

すべてのエラーメッセージは適切な修正方法を提示します：

```
Error: Cannot assign address of const variable 'x' to non-const pointer 'ptr'. 
Use 'const int*' instead of 'int*'
```

```
Error: Cannot assign address of pointer to const (const T*) 'ptr1' to non-const double pointer 'ptr2'. 
The pointee should be 'const T**', not 'T**'
```

```
Error: Cannot assign address of const pointer (T* const) 'ptr1' to non-const double pointer 'ptr2'. 
Use 'const' qualifier appropriately
```

#### 正しい使用パターン

```c++
// パターン1: const変数とconst pointer
const int x = 42;
const int* ptr = &x;
println(*ptr);  // 42

// パターン2: 非const変数もconst pointerで読める
int y = 100;
const int* ptr2 = &y;
println(*ptr2);  // 100

// パターン3: 非const変数と非const pointer
int z = 200;
int* ptr3 = &z;
*ptr3 = 300;  // 値の変更も可能
println(z);   // 300

// パターン4: ダブルポインタの正しい使用
const int val = 42;
const int* ptr_a = &val;
const int** ptr_b = &ptr_a;
println(**ptr_b);  // 42
```

#### 設計思想

**なぜ実行時エラーなのか？**

Cb言語は現在インタープリタとして実装されており、型チェックは実行時に行われます。将来的にコンパイラを実装する際には、これをコンパイル時エラーに変更することも可能です。

**既存機能との関係**

この機能は、既存のconstポインタ機能（`const T*`, `T* const`, `const T* const`）と組み合わせて、完全な型安全性を提供します。

---

## モジュールシステム

### モジュールのインポート

**v0.11.0以降の新構文**:

```cb
import math;
import utils;

int main() {
    int result = math_add(5, 3);
    return 0;
}
```

**ネストしたモジュール**:

```cb
import collections.vector;
import std.allocators.system;

int main() {
    Vector<int> vec;
    return 0;
}
```

**注意**: v0.11.0より、文字列リテラルimport構文（`import "path/to/file.cb";`）は**廃止**されました。
モジュール識別子構文（`import module.path.name;`）を使用してください。

**エラー例**:
```cb
import "math.cb";  // ❌ エラー: String literal import syntax is deprecated
```

**正しい書き方**:
```cb
import math;       // ✅ 正しい
```

### モジュール内の関数定義

**math.cb**:
```c++
export int math_add(int a, int b) {
    return a + b;
}

export int math_multiply(int a, int b) {
    return a * b;
}
```

### プライベート関数

```c++
// exportなしの関数はモジュール内のみ
int internal_helper() {
    return 42;
}

export int public_function() {
    return internal_helper();
}
```

### impl構文のexport/import ✅ (v0.10.0)

implブロック（コンストラクタ、インターフェース実装）もexport/import可能です。

#### implブロックのexport

**point.cb**:
```c++
export struct Point {
    int x;
    int y;
}

// コンストラクタのexport
export impl Point {
    self(int px, int py) {
        self.x = px;
        self.y = py;
    }
    
    void print() {
        println("Point(", self.x, ", ", self.y, ")");
    }
}

// インターフェース実装のexport
interface Printable {
    void print();
}

export impl Printable for Point {
    void print() {
        println("Printable: Point(", self.x, ", ", self.y, ")");
    }
}
```

#### implブロックのimport

**main.cb**:
```c++
import "point.cb";

void main() {
    // エクスポートされたコンストラクタを使用
    Point p(10, 20);
    
    // エクスポートされたメソッドを使用
    p.print();  // Point(10, 20)
    
    // エクスポートされたインターフェース実装を使用
    Printable& printable = p;
    printable.print();  // Printable: Point(10, 20)
}
```

#### 制約事項

1. **export impl**: 構造体自体が`export`されていなければならない
2. **インターフェース実装**: インターフェースも`export`が必要
3. **スコープルール**: implブロックは通常のスコープルールに従う

---

## 入出力

### println関数

```c++
println("Hello, World!");
println("Value:", value);
println("x =", x, "y =", y);
```

### print関数（フォーマット指定子）

#### サポートするフォーマット指定子

| 指定子 | 型 | 説明 |
|--------|-----|------|
| `%d` | int, tiny, short | 整数 |
| `%lld` | long | 長整数 |
| `%u` | unsigned整数 | 符号なし整数 |
| `%s` | string | 文字列 |
| `%c` | char | 文字 |
| `%%` | - | パーセント記号 |

#### 使用例

```c++
int age = 25;
string name = "Alice";
char grade = 'A';

print("Name: %s, Age: %d, Grade: %c", name, age, grade);
print("Percentage: 50%%");
```

---

## エラーハンドリング

### コンパイル時エラー

#### 型不整合

```c++
int x = "string";  // エラー: 型が一致しない
```

#### 配列境界エラー

```c++
int[5] arr;
int value = arr[10];  // エラー: 配列範囲外アクセス
```

#### Union型エラー

```c++
typedef RestrictedUnion = int | string;
bool flag = true;
RestrictedUnion invalid = flag;  // エラー: bool型は許可されていない
```

### ランタイムエラー

#### 整数型範囲チェック

```c++
tiny t = 200;  // エラー: tinyは-128~127
```

#### unsigned型の負値クランプ

```c++
unsigned int ui = -10;  // 警告: 0にクランプ
println(ui);            // 0
```

### デバッグモード

#### 英語デバッグ

```bash
./main --debug program.cb
```

#### 日本語デバッグ

```bash
./main --debug-ja program.cb
```

---

## メモリ管理

### 自動メモリ管理

Cbはガベージコレクションを使用せず、C++ RAII（Resource Acquisition Is Initialization）パターンに基づく自動メモリ管理を採用。

#### スコープベース

```c++
int main() {
    {
        int[1000] large_array;  // スコープ開始時に確保
        // 使用...
    }  // スコープ終了時に自動解放
    
    return 0;
}
```

#### 配列の自動管理

```c++
void process_data() {
    int[100] buffer;
    // bufferは関数終了時に自動解放
}
```

### 動的メモリ管理

#### malloc() - メモリの動的確保

指定されたサイズのメモリブロックを確保します。C言語の`malloc()`に相当します。

**構文**:
```cb
void* malloc(int size)
```

**使用例**:
```cb
// 整数10個分のメモリを確保
int* ptr = malloc(40);  // sizeof(int) * 10 = 40 bytes

if (ptr != NULL) {
    // メモリを使用
    ptr[0] = 10;
    ptr[1] = 20;
    
    // 使用後は必ず解放
    free(ptr);
}
```

**注意事項**:
- 確保に失敗するとNULLを返す
- 確保したメモリは必ず`free()`で解放する必要がある
- 型安全性がないため、キャスト時に注意が必要

#### free() - メモリの解放

`malloc()`で確保したメモリを解放します。

**構文**:
```cb
void free(void* ptr)
```

**使用例**:
```cb
void* data = malloc(100);
// データを使用...
free(data);  // 必ず解放
data = NULL;  // ダングリングポインタを防ぐ
```

**注意事項**:
- 同じポインタを二重に`free()`しない（二重解放）
- 解放後はポインタを使用しない（use-after-free）
- NULLポインタを`free()`しても安全（何もしない）

#### new 演算子 - 型安全な動的確保（v0.11.0 Phase 1aで実装済み）

**実装状態**: ✅ v0.11.0 Phase 1aで実装済み

**構文**:
```cb
T* new T;           // 単一オブジェクト確保
T* new T[size];     // 配列確保
```

**使用例**:
```cb
struct Point {
    int x;
    int y;
}

int main() {
    // 単一オブジェクト確保
    Point* p = new Point;
    p->x = 10;
    p->y = 20;
    
    println("Point: ({p->x}, {p->y})");
    
    // 使用後はdeleteで解放
    delete p;
    
    // 配列確保
    int* arr = new int[10];
    arr[0] = 100;
    delete arr;
    
    return 0;
}
```

**特徴**:
- 型安全: 型から自動的にサイズを計算
- 構造体の自動初期化: メンバーがゼロクリアされる
- ゼロクリア: 確保したメモリは自動的に0で初期化される

**注意**: コンストラクタ引数付きの構文(`new T(args)`)は将来実装予定です。

#### delete 演算子 - 型安全な解放（v0.11.0 Phase 1aで実装済み）

**実装状態**: ✅ v0.11.0 Phase 1aで実装済み

**構文**:
```cb
delete ptr;  // 単一オブジェクトも配列も統一構文
```

**使用例**:
```cb
struct Resource {
    int id;
}

impl Resource {
    self() {
        self.id = 0;
    }
}

int main() {
    // newで確保
    Resource* r = new Resource;
    r->id = 1;
    
    println("Resource ID: {r->id}");
    
    // deleteで解放
    delete r;
    
    // 配列の解放も同じ構文
    int* arr = new int[10];
    delete arr;
    
    return 0;
}
```

**特徴**:
- nullptr セーフ: nullptrの解放は何もしない
- 自動判別: 構造体ポインタと生ポインタを自動で識別
- 統一構文: 配列もオブジェクトも同じ`delete ptr`構文

**注意**: デストラクタの自動呼び出しは将来実装予定です。現在は手動で後処理を行う必要があります。

**malloc/free vs new/delete**:

| 機能 | malloc/free | new/delete |
|------|-------------|------------|
| 型安全性 | ❌ なし（キャスト必要） | ✅ あり（型から自動計算） |
| 初期化 | ❌ なし（ゴミデータ） | ✅ ゼロクリア |
| コンストラクタ | ❌ 呼ばれない | ⚠️ 将来実装予定 |
| デストラクタ | ❌ 呼ばれない | ⚠️ 将来実装予定 |
| サイズ指定 | 手動（バイト数） | 自動（型から計算） |
| 実装状態 | ✅ v0.11.0で実装済み | ✅ v0.11.0 Phase 1aで実装済み |

**注意**: コンストラクタ/デストラクタの自動呼び出しは将来のバージョンで実装予定です。現在は手動で初期化・後処理を行う必要があります。

### 将来実装: スマートポインタ（v0.13.0以降で実装予定）

```cb
// 将来実装予定
let data: unique_ptr<Data> = make_unique<Data>();
let resource: shared_ptr<Resource> = make_shared<Resource>();
```

---

## 標準ライブラリ

### collections.vector - Vector<T>

v0.11.0で双方向リンクリストに完全リファクタリング。

#### 基本的な使い方

```cb
// collections.vectorモジュールのインポート
import collections.vector;

int main() {
    Vector<int> vec = Vector::new();
    
    // 要素の追加
    vec.push_back(10);
    vec.push_back(20);
    vec.push_back(30);
    
    // 要素数の取得
    int len = vec.get_length();  // 3
    
    // 要素の取得
    int first = vec.get(0);   // 10
    int last = vec.get(2);    // 30
    
    return 0;
}
```

#### データ構造

**ノードベース双方向リンクリスト**:
```
[Node]       [Node]       [Node]
┌──────┐    ┌──────┐    ┌──────┐
│ prev │◄───│ prev │◄───│ prev │
├──────┤    ├──────┤    ├──────┤
│ next │───►│ next │───►│ next │
├──────┤    ├──────┤    ├──────┤
│ data │    │ data │    │ data │
└──────┘    └──────┘    └──────┘
   ^                        ^
   │                        │
  head                    tail
```

**ノードメモリレイアウト**:
```
[prev (8 bytes)][next (8 bytes)][data (sizeof(T) bytes)]
```

#### 主要メソッド

| メソッド | 時間計算量 | 説明 |
|---------|-----------|------|
| `push_back(T value)` | O(1) | 末尾に要素追加 |
| `push_front(T value)` | O(1) | 先頭に要素追加 ✨新規 |
| `pop_back()` | O(1) | 末尾要素を削除 |
| `pop_front()` | O(1) | 先頭要素を削除 ✨新規 |
| `get(int index)` | O(n) | 要素取得（インデックス指定） |
| `set(int index, T value)` | O(n) | 要素更新 |
| `delete_at(int index)` | O(n) | 任意位置の要素削除 ✨新規 |
| `find(T value)` | O(n) | 要素検索 ✨新規 |
| `sort()` | O(n²) | ソート（バブルソート） ✨新規 |
| `get_length()` | O(1) | 要素数取得 |
| `clear()` | O(n) | 全要素削除 |

#### API変更（v0.11.0）

**削除されたメソッド**:
- `init()` - コンストラクタで代替
- `reserve()` - リンクリストでは不要
- `get_capacity()` - リンクリストでは不要

**新規追加メソッド**:
- `push_front()` - O(1)での先頭追加
- `pop_front()` - O(1)での先頭削除
- `delete_at()` - 任意位置の削除
- `find()` - 線形探索
- `sort()` - バブルソート実装

#### 使用例

**先頭への追加**:
```cb
Vector<int> vec;
vec.push_front(10);  // [10]
vec.push_front(20);  // [20, 10]
vec.push_front(30);  // [30, 20, 10]
```

**要素の検索**:
```cb
Vector<int> vec;
vec.push_back(10);
vec.push_back(20);
vec.push_back(30);

int index = vec.find(20);  // 1
```

**要素の削除**:
```cb
Vector<int> vec;
vec.push_back(10);
vec.push_back(20);
vec.push_back(30);

vec.delete_at(1);  // [10, 30]
```

**ソート**:
```cb
Vector<int> vec;
vec.push_back(30);
vec.push_back(10);
vec.push_back(20);

vec.sort();  // [10, 20, 30]
```

#### パフォーマンス特性

**v0.10.0（配列ベース） vs v0.11.0（リンクリスト）**:

| 操作 | v0.10.0 | v0.11.0 |
|-----|---------|---------|
| 先頭追加 | O(n) | O(1) ✨ |
| 末尾追加 | O(1) | O(1) |
| 先頭削除 | O(n) | O(1) ✨ |
| 末尾削除 | O(1) | O(1) |
| ランダムアクセス | O(1) | O(n) |
| メモリ | 連続 | 非連続 |

#### アロケータ対応

```cb
import std.allocators.system;

Vector<int> vec;
vec.allocator = SystemAllocator::new();
```

---

## テストフレームワーク

### テストの実行

```bash
# 全テスト実行
make test

# 統合テストのみ
make integration-test

# 単体テストのみ
make unit-test
```

### テスト統計

| バージョン | 統合テスト | 単体テスト | 合計 | 成功率 |
|-----------|-----------|-----------|------|--------|
| v0.9.0 | 2,349 | 30 | 2,379 | 100% |
| v0.9.1 | 2,447 | 30 | 2,477 | 100% |
| v0.9.2 | 2,798 | 30 | 2,828 | 100% |
| v0.10.0 | 2,924 | 30 | 2,954 | 100% |
| **v0.11.0** | **3,341** | **30** | **3,371** | **100%** |

### テストケースの構造

```
tests/
├── cases/
│   ├── pointer/               # ポインタ関連テスト
│   ├── array/                 # 配列テスト
│   ├── struct/                # 構造体テスト
│   ├── interface/             # Interface/Implテスト
│   └── ...
└── integration/
    ├── pointer/               # ポインタ統合テスト
    ├── array/                 # 配列統合テスト
    └── ...
```

---

## 実装状況サマリー

### ✅ 完全実装済み（v0.9.0）

#### 型システム
- **基本型**: tiny, short, int, long, char, string, bool
- **浮動小数点数型**: float, double（演算、配列、構造体メンバー）
- **符号なし整数型**: unsigned修飾子（自動クランプ機能付き）
- **配列型**: 静的配列、多次元配列、配列リテラル
- **構造体**: 定義、初期化、ネストした構造体（多階層対応）
- **Union型**: TypeScript風Union型、型安全性
- **Interface/Impl**: ポリモーフィズム、型抽象化
- **enum型**: 列挙型、自動値割り当て、スコープアクセス
- **typedef**: 型エイリアス、配列型エイリアス、再帰的typedef

#### ポインタシステム
- **宣言と初期化**: `int* ptr = &value;`
- **演算**: `ptr++`, `ptr--`, `ptr + n`, `ptr - n`
- **デリファレンス**: `*ptr` による値の取得・変更
- **アドレス演算子**: `&variable` でアドレス取得
- **16進数表示**: `0x[hex]` 形式での表示
- **構造体ポインタ**: `(*ptr).member` および `ptr->member`
- **Interfaceポインタ**: ポリモーフィックメソッド呼び出し
- **ポインタ配列**: 複数ポインタ管理
- **ネストアクセス**: `(*(*p).nested).value`

#### 演算子
- **算術演算子**: `+`, `-`, `*`, `/`, `%`
- **比較演算子**: `==`, `!=`, `<`, `>`, `<=`, `>=`
- **論理演算子**: `&&`, `||`, `!`
- **ビット演算子**: `&`, `|`, `^`, `~`, `<<`, `>>`
- **複合代入演算子（10種類）**: `+=`, `-=`, `*=`, `/=`, `%=`, `&=`, `|=`, `^=`, `<<=`, `>>=`
- **インクリメント・デクリメント**: 前置 `++x`, `--x` / 後置 `x++`, `x--`
- **三項演算子**: `condition ? true_val : false_val`

#### 制御構造
- **条件分岐**: if/else, else if
- **ループ**: for, while
- **ループ制御**: break, continue
- **関数**: 定義、呼び出し、戻り値、再帰、配列戻り値

#### その他
- **モジュールシステム**: import/export
- **入出力**: println, print（printf風フォーマット）
- **エラーハンドリング**: 型チェック、境界チェック、多言語デバッグ（英語・日本語）

### 🚧 将来実装予定

詳細は [`future_features.md`](future_features.md) を参照してください。

- **参照型**: `int&` による参照渡し
- **動的メモリ管理**: `new`/`delete` 文
- **スマートポインタ**: `unique_ptr`, `shared_ptr`
- **関数ポインタ**: コールバック機能
- **ジェネリクス・テンプレート**: 型パラメータ
- **非同期処理**: goroutine風の並行処理
- **標準ライブラリの拡充**: コレクション、I/O、ネットワーク

---

## 付録

### コーディング規約

#### 命名規則

- **変数・関数**: snake_case
- **型・構造体**: PascalCase
- **定数**: UPPER_CASE

```c++
// 良い例
int user_count;
void process_data();
struct UserProfile;
const int MAX_SIZE = 100;

// 悪い例
int UserCount;          // 変数はsnake_case
void ProcessData();     // 関数はsnake_case
struct user_profile;    // 型はPascalCase
const int maxSize = 100; // 定数はUPPER_CASE
```

#### インデント

- スペース4つを推奨
- 一貫性を保つ

#### コメント

```c++
// 単行コメント

/*
 * 複数行コメント
 * 詳細な説明
 */
```

### パフォーマンスガイドライン

1. **配列サイズ**: コンパイル時に決定される静的配列を使用
2. **ポインタ**: 大きな構造体の受け渡しには参照やポインタを推奨
3. **const**: 変更しない値にはconstを付けて最適化を促進

### 関連リソース

- **仕様書**: `docs/spec.md`
- **Interfaceシステム詳細**: `docs/interface_system.md`
- **リリースノート**: `release_notes/`
- **サンプルコード**: `sample/`
- **テストケース**: `tests/cases/`

---

## 変更履歴

### v0.11.0 Part 1a（2025年11月3日）

**主要な変更**:

1. **Vector<T>の双方向リンクリスト実装**
   - 配列ベースからノードベース双方向リンクリストに完全リファクタリング
   - O(1)での先頭・末尾操作を実現
   - 新規API追加: `push_front()`, `pop_front()`, `delete_at()`, `find()`, `sort()`
   - 削除API: `init()`, `reserve()`, `get_capacity()`

2. **import文の文字列リテラル構文廃止**
   - 文字列リテラル構文（`import "path/to/file.cb";`）を廃止
   - モジュールパス構文（`import module.path.name;`）に統一
   - 41ファイルを更新

3. **テスト拡充**
   - 統合テスト: 2,924個 → 3,341個（+417個）
   - すべてのテストが100%合格

### v0.11.0（2025年10月28日）

**主要な機能追加**:

1. **ジェネリクスシステム**
   - 構造体、関数、enumでのジェネリクス対応
   - 型パラメータ、インスタンス化、型名マングリング

2. **文字列補間**
   - `{}` 内での式埋め込み
   - フォーマット指定子（`:d`, `:x`, `:.2f` など）

3. **デストラクタとRAII**
   - `fn deinit()` 構文
   - LIFO順序での自動呼び出し
   - defer文との統合

4. **パターンマッチング**
   - `match` 文の実装
   - Option<T>とResult<T, E>のマッチング
   - ワイルドカードパターン

### v0.10.0（2025年10月20日）

**主要な機能追加**:

1. **右辺値参照とムーブセマンティクス**
   - `&&` 構文
   - ムーブコンストラクタ
   - `move()` 関数

2. **デフォルト引数**
   - 関数パラメータのデフォルト値
   - コンストラクタでのデフォルト引数

### v0.9.2（2025年10月15日）

**機能追加**:
- 配列の参照渡し（自動）
- 配列を返す関数のサポート強化

### v0.9.1（2025年10月10日）

**機能追加**:
- Const Pointer Safety
- impl内static変数

### v0.9.0（2025年10月5日）

**主要な機能追加**:
- ポインタシステム完全実装
- 浮動小数点数型（float, double）
- ネストした構造体（多階層対応）

---

**ドキュメントバージョン**: v0.11.0 Part 1a  
**最終更新日**: 2025年11月3日  
**言語バージョン**: Cb v0.11.0 - Generics, String Interpolation, Destructors & Vector Refactoring
