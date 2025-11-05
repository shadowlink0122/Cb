# Cb言語 BNF文法定義

**バージョン**: v0.11.0  
**最終更新**: 2025年11月5日

## 概要

このドキュメントはCb言語の完全なBNF（Backus-Naur Form）文法を定義しています。

---

## 表記規則

- `<非終端記号>`: 非終端記号（構文規則）
- `'終端記号'`: 終端記号（キーワード、リテラル）
- `|`: 選択（OR）
- `( )`: グループ化
- `[ ]`: オプション（0回または1回）
- `{ }`: 繰り返し（0回以上）
- `+`: 1回以上の繰り返し

---

## プログラム構造

```
<unary_expression> ::= <postfix_expression>
                     | '++' <unary_expression>
                     | '--' <unary_expression>
                     | <unary_operator> <unary_expression>

<unary_operator> ::= '&'    // アドレス演算子 ✅
                   | '*'    // デリファレンス ✅
                   | '!'    // 論理NOT ✅
                   | '~'    // ビットNOT ✅
                   | '+'    // 単項プラス ✅
                   | '-'    // 単項マイナス ✅m> ::= { <statement> }

<statement> ::= <variable_declaration>
              | <function_declaration>
              | <struct_declaration>
              | <interface_declaration>
              | <impl_block>
              | <typedef_declaration>
              | <union_typedef>
              | <enum_declaration>
              | <import_statement>
              | <export_statement>
              | <expression_statement>
              | <if_statement>
              | <for_statement>
              | <while_statement>
              | <switch_statement>
              | <match_statement>
              | <return_statement>
              | <break_statement>
              | <continue_statement>
              | <block>
```

---

## 変数宣言

```
<variable_declaration> ::= [ 'const' ] [ 'static' ] <type_specifier> <declarator_list> ';'

<declarator_list> ::= <declarator> { ',' <declarator> }

<declarator> ::= <identifier> [ '=' <initializer> ]

<initializer> ::= <expression>
                | <array_literal>
                | <struct_literal>
```

---

## 型システム

### 基本型

```
<type_specifier> ::= [ 'unsigned' ] <base_type>
                   | <pointer_type>
                   | <array_type>
                   | <struct_type>
                   | <interface_type>
                   | <typedef_type>
                   | <enum_type>

<base_type> ::= 'tiny'
              | 'short'
              | 'int'
              | 'long'
              | 'float'
              | 'double'
              | 'char'
              | 'string'
              | 'bool'
              | 'void'

<pointer_type> ::= <type_specifier> '*'                         // ポインタ型 ✅

<reference_type> ::= <type_specifier> '&'                        // 参照型 ✅

<array_type> ::= <type_specifier> '[' <integer_literal> ']' { '[' <integer_literal> ']' }  // 配列型 ✅
```

### 構造体

```
<struct_declaration> ::= 'struct' <identifier> [ <generic_params> ] '{' <member_list> '}' ';'  // 構造体 ✅

<generic_params> ::= '<' <generic_param_list> '>'

<generic_param_list> ::= <generic_param> { ',' <generic_param> }

<generic_param> ::= <identifier> [ ':' <interface_bound> ]

<interface_bound> ::= <identifier> { '+' <identifier> }

<generic_args> ::= '<' <type_list> '>'

<type_list> ::= <type_specifier> { ',' <type_specifier> }

<member_list> ::= <member_declaration> { <member_declaration> }

<member_declaration> ::= [ 'private' ] <type_specifier> <identifier> ';'

<struct_type> ::= 'struct' <identifier>

<struct_literal> ::= '{' <struct_initializer_list> '}'

<struct_initializer_list> ::= <struct_field_init> { ',' <struct_field_init> } [ ',' ]

<struct_field_init> ::= <identifier> ':' <expression>
                      | <expression>
```

