# Cb言語仕様書

## 概要

モダンなC++のようなイメージ。構文はC++ベースにする。

### 影響されている言語とその部分
- **C/C++**
    - 変数宣言、関数宣言
    - RAII（Resource Acquisition Is Initialization）によるメモリ管理
- **TypeScript**
    - 型宣言, typedef
    - ライブラリ、モジュールインポート
    - ラムダ式、仮想関数
- **Go**
    - 配列・スライス
    - 非同期処理（goroutine、channel）
- **Rust**
    - interface, trait
    - ジェネリクス（ライフタイムは入れない）
    - Result型によるエラー処理（例外処理機構は入れない）

### パフォーマンス目標
- Ruby, Pythonより高速
- GCを使わないゼロコスト抽象化
- C++スマートポインタライクなRAII自動メモリ管理

### エラー処理戦略
用途に応じてカスタマイズ可能なエラー処理システム：

1. **Result型モード**（推奨・デフォルト）
   - コンパイラ、システムプログラミング用途
   - `Result<T, E>` 型による明示的エラーハンドリング
   - パターンマッチングによる安全なエラー処理

2. **エラーコードモード**
   - 軽量・高速が要求される用途（組み込み、フロントエンド等）
   - 現在のインタープリター実装で使用
   - 従来のC言語的な戻り値によるエラー処理

3. **ハイブリッドモード**
   - アプリケーション層: Result型
   - システム層: エラーコード
   - コンパイル時フラグで切り替え可能

### メモリ管理戦略
**RAII + スマートポインタによる自動管理**
- `unique_ptr<T>`：排他的所有権
- `shared_ptr<T>`：共有所有権  
- `weak_ptr<T>`：弱参照
- ガベージコレクションは使用しない（パフォーマンス重視）
- C++17/20の`std::optional`、`std::variant`活用

### 将来的にできること
- ビルドし、バイナリを生成できるようにできる
    - IR生成
    - 最適化アルゴリズム
    - 実行ファイル作成
- OSが書けるようになる
    - 低レイヤー領域では Go より便利になれば嬉しい
- ウェブフレームワークを提供できる
    - Railsより明示的に書けること
    - React, Ginなどのように REST API などが使える
- フロントが書ける
    - JavaScript, TypeScript に置き換わるものになる
    - 特に TypeScriptに近いものができれば良い

## 実装状況

### Phase 1: 基本機能 ✅（完成）
- ✅ 型システム（プリミティブ型：tiny, short, int, long, string, char, bool）
- ✅ 変数宣言・初期化（複数変数同時宣言対応）
- ✅ 配列（静的サイズ）・配列リテラル
- ✅ 関数定義・呼び出し（複合代入時の最適化済み）
- ✅ 制御構造（if/else, for, while, break, continue, return）
- ✅ **演算子（算術、比較、論理、代入、複合代入、インクリメント）**
  - ✅ **10種類の複合代入演算子**: `+=`, `-=`, `*=`, `/=`, `%=`, `&=`, `|=`, `^=`, `<<=`, `>>=`
  - ✅ **前置・後置インクリメント/デクリメント**: `++var`, `--var`, `var++`, `var--`
  - ✅ **配列要素複合代入**: `arr[index] += value`、`arr[i*2+1] *= (x+y)`
  - ✅ **自己代入機能**: 基本的な自己代入、配列自己代入、ビット演算自己代入
- ✅ ストレージ修飾子（const, static）
- ✅ 標準出力（print, printf風フォーマット指定子）
- ✅ **Union型システム**（TypeScript風完全実装）
  - ✅ **リテラル値Union**: `typedef Status = 200 | 404 | 500;`
  - ✅ **基本型Union**: `typedef Value = int | string;`
  - ✅ **カスタム型Union**: `typedef ID = UserID | ProductID;`
  - ✅ **構造体Union**: `typedef Entity = User | Product;`
  - ✅ **配列Union**: `typedef ArrayUnion = int[5] | string[3];`
  - ✅ **混合Union**: `typedef Mixed = 42 | int | string;`
  - ✅ **文字列処理**: 再帰的文字列実装を活用した完全な文字列対応
  - ✅ **複合代入**: `+=`, `-=`, `*=`, `/=`, `%=` 演算子完全対応
  - ✅ **型安全性**: 型不一致時の適切なエラーメッセージ
  - ✅ **再帰的typedef互換性**: 継承チェーンでの型検証
  - ✅ **包括的エラーハンドリング**: 13種類の異常系テスト
- ✅ 包括的テストフレームワーク（統合テスト925個、単体テスト26個）
- ✅ 再帰下降パーサーによる構文解析

### Phase 2: 中期目標 ✅/🚧（実装中）
- ✅ **struct 定義** - 構造体機能完全実装（リテラル初期化、配列メンバー、構造体配列）
- ✅ **Union型システム** - TypeScript風Union型完全実装（リテラル値、基本型、カスタム型、構造体、配列Union対応、文字列処理・複合代入演算子完全対応）
- ✅ **typedef システム** - 基本typedef、Union typedef、recursive typedef対応
- 🚧 enum 定義（基本実装済み、Union連携は部分実装）
- 🚧 標準ライブラリ拡充（math.cb, stdio.cb部分実装）
- ❌ Result型エラー処理
- ❌ スマートポインタ（unique_ptr, shared_ptr）

#### 構造体機能詳細 ✅（完全実装）
- **基本構造体定義・使用**: 完全実装 ✅
- **構造体リテラル初期化**: 名前付き・位置指定両対応 ✅
- **構造体配列メンバー**: 個別代入・配列リテラル代入両対応 ✅
- **構造体の配列**: 構造体配列リテラル初期化対応 ✅
- **printf/println統合**: 構造体メンバー・配列要素完全対応 ✅
- **統合テスト**: 767/767 (100%) ✅
- **実践サンプル**: ダイクストラ法アルゴリズム実装 ✅

#### 構造体未実装機能 ❌
- **ネストした構造体**: `obj.member.submember` 未サポート
- **構造体関数引数・戻り値**: 未実装
- **複雑なネストアクセス**: `obj.array[i].member[j]` 未サポート
- **構造体継承**: 未実装

### Phase 3: 長期目標 ❌（未実装）
- ❌ interface/trait システム
- ❌ モジュール・インポートシステム
- ❌ ジェネリクス
- ❌ 非同期処理（goroutine/channel）
- ❌ ラムダ式
- ❌ アトリビュート

## 構文定義（BNF記法）

### 基本構文
```bnf
program         ::= declaration_list

declaration_list ::= declaration
                  | declaration_list declaration

declaration     ::= variable_declaration
                  | function_declaration
                  | struct_declaration
                  | typedef_declaration
                  | union_typedef_declaration
                  | enum_declaration

type_specifier  ::= "void" | "tiny" | "short" | "int" | "long" 
                  | "string" | "char" | "bool" | identifier
                  | type_specifier "[" constant_expression "]"

storage_class   ::= "const" | "static" | "extern"
```

