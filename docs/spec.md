# Cb言語 完全仕様書 v0.9.0

**最終更新**: 2025年10月5日  
**バージョン**: v0.9.0 - ポインタシステム完全実装版

## 目次

1. [言語概要](#言語概要)
2. [型システム](#型システム)
3. [変数と宣言](#変数と宣言)
4. [演算子](#演算子)
5. [制御構造](#制御構造)
6. [関数](#関数)
7. [配列](#配列)
8. [構造体](#構造体)
9. [Union型](#union型)
10. [Interface/Implシステム](#interfaceimplシステム)
11. [ポインタと参照](#ポインタと参照)
12. [モジュールシステム](#モジュールシステム)
13. [入出力](#入出力)
14. [エラーハンドリング](#エラーハンドリング)
15. [メモリ管理](#メモリ管理)

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

**制限事項**:
- 配列参照型（`int[N]&`）は現在サポートされていません
- 参照のポインタ（`int&*`）はサポートされていません
- 参照の参照（`int&&`）はサポートされていません

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

```

---

## モジュールシステム

### モジュールのインポート

```c++
import "math.cb";
import "utils.cb";

int main() {
    int result = math_add(5, 3);
    return 0;
}
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

### 将来実装: スマートポインタ

```c++
// 将来実装予定
unique_ptr<Data> data = make_unique<Data>();
shared_ptr<Resource> resource = make_shared<Resource>();
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

- **統合テスト**: 2349個（100%成功）
- **単体テストト**: 30個（100%成功）
- **総カバレッジ**: 全機能をカバー

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

**ドキュメントバージョン**: v0.9.0  
**最終更新日**: 2025年10月5日  
**言語バージョン**: Cb v0.9.0 - ポインタシステム完全実装版
