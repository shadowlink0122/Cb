# Cb (シーフラット) プログラミング言語

**最新バージョン**: v0.10.0 - Move Semantics & Complete Cleanup  
**リリース日**: 2025年10月12日  
**ベース**: v0.9.2

### 📊 品質指標（v0.10.0）

- **統合テスト**: **2,924個**（100%成功） 🎉
- **Deferテスト**: **131個**（return前cleanup含む） 🎉
- **ユニットテスト**: **30個**（100%成功） 🎉
- **総テスト数**: **2,954個**（100%成功） 🎉
- **テストカバレッジ**: 全機能を網羅的にテスト
- **完全なRIII**: デストラクタとdeferの自動実行

### 🆕 v0.10.0の新機能

**1. � C++互換ムーブセマンティクス**
- **右辺値参照（`T&&`）**: ゼロコストの所有権移動
- **ムーブコンストラクタ**: `self(T&& other)` による効率的なリソース転送
- **move()組み込み関数**: 明示的なムーブ操作
- **デストラクタからの自動削除**: ムーブされたオブジェクトは破棄されない

```cb
struct Resource {
    int id;
    bool allocated;
    
    // ムーブコンストラクタ
    self(Resource&& other) {
        self.id = other.id;
        self.allocated = other.allocated;
        other.allocated = false;  // 元のオブジェクトを無効化
        println("Move constructor");
    }
    
    ~self() {
        if (self.allocated) {
            println("Resource destroyed", self.id);
        }
    }
}

int main() {
    Resource r1(1);
    Resource r2 = move(r1);  // ムーブ、r1は無効化される
}
// 出力:
// Move constructor
// Resource destroyed 1  ← r2のみが破棄される
```

**2. 🔄 return文直前のdefer/デストラクタ実行**
- **スコープ終了としてのreturn**: return実行前に自動クリーンアップ
- **実行順序の保証**: defer文（LIFO）→ デストラクタ（LIFO）→ return
- **複数return経路対応**: どのreturn文でも同じクリーンアップルール

```cb
Resource create_resource() {
    Resource r(1);
    defer println("Defer statement");
    return r;  // ✅ return前にdefer→デストラクタ実行
}

int main() {
    Resource r = create_resource();
    println("After create");
}
// 出力:
// Defer statement      ← return前に実行
// Resource destroyed 1 ← return前に実行
// After create
// Resource destroyed 1 ← main終了時
```

**3. � impl構文のexport/import対応**
- **implブロックのexport**: コンストラクタ、インターフェース実装をモジュール間で共有
- **モジュール境界を越えた実装**: 構造体定義と実装を分離可能

```cb
// module_a.cb
export struct Point { int x; int y; }

export impl Point {
    self(int px, int py) {
        self.x = px;
        self.y = py;
    }
}

// main.cb
import "module_a.cb";

int main() {
    Point p(10, 20);  // ✅ エクスポートされたコンストラクタを使用
}
```

**4. 🏗️ ネスト構造体デストラクタ**
- **値メンバーのデストラクタ**: 構造体が値メンバーを持つ場合、自動的にデストラクタを呼び出し
- **LIFO順の保証**: メンバーは宣言の逆順で破棄される

```cb
struct Inner {
    int id;
    ~self() { println("Inner destroyed", self.id); }
}

struct Outer {
    Inner inner;
    ~self() { println("Outer destroyed"); }
}

int main() {
    Outer o;
    o.inner.id = 1;
}
// 出力:
// Outer destroyed
// Inner destroyed 1  ← 値メンバーのデストラクタも自動実行
```

**5. テストの拡充**
- 全2,954テストが100%成功
- 新規追加: 126テスト（defer 131個、impl export/import 7個）
- ムーブセマンティクスの包括的なテスト

詳細: [`release_notes/v0.10.0.md`](release_notes/v0.10.0.md)

---

### 🔥 v0.9.2の主要機能

**1. 🔴 Long型オーバーフロー修正（重要なバグ修正）**
- Fibonacci計算でF(48)以降が正しく計算されない問題を修正
- `TYPE_LONG`配列の読み取りを64bit値として正しく処理
- 大きな整数リテラル(>INT32_MAX)を自動的にTYPE_LONGに分類