### 変数宣言
```bnf  
variable_declaration ::= storage_class* type_specifier declarator_list ";"

declarator_list ::= declarator
                  | declarator_list "," declarator

declarator      ::= identifier
                  | identifier "=" initializer
                  | identifier "[" constant_expression "]"  
                  | identifier "[" constant_expression "]" "=" array_initializer

initializer     ::= assignment_expression
                  | array_initializer

array_initializer ::= "[" initializer_list "]"
                    | "[" initializer_list "," "]"

initializer_list ::= initializer
                   | initializer_list "," initializer
```

### 型宣言・Union型
```bnf
typedef_declaration ::= "typedef" type_specifier identifier ";"
                      | union_typedef_declaration

union_typedef_declaration ::= "typedef" identifier "=" union_type_list ";"

union_type_list ::= union_type
                  | union_type_list "|" union_type

union_type ::= literal_value
             | type_specifier
             | identifier
             | array_type_specifier

literal_value ::= integer_constant
                | string_literal
                | character_literal
                | boolean_constant

array_type_specifier ::= type_specifier "[" constant_expression "]"
```
function_declaration ::= storage_class* type_specifier identifier 
                        "(" parameter_list ")" compound_statement

parameter_list  ::= "void"
                  | parameter_declaration_list

parameter_declaration_list ::= parameter_declaration  
                             | parameter_declaration_list "," parameter_declaration

parameter_declaration ::= type_specifier identifier
```

### 文（Statement）
```bnf
statement       ::= expression_statement
                  | compound_statement
                  | selection_statement
                  | iteration_statement  
                  | jump_statement
                  | print_statement

compound_statement ::= "{" statement_list "}"

statement_list  ::= statement
                  | statement_list statement

expression_statement ::= expression ";"

selection_statement ::= "if" "(" expression ")" statement
                      | "if" "(" expression ")" statement "else" statement

iteration_statement ::= "while" "(" expression ")" statement
                      | "for" "(" expression ";" expression ";" expression ")" statement
                      | "for" "(" variable_declaration expression ";" expression ")" statement

jump_statement  ::= "break" ";"
                  | "continue" ";"
                  | "return" ";"  
                  | "return" expression ";"

print_statement ::= "print" "(" argument_list ")" ";"
```

### 式（Expression）
```bnf
expression      ::= assignment_expression
                  | expression "," assignment_expression

assignment_expression ::= logical_or_expression
                        | unary_expression assignment_operator assignment_expression

assignment_operator ::= "=" | "+=" | "-=" | "*=" | "/=" | "%=" 
                      | "&=" | "|=" | "^=" | "<<=" | ">>="

logical_or_expression ::= logical_and_expression
                        | logical_or_expression "||" logical_and_expression

logical_and_expression ::= equality_expression
                         | logical_and_expression "&&" equality_expression

equality_expression ::= relational_expression  
                      | equality_expression "==" relational_expression
                      | equality_expression "!=" relational_expression

relational_expression ::= additive_expression
                        | relational_expression "<" additive_expression
                        | relational_expression ">" additive_expression  
                        | relational_expression "<=" additive_expression
                        | relational_expression ">=" additive_expression

additive_expression ::= multiplicative_expression
                      | additive_expression "+" multiplicative_expression
                      | additive_expression "-" multiplicative_expression

multiplicative_expression ::= unary_expression
                            | multiplicative_expression "*" unary_expression
                            | multiplicative_expression "/" unary_expression
                            | multiplicative_expression "%" unary_expression

unary_expression ::= postfix_expression
                   | "++" unary_expression
                   | "--" unary_expression  
                   | "!" unary_expression
                   | "+" unary_expression
                   | "-" unary_expression

postfix_expression ::= primary_expression
                     | postfix_expression "[" expression "]"
                     | postfix_expression "(" argument_list ")"
                     | postfix_expression "++"
                     | postfix_expression "--"

primary_expression ::= identifier
                     | constant
                     | string_literal
                     | character_literal
                     | array_literal
                     | "(" expression ")"

constant        ::= integer_constant | character_constant | boolean_constant

character_literal ::= "'" character "'"
                    | "'" escape_sequence "'"

array_literal   ::= "[" argument_list "]"
                  | "[" "]"

escape_sequence ::= "\n" | "\t" | "\\" | "\'" | "\0" | "\"" | "\r"

argument_list   ::= assignment_expression
                  | argument_list "," assignment_expression
```

## 言語機能詳細

### 変数

#### プリミティブ型（TYPE）✅
- `void`: 戻り値なし型（変数宣言不可）
- `tiny`: 8bit符号付き整数 (-128〜127)
- `short`: 16bit符号付き整数 (-32768〜32767)  
- `int`: 32bit符号付き整数 (-2^31〜2^31-1)
- `long`: 64bit符号付き整数 (-2^63〜2^63-1)
    - 整数型デフォルト値: 0
- `char`: 8bit文字型 (0〜255)
    - 文字リテラル: 'A', '\n', '\t', '\\', '\'', '\0' 等をサポート
    - デフォルト値: '\0'
- `string`: UTF-8文字列
    - デフォルト値: ""（空文字列）
    - 現在は std::string のラッパー
    - 将来: ベアメタル環境対応（ポインタ隠蔽）
- `bool`: 真偽値型
    - デフォルト値: false

#### 配列型システム ✅/🚧
##### 静的配列（現在実装済み）
- `TYPE[SIZE]`: 固定サイズ配列
- 多次元配列対応: `int[10][20]`, `string[5][3][2]`
- **型安全性**: 
  - コンパイル時サイズ検証: `int[2] arr = [1, 2, 3];` → エラー
  - 関数戻り値サイズ検証: `int[5] arr = returnThreeElements();` → エラー
  - 明確なエラーメッセージ提供

##### 動的配列（将来実装予定）🚧
- `TYPE[]`: 可変サイズ配列（Goのsliceライク）
- **共通メソッド**（struct/interface実装後）:
  - `.size()`: 要素数取得
  - `.len()`: 要素数取得（.size()のエイリアス）
  - `.capacity()`: 容量取得
  - `.empty()`: 空配列判定
- **動的配列専用メソッド**:
  - `.push(value)`: 末尾に要素追加
  - `.pop()`: 末尾要素削除・取得
  - `.clear()`: 全要素削除
  - `.reserve(size)`: 容量予約
  - `.resize(size)`: サイズ変更
- **境界チェック**:
  - 配列アクセス時の自動境界検証
  - デバッグモードでの詳細エラー情報
- **メモリ管理**:
  - RAII原則による自動メモリ管理
  - 効率的な再割り当て戦略（capacity doubling等）

##### 配列型安全性の将来強化 🚧
- **型レベルでのサイズ検証**:
  ```cb
  int[3] a = [1, 2, 3];
  int[5] b = [1, 2, 3, 4, 5];
  a = b;  // エラー: サイズが異なる静的配列間の代入は不可
  ```
- **関数引数での型チェック**:
  ```cb
  void func(int[5] arr);  // 5要素の配列のみ受け入れ
  int[3] small_array = [1, 2, 3];
  func(small_array);  // エラー: サイズ不一致
  ```
- **配列スライス操作**（動的配列）:
  ```cb
  int[] arr = [1, 2, 3, 4, 5];
  int[] slice = arr[1:4];  // [2, 3, 4]を取得
  ```

#### 型宣言方法 ✅
```cb
TYPE 変数;                     // デフォルト値で初期化
TYPE 変数 = 値;                // 明示的初期化
TYPE[SIZE] 変数;               // 配列宣言
TYPE[SIZE] 変数 = {値1, 値2, ...}; // 配列初期化
TYPE[SIZE1][SIZE2] 変数;       // 多次元配列宣言
```

#### ストレージ修飾子 ✅
C/C++と同等の機能を提供:
- `const`: 定数、変更不可（Rustのmutの逆）
- `static`: 静的ストレージ期間
- `extern`: 外部リンケージ（将来のmoduleシステムと連携）

### 関数 ✅
```cb
// void関数（戻り値なし）
void FUNC_NAME() { 
    // 文;
}

