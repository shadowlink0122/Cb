# 文字列補間 (String Interpolation) - 設計ドキュメント

## 概要

文字列補間は、文字列リテラル内に式を埋め込み、実行時にその値を文字列に展開する機能です。Python/C#スタイルの`{式}`構文を採用し、変数や式を直接埋め込むシンプルで直感的な記述を実現します。

**設計日**: 2025年10月27日  
**ステータス**: 📋 設計中  
**対象バージョン**: v0.11.0

---

## 設計目標

### 主要目標
1. **可読性の向上**: 文字列連結よりも直感的で読みやすい構文
2. **型安全性**: コンパイル時の型チェック
3. **強力なフォーマット**: 数値、文字列、配列などの柔軟な表示制御
4. **パフォーマンス**: 効率的な文字列構築
5. **既存機能との互換性**: 既存の文字列機能を破壊しない

### 影響を受けた言語

| 言語 | 構文例 | 採用する要素 |
|------|--------|-------------|
| Python | `f"Hello, {name}!"` | 直接埋め込み、式の評価 |
| C# | `$"Hello, {name}!"` | 補間構文、シンプルさ |
| JavaScript | `` `Hello, ${name}!` `` | テンプレートリテラル |
| Kotlin | `"Hello, $name!"` | 簡潔な記法 |

**Cb言語の選択**: `{式}`構文（Python/C#スタイル + フォーマット指定）
- 理由: 
  - `{変数}`で変数を直接参照できる
  - `{a + b}`のように式を直接記述できる
  - フォーマット指定子`{式:format}`で強力な表示制御が可能
  - 位置引数が不要で、より直感的
  - 既存の文字列リテラルと明確に区別できる

---

## 構文仕様

### 基本構文

#### 1. 変数の直接埋め込み
```cb
string name = "World";
int count = 42;

// 単一の変数
println("Hello, {name}!");
// 出力: Hello, World!

// 複数の変数
println("Name: {name}, Count: {count}");
// 出力: Name: World, Count: 42
```

#### 2. 式の評価
```cb
int a = 10;
int b = 20;

// 算術式
println("Sum: {a + b}");
// 出力: Sum: 30

// 複雑な式
println("Result: {(a + b) * 2}");
// 出力: Result: 60
```

#### 3. フォーマット指定
```cb
double pi = 3.14159265;

// 小数点以下の桁数指定
println("Pi: {pi:.2}");
// 出力: Pi: 3.14

int num = 255;

// 16進数表示
println("Hex: {num:x}");
// 出力: Hex: ff
```

---

### フォーマット指定子

フォーマット指定は`{式:format}`の形式で記述します。

#### 整数フォーマット

```cb
int num = 255;

// デフォルト（10進数）
println("{num}");                // "255"

// 16進数（小文字）
println("{num:x}");              // "ff"

// 16進数（大文字）
println("{num:X}");              // "FF"

// 8進数
println("{num:o}");              // "377"

// 2進数
println("{num:b}");              // "11111111"

// 幅指定（右寄せ、空白パディング）
println("{num:5}");              // "  255"

// ゼロパディング
println("{num:05}");             // "00255"

// 左寄せ
println("{num:<5}");             // "255  "

// 中央寄せ
println("{num:^5}");             // " 255 "
```

#### 浮動小数点フォーマット

```cb
double pi = 3.14159265358979;

// デフォルト
println("{pi}");                 // "3.141593"

// 小数点以下の桁数指定
println("{pi:.2}");              // "3.14"
println("{pi:.5}");              // "3.14159"

// 幅と精度の両方指定
println("{pi:8.2}");             // "    3.14"

// 科学的記法
println("{pi:e}");               // "3.141593e+00"
println("{pi:.2e}");             // "3.14e+00"

// 科学的記法（大文字）
println("{pi:E}");               // "3.141593E+00"
```

#### 文字列フォーマット

