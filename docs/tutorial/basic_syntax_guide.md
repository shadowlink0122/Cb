# Cb言語 基本構文ガイド

このガイドでは、Cb言語の基本的な構文と使い方を学びます。

## 目次

1. [プログラムの基本構造](#1-プログラムの基本構造)
2. [変数と型](#2-変数と型)
3. [演算子](#3-演算子)
4. [制御構造](#4-制御構造)
5. [関数](#5-関数)
6. [配列](#6-配列)
7. [構造体](#7-構造体)
8. [ポインタ](#8-ポインタ)
9. [Interface/Impl](#9-interfaceimpl)
10. [よくある間違い](#10-よくある間違い)

---

## 1. プログラムの基本構造

Cbプログラムは、`main`関数から始まります。

```cb
func int main() {
    println("Hello, Cb!");
    return 0;
}
```

### ポイント
- `func` キーワードで関数を定義
- `main` 関数は必須
- `return 0;` で正常終了を示す
- セミコロン(`;`)で文を終了

---

## 2. 変数と型

### 2.1 基本型

```cb
func int main() {
    // 整数型
    tiny t = 100;      // 8bit整数 (-128 ~ 127)
    short s = 1000;    // 16bit整数 (-32768 ~ 32767)
    int i = 100000;    // 32bit整数
    long l = 10000000; // 64bit整数
    
    // 浮動小数点型
    float f = 3.14;    // 32bit浮動小数点
    double d = 2.718;  // 64bit浮動小数点
    
    // 文字列・文字・真偽値
    string str = "Hello";
    char c = 'A';
    bool flag = true;
    
    return 0;
}
```

### 2.2 unsigned修飾子

```cb
func int main() {
    unsigned tiny ut = 255;
    unsigned int ui = 4000000000;
    
    // 負値は自動的に0にクランプ（警告が出る）
    unsigned int x = -10;  // x は 0 になる
    
    return 0;
}
```

### 2.3 const修飾子

```cb
func int main() {
    const int MAX = 100;
    // MAX = 200;  // エラー: constは変更できない
    
    int value = 42;
    const int* ptr = &value;
    // *ptr = 100;  // エラー: const経由の変更は禁止
    
    return 0;
}
```

---

## 3. 演算子

### 3.1 算術演算子

```cb
func int main() {
    int a = 10;
    int b = 3;
    
    int sum = a + b;      // 13
    int diff = a - b;     // 7
    int prod = a * b;     // 30
    int quot = a / b;     // 3
    int rem = a % b;      // 1
    
    return 0;
}
```

### 3.2 比較演算子

```cb
func int main() {
    int x = 10;
    int y = 20;
    
    bool eq = (x == y);   // false
    bool ne = (x != y);   // true
    bool lt = (x < y);    // true
    bool gt = (x > y);    // false
    bool le = (x <= y);   // true
    bool ge = (x >= y);   // false
    
    return 0;
}
```

### 3.3 論理演算子

```cb
func int main() {
    bool a = true;
    bool b = false;
    
    bool and_result = a && b;  // false
    bool or_result = a || b;   // true
    bool not_result = !a;      // false
    
    return 0;
}
```

### 3.4 ビット演算子

```cb
func int main() {
    int a = 12;  // 0b1100
    int b = 10;  // 0b1010
    
    int and_bit = a & b;   // 0b1000 = 8
    int or_bit = a | b;    // 0b1110 = 14
    int xor_bit = a ^ b;   // 0b0110 = 6
    int not_bit = ~a;      // ビット反転
    int left = a << 1;     // 0b11000 = 24
    int right = a >> 1;    // 0b0110 = 6
    
    return 0;
}
```

### 3.5 複合代入演算子

```cb
func int main() {
    int x = 10;
    
    x += 5;   // x = x + 5;  → 15
    x -= 3;   // x = x - 3;  → 12
    x *= 2;   // x = x * 2;  → 24
    x /= 4;   // x = x / 4;  → 6
    x %= 4;   // x = x % 4;  → 2
    
    x &= 3;   // x = x & 3;  → 2
    x |= 4;   // x = x | 4;  → 6
    x ^= 2;   // x = x ^ 2;  → 4
    x <<= 1;  // x = x << 1; → 8
    x >>= 2;  // x = x >> 2; → 2
    
    return 0;
}
```

### 3.6 インクリメント・デクリメント

```cb
func int main() {
    int x = 10;
    
    // 前置
    int a = ++x;  // x = 11, a = 11
    int b = --x;  // x = 10, b = 10
    
    // 後置
    int c = x++;  // c = 10, x = 11
    int d = x--;  // d = 11, x = 10
    
    return 0;
}
```

### 3.7 三項演算子

```cb
func int main() {
    int x = 10;
    int y = 20;
    
    int max = (x > y) ? x : y;  // max = 20
    
    println("Max:", max);
    return 0;
}
```

---

## 4. 制御構造

### 4.1 if文

```cb
func int main() {
    int score = 85;
    
    if (score >= 90) {
        println("優秀");
    } else if (score >= 70) {
        println("良好");
    } else if (score >= 60) {
        println("合格");
    } else {
        println("不合格");
    }
    
    return 0;
}
```

### 4.2 for文

```cb
func int main() {
    // 基本的なforループ
    for (int i = 0; i < 10; i++) {
        println(i);
    }
    
    // ネストしたループ
    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 3; j++) {
            println(i, j);
        }
    }
    
    return 0;
}
```

### 4.3 while文

```cb
func int main() {
    int count = 0;
    
    while (count < 5) {
        println("Count:", count);
        count++;
    }
    
    return 0;
}
```

### 4.4 break/continue

```cb
func int main() {
    // break: ループを抜ける
    for (int i = 0; i < 10; i++) {
        if (i == 5) {
            break;
        }
        println(i);  // 0, 1, 2, 3, 4
    }
    
    // continue: 次の繰り返しへ
    for (int i = 0; i < 10; i++) {
        if (i % 2 == 0) {
            continue;
        }
        println(i);  // 1, 3, 5, 7, 9
    }
    
    return 0;
}
```

---

## 5. 関数

### 5.1 基本的な関数

```cb
func int add(int a, int b) {
    return a + b;
}

func void greet(string name) {
    println("Hello,", name, "!");
}

func int main() {
    int sum = add(10, 20);
    println("Sum:", sum);
    
    greet("World");
    
    return 0;
}
```

### 5.2 再帰関数

```cb
func int factorial(int n) {
    if (n <= 1) {
        return 1;
    }
    return n * factorial(n - 1);
}

func int main() {
    int result = factorial(5);
    println("5! =", result);  // 120
    return 0;
}
```

### 5.3 参照渡し

```cb
func void increment(int& ref) {
    ref++;
}

func int main() {
    int value = 10;
    increment(value);
    println(value);  // 11
    return 0;
}
```

---

## 6. 配列

### 6.1 1次元配列

```cb
func int main() {
    // 宣言と初期化
    int[5] arr = [10, 20, 30, 40, 50];
    
    // アクセス
    println(arr[0]);  // 10
    arr[2] = 100;
    println(arr[2]);  // 100
    
    // ループでアクセス
    for (int i = 0; i < 5; i++) {
        println(arr[i]);
    }
    
    return 0;
}
```

### 6.2 多次元配列

```cb
func int main() {
    // 2次元配列
    int[3][3] matrix = [
        [1, 2, 3],
        [4, 5, 6],
        [7, 8, 9]
    ];
    
    // アクセス
    println(matrix[1][1]);  // 5
    
    // ネストループ
    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 3; j++) {
            print("%d ", matrix[i][j]);
        }
        println("");
    }
    
    return 0;
}
```

### 6.3 配列とポインタ

```cb
func int main() {
    int[5] arr = [1, 2, 3, 4, 5];
    int* ptr = &arr[0];
    
    // ポインタ演算
    println(*ptr);      // 1
    println(*(ptr+1));  // 2
    println(*(ptr+2));  // 3
    
    return 0;
}
```

---

## 7. 構造体

### 7.1 基本的な構造体

```cb
struct Point {
    int x;
    int y;
};

func int main() {
    Point p;
    p.x = 10;
    p.y = 20;
    
    println("Point:", p.x, p.y);
    
    return 0;
}
```

### 7.2 ネストした構造体

```cb
struct Address {
    string city;
    int zip;
};

struct Person {
    string name;
    int age;
    Address addr;
};

func int main() {
    Person person;
    person.name = "Alice";
    person.age = 30;
    person.addr.city = "Tokyo";
    person.addr.zip = 1000001;
    
    println(person.name, "lives in", person.addr.city);
    
    return 0;
}
```

### 7.3 構造体配列

```cb
struct Point {
    int x;
    int y;
};

func int main() {
    Point[3] points;
    
    points[0].x = 0;
    points[0].y = 0;
    points[1].x = 10;
    points[1].y = 20;
    points[2].x = 30;
    points[2].y = 40;
    
    for (int i = 0; i < 3; i++) {
        println("Point", i, ":", points[i].x, points[i].y);
    }
    
    return 0;
}
```

---

## 8. ポインタ

### 8.1 基本的なポインタ

```cb
func int main() {
    int value = 42;
    int* ptr = &value;  // アドレス取得
    
    println("Value:", value);     // 42
    println("Address:", ptr);     // 0x...
    println("Deref:", *ptr);      // 42
    
    *ptr = 100;  // ポインタ経由で変更
    println("Value:", value);     // 100
    
    return 0;
}
```

### 8.2 ポインタ演算

```cb
func int main() {
    int[5] arr = [10, 20, 30, 40, 50];
    int* ptr = &arr[0];
    
    println(*ptr);      // 10
    ptr++;              // 次の要素へ
    println(*ptr);      // 20
    ptr = ptr + 2;      // 2つ進む
    println(*ptr);      // 40
    
    return 0;
}
```

### 8.3 構造体ポインタ

```cb
struct Point {
    int x;
    int y;
};

func int main() {
    Point p;
    p.x = 10;
    p.y = 20;
    
    Point* ptr = &p;
    
    // デリファレンス構文
    (*ptr).x = 30;
    
    // アロー演算子（簡潔）
    ptr->y = 40;
    
    println(p.x, p.y);  // 30 40
    
    return 0;
}
```

### 8.4 関数ポインタ

```cb
func int add(int a, int b) {
    return a + b;
}

func int multiply(int a, int b) {
    return a * b;
}

func int main() {
    // 関数ポインタの宣言
    int(*op)(int, int) = &add;
    
    // 呼び出し
    int result = op(5, 3);
    println(result);  // 8
    
    // 再代入
    op = &multiply;
    result = op(5, 3);
    println(result);  // 15
    
    return 0;
}
```

---

## 9. Interface/Impl

### 9.1 基本的なInterface

```cb
interface Printable {
    void print();
};

struct Message {
    string text;
};

impl Printable for Message {
    void print() {
        println("Message:", self.text);
    }
};

func int main() {
    Message msg;
    msg.text = "Hello";
    msg.print();  // Message: Hello
    
    return 0;
}
```

### 9.2 ポリモーフィズム

```cb
interface Shape {
    int area();
};

struct Rectangle {
    int width;
    int height;
};

struct Circle {
    int radius;
};

impl Shape for Rectangle {
    int area() {
        return self.width * self.height;
    }
};

impl Shape for Circle {
    int area() {
        return 3 * self.radius * self.radius;
    }
};

func int main() {
    Rectangle rect;
    rect.width = 10;
    rect.height = 5;
    
    Circle circle;
    circle.radius = 7;
    
    println("Rectangle area:", rect.area());  // 50
    println("Circle area:", circle.area());   // 147
    
    return 0;
}
```

---

## 10. よくある間違い

### 10.1 セミコロン忘れ

```cb
// ❌ 間違い
int x = 10

// ✅ 正しい
int x = 10;
```

### 10.2 構造体定義後のセミコロン

```cb
// ❌ 間違い
struct Point {
    int x;
    int y;
}

// ✅ 正しい
struct Point {
    int x;
    int y;
};
```

### 10.3 配列の境界外アクセス

```cb
// ❌ 実行時エラー
int[5] arr = [1, 2, 3, 4, 5];
int x = arr[10];  // エラー: 境界外アクセス

// ✅ 正しい
int x = arr[4];  // 最後の要素
```

### 10.4 const変数の変更

```cb
// ❌ コンパイルエラー
const int MAX = 100;
MAX = 200;  // エラー: constは変更できない

// ✅ 正しい
int max = 100;
max = 200;  // OK
```

### 10.5 未初期化変数の使用

```cb
// ❌ 値が不定
int x;
println(x);  // 未初期化

// ✅ 正しい
int x = 0;
println(x);  // 0
```

### 10.6 return文の忘れ

```cb
// ❌ 戻り値がない
func int getValue() {
    int x = 42;
    // return忘れ
}

// ✅ 正しい
func int getValue() {
    int x = 42;
    return x;
}
```

### 10.7 ポインタのnullチェック忘れ

```cb
// ❌ nullポインタ参照
int* ptr = nullptr;
int x = *ptr;  // エラー

// ✅ 正しい
int* ptr = nullptr;
if (ptr != nullptr) {
    int x = *ptr;
}
```

---

## まとめ

このガイドでは、Cb言語の基本的な構文を学びました。

### 次のステップ

1. [サンプルコード集](sample_code_collection.md)で実践的な例を見る
2. [言語仕様書](../spec.md)で詳細を確認
3. 自分でプログラムを書いて試す

### 参考資料

- [Cb言語仕様書](../spec.md)
- [BNF文法定義](../BNF.md)
- [サンプルコード](../../sample/)

Happy Coding! 🎉