// 値を返す関数
TYPE FUNC_NAME(void) { 
    // 文;
    return VALUE; 
}

// 引数を取る関数
TYPE FUNC_NAME(TYPE ARG1, TYPE ARG2, ...) { 
    // 文; 
    return VALUE; 
}
```
- VALUE の型は TYPE と一致する必要がある
- 引数も戻り値も配列型に対応

### 型宣言・作成 ✅
```cb
// 基本型エイリアス
typedef 既存型 新しい型名;

// Union型宣言（TypeScript風）
typedef Union型名 = 型1 | 型2 | 型3 | ...;

// 例
typedef UserId = int;
typedef UserName = string;

// リテラル値Union
typedef HttpStatus = 200 | 404 | 500;
typedef Direction = "up" | "down" | "left" | "right";

// 基本型Union
typedef NumericValue = int | long | string;

// カスタム型Union
typedef ID = UserId | ProductId;

// 構造体Union
typedef Entity = User | Product;

// 配列Union
typedef ArrayUnion = int[5] | string[3];

// 混合Union（リテラル値と型の組み合わせ）
typedef MixedUnion = 42 | int | string;
```

### Union型システム（完全実装） ✅

#### TypeScript風Union型構文
```cb
// リテラル値Union - 特定の値のみ許可
typedef HttpStatus = 200 | 404 | 500;
typedef Direction = "up" | "down" | "left" | "right";
typedef Priority = 1 | 2 | 3;

// 基本型Union - 複数の基本型を組み合わせ
typedef NumericValue = int | long | string;
typedef BoolOrString = bool | string;
typedef AnyBasic = int | string | bool;

// カスタム型Union - 定義したtypedefを組み合わせ
typedef UserID = int;
typedef ProductID = string;
typedef ID = UserID | ProductID;  // 再帰的typedef対応

// 構造体Union - 異なる構造体型を組み合わせ
struct User {
    int id;
    string name;
}

struct Product {
    string code;
    int price;
}

typedef Entity = User | Product;

// 配列Union - 異なる配列型を組み合わせ
typedef ArrayUnion = int[5] | string[3] | bool[2];
typedef MultiDimArrayUnion = int[3][3] | string[2][4];

// 混合Union - リテラル値と型を組み合わせ
typedef MixedUnion = 42 | int | string;
typedef ComplexMixed = 200 | "success" | int | bool;
```

#### Union型の型検証システム
```cb
int main() {
    // リテラル値Unionの厳密な検証
    HttpStatus status = 200;  // OK: 許可されたリテラル値
    // HttpStatus invalid = 301;  // エラー: 許可されていない値
    
    // 基本型Unionの使用
    NumericValue value1 = 42;        // int値 -> OK
    NumericValue value2 = "hello";   // string値 -> OK
    // NumericValue invalid = true;  // bool型 -> エラー
    
    // カスタム型Unionの再帰的型検証
    UserID user_id = 12345;
    ID general_id = user_id;         // UserID -> ID: 互換性OK
    
    // 構造体Unionの型互換性
    User alice = {id: 1, name: "Alice"};
    Entity entity = alice;           // User -> Entity: OK
    
    // 混合Unionでの型検証
    MixedUnion mixed1 = 42;          // リテラル42 -> OK
    MixedUnion mixed2 = 100;         // int型 -> OK
    MixedUnion mixed3 = "test";      // string型 -> OK
    // MixedUnion invalid = true;    // bool型 -> エラー
    
    return 0;
}
```

#### Union型エラーハンドリング
Union型システムは13種類の包括的エラー検証を提供：

1. **無効なリテラル値エラー**
```cb
typedef LimitedValues = 1 | 2 | 3;
LimitedValues val = 5;  // エラー: 5は許可されていない
```

2. **型不一致エラー**
```cb
typedef IntOrString = int | string;
bool flag = true;
IntOrString val = flag;  // エラー: bool型は許可されていない
```

3. **未定義型エラー**
```cb
typedef BadUnion = int | UndefinedType;  // エラー: UndefinedTypeは存在しない
```

4. **カスタム型互換性エラー**
```cb
typedef UserID = int;
typedef ProductID = string;
typedef RestrictedID = UserID;  // ProductIDは許可されない

ProductID pid = "P123";
RestrictedID rid = pid;  // エラー: ProductID -> UserIDの変換は不可
```

5. **構造体型エラー**
```cb
struct User { int id; }
struct Product { string code; }
typedef UserOnly = User;

Product prod = {"P123"};
UserOnly user_val = prod;  // エラー: Product -> Userの変換は不可
```

6. **配列型不一致エラー**
```cb
typedef IntArrayUnion = int[3];
string[3] str_arr = ["a", "b", "c"];
IntArrayUnion arr = str_arr;  // エラー: string[3] -> int[3]は不可
```

**特徴**:
- **TypeScript風セマンティクス**: 直感的なUnion型構文
- **厳密な型検証**: リテラル値と明示的型の明確な区別
- **再帰的typedef対応**: UserID -> ID -> int のような継承チェーン
- **包括的エラー処理**: 13種類の異常系を完全にカバー
- **実行時型安全性**: 不正な型代入を実行時に検出
- **混合Union対応**: リテラル値と型を自由に組み合わせ可能

#### 基本構造体（実装済み）✅
```cb
struct 構造体名 { 
    TYPE メンバ1; 
    TYPE メンバ2;
    TYPE[SIZE] 配列メンバ;
    // ...
}