**ジェネリクス構造体の例** (v0.11.0):
```cb
struct Box<T> {
    T value;
};

struct Pair<K, V> {
    K key;
    V value;
};

// 使用例
Box<int> int_box;
Pair<string, int> age_pair;
```

### Interface/Impl

```
<interface_declaration> ::= 'interface' <identifier> '{' <method_signature_list> '}' ';'

<method_signature_list> ::= <method_signature> { <method_signature> }

<method_signature> ::= <type_specifier> <identifier> '(' <parameter_list> ')' ';'

<impl_block> ::= 'impl' <identifier> [ <generic_params> ] [ 'for' <type_specifier> ] '{' <impl_body> '}' ';'

<impl_body> ::= { <impl_member> }

<impl_member> ::= <method_declaration>
                | <static_variable>
                | <constructor_declaration>
                | <destructor_declaration>

<constructor_declaration> ::= 'self' '(' <parameter_list> ')' <block>

<destructor_declaration> ::= '~' 'self' '(' ')' <block>

<method_list> ::= <method_declaration> { <method_declaration> }

<method_declaration> ::= [ 'private' ] <type_specifier> <identifier> '(' <parameter_list> ')' <block>

<interface_type> ::= <identifier>
```

### typedef

```
<typedef_declaration> ::= 'typedef' <type_specifier> <identifier> ';'
                        | 'typedef' <array_type> <identifier> ';'

<union_typedef> ::= 'typedef' <identifier> '=' <union_type_list> ';'

<union_type_list> ::= <union_member> { '|' <union_member> }

<union_member> ::= <type_specifier>
                 | <literal>
                 | <identifier>
```

### enum

```
<enum_declaration> ::= 'enum' <identifier> [ <generic_params> ] '{' <enum_member_list> '}' ';'
                     | 'typedef' 'enum' <identifier> '{' <enum_member_list> '}' <identifier> ';'

<enum_member_list> ::= <enum_member> { ',' <enum_member> } [ ',' ]

<enum_member> ::= <identifier> [ '=' <integer_literal> ]
                | <identifier> '(' <type_specifier> ')'  // 関連値を持つvariant (v0.11.0)

<enum_type> ::= 'enum' <identifier>

<enum_access> ::= <identifier> '::' <identifier> [ '(' <expression> ')' ]
```

**ジェネリクスenumの例** (v0.11.0):
```cb
enum Option<T> {
    Some(T),
    None
};

enum Result<T, E> {
    Ok(T),
    Err(E)
};

// 使用例
Option<int> some_val = Option<int>::Some(42);
Result<int, string> ok = Result<int, string>::Ok(100);
```

---

## 関数

```
<function_declaration> ::= [ 'export' ] <type_specifier> <identifier> [ <generic_params> ] '(' <parameter_list> ')' <block>

<parameter_list> ::= [ <parameter> { ',' <parameter> } ]

<parameter> ::= [ 'const' ] <type_specifier> <identifier> [ '=' <default_value> ]

<default_value> ::= <expression>
```

**ジェネリクス関数の例** (v0.11.0):
```cb
T identity<T>(T value) {
    return value;
}

T max<T>(T a, T b) {
    return a > b ? a : b;
}

// 使用例
int x = identity<int>(42);
int m = max<int>(10, 20);
```

---

## 制御構造

### if文

```
<if_statement> ::= 'if' '(' <expression> ')' <statement_or_block> [ <else_part> ]

<else_part> ::= 'else' 'if' '(' <expression> ')' <statement_or_block> [ <else_part> ]
              | 'else' <statement_or_block>

<statement_or_block> ::= <block>
                       | <statement>
```

### ループ

```
<for_statement> ::= 'for' '(' <for_init> ';' <for_condition> ';' <for_update> ')' <statement_or_block>

<for_init> ::= <variable_declaration>
             | <expression>
             | ε

<for_condition> ::= <expression>
                  | ε

<for_update> ::= <expression>
               | ε

<while_statement> ::= 'while' '(' <expression> ')' <statement_or_block>
```