```cb
string name = "Alice";

// デフォルト
println("{name}");               // "Alice"

// 幅指定（右寄せ）
println("{name:10}");            // "     Alice"

// 左寄せ
println("{name:<10}");           // "Alice     "

// 中央寄せ
println("{name:^10}");           // "  Alice   "

// 最大長指定（切り詰め）
println("{name:.3}");            // "Ali"
```

#### カスタムフィル文字

```cb
int num = 42;

// アスタリスクでパディング
println("{num:*>5}");            // "***42"

// ハイフンでパディング
println("{num:-<5}");            // "42---"

// ドットで中央寄せ
println("{num:.^7}");            // "..42..."
```

#### フォーマット指定子の構文

```
{式:[fill][align][sign][#][0][width][.precision][type]}
```

| 要素 | 説明 | 例 |
|------|------|-----|
| `式` | 変数、リテラル、または式 | `{name}`, `{a + b}` |
| `fill` | パディング文字（省略時は空白） | `{num:*>5}` |
| `align` | 配置 (`<`左, `^`中央, `>`右) | `{name:<10}` |
| `sign` | 符号表示 (`+`常に, `-`負のみ, ` `空白) | `{num:+}` |
| `#` | 代替形式（0x、0b プレフィックス） | `{num:#x}` |
| `0` | ゼロパディング | `{num:05}` |
| `width` | 最小幅 | `{num:10}` |
| `.precision` | 精度（浮動小数点）または最大幅（文字列） | `{pi:.2}` |
| `type` | 型指定子 | `{num:x}`, `{pi:e}` |

**型指定子一覧**:

| 指定子 | 意味 | 適用型 |
|--------|------|--------|
| `b` | 2進数 | 整数 |
| `o` | 8進数 | 整数 |
| `x` | 16進数（小文字） | 整数 |
| `X` | 16進数（大文字） | 整数 |
| `e` | 科学的記法（小文字） | 浮動小数点 |
| `E` | 科学的記法（大文字） | 浮動小数点 |
| `f` | 固定小数点 | 浮動小数点 |
| `p` | ポインタ形式 | ポインタ |
| `?` | デバッグ形式 | すべて |

---

### サポートする式

#### 1. 変数参照
```cb
int age = 25;
println("Age: {age}");
// 出力: Age: 25

double height = 175.5;
println("Height: {height:.1}cm");
// 出力: Height: 175.5cm
```

#### 2. 算術式
```cb
int a = 10, b = 20;
println("Sum: {a + b}");
// 出力: Sum: 30

println("Product: {a * b}");
// 出力: Product: 200
```

#### 3. 関数呼び出し
```cb
int square(int x) { return x * x; }

println("Square of 5: {square(5)}");
// 出力: Square of 5: 25

println("Formatted: {square(5):04}");
// 出力: Formatted: 0025
```

#### 4. メンバーアクセス
```cb
struct Person {
    string name;
    int age;
    double height;
}

Person p = Person{"Alice", 30, 165.5};

println("Name: {p.name}, Age: {p.age}, Height: {p.height:.1}cm");
// 出力: Name: Alice, Age: 30, Height: 165.5cm
```

#### 5. 配列要素アクセス
```cb
int[] nums = [10, 20, 30, 40, 50];

println("Third element: {nums[2]}");
// 出力: Third element: 30

println("Hex: {nums[4]:x}");
// 出力: Hex: 32
```

#### 6. ネストした式
```cb
int x = 5;

println("Result: {x * 2 + 10}");
// 出力: Result: 20

println("Binary: {(x << 2) | 1:b}");
// 出力: Binary: 10101
```

#### 7. 条件式（三項演算子）
```cb
int score = 85;

println("Grade: {score >= 80 ? \"A\" : \"B\"}");
// 出力: Grade: A

println("Status: {score >= 60 ? \"Pass\" : \"Fail\"}");
// 出力: Status: Pass
```

---

### 型変換ルール