// 使用例
struct Person {
    string name;
    int age;
    int grades[5];
    bool is_active;
}

int main() {
    Person p;
    p.name = "Alice";
    p.age = 25;
    p.is_active = true;
    
    // 配列メンバーの個別代入
    p.grades[0] = 85;
    p.grades[1] = 92;
    
    // 配列リテラル代入
    p.grades = [85, 92, 78, 90, 88];
    
    return 0;
}
```

#### 構造体リテラル初期化（実装済み）✅
```cb
struct Point {
    int x;
    int y;
    string label;
}

int main() {
    // 名前付き初期化
    Point p1 = {x: 10, y: 20, label: "Origin"};
    
    // 位置指定初期化
    Point p2 = {30, 40, "Target"};
    
    // 構造体配列初期化
    Point[3] points = [
        {x: 0, y: 0, label: "Start"},
        {10, 10, "Middle"},
        {x: 20, y: 20, label: "End"}
    ];
    
    return 0;
}
```

#### 構造体配列メンバー（実装済み）✅
```cb
struct Matrix {
    string name;
    int data[6];  // 2x3行列として使用
    int rows;
    int cols;
}

int main() {
    Matrix m;
    m.name = "Sample Matrix";
    m.rows = 2;
    m.cols = 3;
    
    // 配列リテラル代入
    m.data = [1, 2, 3, 4, 5, 6];
    
    // 個別要素代入
    m.data[0] = 10;
    m.data[5] = 60;
    
    // printf統合
    print("Matrix %s: [%d, %d, %d]", m.name, m.data[0], m.data[1], m.data[2]);
    
    return 0;
}
```

#### 構造体の配列（実装済み）✅
```cb
struct Employee {
    string name;
    int salary;
    int department_id;
}

int main() {
    Employee team[3];
    
    // 構造体配列リテラル初期化
    team[0] = {name: "Alice", salary: 50000, department_id: 0};
    team[1] = {name: "Bob", salary: 55000, department_id: 2};
    team[2] = {name: "Charlie", salary: 60000, department_id: 1};
    
    // 配列要素のメンバーアクセス
    print("Employee: %s, Salary: $%d", team[0].name, team[0].salary);
    
    return 0;
}
```

#### 未実装機能 🚧/❌

##### ネストした構造体（未実装）❌
```cb
struct Address {
    string street;
    string city;
    int zipcode;
}

struct Company {
    string name;
    Address address;  // ❌ ネストした構造体未サポート
    int employee_count;
}

int main() {
    Company tech_corp;
    tech_corp.name = "Tech Corp";
    
    // ❌ エラー: ネストしたメンバーアクセス未サポート
    tech_corp.address.street = "123 Main St";
    
    return 0;
}
```

##### 構造体の関数引数・戻り値（未実装）❌
```cb
struct Rectangle {
    int width;
    int height;
}

// ❌ 未実装: 構造体引数
int calculate_area(Rectangle rect) {
    return rect.width * rect.height;
}

// ❌ 未実装: 構造体戻り値
Rectangle create_rectangle(int w, int h) {
    Rectangle r = {width: w, height: h};
    return r;
}
```

##### 構造体継承（未実装）❌
```cb
// 継承（将来実装）
struct 派生構造体名 extends 基底構造体名 { 
    TYPE 追加メンバ;
    // ...
}
```

##### 複雑なネストした配列アクセス（未実装）❌
```cb
struct Student {
    string name;
    int grades[3];
}

struct Course {
    Student students[2];
}

int main() {
    Course math_course;
    
    // ❌ エラー: 構造体配列の構造体メンバー配列アクセス
    math_course.students[0].grades[0] = 85;
    
    return 0;
}
```

**現在の制限事項**:
- ネストした構造体メンバーアクセス (`obj.member.submember`) は未サポート
- 構造体の関数引数・戻り値は未実装
- 複雑なネストした配列アクセス (`obj.array[i].member[j]`) は未サポート
- 構造体継承は未実装

**回避策**:
- フラット構造体を使用してネストを避ける
- 構造体メンバーを個別に関数に渡す
- 1次元配列を使用して多次元アクセスを手動計算で実現

### 列挙型 🚧
C、TypeScriptライクな列挙型:
```cb  
enum Color {
    RED,
    GREEN, 
    BLUE
}

enum Status {
    SUCCESS = 0,
    ERROR = 1,
    PENDING = 2
}
```

### インターフェース定義 ❌
関数シグネチャをまとめて定義:
```cb
interface Drawable {
    void draw();
    void move(int x, int y);
    int getArea();
}

// インターフェースの継承
interface Shape extends Drawable {
    void resize(int scale);
}
```

### インターフェース実装 ❌
```cb
impl Drawable for Rectangle {
    void draw() {
        // 実装
    }
    
    void move(int x, int y) {
        // 実装  
    }
    
    int getArea() {
        // 実装
        return width * height;
    }
}
```

### アトリビュート ❌
Rustライクなコンパイル時メタデータ:
```cb
#[no_std]           // 標準ライブラリを使用しない
#[derive(Debug)]    // デバッグ出力の自動実装
#[derive(Copy)]     // コピーセマンティクスの自動実装
#[derive(Eq)]       // 等価比較の自動実装
struct Point {
    int x;
    int y;
}
```

### モジュールシステム ❌
Rustのmodule + TypeScriptのimport/export:

#### エクスポート
```cb
export TYPE;                    // 型をエクスポート
export 変数;                    // 変数をエクスポート  
export 関数宣言;                // 関数をエクスポート
export 関数 { /* 実装 */ };     // インライン関数エクスポート
export interface インターフェース名; // インターフェースをエクスポート
export { 宣言1, 宣言2, ... };   // 複数まとめてエクスポート  
export default 宣言;            // デフォルトエクスポート
```

#### インポート  
```cb
import file;                              // 全てインポート
import folder.file;                       // パス指定インポート
import folder.file.specific_item;         // 個別インポート
import folder.file.{ item1, item2, ... }; // 複数個別インポート
```

#### モジュール宣言
```cb
mod module_name {
    // モジュール内容
}
```

### エラー処理システム 🚧

#### Result型（推奨）
```cb
// Result型の定義（将来実装）
enum Result<T, E> {
    Ok(T),
    Err(E)
}

// 使用例
Result<int, string> divide(int a, int b) {
    if (b == 0) {
        return Err("Division by zero");
    }
    return Ok(a / b);
}

