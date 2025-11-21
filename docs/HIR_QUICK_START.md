# HIRコンパイラ クイックスタート

## HIRとは？

HIR (High-level Intermediate Representation) は、Cbの抽象構文木（AST）とC++コード生成の間に位置する中間表現です。

```
Cbソース → AST → HIR → C++コード → ネイティブバイナリ
```

## 使い方

### コンパイルして実行

```bash
# Cbプログラムをコンパイル
./cb compile program.cb -o output

# 実行
./output
```

### インタープリタモード（開発用）

```bash
# 直接実行（HIRを経由しない）
./cb program.cb
```

## サポートされている基本構文

### ✅ 完全サポート

#### 変数と型
```cb
int x = 10;
double d = 3.14;
float f = 2.718;
bool b = true;
string s = "hello";
```

#### 演算子
```cb
// 算術
int result = a + b - c * d / e % f;

// 比較
bool cmp = x > y && z <= w;

// 論理
bool logic = a && b || !c;

// インクリメント/デクリメント
i++;
++i;
i--;
--i;

// 三項演算子
int max = a > b ? a : b;
```

#### 制御構造
```cb
// if-else
if (condition) {
    // ...
} else {
    // ...
}

// while
while (condition) {
    // ...
}

// for
for (int i = 0; i < 10; i = i + 1) {
    // ...
}

// break, continue
for (int i = 0; i < 100; i = i + 1) {
    if (i == 50) break;
    if (i % 2 == 0) continue;
}
```

#### 関数
```cb
int add(int a, int b) {
    return a + b;
}

void print_hello() {
    println("Hello!");
}

// 再帰関数
int factorial(int n) {
    if (n <= 1) return 1;
    return n * factorial(n - 1);
}
```

#### 構造体
```cb
struct Point {
    int x;
    int y;
};

struct Circle {
    Point center;
    int radius;
};

void main() {
    Point p;
    p.x = 10;
    p.y = 20;
    
    Circle c;
    c.center.x = 0;
    c.center.y = 0;
    c.radius = 5;
}
```

#### 列挙型
```cb
enum Status {
    OK,
    ERROR,
    PENDING
};

void main() {
    Status s = Status.OK;
}
```

#### ジェネリクス
```cb
struct Box<T> {
    T value;
};

T max<T>(T a, T b) {
    return a > b ? a : b;
}

void main() {
    Box<int> box;
    box.value = 42;
    
    int m = max<int>(10, 20);
}
```

#### FFI (外部関数インターフェース)
```cb
use foreign.m {
    double sqrt(double x);
    double pow(double base, double exp);
}

void main() {
    double result = m.sqrt(16.0);
    println(result);  // 4.0
}
```

#### プリプロセッサ
```cb
#define DEBUG
#define VERSION "1.0.0"

#ifdef DEBUG
    println("Debug mode");
#endif

#ifndef RELEASE
    #warning "Not in release mode"
#endif
```

## テスト結果

### HIR統合テスト
- **全85テスト成功** (100%)
  - HIR基本: 2/2 ✅
  - println: 4/4 ✅
  - ジェネリクス: 47/47 ✅
  - FFI: 10/10 ✅
  - プリプロセッサ: 21/21 ✅

### 標準ライブラリテスト
- **全33テスト成功** (100%)
  - Allocator: 2/2 ✅
  - Vector: 11/11 ✅
  - Queue: 5/5 ✅
  - Map: 7/7 ✅
  - Async/Await: 8/8 ✅

## 完全な例

```cb
// 素数判定プログラム
void main() {
    println("Prime numbers from 2 to 50:");
    
    for (int n = 2; n <= 50; n = n + 1) {
        bool is_prime = true;
        
        int i = 2;
        while (i * i <= n) {
            if (n % i == 0) {
                is_prime = false;
                break;
            }
            i = i + 1;
        }
        
        if (is_prime) {
            println(n);
        }
    }
}
```

実行方法：
```bash
./cb compile prime.cb -o prime
./prime
```

## 既知の制限

### ⚠️ 部分サポート

#### Interface/Impl の多態性
インターフェースを使った多態性は現在開発中です。
```cb
interface Shape {
    int area();
}

impl Shape for Rectangle {
    // ...
}

void main() {
    Rectangle rect;
    Shape shape = rect;  // ⚠️ 現在サポートされていません
}
```

**回避策**: 現在のところ、構造体を直接使用してください。

#### 固定サイズ型 (int8, uint32など)
基本型 (`int`, `double`, `float`, `bool`, `string`) を使用してください。

### 🚫 サポートされていない機能

- ポインタ操作（一部のみサポート）
- ユニオン型（一部のみサポート）
- 複雑な配列操作（基本は動作）

## トラブルシューティング

### コンパイルエラー

**エラー**: `Unexpected token`
```
Solution: 構文を確認してください。セミコロンや中括弧の欠落が一般的な原因です。
```

**エラー**: `Undefined type`
```
Solution: 型名が正しいか確認してください。int8/uint32などは現在サポートされていません。
```

### 実行時エラー

**問題**: 構造体のパラメータ渡しでエラー
```
Solution: インタープリタモードではなく、コンパイラモードを使用してください：
./cb compile program.cb -o output
```

## より詳しい情報

- **HIR実装状況**: `HIR_STATUS_BASIC_SYNTAX.md`
- **HIR設計ドキュメント**: `HIR_COMPILER_STATUS.md`
- **統合テスト**: `tests/integration/run_hir_tests.sh`

## 次のステップ

1. **簡単なプログラムを書く**: 基本的な演算と制御構造から始めましょう
2. **構造体を使う**: データ構造を定義して使用してみましょう
3. **関数を定義する**: コードを関数に分割して整理しましょう
4. **ジェネリクスを試す**: 汎用的なデータ構造を作成しましょう
5. **FFIを使う**: C/C++ライブラリの関数を呼び出してみましょう

Happy coding with Cb! 🎉
