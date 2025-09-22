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
- ✅ 変数宣言・初期化
- ✅ 配列（静的サイズ）・配列リテラル
- ✅ 関数定義・呼び出し
- ✅ 制御構造（if/else, for, while, break, return）
- ✅ 演算子（算術、比較、論理、代入、インクリメント）
- ✅ ストレージ修飾子（const, static）
- ✅ 標準出力（print, printf風フォーマット指定子）
- ✅ 包括的テストフレームワーク（統合テスト50+、単体テスト26個）

### Phase 2: 中期目標 🚧（実装中）
- 🚧 typedef システム
- 🚧 struct 定義
- 🚧 enum 定義  
- 🚧 標準ライブラリ拡充
- ❌ Result型エラー処理
- ❌ スマートポインタ（unique_ptr, shared_ptr）

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

### 関数宣言
```bnf
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
- 配列型: `TYPE[SIZE]` （静的配列）
    - 動的配列 `TYPE[]` は将来実装予定
    - 多次元配列対応（例: `int[10][20]`）

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

### 型宣言・作成 🚧
```cb
// 型エイリアス（将来実装）
typedef 既存型 新しい型名;

// 例
typedef int UserId;
typedef string[100] LargeString;
```

### 構造体定義 🚧
```cb
// 基本構造体
struct 構造体名 { 
    TYPE メンバ1; 
    TYPE メンバ2;
    // ...
}

// 継承（将来実装）
struct 派生構造体名 extends 基底構造体名 { 
    TYPE 追加メンバ;
    // ...
}
```

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
    
    // 配列要素の変更
    numbers[0] = 100;
    names[1] = "Bobby";
    chars[2] = 'Z';
    
    // ループでの配列処理
    for (int i = 0; i < 5; i++) {
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
        total += arr[i];
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
    
    // while文
    int count = 0;
    while (count < 5) {
        print("カウント: %d", count);
        count++;
    }
    
    // for文
    for (int i = 1; i <= 10; i++) {
        if (i % 2 == 0) {
            print("%d は偶数", i);
        } else {
            print("%d は奇数", i);
        }
    }
    
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

### エラーハンドリング（現在の実装）
```cb
int divide_safe(int a, int b) {
    // 現在は型範囲チェックが自動的に行われる
    // ゼロ除算は実行時エラーとなる
    return a / b;
}

int main() {
    int result;
    
    // 正常な計算
    result = divide_safe(10, 2);
    print("10 / 2 = %d", result);
    
    // 型範囲外の値は自動的に検出される
    tiny small = 200; // エラー: tiny型は127まで
    
    return 0;
}
```

## 開発環境とツール

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