// パターンマッチングによるエラー処理
int main() {
    Result<int, string> result = divide(10, 2);
    match result {
        Ok(value) => print("Result: %d", value),
        Err(error) => print("Error: %s", error)
    }
    return 0;
}
```

#### エラーコード（軽量モード）
```cb
// 現在の実装（インタープリターモード）
int divide(int a, int b, int* result) {
    if (b == 0) {
        return -1; // エラーコード
    }
    *result = a / b;
    return 0; // 成功
}
```

### スマートポインタ・メモリ管理 🚧
```cb
// unique_ptr - 排他的所有権（将来実装）
unique_ptr<MyStruct> ptr = make_unique<MyStruct>();
ptr->member = 42;
// 自動的にデストラクタ呼び出し、メモリ解放

// shared_ptr - 共有所有権（将来実装）  
shared_ptr<MyStruct> ptr1 = make_shared<MyStruct>();
shared_ptr<MyStruct> ptr2 = ptr1; // 参照カウント+1
// 最後の参照が消えた時に自動メモリ解放

// weak_ptr - 弱参照（将来実装）
weak_ptr<MyStruct> weak = ptr1;
if (auto locked = weak.lock()) {
    // 有効な場合のみアクセス
}
```

## 標準機能

### 標準出力 ✅
Cのprintfライクな関数を提供:

#### print関数
```cb
// 基本的な値出力（改行なし）
print(42);              // "42"
print("Hello");         // "Hello"
print(variable);        // 変数の値

// printf風フォーマット指定子対応（推奨）
print("%d", 42);        // "42"
print("%s", "Hello");   // "Hello"  
print("%c", 'A');       // "A"
print("%d + %d = %d", 10, 20, 30);  // "10 + 20 = 30"
```

#### サポートするフォーマット指定子
- `%d`: 整数（tiny, short, int）
- `%lld`: 長整数（long）
- `%s`: 文字列（string）
- `%c`: 文字（char）
- `%%`: パーセント記号のエスケープ

#### 注意事項
- フォーマット指定子を使用する場合は必ず引数を指定
- 引数なしの場合はフォーマット指定子は無視される

### 制御構造 ✅

#### ループ
```cb
// for文
for (int i = 0; i < 10; i++) {
    print(i);
}

// while文
while (condition) {
    if (break_condition) break;
}
```

#### 条件分岐
```cb
// if-else文
if (condition) {
    // then節
} else if (other_condition) {
    // else if節
} else {
    // else節
}
```

## 高度な機能（長期計画）

### 並行プログラミング ❌
Goライクなgoroutine + channel:
```cb
// goroutine（軽量スレッド）
go my_function(args);

// channel（通信チャネル）
chan<int> ch = make_chan<int>();
```

### ジェネリクス ❌
```cb
// ジェネリック関数
T max<T>(T a, T b) where T: Comparable {
    return (a > b) ? a : b;
}
```

## コンパイル・実行オプション

### エラー処理モード選択
```bash
# Result型モード（デフォルト）
cb-compile --error-handling=result program.cb

# エラーコードモード（軽量）  
cb-compile --error-handling=codes program.cb
```

### メモリ管理設定
```bash
# RAII + スマートポインタ（デフォルト）
cb-compile --memory=smart program.cb

# 手動管理（組み込み向け）
cb-compile --memory=manual program.cb
```

## 実装例・チュートリアル

### Hello World
```cb
int main() {
    print("Hello, World!");
    return 0;
}
```

### 複合代入演算子とインクリメント/デクリメント ✅
```cb
int main() {
    // 算術複合代入演算子
    int a = 10;
    a += 5;     // a = a + 5 → 15
    a -= 3;     // a = a - 3 → 12  
    a *= 2;     // a = a * 2 → 24
    a /= 4;     // a = a / 4 → 6
    a %= 5;     // a = a % 5 → 1
    
    // ビット演算複合代入演算子
    int b = 12; // 1100 (binary)
    b &= 10;    // b = b & 10 → 8 (1000)
    b |= 3;     // b = b | 3 → 11 (1011)
    b ^= 5;     // b = b ^ 5 → 14 (1110)
    
    // シフト演算複合代入演算子
    int c = 4;
    c <<= 2;    // c = c << 2 → 16
    c >>= 3;    // c = c >> 3 → 2
    
    // 配列要素への複合代入（高度な使用法）
    int[5] arr = [1, 2, 3, 4, 5];
    arr[0] += 10;           // 基本的な配列要素複合代入
    arr[1] *= arr[2];       // 配列要素同士の計算
    arr[i*2+1] += (x+y);    // 複雑なインデックス式と初期化式
    
    // 前置インクリメント/デクリメント
    int x = 5;
    ++x;        // x = 6 (値を変更してから使用)
    --x;        // x = 5 (値を変更してから使用)
    
    // 後置インクリメント/デクリメント（文として）
    x++;        // x = 6 (使用してから値を変更)
    x--;        // x = 5 (使用してから値を変更)
    
    // 自己代入パターン
    x = x;         // 完全な自己代入
    x = x + 10;    // 自己参照による計算
    x = x << 1;    // ビット演算による自己代入
    // x = x & 0xFF;  // TODO: ビットマスクによる自己代入
    
    // すべての複合代入演算子は右から左への結合（右結合）
    // a += b += c; は a += (b += c); と解釈される
    
    return 0;
}
```

### 自己代入機能の包括実装 ✅
```cb
int main() {
    // 基本的な自己代入
    int value = 5;
    value = value;              // 完全な自己代入
    value = value + 5;          // 自己参照加算 → 10
    value = value * 3;          // 自己参照乗算 → 30
    
    println("Basic self-assignment: %d", value);
    
    // 配列要素の自己代入
    int[3] numbers;
    numbers[0] = 2;
    numbers[1] = 4; 
    numbers[2] = 6;
    
    // 配列要素同士の自己代入
    numbers[0] = numbers[0] * 2;        // 2 * 2 = 4
    numbers[1] = numbers[1] + numbers[0]; // 4 + 4 = 8
    numbers[2] = numbers[2] - numbers[1]; // 6 - 8 = -2
    
    for (int i = 0; i < 3; i++) {
        println("numbers[%d] = %d", i, numbers[i]);
    }
    
    // ビット演算による自己代入
    int bits = 12;  // 1100 in binary
    bits = bits & 5;    // 12 & 5 = 1100 & 0101 = 0100 = 4
    bits = bits | 8;    // 4 | 8 = 0100 | 1000 = 1100 = 12
    bits = bits ^ 3;    // 12 ^ 3 = 1100 ^ 0011 = 1111 = 15
    bits = bits << 1;   // 15 << 1 = 1111 << 1 = 11110 = 30
    bits = bits >> 2;   // 30 >> 2 = 11110 >> 2 = 0111 = 7
    
    println("Bitwise self-assignment result: %d", bits);
    
    // 複合代入演算子（自己代入の省略形）
    int compound = 10;
    compound += compound;    // compound = compound + compound = 20
    compound *= 2;          // compound = compound * 2 = 40  
    compound >>= 1;         // compound = compound >> 1 = 20
    compound &= 15;         // compound = compound & 15 = 4
    
    println("Compound self-assignment result: %d", compound);
    
    return 0;
}
```

**サポートする複合代入演算子（10種類）**:
1. `+=` - 加算代入
2. `-=` - 減算代入  
3. `*=` - 乗算代入
4. `/=` - 除算代入
5. `%=` - 剰余代入
6. `&=` - ビット論理積代入
7. `|=` - ビット論理和代入
8. `^=` - ビット排他的論理和代入
9. `<<=` - 左シフト代入
10. `>>=` - 右シフト代入

**特徴**:
- すべてのプリミティブ型（tiny, short, int, long）に対応
- 配列要素への複合代入サポート
- 複雑な式での複合代入サポート
- 型安全性を保った自動型変換

### 型システムの基本
```cb
int main() {
    // 整数型
    tiny small_num = 42;        // -128〜127
    short medium_num = 1000;    // -32768〜32767
    int large_num = 100000;     // -2^31〜2^31-1
    long huge_num = 1000000000; // -2^63〜2^63-1
    
    // 文字型
    char letter = 'A';
    char newline = '\n';
    char tab = '\t';
    
    // 文字列型
    string message = "Cb言語";
    
    // 論理型
    bool flag = true;
    
    // 型別の出力
    print("tiny: %d", small_num);
    print("short: %d", medium_num);
    print("int: %d", large_num);
    print("long: %lld", huge_num);
    print("char: %c", letter);
    print("string: %s", message);
    
    return 0;
}
```

### 配列の基本操作
```cb
int main() {
    // 配列宣言と初期化
    int[5] numbers = [10, 20, 30, 40, 50];
    string[3] names = ["Alice", "Bob", "Charlie"];
    char[4] chars = ['A', 'B', 'C', 'D'];
    
    // 配列要素へのアクセス
    print("numbers[0] = %d", numbers[0]);
    print("names[1] = %s", names[1]);
    print("chars[2] = %c", chars[2]);
    
    // 配列要素の変更（複合代入演算子使用）
    numbers[0] += 90;        // numbers[0] = 100
    names[1] = "Bobby";      // 文字列代入
    chars[2] = 'Z';          // 文字代入
    
    // 配列の複合代入演算子の活用
    for (int i = 0; i < 5; i++) {
        numbers[i] *= 2;     // 各要素を2倍
        print("numbers[%d] = %d", i, numbers[i]);
    }
    
    return 0;
}
```

### 関数の定義と呼び出し
```cb
// 値を返さない関数
void print_separator() {
    print("===================");
}