**2. 🛡️ Const型安全性の部分実装**
- `const T*` (pointed-to constness) のチェック
- `T* const` (pointer constness) のチェック
- 関数パラメータでのconst違反検出

**3. 🔄 配列の参照渡し（関数引数）**
- 関数に配列を渡す際、C/C++と同様に参照として渡される
- 関数内での配列要素の変更が呼び出し元に反映される

**4. 🏗️ 構造体メンバの再帰的代入とネスト初期化**
- ネストした構造体メンバへの直接代入をサポート
- 4レベル以上の深いネストにも対応
- 宣言時のメンバーアクセス初期化に対応

詳細: [`release_notes/v0.9.2.md`](release_notes/v0.9.2.md)

---

### 🔥 v0.9.1の主要機能

**1. Constポインタ安全性機能**
- `const T*` - ポインタ経由の変更を禁止
- `T* const` - ポインタの再代入を禁止
- `const T* const` - 両方を禁止
- コンパイル時のconst違反検出
- 包括的なエラーメッセージ

**2. 多次元配列ポインタ**
- 2D/3D配列要素へのポインタ取得 (`&matrix[i][j]`)
- ポインタ演算によるメモリアクセス
- Row-major orderのフラットメモリレイアウト

**3. 大規模リファクタリング（Phase 5-8）**
- ディレクトリ構造の完全な再編成
- DRY原則の適用とコード重複の削減
- 230ファイル変更、46,382行追加
- 保守性・テスト性の大幅向上

詳細: [`release_notes/v0.9.1.md`](release_notes/v0.9.1.md)

---

**Cb（シーフラット）**は、C++の表現力とTypeScriptの型安全性を融合した、モダンな静的型付けプログラミング言語です。

再帰下降パーサーを使用してAST（抽象構文木）を構築し、C++でASTを逐次実行するインタープリターとして動作します。

### 設計思想

- **ゼロコスト抽象化**: ランタイムオーバーヘッドを最小化
- **型安全性**: コンパイル時の厳密な型チェック
- **明示的なメモリ管理**: ガベージコレクションなし、RAIIベース
- **実用性重視**: 学習コストを抑えつつ、実用的な機能を提供

---

## 🎯 主要特徴

### ✅ 完全実装済み機能（v0.10.0）

#### 🚀 ムーブセマンティクス（v0.10.0完全実装）
- **✅ 右辺値参照**: `T&&` による所有権移動
- **✅ ムーブコンストラクタ**: `self(T&& other)` でリソース転送
- **✅ move()関数**: 明示的なムーブ操作
- **✅ デストラクタからの自動削除**: ムーブされたオブジェクトは破棄されない
- **✅ ネスト構造体デストラクタ**: 値メンバーの自動破棄（LIFO順）
- **✅ コピーコンストラクタ**: `self(const T& other)` で値のコピー

#### 🔄 スコープとクリーンアップ（v0.10.0完全実装）
- **✅ return前のdefer実行**: return文実行前にdefer文を自動実行（LIFO順）
- **✅ return前のデストラクタ実行**: return文実行前にデストラクタを自動実行（LIFO順）
- **✅ 複数return経路対応**: どのreturn文でも同じクリーンアップルール
- **✅ 実行順序の保証**: defer → デストラクタ → return値の評価

#### 📦 モジュールシステム（v0.10.0強化）
- **✅ impl export/import**: コンストラクタ、インターフェース実装の共有
- **✅ export impl構文**: `export impl S { ... }` でimplブロックをエクスポート
- **✅ インターフェース実装のexport**: `export impl I for S { ... }`
- **✅ モジュール境界を越えた実装**: 構造体定義と実装を分離可能

#### 🎯 ポインタシステム（v0.9.0完全実装）
- **✅ 基本ポインタ**: アドレス演算子（`&`）、デリファレンス（`*`）
- **✅ 宣言時初期化**: `int* p = &arr[0];` 構文サポート
- **✅ ポインタ演算**: `p++`, `p--`, `p + n`, `p - n` 完全対応
- **✅ 16進数アドレス表示**: ポインタ値を `0x[hex]` 形式で表示
- **✅ 構造体ポインタ**: `(*ptr).member` 構文でメンバーアクセス
- **✅ アロー演算子**: `ptr->member` による簡潔なメンバーアクセス
- **✅ Interfaceポインタ**: ポリモーフィックメソッド呼び出し
- **✅ 関数ポインタ**: 関数のアドレス取得、呼び出し、コールバック
- **✅ ポインタ配列**: `int*[N]` 形式（初期化付き宣言）
- **✅ 参照型**: `int&`, `Struct&`, `T&&` による参照渡し

