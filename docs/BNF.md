# Cb言語 BNF文法定義

**バージョン**: v0.10.0  
**最終更新**: 2025年10月12日

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
<struct_declaration> ::= 'struct' <identifier> '{' <member_list> '}' ';'  // 構造体 ✅

<member_list> ::= <member_declaration> { <member_declaration> }

<member_declaration> ::= [ 'private' ] <type_specifier> <identifier> ';'

<struct_type> ::= 'struct' <identifier>

<struct_literal> ::= '{' <struct_initializer_list> '}'

<struct_initializer_list> ::= <struct_field_init> { ',' <struct_field_init> } [ ',' ]

<struct_field_init> ::= <identifier> ':' <expression>
                      | <expression>
```

### Interface/Impl

```
<interface_declaration> ::= 'interface' <identifier> '{' <method_signature_list> '}' ';'

<method_signature_list> ::= <method_signature> { <method_signature> }

<method_signature> ::= <type_specifier> <identifier> '(' <parameter_list> ')' ';'

<impl_block> ::= 'impl' <identifier> 'for' <type_specifier> '{' <method_list> '}' ';'

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
<enum_declaration> ::= 'enum' <identifier> '{' <enum_member_list> '}' ';'
                     | 'typedef' 'enum' <identifier> '{' <enum_member_list> '}' <identifier> ';'

<enum_member_list> ::= <enum_member> { ',' <enum_member> } [ ',' ]

<enum_member> ::= <identifier> [ '=' <integer_literal> ]

<enum_type> ::= 'enum' <identifier>

<enum_access> ::= <identifier> '::' <identifier>
```

---

## 関数

```
<function_declaration> ::= [ 'export' ] <type_specifier> <identifier> '(' <parameter_list> ')' <block>

<parameter_list> ::= [ <parameter> { ',' <parameter> } ]

<parameter> ::= [ 'const' ] <type_specifier> <identifier>
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

### その他

```
<return_statement> ::= 'return' [ <expression> ] ';'

<break_statement> ::= 'break' ';'

<continue_statement> ::= 'continue' ';'

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

<string_literal> ::= '"' { <char> | <escape_sequence> } '"'

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
<import_statement> ::= 'import' <string_literal> ';'

<export_statement> ::= 'export' <function_declaration>
                     | 'export' <variable_declaration>
                     | 'export' <impl_block>
                     | 'export' <interface_impl_block>
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
break, case, char, const, continue, default, do, double, else, enum,
export, extern, float, for, goto, if, impl, import, int, interface,
long, return, short, signed, sizeof, static, string, struct, switch,
tiny, typedef, union, unsigned, void, while, bool, true, false, self
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

**BNF定義 v0.9.0**  
最終更新: 2025年10月5日