// 値を返す関数
int add(int a, int b) {
    return a + b;
}

// 配列を引数に取る関数（将来実装）
int sum_array(int[] arr, int size) {
    int total = 0;
    for (int i = 0; i < size; i++) {
        total += arr[i];  // 複合代入演算子を使用
    }
    return total;
}

// 文字列を扱う関数
string greet(string name) {
    return "Hello, " + name + "!"; // 文字列連結は将来実装
}

int main() {
    print_separator();
    
    int result = add(10, 20);
    print("10 + 20 = %d", result);
    
    print_separator();
    
    return 0;
}
```

### 制御構造の活用
```cb
int main() {
    // if-else文
    int score = 85;
    char grade;
    
    if (score >= 90) {
        grade = 'A';
    } else if (score >= 80) {
        grade = 'B';
    } else if (score >= 70) {
        grade = 'C';
    } else {
        grade = 'F';
    }
    
    print("スコア: %d, グレード: %c", score, grade);
    
    // while文（後置インクリメント使用）
    int count = 0;
    while (count < 5) {
        print("カウント: %d", count);
        count++;  // 後置インクリメント
    }
    
    // for文（複合代入演算子使用）
    for (int i = 1; i <= 10; i++) {
        if (i % 2 == 0) {
            print("%d は偶数", i);
        } else {
            print("%d は奇数", i);
        }
    }
    
    // 複合代入演算子を使った計算
    int sum = 0;
    for (int j = 1; j <= 100; j++) {
        sum += j;  // sum = sum + j と同等
    }
    print("1から100までの合計: %d", sum);
    
    return 0;
}
```

### const修飾子の使用
```cb
int main() {
    // 定数の宣言
    const int MAX_USERS = 100;
    const string SYSTEM_NAME = "Cb System";
    const char SEPARATOR = '-';
    
    // 配列サイズにconst値を使用
    int[MAX_USERS] user_ids;
    
    // const配列
    const int[3] PRIME_NUMBERS = [2, 3, 5];
    
    print("システム名: %s", SYSTEM_NAME);
    print("最大ユーザー数: %d", MAX_USERS);
    print("区切り文字: %c", SEPARATOR);
    
    for (int i = 0; i < 3; i++) {
        print("素数[%d] = %d", i, PRIME_NUMBERS[i]);
    }
    
    return 0;
}
```

### 構造体の実践的な使用例 ✅
```cb
// 学生管理システムの例
struct Student {
    string name;
    int student_id;
    int grades[5];  // 5科目の成績
    bool is_enrolled;
    char grade_letter;
}

struct Course {
    string course_name;
    string instructor;
    int max_students;
    int enrolled_count;
}