#### 🎨 Interface/Implシステム（v0.9.0強化）
- **✅ impl内static変数**: implブロック内でstatic変数をサポート
- **✅ 型ごとの独立性**: `impl I for A`と`impl I for B`で異なるstatic変数を持つ
- **✅ 永続的な状態管理**: プログラム実行中ずっと保持される
- **✅ const修飾子サポート**: `static const int MAX = 100;` が可能
- **✅ 初期化式サポート**: `static int counter = 0;` のような初期化が可能

#### 🏗️ 型システム
- **基本型**: `tiny`, `short`, `int`, `long`, `string`, `char`, `bool`
- **unsigned修飾子**: すべての整数型に適用可能、負値は自動的に0にクランプ
- **配列型**: 静的配列・多次元配列・配列リテラル
- **構造体システム**: 定義・初期化（末尾カンマ対応）・多次元配列メンバー
- **Union型システム**: TypeScript風Union型・型安全性・エラーハンドリング
- **Interface/Implシステム**: 型安全なポリモーフィズム・メソッド定義
- **typedef**: 型エイリアス・配列型エイリアス・再帰的typedef

#### ⚙️ 演算子
- **算術演算子**: `+`, `-`, `*`, `/`, `%`
- **比較演算子**: `==`, `!=`, `<`, `>`, `<=`, `>=`
- **論理演算子**: `&&`, `||`, `!`
- **ビット演算子**: `&`, `|`, `^`, `~`, `<<`, `>>`
- **複合代入演算子（10種類）**: `+=`, `-=`, `*=`, `/=`, `%=`, `&=`, `|=`, `^=`, `<<=`, `>>=`
- **インクリメント・デクリメント**: 前置 `++x`, `--x` / 後置 `x++`, `x--`
- **アドレス演算子**: `&variable`（アドレス取得）
- **デリファレンス演算子**: `*pointer`（値の取得）
- **三項演算子**: `condition ? true_val : false_val`

#### 🎮 制御構造
- **条件分岐**: `if`, `else`, `else if`
- **ループ**: `for`, `while`
- **ループ制御**: `break`, `continue`
- **関数**: 定義・呼び出し・戻り値・再帰

#### 📚 モジュールシステム
- **import/export**: モジュール間での関数・型の共有
- **プライベート関数**: exportしない関数はモジュール内のみで使用可能

#### 🖨️ 入出力
- **println関数**: 可変長引数での出力
- **print関数**: printf風フォーマット指定子対応
  - `%d`: 整数 (tiny, short, int)
  - `%lld`: 長整数 (long)
  - `%u`: 符号なし整数
  - `%s`: 文字列 (string)
  - `%c`: 文字 (char)
  - `%%`: パーセント記号のエスケープ

#### 🐛 エラーハンドリング・デバッグ
- **多言語対応エラーメッセージ**: 英語・日本語でのエラー表示
- **包括的な型範囲チェック**: 全整数型で自動範囲チェック
- **unsigned負値クランプ**: 負値代入時に自動的に0にクランプ（警告付き）
- **詳細なデバッグ機能**: `--debug`（英語）、`--debug-ja`（日本語）オプション
- **UTF-8文字列処理**: 日本語を含む文字列の適切な処理

### 📊 品質指標（v0.9.0）

- **統合テスト**: **2424個**（100%成功） 🎉
- **単体テスト**: **30個**（100%成功） 🎉
- **v0.9.0新規テスト**: **33個追加**
  - 関数ポインタテスト: 5個（100%成功） �
  - ポインタ配列テスト: 2個（100%成功） 🆕
  - impl staticテスト: 26個（100%成功）
- **テストカバレッジ**: 全機能を網羅的にテスト
- **完全な型安全性**: 境界値・型不整合の自動検出

---

