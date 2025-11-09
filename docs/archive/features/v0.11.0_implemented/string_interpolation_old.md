# 文字列補間 (String Interpolation) - 設計ドキュメント

## 概要

文字列補間は、文字列リテラル内に式を埋め込み、実行時にその値を文字列に展開する機能です。Rust風の`{}`構文を採用し、シンプルで読みやすく、強力なフォーマット機能を提供します。

## 設計日
2025年10月27日

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
| Rust | `format!("Hello, {}!", name)` | `{}`構文、フォーマット指定子 |
| Python | `f"Hello, {name}!"` | シンプルさ |
| C# | `$"Hello, {name}!"` | 補間構文 |
| C/C++ | `printf("%.2f", 3.14)` | フォーマット指定の概念 |

**Cb言語の選択**: `{}`構文（Rustスタイル + フォーマット指定）
- 理由: 
  - `{}`はシンプルで読みやすい
  - `$`や`\`のようなエスケープ文字が不要
  - フォーマット指定子`{:format}`で強力な表示制御が可能
  - 既存の文字列リテラルと明確に区別できる

## 構文仕様

### 基本構文

#### 1. 位置指定（プレースホルダー）
```cb
string name = "World";
int count = 42;

// 単一の値
println("Hello, {}!", name);
// 出力: Hello, World!

// 複数の値（順番に埋め込まれる）
println("Name: {}, Count: {}", name, count);
// 出力: Name: World, Count: 42
```

#### 2. インデックス指定
```cb
println("{0} + {1} = {2}", 10, 20, 30);
// 出力: 10 + 20 = 30

// 同じ値を複数回使用
println("{0} * {0} = {1}", 5, 25);
// 出力: 5 * 5 = 25
```

#### 3. 名前付き引数（将来拡張）
```cb
println("Name: {name}, Age: {age}", name="Alice", age=25);
// 出力: Name: Alice, Age: 25
```

### フォーマット指定子

フォーマット指定は`{:format}`の形式で記述します。

#### 整数フォーマット

```cb
int num = 255;

// デフォルト（10進数）
println("{}", num);              // "255"

// 16進数（小文字）
println("{:x}", num);            // "ff"

// 16進数（大文字）
println("{:X}", num);            // "FF"

// 8進数
println("{:o}", num);            // "377"

// 2進数
println("{:b}", num);            // "11111111"

// 幅指定（右寄せ、空白パディング）
println("{:5}", num);            // "  255"

// ゼロパディング
println("{:05}", num);           // "00255"

// 左寄せ
println("{:<5}", num);           // "255  "

// 中央寄せ
println("{:^5}", num);           // " 255 "
```

#### 浮動小数点フォーマット

```cb
double pi = 3.14159265358979;

// デフォルト
println("{}", pi);               // "3.141593"

// 小数点以下の桁数指定
println("{:.2}", pi);            // "3.14"
println("{:.5}", pi);            // "3.14159"

// 幅と精度の両方指定
println("{:8.2}", pi);           // "    3.14"

// 科学的記法
println("{:e}", pi);             // "3.141593e+00"
println("{:.2e}", pi);           // "3.14e+00"

// 科学的記法（大文字）
println("{:E}", pi);             // "3.141593E+00"
```

#### 文字列フォーマット

```cb
string name = "Alice";

// デフォルト
println("{}", name);             // "Alice"

// 幅指定（右寄せ）
println("{:10}", name);          // "     Alice"

// 左寄せ
println("{:<10}", name);         // "Alice     "

// 中央寄せ
println("{:^10}", name);         // "  Alice   "

// 最大長指定（切り詰め）
println("{:.3}", name);          // "Ali"
```

#### カスタムフィル文字

```cb
int num = 42;

// アスタリスクでパディング
println("{:*>5}", num);          // "***42"

// ハイフンでパディング
println("{:-<5}", num);          // "42---"

// ドットで中央寄せ
println("{:.^7}", num);          // "..42..."
```

### サポートする式

### サポートする式

#### 1. 変数参照
```cb
int age = 25;
println("Age: {}", age);
// 出力: Age: 25

double height = 175.5;
println("Height: {:.1}cm", height);
// 出力: Height: 175.5cm
```

#### 2. 算術式
```cb
int a = 10, b = 20;
println("Sum: {}", a + b);
// 出力: Sum: 30

println("Product: {}", a * b);
// 出力: Product: 200
```

#### 3. 関数呼び出し
```cb
int square(int x) { return x * x; }