### switch文

```
<switch_statement> ::= 'switch' '(' <expression> ')' '{' { <switch_case> } [ <default_case> ] '}'

<switch_case> ::= 'case' <constant_expression> ':' { <statement> }

<default_case> ::= 'default' ':' { <statement> }
```

### match文（パターンマッチング）

```
<match_statement> ::= 'match' '(' <expression> ')' '{' <match_arm_list> '}'

<match_arm_list> ::= <match_arm> { ',' <match_arm> } [ ',' ]

<match_arm> ::= <pattern> '=>' ( <statement> | <block> )

<pattern> ::= <enum_pattern>
            | <wildcard_pattern>

<enum_pattern> ::= <identifier> [ '(' <pattern_binding_list> ')' ]

<pattern_binding_list> ::= <pattern_binding> { ',' <pattern_binding> }

<pattern_binding> ::= <identifier>
                    | '_'

<wildcard_pattern> ::= '_'
```

### その他

```
<return_statement> ::= 'return' [ <expression> ] ';'

<break_statement> ::= 'break' ';'

<continue_statement> ::= 'continue' ';'

<defer_statement> ::= 'defer' <statement>

<block> ::= '{' { <statement> } '}'
```

---

## 式

### 基本式

```
<expression> ::= <assignment_expression>

<assignment_expression> ::= <ternary_expression>
                          | <unary_expression> <assignment_operator> <assignment_expression>

<assignment_operator> ::= '='
                        | '+=' | '-=' | '*=' | '/=' | '%='
                        | '&=' | '|=' | '^=' | '<<=' | '>>='

<ternary_expression> ::= <logical_or_expression> [ '?' <expression> ':' <ternary_expression> ]
```

### 論理演算

```
<logical_or_expression> ::= <logical_and_expression> { '||' <logical_and_expression> }

<logical_and_expression> ::= <bitwise_or_expression> { '&&' <bitwise_or_expression> }
```

### ビット演算

```
<bitwise_or_expression> ::= <bitwise_xor_expression> { '|' <bitwise_xor_expression> }

<bitwise_xor_expression> ::= <bitwise_and_expression> { '^' <bitwise_and_expression> }

<bitwise_and_expression> ::= <equality_expression> { '&' <equality_expression> }
```

### 比較演算

```
<equality_expression> ::= <relational_expression> { <equality_operator> <relational_expression> }

<equality_operator> ::= '==' | '!='

<relational_expression> ::= <shift_expression> { <relational_operator> <shift_expression> }

<relational_operator> ::= '<' | '<=' | '>' | '>='
```

### シフト演算

```
<shift_expression> ::= <additive_expression> { <shift_operator> <additive_expression> }

<shift_operator> ::= '<<' | '>>'
```

### 算術演算

```
<additive_expression> ::= <multiplicative_expression> { <additive_operator> <multiplicative_expression> }

<additive_operator> ::= '+' | '-'

<multiplicative_expression> ::= <unary_expression> { <multiplicative_operator> <unary_expression> }

<multiplicative_operator> ::= '*' | '/' | '%'
```

### 単項演算

```
<unary_expression> ::= <postfix_expression>
                     | '++' <unary_expression>
                     | '--' <unary_expression>
                     | <unary_operator> <unary_expression>

<unary_operator> ::= '&'    // アドレス演算子
                   | '*'    // デリファレンス
                   | '!'    // 論理NOT
                   | '~'    // ビットNOT
                   | '+'    // 正号
                   | '-'    // 負号
```

### 後置演算

