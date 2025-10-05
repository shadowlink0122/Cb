# Cb (C-flat) プログラミング言語

**最新バージョン**: v0.9.0 - ポインタシステム完全実装版  
**リリース日**: 2025年10月5日

## 📖 概要

**Cb（シーフラット）**は、C++の表現力とTypeScriptの型安全性を融合した、モダンな静的型付けプログラミング言語です。

再帰下降パーサーを使用してAST（抽象構文木）を構築し、C++でASTを逐次実行するインタープリターとして動作します。

### 設計思想

- **ゼロコスト抽象化**: ランタイムオーバーヘッドを最小化
- **型安全性**: コンパイル時の厳密な型チェック
- **明示的なメモリ管理**: ガベージコレクションなし、RAIIベース
- **実用性重視**: 学習コストを抑えつつ、実用的な機能を提供

---

## 🎯 主要特徴

### ✅ 完全実装済み機能（v0.9.0）

#### 🎯 ポインタシステム（v0.9.0新機能）
- **✅ 宣言時初期化**: `int* p = &arr[0];` 構文サポート
- **✅ ポインタ演算**: `p++`, `p--`, `p + n`, `p - n` 完全対応
- **✅ デリファレンス**: `*ptr` による値の取得・変更
- **✅ 16進数アドレス表示**: ポインタ値を `0x[hex]` 形式で表示
- **✅ 構造体ポインタ**: `(*ptr).member` 構文でメンバーアクセス
- **✅ Interfaceポインタ**: `(*shape_ptr).area()` でポリモーフィックメソッド呼び出し
- **✅ アドレス演算子**: `&variable` でアドレス取得
- **✅ ポインタ配列**: `int* ptrs[10];` で複数ポインタ管理

#### 🎨 Interface/Implシステム（v0.9.0強化）
- **✅ impl内static変数 🆕**: implブロック内でstatic変数をサポート
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

- **統合テスト**: **2234個**（2232個成功、2個既存の不具合） 
- **単体テスト**: **30個**（100%成功） 🎉
- **v0.9.0新規テスト**: **26個追加**
  - impl staticテスト: 26個（100%成功） 🎉
  - ポインタテスト: 127個（既存）
- **テストカバレッジ**: 全機能を網羅的にテスト
- **完全な型安全性**: 境界値・型不整合の自動検出

**注**: 既存の2つの失敗は、impl static機能とは無関係な既知の問題です。

---

## 🚀 使い方

### ビルド

```bash
make
```

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
    
    // 宣言時初期化（v0.9.0新機能）
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
    
    // デリファレンス構文でメンバーアクセス
    println("Original: p.x =", p.x, "p.y =", p.y);
    
    (*ptr).x = 30;
    (*ptr).y = 40;
    
    println("Modified: p.x =", p.x, "p.y =", p.y);
    
    return 0;
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
    Rectangle rect = {width: 10, height: 5};
    
    // Interfaceポインタでポリモーフィズム
    Shape* shape_ptr = &rect;
    int a = (*shape_ptr).area();
    
    println("Area:", a);  // 50
    
    return 0;
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
    
    return 0;
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
    
    return 0;
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
    return 0;
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
    return 0;
}
```

---

## 📚 ドキュメント

### 完全仕様書

詳細な言語仕様は [`docs/spec.md`](docs/spec.md) を参照してください。

### その他のドキュメント

- **Interface/Implシステム**: [`docs/interface_system.md`](docs/interface_system.md)
- **リリースノート**: [`release_notes/`](release_notes/)
- **サンプルコード**: [`sample/`](sample/)

### アーカイブドキュメント

過去の実装記録やプログレスレポートは [`docs/archive/`](docs/archive/) に保存されています。

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
# 全テスト実行（2349個 + 30個）
make test

# カテゴリ別実行
make integration-test    # 統合テスト（2349個）
make unit-test          # 単体テスト（30個）
```

### テスト成功率

- **v0.9.0**: 2349/2349 統合テスト + 30/30 単体テスト = **100%成功** 🎉

---

## 🔧 開発状況

### v0.9.0 - ポインタシステム完全実装版（2025年10月5日）

#### ✅ 新機能
- ポインタ宣言時初期化（`int* p = &arr[0];`）
- 16進数アドレス表示（`0x[hex]`形式）
- 構造体ポインタ操作（`(*ptr).member`）
- Interfaceポインタ（`(*iface_ptr).method()`）
- 127個の包括的ポインタテスト追加

#### 📈 テスト統計
- 統合テスト: 2222個 → **2349個**（+127個）
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

**Cb言語 v0.9.0 - ポインタシステム完全実装版**  
*C++の表現力とTypeScriptの型安全性を融合したモダン言語*

🎉 **2349個の統合テスト + 30個の単体テスト = 100%成功** 🎉