| 型 | デフォルト変換 | フォーマット例 |
|----|--------------|--------------|
| `int`, `long`, `short`, `tiny` | 10進数文字列 | `{:x}` (16進), `{:b}` (2進) |
| `unsigned` 系 | 10進数文字列 | `{:05}` (ゼロパディング) |
| `float`, `double` | 小数点6桁 | `{:.2}` (小数点2桁) |
| `string` | そのまま | `{:10}` (幅指定) |
| `bool` | `"true"` / `"false"` | フォーマット不可 |
| `char` | 1文字の文字列 | フォーマット不可 |
| ポインタ | `0x` + アドレス（16進） | `{:p}` (ポインタ形式) |
| 配列 | 要素をカンマ区切り | `{:?}` (デバッグ形式) |
| 構造体 | カスタム実装 | `Display` trait 実装が必要 |

---

### エスケープ

リテラルとして`{`や`}`を使用したい場合はダブルエスケープ：

```cb
string literal = "Set: {{1, 2, 3}}";
// "Set: {1, 2, 3}" (展開されない)

string price = "Price: {{}} yen";
// "Price: {} yen"
```

---

### 複数の補間

1つの文字列に複数の補間を含めることが可能：

```cb
string name = "Alice";
int age = 30;
string city = "Tokyo";

println("Profile: Name={name}, Age={age}, City={city}");
// 出力: Profile: Name=Alice, Age=30, City=Tokyo
```

### 同じ変数の再利用

```cb
int x = 5;
println("{x} * {x} = {x * x}");
// 出力: 5 * 5 = 25
```

---

## 文法定義（BNF拡張）

```bnf
string_literal ::= '"' (string_char | interpolation)* '"'

interpolation ::= '{' expression [':' format_spec] '}'
                | '{{' 
                | '}}'

expression ::= variable
             | literal
             | function_call
             | member_access
             | array_access
             | binary_expression
             | unary_expression
             | ternary_expression
             | '(' expression ')'

format_spec ::= [fill] [align] [sign] ['#'] ['0'] [width] ['.' precision] [type]

fill ::= any_char

align ::= '<' | '^' | '>'

sign ::= '+' | '-' | ' '

width ::= digit+

precision ::= digit+

type ::= 'b' | 'o' | 'x' | 'X' | 'e' | 'E' | 'f' | 'p' | '?'

string_char ::= [^"\\{}]
              | '\\' escape_sequence

escape_sequence ::= 'n' | 't' | '\\' | '"' | '\'' | '{' | '}'
```

---

## 実装戦略

### フェーズ1: 基本実装
1. **Lexer拡張**: `{`, `}`トークンの処理
2. **Parser拡張**: interpolation構文の解析
3. **AST拡張**: `InterpolatedStringNode`の追加
4. **評価器**: 基本的な変数展開

### フェーズ2: フォーマット実装
1. **フォーマット指定子パーサー**: `:format`の解析
2. **Formatter実装**: 各型用のフォーマッタ
3. **型別フォーマット**: 整数、浮動小数点、文字列

### フェーズ3: 高度な機能
1. **インデックス指定**: `{0}`, `{1}`のサポート
2. **式の評価**: `{a + b}`のような式のサポート
3. **最適化**: 文字列連結の効率化

---

## パフォーマンス考察

### 最適化戦略
1. **事前計算**: コンパイル時に文字列サイズを推定
2. **メモリ確保**: 一度の`reserve()`で十分な容量を確保
3. **直接追加**: `std::ostringstream`ではなく直接文字列に追加

### 実装例（C++バックエンド）
```cpp
// 効率的な実装
std::string result;
result.reserve(estimated_size);  // 事前確保

result.append("Hello, ");
result.append(format_value(name));  // 型に応じたフォーマット
result.append("!");
```

---

## エラーハンドリング

### コンパイルエラー
1. **型不一致**: 引数の数が不足している場合
2. **無効なフォーマット**: サポートされていないフォーマット指定子
3. **構文エラー**: 閉じ括弧がない、など