int main() {
    // 個別学生の作成
    Student alice;
    alice.name = "Alice Johnson";
    alice.student_id = 1001;
    alice.is_enrolled = true;
    alice.grade_letter = 'A';
    
    // 配列リテラル代入
    alice.grades = [95, 87, 92, 89, 94];
    
    // 構造体配列による複数学生管理
    Student[3] class_roster = [
        {name: "Bob Smith", student_id: 1002, is_enrolled: true, 
         grade_letter: 'B', grades: [82, 78, 85, 80, 79]},
        {name: "Charlie Brown", student_id: 1003, is_enrolled: true,
         grade_letter: 'A', grades: [91, 93, 89, 95, 92]},
        {name: "Diana Wilson", student_id: 1004, is_enrolled: false,
         grade_letter: 'C', grades: [73, 75, 72, 78, 76]}
    ];
    
    // コース情報
    Course math_course = {
        course_name: "Advanced Mathematics",
        instructor: "Dr. Einstein",
        max_students: 30,
        enrolled_count: 3
    };
    
    // 学生情報の出力
    print("=== %s ===", math_course.course_name);
    print("担当: %s", math_course.instructor);
    print("登録学生数: %d/%d", math_course.enrolled_count, math_course.max_students);
    print("");
    
    // 個別学生情報出力
    print("学生: %s (ID: %d)", alice.name, alice.student_id);
    print("在籍状況: %s", alice.is_enrolled ? "在籍中" : "退学");
    print("総合評価: %c", alice.grade_letter);
    print("成績: [%d, %d, %d, %d, %d]", 
          alice.grades[0], alice.grades[1], alice.grades[2], 
          alice.grades[3], alice.grades[4]);
    
    // クラス全体の成績処理
    print("\n=== クラス名簿 ===");
    for (int i = 0; i < 3; i++) {
        if (class_roster[i].is_enrolled) {
            int total = 0;
            for (int j = 0; j < 5; j++) {
                total += class_roster[i].grades[j];
            }
            int average = total / 5;
            
            print("%d. %s (ID: %d) - 平均: %d点 (評価: %c)", 
                  i + 1, class_roster[i].name, class_roster[i].student_id,
                  average, class_roster[i].grade_letter);
        }
    }
    
    return 0;
}
```

### ダイクストラ法アルゴリズム実装例 ✅
```cb
// エッジ（辺）を表す構造体
struct Edge {
    int to;     // 接続先のノード
    int weight; // エッジの重み（距離・コスト）
};

// 無限大を表す定数
const int INF = 999999;
const int MAX_NODES = 6;
const int MAX_EDGES = 20;

// グローバル変数でグラフデータを管理
int node_count = 6;
int[6] distances;         // 各ノードへの最短距離
bool[6] visited;          // 訪問済みフラグ
Edge[20] edges;           // エッジ配列
int edge_count = 0;
int[36] adjacency_matrix; // 6x6の隣接行列

// グラフの初期化
void init_graph() {
    for (int i = 0; i < MAX_NODES; i++) {
        distances[i] = INF;
        visited[i] = false;
    }
    
    // 隣接行列を初期化（INFで埋める）
    for (int i = 0; i < 36; i++) {
        adjacency_matrix[i] = INF;
    }
    
    // 対角線要素は0（自分自身への距離）
    for (int i = 0; i < MAX_NODES; i++) {
        adjacency_matrix[i * MAX_NODES + i] = 0;
    }
    
    edge_count = 0;
}

// エッジの追加
void add_edge(int from, int to, int weight) {
    // 隣接行列に重みを設定
    adjacency_matrix[from * MAX_NODES + to] = weight;
    
    // エッジ配列にも記録
    edges[edge_count].to = to;
    edges[edge_count].weight = weight;
    edge_count++;
}

// 最小距離のノードを見つける
int find_min_distance_node() {
    int min_distance = INF;
    int min_node = -1;
    
    for (int i = 0; i < node_count; i++) {
        if (!visited[i] && distances[i] < min_distance) {
            min_distance = distances[i];
            min_node = i;
        }
    }
    
    return min_node;
}

// ダイクストラ法の実行
void dijkstra(int start) {
    // 開始ノードの距離を0に設定
    distances[start] = 0;
    
    println("Starting Dijkstra from node %d", start);
    
    // すべてのノードを処理するまで繰り返し
    for (int count = 0; count < node_count; count++) {
        // 最小距離の未訪問ノードを選択
        int current = find_min_distance_node();
        
        if (current == -1) break; // すべてのノードを処理完了
        
        visited[current] = true;
        
        println("Processing node %d (distance: %d)", current, distances[current]);
        
        // 隣接ノードの距離を更新
        for (int neighbor = 0; neighbor < node_count; neighbor++) {
            int weight = adjacency_matrix[current * MAX_NODES + neighbor];
            
            if (!visited[neighbor] && weight != INF) {
                int new_distance = distances[current] + weight;
                if (new_distance < distances[neighbor]) {
                    distances[neighbor] = new_distance;
                    println("  Updated node %d: distance = %d", neighbor, new_distance);
                }
            }
        }
    }
}

int main() {
    println("=== Dijkstra's Shortest Path Algorithm ===");
    println("Using struct and array-based graph representation\n");
    
    // グラフの初期化
    init_graph();
    
    // エッジの追加（サンプルグラフ）
    add_edge(0, 1, 2);   // 0 -> 1: cost 2
    add_edge(0, 3, 1);   // 0 -> 3: cost 1
    add_edge(0, 4, 5);   // 0 -> 4: cost 5
    add_edge(1, 2, 3);   // 1 -> 2: cost 3
    add_edge(2, 5, 1);   // 2 -> 5: cost 1
    add_edge(3, 4, 2);   // 3 -> 4: cost 2
    add_edge(4, 5, 1);   // 4 -> 5: cost 1
    
    println("Graph created with %d nodes and %d edges", node_count, edge_count);
    
    // ダイクストラ法の実行
    int start_node = 0;
    dijkstra(start_node);
    
    // 結果の表示
    println("\n=== Results ===");
    println("Shortest distances from node %d:", start_node);
    for (int i = 0; i < node_count; i++) {
        if (distances[i] == INF) {
            println("Node %d: UNREACHABLE", i);
        } else {
            println("Node %d: %d", i, distances[i]);
        }
    }
    
    return 0;
}
```

**実行結果**:
```
=== Dijkstra's Shortest Path Algorithm ===
Using struct and array-based graph representation

Graph created with 6 nodes and 7 edges
Starting Dijkstra from node 0
Processing node 0 (distance: 0)
  Updated node 1: distance = 2
  Updated node 3: distance = 1
  Updated node 4: distance = 5
Processing node 3 (distance: 1)
  Updated node 4: distance = 3
Processing node 1 (distance: 2)
  Updated node 2: distance = 5
Processing node 4 (distance: 3)
  Updated node 5: distance = 4
Processing node 5 (distance: 4)
Processing node 2 (distance: 5)

=== Results ===
Shortest distances from node 0:
Node 0: 0
Node 1: 2
Node 2: 5
Node 3: 1
Node 4: 3
Node 5: 4
```

### 構造体と配列の高度な組み合わせ ✅
```cb
// 行列演算システムの例
struct Matrix {
    string name;
    int rows;
    int cols;
    int data[9];  // 3x3行列として使用
}

// ベクトル構造体
struct Vector3D {
    int x;
    int y; 
    int z;
    string label;
}