```
<postfix_expression> ::= <primary_expression> { <postfix_operator> }

<postfix_operator> ::= '[' <expression> ']'                    // 配列アクセス ✅
                     | '(' <argument_list> ')'                 // 関数呼び出し ✅
                     | '.' <identifier>                        // メンバーアクセス ✅
                     | '->' <identifier>                       // ポインタメンバーアクセス ✅
                     | '++'                                    // 後置インクリメント ✅
                     | '--'                                    // 後置デクリメント ✅

<argument_list> ::= [ <expression> { ',' <expression> } ]
```

### 基本要素

```
<primary_expression> ::= <identifier>
                       | <literal>
                       | <array_literal>
                       | <struct_literal>
                       | <enum_access>
                       | '(' <expression> ')'
                       | 'self' '.' <identifier>              // implブロック内のみ ✅
                       | <function_pointer>                   // 関数ポインタ ✅

<function_pointer> ::= '&' <identifier>                       // 関数アドレス ✅

<literal> ::= <integer_literal>
            | <float_literal>
            | <char_literal>
            | <string_literal>
            | <boolean_literal>
```

---

## リテラル

```
<integer_literal> ::= <decimal_literal>
                    | <hex_literal>
                    | <binary_literal>
                    | <octal_literal>

<decimal_literal> ::= <digit>+

<hex_literal> ::= '0x' <hex_digit>+

<binary_literal> ::= '0b' ('0' | '1')+

<octal_literal> ::= '0' <octal_digit>+

<float_literal> ::= <digit>+ '.' <digit>+ [ <exponent> ] [ 'f' ]
                  | <digit>+ <exponent> [ 'f' ]

<exponent> ::= ('e' | 'E') ['+' | '-'] <digit>+

<char_literal> ::= "'" (<char> | <escape_sequence>) "'"

### 文字列補間構文

```
<string_literal> ::= '"' { <char> | <escape_sequence> | <interpolation> } '"'

<interpolation> ::= '{' <expression> [ ':' <format_specifier> ] '}'  // 文字列補間 (v0.11.0)

<format_specifier> ::= [ <width> ] [ '.' <precision> ] <format_type>

<width> ::= <digit>+

<precision> ::= <digit>+

<format_type> ::= 'd'    // 10進数
                | 'x'    // 16進数（小文字）
                | 'X'    // 16進数（大文字）
                | 'o'    // 8進数
                | 'b'    // 2進数
                | 'f'    // 浮動小数点数
                | 'e'    // 指数表記（小文字）
                | 'E'    // 指数表記（大文字）
                | 'g'    // 汎用フォーマット（小文字）
                | 'G'    // 汎用フォーマット（大文字）
```

**文字列補間の例** (v0.11.0):
```cb
int x = 42;
string name = "Alice";

// 基本的な変数埋め込み
string msg = "Hello, {name}! The answer is {x}";

// 式の埋め込み
int a = 10;
int b = 20;
println("Sum: {a + b}");  // "Sum: 30"

// フォーマット指定
int num = 255;
println("Hex: {num:x}");      // "Hex: ff"
println("Binary: {num:b}");   // "Binary: 11111111"

double pi = 3.14159;
println("Pi: {pi:.2f}");      // "Pi: 3.14"

// 構造体メンバーアクセス
struct Point {
    int x;
    int y;
};
Point p;
p.x = 10;
p.y = 20;
println("Point: ({p.x}, {p.y})");  // "Point: (10, 20)"

// エスケープ
println("Use {{}} for braces");  // "Use {} for braces"
```

<boolean_literal> ::= 'true' | 'false'

<array_literal> ::= '[' <expression_list> ']'

<expression_list> ::= [ <expression> { ',' <expression> } [ ',' ] ]
```

### エスケープシーケンス

```
<escape_sequence> ::= '\n'   // 改行
                    | '\t'   // タブ
                    | '\\'   // バックスラッシュ
                    | '\''   // シングルクォート
                    | '\"'   // ダブルクォート
                    | '\0'   // ヌル文字
```

---

## モジュールシステム