## 🚀 クイックスタート

### 1. ビルド

```bash
# リポジトリをクローン
git clone https://github.com/shadowlink0122/Cb.git
cd Cb

# ビルド
make
```

### 2. 初めてのプログラム

`hello.cb`を作成:

```cb
func int main() {
    println("Hello, Cb World!");
    return 0;
}
```

実行:

```bash
./main hello.cb
```

出力:
```
Hello, Cb World!
```

### 3. サンプルプログラムを試す

```bash
# FizzBuzz
./main sample/fizzbuzz.cb

# フィボナッチ数列
./main sample/fibonacci.cb

# ポインタのデモ
./main sample/function_pointer_demo.cb
```

### 4. もっと学ぶ

- 📘 [基本構文ガイド](docs/tutorial/basic_syntax_guide.md) - 言語の基本を学ぶ
- 💡 [サンプルコード集](docs/tutorial/sample_code_collection.md) - 実践的な例
- ⚠️ [よくある間違い](docs/tutorial/common_mistakes.md) - トラブルシューティング

---

## 🔧 使い方

### プログラムの実行

```bash
./main program.cb
```

### デバッグモード

```bash
# 英語デバッグ
./main --debug program.cb

# 日本語デバッグ
./main --debug-ja program.cb
```

### テスト実行

```bash
# 全テスト実行
make test

# 統合テストのみ
make integration-test

# 単体テストのみ
make unit-test
```

---

## 📝 サンプルコード

### ポインタの基本操作

```c++
int main() {
    int[5] arr = [10, 20, 30, 40, 50];
    
    // 宣言時初期化
    int* ptr = &arr[0];
    
    println("Initial value: *ptr =", *ptr);  // 10
    println("Address: ptr =", ptr);           // 0x7fff... (16進数)
    
    // ポインタ演算
    ptr++;
    println("After ptr++: *ptr =", *ptr);    // 20
    
    ptr = ptr + 2;
    println("After ptr+2: *ptr =", *ptr);    // 40
    
    // 値の変更
    *ptr = 100;
    println("After *ptr=100: arr[3] =", arr[3]);  // 100
}
```

### 関数ポインタ 🆕

```c++
int add(int a, int b) {
    return a + b;
}

int subtract(int a, int b) {
    return a - b;
}

int main() {
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

### コールバック関数 🆕

```c++
int apply(int* callback, int x, int y) {
    return callback(x, y);
}

int multiply(int a, int b) {
    return a * b;
}

int main() {
    int result = apply(&multiply, 6, 7);
    println(result);  // 42
}
```

### 参照型 🆕

```c++
void increment(int& ref) {
    ref++;
}

struct Point {
    int x;
    int y;
};

void move_point(Point& p, int dx, int dy) {
    p.x = p.x + dx;
    p.y = p.y + dy;
}

int main() {
    int value = 10;
    increment(value);
    println(value);  // 11
    
    Point p;
    p.x = 5;
    p.y = 10;
    move_point(p, 3, 4);
    println(p.x, p.y);  // 8 14
}
```

### 構造体ポインタとアロー演算子

```c++
struct Point {
    int x;
    int y;
};

int main() {
    Point p;
    p.x = 10;
    p.y = 20;
    Point* ptr = &p;
    
    // デリファレンス構文
    (*ptr).x = 30;
    (*ptr).y = 40;
    println(p.x, p.y);  // 30 40
    
    // アロー演算子（簡潔）
    ptr->x = 50;
    ptr->y = 60;
    println(p.x, p.y);  // 50 60
}
```

### Interfaceとポインタ

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
    Rectangle rect;
    rect.width = 10;
    rect.height = 5;
    
    // Interfaceポインタでポリモーフィズム
    Shape* shape_ptr = &rect;
    int a = (*shape_ptr).area();
    
    println("Area:", a);  // 50
}
```

### Union型

```c++
typedef Status = 200 | 404 | 500;
typedef StringOrInt = string | int;

int main() {
    Status code = 200;
    println("HTTP Status:", code);
    
    StringOrInt value = 42;
    println("Value:", value);
    
    value = "Hello";
    println("Value:", value);
}
```

### 複合代入演算子