int main() {
    // 3x3単位行列の作成
    Matrix identity = {
        name: "Identity Matrix",
        rows: 3,
        cols: 3,
        data: [1, 0, 0,   // 第1行
               0, 1, 0,   // 第2行  
               0, 0, 1]   // 第3行
    };
    
    // 変換行列
    Matrix transform;
    transform.name = "Transform Matrix";
    transform.rows = 3;
    transform.cols = 3;
    
    // 配列リテラル代入
    transform.data = [2, 0, 0,
                      0, 2, 0,
                      0, 0, 1];
    
    // ベクトル配列
    Vector3D[4] vertices = [
        {x: 1, y: 1, z: 0, label: "Top-Right"},
        {x: -1, y: 1, z: 0, label: "Top-Left"},
        {x: -1, y: -1, z: 0, label: "Bottom-Left"}, 
        {x: 1, y: -1, z: 0, label: "Bottom-Right"}
    ];
    
    // 行列情報出力
    print("=== %s ===", identity.name);
    print("サイズ: %dx%d", identity.rows, identity.cols);
    for (int i = 0; i < 3; i++) {
        print("[%d %d %d]", 
              identity.data[i*3], identity.data[i*3+1], identity.data[i*3+2]);
    }
    
    print("\n=== %s ===", transform.name);
    for (int i = 0; i < 3; i++) {
        print("[%d %d %d]", 
              transform.data[i*3], transform.data[i*3+1], transform.data[i*3+2]);
    }
    
    // ベクトル情報出力
    print("\n=== 頂点座標 ===");
    for (int i = 0; i < 4; i++) {
        print("%s: (%d, %d, %d)", 
              vertices[i].label, vertices[i].x, vertices[i].y, vertices[i].z);
    }
    
    // 簡単な行列-ベクトル積演算（最初の頂点のみ）
    Vector3D result;
    result.label = "Transformed";
    result.x = transform.data[0] * vertices[0].x + 
               transform.data[1] * vertices[0].y + 
               transform.data[2] * vertices[0].z;
    result.y = transform.data[3] * vertices[0].x + 
               transform.data[4] * vertices[0].y + 
               transform.data[5] * vertices[0].z;
    result.z = transform.data[6] * vertices[0].x + 
               transform.data[7] * vertices[0].y + 
               transform.data[8] * vertices[0].z;
    
    print("\n=== 変換結果 ===");
    print("元座標: (%d, %d, %d)", vertices[0].x, vertices[0].y, vertices[0].z);
    print("変換後: (%d, %d, %d)", result.x, result.y, result.z);
    
    return 0;
}
```

### エラーハンドリング（現在の実装）
```cb
int divide_safe(int a, int b) {
    // 現在は型範囲チェックが自動的に行われる
    // ゼロ除算は実行時エラーとなる
    return a / b;
}

int main() {
    int result;
    
    // 正常な計算（複合代入演算子使用）
    result = divide_safe(10, 2);
    result *= 3;  // result = result * 3
    print("(10 / 2) * 3 = %d", result);
    
    // 型範囲外の値は自動的に検出される
    tiny small = 200; // エラー: tiny型は127まで
    
    return 0;
}
```

### 必要な環境
- **C++17対応コンパイラ**: g++, clang++
- **flex**: 字句解析器生成ツール
- **bison**: 構文解析器生成ツール
- **make**: ビルドシステム

### インストール手順

#### Ubuntu/Debian
```bash
sudo apt-get update
sudo apt-get install build-essential flex bison
```

#### macOS
```bash
brew install flex bison
```

#### CentOS/RHEL
```bash
sudo yum install gcc-c++ flex bison make
```

### プロジェクトのビルド
```bash
# プロジェクトをクローン
git clone https://github.com/username/Cb.git
cd Cb

# ビルド
make clean
make all

# テスト実行
make test
```

### デバッグオプション
```bash
# 英語でのデバッグ情報
./main --debug sample/test.cb

# 日本語でのデバッグ情報
./main --debug-ja sample/test.cb

# 環境変数でのデバッグ有効化
export CB_DEBUG_MODE=1
./main sample/test.cb
```

## トラブルシューティング

### よくあるエラーと対処法

#### 型範囲外エラー
```
Error: Type range exceeded for tiny: 200 (valid range: -128 to 127)
```
**対処法**: より大きな型（short, int, long）を使用する

#### 配列境界外アクセス
```
Error: Array index out of bounds: index 5 for array of size 3
```
**対処法**: 配列のインデックスが0からsize-1の範囲内にあることを確認

#### 文字リテラルエラー
```
Error: Invalid character literal: 'あ'
```
**対処法**: char型はASCII文字（0-255）のみサポート。Unicode文字にはstring型を使用

#### コンパイルエラー
```
flex: command not found
```
```
**対処法**: flexとbisonをインストールする

## パフォーマンス・ベンチマーク

### 目標性能
- **Ruby/Python比**: 10-50倍高速
- **JavaScript比**: 2-5倍高速  
- **C++比**: 80-95%の性能（RAII等のオーバーヘッド込み）

### メモリ使用量
- **ガベージコレクション不使用**: 予測可能なメモリ使用量
- **RAII**: 自動リソース管理によるメモリリーク防止
- **ゼロコスト抽象化**: 実行時オーバーヘッドの最小化

## ロードマップ・将来計画

### Phase 2: 中級機能（実装予定）
- **struct定義**: カスタムデータ型
- **enum定義**: 列挙型のサポート
- **typedef**: 型エイリアス
- **標準ライブラリ拡充**: math, string, io モジュール
- **浮動小数点数**: float, double 型
- **動的配列**: 可変長配列のサポート

### Phase 3: 上級機能（長期計画）  
- **interface/trait**: Rustライクな抽象化
- **ジェネリクス**: 型パラメータ化
- **モジュールシステム**: import/export
- **Result型**: 安全なエラーハンドリング
- **スマートポインタ**: unique_ptr, shared_ptr
- **並行処理**: goroutine/channelライクな機能

### Phase 4: エコシステム（将来構想）
- **パッケージマネージャ**: cb-pkg
- **LSP対応**: エディタサポート  
- **WebAssembly**: ブラウザ実行
- **クロスコンパイル**: マルチプラットフォーム対応
- **FFI**: C/C++ライブラリ連携

## コミュニティ・コントリビューション

### 貢献方法
1. **Issue報告**: バグ報告・機能提案
2. **Pull Request**: コード改善・新機能実装  
3. **ドキュメント**: 仕様書・チュートリアルの改善
4. **テストケース**: テストカバレッジの向上

### 開発者向けリソース
- **アーキテクチャ**: src/内のモジュール構成
- **テストフレームワーク**: tests/内の統合テスト・単体テスト
- **コーディング規約**: C++17 modern style
- **デバッグ手法**: --debug, --debug-jaオプション

### ライセンス
このプロジェクトはMITライセンスの下で公開されています。
詳細は[LICENSE](../LICENSE)ファイルを参照してください。

---

*Cb言語は実用的で高性能なプログラミング言語を目指して開発を続けています。*
*コミュニティの皆様からのフィードバックをお待ちしております。*