```
<import_statement> ::= 'import' <module_path> ';'  // v0.11.0で文字列リテラル構文を廃止

<module_path> ::= <identifier> { '.' <identifier> }

<export_statement> ::= 'export' <function_declaration>
                     | 'export' <variable_declaration>
                     | 'export' <impl_block>
                     | 'export' <interface_impl_block>
```

**注意**: v0.11.0より、文字列リテラルimport構文（`import "path/to/file.cb";`）は廃止されました。

**旧構文（廃止）**:
```cb
import "path/to/module.cb";  // ❌ 廃止
```

**新構文**:
```cb
import module.path.name;  // ✅ v0.11.0以降
import collections.vector;
import std.allocators.system;
```

---

## 識別子と文字

```
<identifier> ::= <letter> { <letter> | <digit> | '_' }

<letter> ::= 'a'..'z' | 'A'..'Z' | '_'

<digit> ::= '0'..'9'

<hex_digit> ::= '0'..'9' | 'a'..'f' | 'A'..'F'

<octal_digit> ::= '0'..'7'

<char> ::= <任意のUTF-8文字（制御文字とバックスラッシュを除く）>
```

---

## コメント

```
<comment> ::= <line_comment>
            | <block_comment>

<line_comment> ::= '//' { <任意の文字> } <改行>

<block_comment> ::= '/*' { <任意の文字> } '*/'
```

---

## 演算子優先順位（高→低）

| レベル | 演算子 | 結合性 |
|--------|--------|--------|
| 1 | `()` `[]` `.` `->` | 左→右 |
| 2 | `++` `--` (前置) `&` `*` (単項) `!` `~` `+` `-` (単項) | 右→左 |
| 3 | `*` `/` `%` | 左→右 |
| 4 | `+` `-` | 左→右 |
| 5 | `<<` `>>` | 左→右 |
| 6 | `<` `<=` `>` `>=` | 左→右 |
| 7 | `==` `!=` | 左→右 |
| 8 | `&` | 左→右 |
| 9 | `^` | 左→右 |
| 10 | `|` | 左→右 |
| 11 | `&&` | 左→右 |
| 12 | `||` | 左→右 |
| 13 | `?:` | 右→左 |
| 14 | `=` `+=` `-=` `*=` `/=` `%=` `&=` `|=` `^=` `<<=` `>>=` | 右→左 |
| 15 | `++` `--` (後置) | 左→右 |

---

## キーワード一覧

```
型関連:
  tiny, short, int, long, float, double, char, string, bool, void
  unsigned, const, static

構造:
  struct, interface, impl, typedef, enum, union, private

制御:
  if, else, for, while, break, continue, return

関数:
  export, import

リテラル:
  true, false

特殊:
  self
```

---

## 予約語

以下のキーワードは予約語として識別子に使用できません：

```
break, case, char, const, continue, default, defer, do, double, else, enum,
export, extern, float, for, goto, if, impl, import, int, interface,
long, match, return, short, signed, sizeof, static, string, struct, switch,
tiny, typedef, union, unsigned, void, while, bool, true, false, self,
malloc, free, new, delete, array_get, array_set
```

---

## 組み込み関数とメモリ管理

### メモリ管理関数

```
<malloc_call> ::= 'malloc' '(' <expression> ')'
<free_call> ::= 'free' '(' <expression> ')'
<new_expression> ::= 'new' <type_specifier> [ '(' <argument_list> ')' ]  // 計画中
<delete_statement> ::= 'delete' <expression> ';'  // 計画中
```

**malloc/free の例**:
```cb
// メモリ確保
void* ptr = malloc(40);  // 40バイト確保
int* int_ptr = ptr;      // 型キャスト

// 使用
int_ptr[0] = 10;
int_ptr[1] = 20;

// 解放
free(ptr);
ptr = NULL;
```