```c++
int main() {
    int x = 10;
    
    x += 5;    // x = 15
    x *= 2;    // x = 30
    x >>= 1;   // x = 15
    x &= 7;    // x = 7
    
    println("Final value:", x);
    
    // 配列要素への複合代入
    int[5] arr = [1, 2, 3, 4, 5];
    arr[0] += 10;   // arr[0] = 11
    arr[2] *= 3;    // arr[2] = 9
}
```

### FizzBuzz

```c++
int main() {
    for (int i = 1; i <= 100; i++) {
        if (i % 15 == 0) {
            println("FizzBuzz");
        } else if (i % 3 == 0) {
            println("Fizz");
        } else if (i % 5 == 0) {
            println("Buzz");
        } else {
            println(i);
        }
    }
}
```

### フィボナッチ数列

```c++
int fibonacci(int n) {
    if (n <= 1) {
        return n;
    }
    return fibonacci(n - 1) + fibonacci(n - 2);
}

int main() {
    for (int i = 0; i < 10; i++) {
        println("fib(", i, ") =", fibonacci(i));
    }
}
```

---

## � チラシ・宣伝資料

Cb言語の概要をまとめたチラシ（A4・2ページ）をご用意しています：

- **🎨 カラー版**: [Cb_Language_Flyer_Color.html](https://htmlpreview.github.io/?https://github.com/shadowlink0122/Cb/blob/main/docs/flyer/Cb_Language_Flyer_Color.html) - デジタル表示・カラー印刷用
- **🖨️ 白黒版**: [Cb_Language_Flyer_BW.html](https://htmlpreview.github.io/?https://github.com/shadowlink0122/Cb/blob/main/docs/flyer/Cb_Language_Flyer_BW.html) - モノクロ印刷最適化版

**内容**: v0.10.0の主要機能、技術スタック、開発背景、イベント情報（2025/10/25 OSSカンファレンス Tokyo/Fall）を掲載

---

## �📚 ドキュメント

### 🎓 チュートリアル（初学者向け）

- **基本構文ガイド**: [`docs/tutorial/basic_syntax_guide.md`](docs/tutorial/basic_syntax_guide.md) - 言語の基本を学ぶ
- **サンプルコード集**: [`docs/tutorial/sample_code_collection.md`](docs/tutorial/sample_code_collection.md) - 実践的なプログラム例
- **よくある間違い**: [`docs/tutorial/common_mistakes.md`](docs/tutorial/common_mistakes.md) - トラブルシューティングガイド

### 📖 仕様書・リファレンス

- **言語仕様書**: [`docs/spec.md`](docs/spec.md) - Cb言語の完全な仕様
- **BNF文法定義**: [`docs/BNF.md`](docs/BNF.md) - 文法の形式的定義
- **ドキュメントガイド**: [`docs/README.md`](docs/README.md) - ドキュメント全体のナビゲーション

### 📝 リリースノート

- **v0.10.0**: [`release_notes/v0.10.0.md`](release_notes/v0.10.0.md) - ムーブセマンティクス & 完全なクリーンアップ 🆕
- **v0.9.2**: [`release_notes/v0.9.2.md`](release_notes/v0.9.2.md) - Long型修正 & Const安全性
- **v0.9.1**: [`release_notes/v0.9.1.md`](release_notes/v0.9.1.md) - リファクタリング Phase 1-2 完了版
- **v0.9.0**: [`release_notes/v0.9.0.md`](release_notes/v0.9.0.md) - 関数ポインタ完全実装版

### 実装計画・設計ドキュメント

今後の実装計画や設計ドキュメントは [`docs/todo/`](docs/todo/) に格納されています：
- **v0.11.0実装計画**: [`docs/todo/v0.11.0_implementation_plan.md`](docs/todo/v0.11.0_implementation_plan.md) - スコープ、ジェネリクス、エラーハンドリング 🆕
- **v0.11.0ジェネリクス仕様**: [`docs/todo/v0.11.0_generics_spec.md`](docs/todo/v0.11.0_generics_spec.md) - 詳細な型システム設計 🆕
- **v0.10.1実装計画**: [`docs/todo/v0.10.1_implementation_plan.md`](docs/todo/v0.10.1_implementation_plan.md) - ムーブセマンティクス完成 🆕
- **実装ロードマップ**: [`docs/todo/implementation_roadmap.md`](docs/todo/implementation_roadmap.md)

### サンプルコード

[`sample/`](sample/) フォルダに様々なサンプルプログラムがあります：
- `function_pointer_demo.cb` - 関数ポインタのデモ 🆕
- `comprehensive_demo.cb` - 全機能のデモ
- `fibonacci.cb` - フィボナッチ数列
- `fizzbuzz.cb` - FizzBuzz

### アーカイブドキュメント

過去の実装記録は [`docs/archive/`](docs/archive/) に保存されています。

---

## 🧪 テストフレームワーク

### テストの構造

```
tests/
├── cases/                  # テストケース（Cbソースファイル）
│   ├── pointer/           # ポインタテスト（127個）
│   ├── array/             # 配列テスト
│   ├── struct/            # 構造体テスト
│   ├── interface/         # Interface/Implテスト
│   ├── typedef/           # typedefテスト
│   └── ...
└── integration/           # 統合テスト（C++テストハーネス）
    ├── pointer/
    ├── array/
    └── ...
```

### テストの実行

```bash
# 全テスト実行（2,506個 + 50個）
make test

# カテゴリ別実行
make integration-test    # 統合テスト（2,506個）
make unit-test          # 単体テスト（50個）
```

### テスト成功率

- **v0.10.0**: 2,924/2,924 統合テスト + 30/30 単体テスト = **100%成功** 🎉
- **v0.9.2**: 2,798/2,798 統合テスト + 30/30 単体テスト = **100%成功** 🎉
- **v0.9.1**: 2,447/2,447 統合テスト + 30/30 単体テスト = **100%成功** 🎉
- **v0.9.0**: 2,349/2,349 統合テスト + 30/30 単体テスト = **100%成功** 🎉

---

## 🔧 開発状況

### v0.10.0 - Move Semantics & Complete Cleanup（2025年10月12日）

#### ✅ 主要新機能
- **ムーブセマンティクス**: 右辺値参照（`T&&`）、ムーブコンストラクタ、move()関数
- **return前のクリーンアップ**: defer文とデストラクタの自動実行
- **impl export/import**: コンストラクタとインターフェース実装の共有
- **ネスト構造体デストラクタ**: 値メンバーの自動破棄（LIFO順）

#### 📈 テスト統計
- 統合テスト: 2798個 → **2924個**（+126個）
- Deferテスト: **131個**（return前cleanup含む）
- 成功率: **100%**

#### 📚 ドキュメント
- [`release_notes/v0.10.0.md`](release_notes/v0.10.0.md) - リリースノート
- [`docs/spec.md`](docs/spec.md) - 言語仕様書（v0.10.0対応）
- [`docs/todo/v0.11.0_implementation_plan.md`](docs/todo/v0.11.0_implementation_plan.md) - 次期バージョン計画

### v0.9.2 - Long Type Fix & Const Safety（2025年10月10日）

#### ✅ 主要改善
- **Long型オーバーフロー修正**: Fibonacci(92)まで正確に計算可能
- **Const型安全性**: `const T*` と `T* const` のチェック
- **配列の参照渡し**: 関数引数での配列変更が呼び出し元に反映
- **構造体メンバの再帰的代入**: 4レベル以上の深いネストに対応

#### 📈 テスト統計
- 統合テスト: **2798個**（100%成功）
- 実行時間: 最適化により高速化

### v0.9.1 - リファクタリング Phase 1-2 完了版（2025年1月）

#### 🏗️ 主要改善
- **Phase 1-2: パーサー分離**: 5つのパーサークラスに分割
- **DRY原則の適用**: コード重複の削減
- **大規模リファクタリング**: 230ファイル変更、46,382行追加

#### 📈 テスト統計
- 統合テスト: **2447個**（100%成功）
- 後方互換性: 完全維持

### v0.9.0 - 関数ポインタ完全実装版（2025年10月6日）

#### ✅ 主要新機能
- **関数ポインタ**: 宣言、初期化、呼び出し、コールバック
- **ポインタ配列**: `int*[N]` 形式
- **参照型**: `int&`, `Struct&` による参照渡し
- **impl内static変数**: Interface/Implシステムの強化

#### 📈 テスト統計
- 統合テスト: 2222個 → **2424個**（+202個）
- 成功率: **100%**

### 過去のリリース

- **v0.8.1**: テスト拡充（1386個達成）
- **v0.8.0**: 構造体・Union・Interface完全実装
- **v0.7.1**: 10種類の複合代入演算子実装
- **v0.7.0**: インクリメント・デクリメント演算子実装
- **v0.6.0**: 初期リリース

詳細は [`release_notes/`](release_notes/) を参照してください。

---

## ✅ v0.9.0時点で実装済みの機能

以下の機能はすでに完全実装されています：

### 浮動小数点数型 ✅
- `float` (32bit単精度) および `double` (64bit倍精度)
- 四則演算、比較演算、複合代入演算子
- 配列、構造体メンバーとして使用可能
- テスト: `tests/cases/float_double_unsigned/`

### enum型 ✅
- 列挙型の定義と自動値割り当て
- `Color::RED` 形式のスコープアクセス
- `typedef enum` 構文サポート
- テスト: `tests/cases/enum/`

### ネストした構造体 ✅
- 構造体メンバーに別の構造体を含む
- 多階層ネスト（3階層以上）対応
- `obj.member.submember` 形式のアクセス
- テスト: `tests/cases/struct/test_nested_*.cb`

### アロー演算子 ✅
- `ptr->member` 構文で構造体ポインタのメンバーアクセス
- `(*ptr).member` と同等の糖衣構文
- ネストしたアクセス `ptr->nested->value` 対応
- テスト: `tests/cases/pointer/test_arrow_*.cb`

---

## 🚧 v0.10.0 開発中の機能

### ✅ impl内static変数（100%完了）

**ステータス**: Phase 1-3完了、テスト済み

implブロック内でstatic変数を宣言し、Interface/Implシステムの表現力を向上させます。

```c++
interface Counter {
    int increment();
};

impl Counter for Point {
    static int shared_counter = 0;  // impl全体で共有
    
    int increment() {
        shared_counter++;
        return shared_counter;
    }
}
```

**実装内容**:
- ✅ Parser拡張: implブロック内での`static`宣言解析
- ✅ Interpreter拡張: impl static変数のストレージと管理
- ✅ 変数検索統合: `find_variable()`でimpl static変数を検索
- ✅ Interface method呼び出し時のコンテキスト自動設定
- ✅ 例外安全な実装（RAII風クリーンアップ）

**テスト結果**: 6つのテストケースすべて成功

詳細: [`docs/impl_static_status.md`](docs/impl_static_status.md)

---

## 🚧 将来の拡張予定

詳細は [`docs/future_features.md`](docs/future_features.md) を参照してください。

### v0.10.0（予定）
- **impl内static変数**: Interface/Implシステムの機能拡張（進行中）
- **多次元配列へのポインタ**: `int (*ptr)[5]` 構文サポート
- **動的メモリ管理**: `new`/`delete` 文の実装

### v1.0.0に向けて
- **スマートポインタ**: `unique_ptr`, `shared_ptr`
- **関数ポインタ**: コールバック機能
- **ジェネリクス**: テンプレート・ジェネリクス機能
- **非同期処理**: goroutine風の並行処理
- **標準ライブラリの拡充**: コレクション、I/O、ネットワーク

---

## 🤝 コントリビューション

このプロジェクトは現在個人開発ですが、フィードバックや提案を歓迎します。

### コーディング規約

- **変数・関数**: `snake_case`
- **型・構造体**: `PascalCase`
- **定数**: `UPPER_CASE`
- **インデント**: スペース4つ

---

## 📄 ライセンス

このプロジェクトのライセンスは現在検討中です。

---

## 📞 連絡先

プロジェクトに関する質問や提案がある場合は、GitHubのIssueを通じてご連絡ください。

---

**Cb言語 v0.9.1 - リファクタリング Phase 1-2 完了版**  
*C++の表現力とTypeScriptの型安全性を融合したモダン言語*

🎉 **2380個の統合テスト + 30個の単体テスト = 100%成功** 🎉  
🏗️ **大規模リファクタリング完了 - 保守性・拡張性が大幅向上** 🏗️