### ランタイムエラー
1. **インデックス範囲外**: `{10}`で引数が3つしかない場合
2. **型変換失敗**: フォーマット不可能な型

---

## テスト計画

### 基本機能テスト
- [ ] 単一変数の展開
- [ ] 複数変数の展開
- [ ] 式の評価
- [ ] 各型の変換

### フォーマットテスト
- [ ] 整数フォーマット（進数変換）
- [ ] 浮動小数点フォーマット（精度指定）
- [ ] 文字列フォーマット（幅、配置）
- [ ] カスタムフィル文字

### エッジケーステスト
- [ ] 空の文字列
- [ ] エスケープシーケンス
- [ ] ネストした式
- [ ] 大量の補間

---

## 使用例

### 実用的な例

#### ログ出力
```cb
int line = 42;
string file = "main.cb";
string level = "ERROR";

println("[{level}] {file}:{line} - Memory allocation failed");
// [ERROR] main.cb:42 - Memory allocation failed
```

#### テーブル表示
```cb
string name1 = "Alice";
string name2 = "Bob";
int age1 = 25;
int age2 = 30;
double score1 = 95.5;
double score2 = 87.3;

println("| {"Name":^10} | {"Age":^10} | {"Score":^10} |");
println("|{"":-^12}|{"":-^12}|{"":-^12}|");
println("| {name1:<10} | {age1:>10} | {score1:>10.2} |");
println("| {name2:<10} | {age2:>10} | {score2:>10.2} |");

// |    Name    |    Age     |   Score    |
// |------------|------------|------------|
// | Alice      |         25 |      95.50 |
// | Bob        |         30 |      87.30 |
```

#### デバッグ出力
```cb
int x = 42;
double y = 3.14;
string z = "test";

println("Debug: x={x:?}, y={y:?}, z={z:?}");
// Debug: x=42, y=3.14, z="test"
```

#### プログレスバー
```cb
int progress = 65;
int total = 100;
double percent = (progress as double) / (total as double) * 100.0;

println("Progress: [{progress}/{total}] {percent:.1}%");
// Progress: [65/100] 65.0%
```

#### 座標表示
```cb
double x = 12.3456;
double y = 78.9012;

println("Position: ({x:.2}, {y:.2})");
// Position: (12.35, 78.90)

println("Position (padded): ({x:8.2}, {y:8.2})");
// Position (padded): (   12.35,    78.90)
```
## 互換性と移行

### 既存コードへの影響
- 既存の文字列リテラルは一切影響を受けない
- `{`や`}`を含む文字列は`{{`、`}}`でエスケープ必要

### 移行ガイドライン
```cb
// 従来の方法
string msg = "Hello, " + name + "!";
string info = "Count: " + to_string(count);

// 新しい方法
string msg = "Hello, {name}!";
string info = "Count: {count}";
```
## 将来の拡張

1. **カスタムフォーマッタ**: ユーザー定義型のフォーマット
2. **より複雑な式**: ラムダ式、メソッドチェーン
3. **条件付きフォーマット**: `{value:format if condition}`
4. **国際化対応**: ロケール依存のフォーマット
5. **複数行補間**: 長い式を複数行に分割

---

## 参考資料

- [Python f-strings](https://peps.python.org/pep-0498/)
- [C# String Interpolation](https://learn.microsoft.com/en-us/dotnet/csharp/language-reference/tokens/interpolated)
- [Kotlin String Templates](https://kotlinlang.org/docs/strings.html#string-templates)
- [Swift String Interpolation](https://docs.swift.org/swift-book/LanguageGuide/StringsAndCharacters.html)
- [Rust std::fmt](https://doc.rust-lang.org/std/fmt/)

---

## ステータス

- [x] 設計完了
- [ ] 実装開始
- [ ] 基本機能実装
- [ ] フォーマット機能実装
- [ ] テスト完了
- [ ] ドキュメント完了