println("Square of 5: {}", square(5));
// 出力: Square of 5: 25

println("Formatted: {:04}", square(5));
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

println("Name: {}, Age: {}, Height: {:.1}cm", p.name, p.age, p.height);
// 出力: Name: Alice, Age: 30, Height: 165.5cm
```

#### 5. 配列要素アクセス
```cb
int[] nums = [10, 20, 30, 40, 50];

println("Third element: {}", nums[2]);
// 出力: Third element: 30

println("Hex: {:x}", nums[4]);
// 出力: Hex: 32
```

#### 6. ネストした式
```cb
int x = 5;

println("Result: {}", x * 2 + 10);
// 出力: Result: 20

println("Binary: {:b}", (x << 2) | 1);
// 出力: Binary: 10101
```

#### 7. 条件式（三項演算子）
```cb
int score = 85;

println("Grade: {}", score >= 80 ? "A" : "B");
// 出力: Grade: A

println("Status: {}", score >= 60 ? "Pass" : "Fail");
// 出力: Status: Pass
```

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
| `float`, `double` | `std::to_string()` | `3.14` → `"3.140000"` |
| `char` | 文字として追加 | `'A'` → `"A"` |
| `string` | そのまま追加 | `"text"` → `"text"` |
| `bool` | `"true"` / `"false"` | `true` → `"true"` |
| `pointer` | アドレス16進数表記 | `ptr` → `"0x7fff5fbff8a0"` |

### エスケープ

リテラルとして`${`を使用したい場合はエスケープ：

```cb
string literal = "Price: \\${100}";
// "Price: ${100}" (展開されない)
```

### 複数の補間

1つの文字列に複数の補間を含めることが可能：

```cb
string name = "Alice";
int age = 30;
string city = "Tokyo";
string profile = "Name: ${name}, Age: ${age}, City: ${city}";
// "Name: Alice, Age: 30, City: Tokyo"
```

## 文法定義（BNF拡張）

```bnf
string_literal ::= '"' (string_char | interpolation)* '"'

interpolation ::= '${' expression '}'

string_char ::= [^"\\$]
              | '\\' escape_sequence
              | '\\$'              # エスケープされた$

escape_sequence ::= 'n' | 't' | '\\' | '"' | '\''
```

## 実装戦略

### フェーズ1: 字句解析 (Lexer)

**目標**: `${}`を特殊トークンとして認識

```cpp
// 新しいトークン型
enum TokenType {
    // ...既存のトークン
    TOKEN_STRING_INTERPOLATION_START,  // "${"
    TOKEN_STRING_INTERPOLATION_END,    // "}" (文字列内)
    TOKEN_STRING_PART,                 // 補間の間の文字列部分
};
```

**アルゴリズム**:
1. `"`を検出したら文字列モードに入る
2. 文字列モード内で`${`を検出
   - それまでの文字列部分を`TOKEN_STRING_PART`としてemit
   - `TOKEN_STRING_INTERPOLATION_START`をemit
   - 式解析モードに入る
3. `}`を検出したら文字列モードに戻る
   - `TOKEN_STRING_INTERPOLATION_END`をemit
4. `"`を検出したら文字列終了

### フェーズ2: 構文解析 (Parser)

**AST構造**:

```cpp
struct StringInterpolationNode : ASTNode {
    std::vector<StringPartOrExpression> parts;
};

struct StringPartOrExpression {
    bool is_expression;
    
    // is_expression == false の場合
    std::string string_part;
    
    // is_expression == true の場合
    std::unique_ptr<ASTNode> expression;
};
```

**パース処理**:

```cpp
std::unique_ptr<ASTNode> Parser::parse_string_literal() {
    if (!has_interpolation()) {
        // 従来の文字列リテラル
        return parse_simple_string();
    }
    
    auto interpolation = std::make_unique<StringInterpolationNode>();
    
    while (current_token.type != TOKEN_STRING_END) {
        if (current_token.type == TOKEN_STRING_PART) {
            // 通常の文字列部分
            StringPartOrExpression part;
            part.is_expression = false;
            part.string_part = current_token.value;
            interpolation->parts.push_back(part);
            advance();
        }
        else if (current_token.type == TOKEN_STRING_INTERPOLATION_START) {
            // 補間式
            advance(); // ${を消費
            StringPartOrExpression part;
            part.is_expression = true;
            part.expression = parse_expression();
            interpolation->parts.push_back(part);
            expect(TOKEN_STRING_INTERPOLATION_END); // }
        }
    }
    
    return interpolation;
}
```

### フェーズ3: インタプリタ実行

**評価処理**:

```cpp
Value Interpreter::evaluate_string_interpolation(
    StringInterpolationNode* node) {
    
    std::string result;
    
    for (const auto& part : node->parts) {
        if (!part.is_expression) {
            // 通常の文字列部分
            result += part.string_part;
        } else {
            // 式を評価して文字列に変換
            Value expr_value = evaluate(part.expression.get());
            result += value_to_string(expr_value);
        }
    }
    
    return Value::make_string(result);
}

std::string Interpreter::value_to_string(const Value& value) {
    switch (value.type) {
        case ValueType::INT:
            return std::to_string(value.int_value);
        case ValueType::LONG:
            return std::to_string(value.long_value);
        case ValueType::STRING:
            return value.string_value;
        case ValueType::CHAR:
            return std::string(1, value.char_value);
        case ValueType::BOOL:
            return value.bool_value ? "true" : "false";
        case ValueType::POINTER:
            return pointer_to_string(value.pointer_value);
        // ... 他の型
        default:
            throw RuntimeError("Cannot convert type to string");
    }
}
```

## 実装ファイル

### 修正が必要なファイル

| ファイル | 修正内容 | 優先度 |
|---------|---------|--------|
| `src/common/token.h` | 新しいトークン型定義 | 高 |
| `src/frontend/lexer/lexer.h` | 文字列補間の字句解析 | 高 |
| `src/frontend/lexer/lexer.cpp` | 補間検出ロジック | 高 |
| `src/common/ast.h` | `StringInterpolationNode` 追加 | 高 |
| `src/frontend/parser/expressions/primary.cpp` | 文字列リテラルパース拡張 | 高 |
| `src/backend/interpreter/evaluator/primary_evaluator.cpp` | 補間評価ロジック | 高 |
| `src/backend/interpreter/utils/value_converter.cpp` | `value_to_string()` 実装 | 中 |

### 新規作成が推奨されるファイル

| ファイル | 目的 |
|---------|------|
| `src/frontend/lexer/string_interpolation_lexer.cpp` | 補間専用の字句解析ロジック |
| `tests/cases/string_interpolation/` | 補間テストケース群 |
| `tests/integration/string_interpolation/test_string_interpolation.hpp` | 統合テスト |

## テスト戦略

### 単体テスト（優先度順）

#### Phase 1: 基本機能
```cb
// test_interpolation_basic.cb
string name = "World";
string result = "Hello, ${name}!";
assert(result == "Hello, World!");
```

#### Phase 2: 型変換
```cb
// test_interpolation_types.cb
int num = 42;
long big = 1000000L;
char c = 'X';
bool flag = true;

assert("Num: ${num}" == "Num: 42");
assert("Big: ${big}" == "Big: 1000000");
assert("Char: ${c}" == "Char: X");
assert("Flag: ${flag}" == "Flag: true");
```

#### Phase 3: 式評価
```cb
// test_interpolation_expressions.cb
int a = 10, b = 20;
assert("Sum: ${a + b}" == "Sum: 30");
assert("Product: ${a * b}" == "Product: 200");
assert("Result: ${a + b * 2}" == "Result: 50");
```

#### Phase 4: 複雑な式
```cb
// test_interpolation_complex.cb
struct Point { int x; int y; }
Point p = Point{10, 20};
int[] arr = [1, 2, 3];

assert("Point: (${p.x}, ${p.y})" == "Point: (10, 20)");
assert("Array[1]: ${arr[1]}" == "Array[1]: 2");
```

#### Phase 5: 複数補間
```cb
// test_interpolation_multiple.cb
string first = "John";
string last = "Doe";
int age = 30;

string profile = "${first} ${last} is ${age} years old";
assert(profile == "John Doe is 30 years old");
```

#### Phase 6: エスケープ
```cb
// test_interpolation_escape.cb
string price = "\\${100}";
assert(price == "${100}");

string mixed = "Price: \\${100}, Tax: ${10 + 5}";
assert(mixed == "Price: ${100}, Tax: 15");
```

#### Phase 7: エッジケース
```cb
// test_interpolation_edge_cases.cb
// 空の補間
string empty = "${\"\"}"
assert(empty == "");

// ネストした文字列
string nested = "Outer: ${\"Inner: Hello\"}";
assert(nested == "Outer: Inner: Hello");

// 補間のみの文字列
string only_interp = "${42}";
assert(only_interp == "42");
```

### 統合テスト

```cpp
// tests/integration/string_interpolation/test_string_interpolation.hpp
namespace StringInterpolationTests {
    void run_all_tests() {
        // 基本テスト
        RUN_TEST(test_interpolation_basic);
        RUN_TEST(test_interpolation_types);
        RUN_TEST(test_interpolation_expressions);
        
        // 複雑なテスト
        RUN_TEST(test_interpolation_complex);
        RUN_TEST(test_interpolation_multiple);
        
        // エッジケース
        RUN_TEST(test_interpolation_escape);
        RUN_TEST(test_interpolation_edge_cases);
        
        // エラーハンドリング
        RUN_TEST(test_interpolation_errors);
    }
}
```

### パフォーマンステスト

```cb
// test_interpolation_performance.cb
void benchmark_interpolation() {
    int iterations = 10000;
    
    // 補間版
    long start1 = get_time_ms();
    for (int i = 0; i < iterations; i++) {
        string s = "Value: ${i}, Double: ${i * 2}";
    }
    long time1 = get_time_ms() - start1;
    
    // 連結版
    long start2 = get_time_ms();
    for (int i = 0; i < iterations; i++) {
        string s = "Value: " + to_string(i) + ", Double: " + to_string(i * 2);
    }
    long time2 = get_time_ms() - start2;
    
    println("Interpolation: ${time1}ms");
    println("Concatenation: ${time2}ms");
}
```

## エラーハンドリング

### コンパイルエラー

```cb
// 1. 閉じ括弧なし
string bad1 = "Hello ${name";
// Error: Unclosed string interpolation at line X

// 2. 不正な式
string bad2 = "Value: ${+}";
// Error: Invalid expression in string interpolation at line X

// 3. 未定義変数
string bad3 = "Hello ${undefined_var}";
// Error: Undefined variable 'undefined_var' in interpolation at line X
```

### ランタイムエラー

```cb
// 1. 変換不可能な型
struct CustomType { int x; }
CustomType obj = CustomType{10};
string bad = "Object: ${obj}";
// RuntimeError: Cannot convert struct to string at line X
// Suggestion: Implement toString() method or use member access

// 2. Null pointer
int* ptr = null;
string bad = "Pointer: ${ptr}";
// RuntimeError: Cannot dereference null pointer at line X
```

## 最適化戦略

### 1. コンパイル時最適化

**定数畳み込み**:
```cb
// 入力
string msg = "Result: ${10 + 20}";

// 最適化後
string msg = "Result: 30";
```

### 2. 効率的な文字列構築

**StringBuilder パターン**:
```cpp
// 非効率（繰り返し再割り当て）
std::string result;
result += part1;
result += part2;
result += part3;

// 効率的（事前サイズ予約）
std::string result;
result.reserve(estimated_size);
result += part1;
result += part2;
result += part3;
```

### 3. キャッシング

頻繁に使用される変換（`int`→`string`）をキャッシュ：

```cpp
static std::unordered_map<int, std::string> int_to_string_cache;

std::string cached_int_to_string(int value) {
    if (value >= -100 && value <= 1000) {
        auto it = int_to_string_cache.find(value);
        if (it != int_to_string_cache.end()) {
            return it->second;
        }
        auto str = std::to_string(value);
        int_to_string_cache[value] = str;
        return str;
    }
    return std::to_string(value);
}
```

## 将来的な拡張

### 1. フォーマット指定子

```cb
double pi = 3.14159265;
string formatted = "Pi: ${pi:.2f}";  // "Pi: 3.14"

int num = 42;
string hex = "Hex: ${num:x}";  // "Hex: 2a"
string padded = "Padded: ${num:05}";  // "Padded: 00042"
```

### 2. カスタム型変換

```cb
struct Person {
    string name;
    int age;
    
    string toString() {
        return "${name} (${age})";  // 再帰的補間
    }
}

Person p = Person{"Alice", 30};
string info = "Person: ${p}";
// "Person: Alice (30)" (toString()を自動呼び出し)
```

### 3. 多言語対応

```cb
// ロケール対応数値フォーマット
int num = 1234567;
string ja = "${num:ja}";  // "1,234,567" (日本語)
string de = "${num:de}";  // "1.234.567" (ドイツ語)
```

## 既存機能との互換性

### 破壊的変更なし

- 既存の文字列リテラル `"Hello"` は完全に動作
- 既存の文字列連結 `"Hello" + name` も引き続き使用可能
- エスケープシーケンス `\n`, `\t` など全て動作

### 移行パス

```cb
// 旧スタイル（引き続き有効）
string old = "Hello, " + name + "! Age: " + to_string(age);

// 新スタイル（推奨）
string new = "Hello, ${name}! Age: ${age}";
```

## タイムライン

| フェーズ | 期間 | タスク |
|---------|-----|--------|
| Phase 1 | 1-2日 | 字句解析実装 + 基本テスト |
| Phase 2 | 2-3日 | 構文解析 + AST構築 |
| Phase 3 | 2-3日 | インタプリタ評価 + 型変換 |
| Phase 4 | 1-2日 | エラーハンドリング |
| Phase 5 | 2-3日 | 統合テスト + エッジケース |
| Phase 6 | 1-2日 | パフォーマンス最適化 |
| Phase 7 | 1日 | ドキュメント整備 |

**合計見積もり**: 10-16日（約2-3週間）

## 成功基準

### 必須要件（v1.0）
- ✅ 基本的な変数補間 (`${variable}`)
- ✅ 全ての基本型の変換（int, string, char, bool等）
- ✅ 算術式の評価 (`${a + b}`)
- ✅ メンバーアクセス (`${obj.field}`)
- ✅ 配列アクセス (`${arr[i]}`)
- ✅ 複数補間のサポート
- ✅ エスケープ処理 (`\\${`)
- ✅ 包括的なテストスイート（50+ テスト）
- ✅ エラーメッセージの明確化

### オプション要件（v1.1以降）
- ⏳ フォーマット指定子 (`${value:.2f}`)
- ⏳ カスタム型の`toString()`メソッド
- ⏳ パフォーマンス最適化（キャッシング）
- ⏳ 三項演算子サポート (`${x > 0 ? "pos" : "neg"}`)

## リスク分析

| リスク | 影響 | 確率 | 対策 |
|-------|-----|------|------|
| 字句解析の複雑化 | 高 | 中 | 段階的実装、十分なテスト |
| 既存文字列処理との競合 | 高 | 低 | 互換性テスト、後方互換保証 |
| パフォーマンス劣化 | 中 | 低 | ベンチマーク、最適化 |
| エッジケースのバグ | 中 | 中 | 包括的テストスイート |
| 型変換の不整合 | 中 | 低 | 明確な変換ルール定義 |

## 参考資料

### 類似機能を持つ言語の仕様

1. **JavaScript (Template Literals)**
   - [MDN: Template Literals](https://developer.mozilla.org/en-US/docs/Web/JavaScript/Reference/Template_literals)

2. **Python (f-strings)**
   - [PEP 498 - Literal String Interpolation](https://www.python.org/dev/peps/pep-0498/)

3. **Kotlin (String Templates)**
   - [Kotlin String Templates](https://kotlinlang.org/docs/strings.html#string-templates)

4. **Swift (String Interpolation)**
   - [Swift String Interpolation](https://docs.swift.org/swift-book/LanguageGuide/StringsAndCharacters.html)

5. **Ruby (String Interpolation)**
   - [Ruby String Interpolation](https://ruby-doc.org/core-3.0.0/String.html#class-String-label-String+Interpolation)

## まとめ

文字列補間機能は、Cb言語の表現力と可読性を大幅に向上させる重要な機能です。

**主要な利点**:
- ✨ コードの可読性向上
- 🚀 開発効率の改善
- 🔒 型安全性の維持
- ⚡ 既存機能との完全互換性

**実装の方針**:
1. **段階的実装**: 基本機能から開始し、徐々に拡張
2. **十分なテスト**: 各フェーズで包括的なテスト
3. **パフォーマンス重視**: 効率的な実装を心がける
4. **後方互換性**: 既存コードを破壊しない

この設計ドキュメントに基づき、確実で高品質な実装を目指します。

---

**次のステップ**: Phase 1（字句解析）の実装開始

**関連ファイル**:
- 設計: `docs/features/string_interpolation.md` (このファイル)
- 実装レポート: `docs/features/string_interpolation_implementation.md` (実装後に作成)
- テストケース: `tests/cases/string_interpolation/`
- 統合テスト: `tests/integration/string_interpolation/test_string_interpolation.hpp`