**new/delete の例（計画中）**:
```cb
// 型安全な確保
int* ptr = new int;
*ptr = 42;
delete ptr;

// 構造体
struct Point {
    int x;
    int y;
};

Point* p = new Point;
p->x = 10;
delete p;  // デストラクタ自動呼び出し
```

### 配列操作関数

```
<array_get_call> ::= 'array_get' '(' <expression> ',' <expression> ',' <expression> ')'
<array_set_call> ::= 'array_set' '(' <expression> ',' <expression> ',' <expression> ',' <expression> ')'
```

**配列境界チェック付きアクセス**:
```cb
int[5] arr = [10, 20, 30, 40, 50];

// 安全な取得
int value = array_get(arr, 2, 5);  // 30

// 安全な設定
array_set(arr, 2, 100, 5);  // arr[2] = 100

// 範囲外アクセスは実行時エラー
// int bad = array_get(arr, 10, 5);  // Error: Array index out of bounds
```

### 組み込みジェネリック型

```
<option_type> ::= 'Option' '<' <type_specifier> '>'
<result_type> ::= 'Result' '<' <type_specifier> ',' <type_specifier> '>'

<option_construction> ::= 'Option' '<' <type_specifier> '>' '::' ( 'Some' '(' <expression> ')' | 'None' )
<result_construction> ::= 'Result' '<' <type_specifier> ',' <type_specifier> '>' '::' ( 'Ok' '(' <expression> ')' | 'Err' '(' <expression> ')' )
```

**Option<T> の例**:
```cb
enum Option<T> {
    Some(T),
    None
};

Option<int> find_index(int[10] arr, int value) {
    for (int i = 0; i < 10; i++) {
        if (arr[i] == value) {
            return Option<int>::Some(i);
        }
    }
    return Option<int>::None;
}

// 使用
Option<int> result = find_index(arr, 42);
match (result) {
    Some(index) => println("Found at {index}"),
    None => println("Not found")
}
```

**Result<T, E> の例**:
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

// 使用
Result<int, string> res = divide(10, 2);
match (res) {
    Ok(value) => println("Result: {value}"),
    Err(error) => println("Error: {error}")
}
```

---

## サンプルプログラム

### 基本的な構造

```c++
// 変数宣言
int x = 10;
const int MAX = 100;
unsigned int counter = 0;

// 配列
int[5] arr = [1, 2, 3, 4, 5];
int[3][3] matrix = [[1,2,3], [4,5,6], [7,8,9]];

// 構造体
struct Point {
    int x;
    int y;
};

struct Account {
    int id;
    private int balance;  // privateメンバー
};

Point p = {x: 10, y: 20};

// 関数
int add(int a, int b) {
    return a + b;
}

// Interface/Impl
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
    
    private int calculatePerimeter() {
        return 2 * (self.width + self.height);
    }
};

// ポインタ
int* ptr = &x;
*ptr = 20;
ptr++;

// 制御構造
if (x > 10) {
    println("x is greater than 10");
} else {
    println("x is 10 or less");
}

for (int i = 0; i < 10; i++) {
    println(i);
}

while (counter < MAX) {
    counter++;
}

// typedef
typedef int Integer;
typedef int[5] IntArray;
typedef Status = 200 | 404 | 500;

// enum
enum Color {
    RED = 0,
    GREEN = 1,
    BLUE = 2
};

Color c = Color::RED;
```

---

## 文法の特徴

### 1. 型安全性

- 静的型付け
- コンパイル時の厳密な型チェック
- Union型による柔軟な型表現

### 2. C/C++互換性

- C風の基本構文
- ポインタと参照
- 構造体とenum

### 3. モダンな機能

- Interface/Implシステム（Rust風）
- Union型（TypeScript風）
- モジュールシステム

### 4. 明示的なメモリ管理

- ガベージコレクションなし
- RAIIベースの自動メモリ管理
- 静的配列のみ（動的配列は将来実装）

---

**BNF定義 v0.11.0**  
最終更新: 2025年11月3日
