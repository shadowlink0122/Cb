# Cb (C-flat) プログラミング言語

## 概要

C++で作成した静的型付きプログラミング言語です。
読み方は「シーフラット」（C++, C#があるので敢えて逆を行ってみました）

再帰下降パーサーを使用してAST（抽象構文木）を構築し、C++でASTを逐次実行するインタープリターとして動作します。

**最新バージョン**: v0.7.1 - 多次元配列戻り値処理の完全対応

## 特徴

### 型システム ✅
- **静的型付き言語** - 型は宣言時に決定、型チェックはパース時・実行時
- **整数型**: `tiny` (8bit), `short` (16bit), `int` (32bit), `long` (64bit)
- **文字列型**: UTF-8対応の文字列処理 (`string`)
- **文字型**: ASCII文字型 (`char`) - 0-255の範囲をサポート
- **論理型**: 真偽値型 (`bool`)
- **配列型**: 各型の静的配列をサポート（例: `int[10]`, `string[5]`）
- **配列リテラル**: `[1, 2, 3]` 形式での初期化と包括的な型チェック
- **Union型システム**: TypeScript風のUnion型完全実装
  - リテラル値Union: `typedef Status = 200 | 404 | 500;`
  - 基本型Union: `typedef NumericValue = int | lon### テストカバレッジ
- **1116個の統合テストケース**: 全機能の動作検証（100%成功率）
- **26個の単体テスト**: モジュール別詳細テスト
- **型安全性テスト**: 境界値・型不整合の検出確認
- **構造体機能テスト**: リテラル初期化、配列メンバー、構造体配列の完全検証
  - **定数サイズ配列メンバー初期化**: 個別代入でのprintf出力修正完了
  - **sync_struct_members_from_direct_access**: 構造体メンバー同期機能の完全修正
- **Union型システムテスト**: TypeScript風Union型の包括的検証（179テストケース）
  - リテラル値Union、基本型Union、カスタム型Union
  - 構造体Union、配列Union、混合Union
  - Union型文字列処理: 再帰的文字列実装との完全統合
  - Union型複合代入演算子: 構造体メンバーでの完全対応
  - エラーケース検証（15種類の異常系テスト）
- **多次元配列戻り値処理**: typedef配列関数の完全対応
  - **関数戻り値の多次元配列展開**: `Matrix2D create_matrix()`等の2次元配列戻り値を正確に処理
  - **Variable Manager改善**: 配列戻り値処理で多次元配列判定・全要素展開を実装
  - **Statement Executor強化**: ReturnException処理での多次元配列対応
  - **Array Manager境界チェック**: multidim_array_values配列の安全なアクセス保証
- **関数実行回数検証**: 複合代入での重複実行問題修正確認
- **自己代入テスト**: 基本、配列、ビット演算による自己代入パターン
- **国際化テスト**: 多言語エラーメッセージの検証
- **自動テストフレームワーク**: `make test`で完全自動化
- **パフォーマンス計測**: カテゴリ別実行時間測定機能
- **100%テスト成功率**: 完全な品質保証体制g;`
  - カスタム型Union: `typedef CustomUnion = UserID | ProductID;`
  - 構造体Union: `typedef EntityUnion = User | Product;`
  - 配列Union: `typedef ArrayUnion = int[5] | string[3];`
  - 混合Union: `typedef MixedUnion = 42 | int | string;`
  - 再帰的typedef対応と継承型チェック
- **const修飾子**: 定数宣言のサポート

### 制御構造 ✅
- **Cライクな制御構造**: `if`/`else`, `else if`, `for`, `while`, `break`, `continue`, `return`
- **ブロック文とブロックなし単文**の両方をサポート
- **関数定義と呼び出し**
- **ループ制御**: `break`と`continue`でのループフロー制御

### 変数宣言 ✅
- **複数変数同時宣言**: `int a, b, c;` 形式での一度に複数変数の宣言
- **初期化付き宣言**: `int x = 5, y = 10;` 形式での初期化
- **配列宣言**: `int[5] arr1, arr2;` 形式での複数配列宣言

### 演算子 ✅
- **Cライクな演算子優先順位**（`&&`, `||`, `==`, `!=`, `<`, `>`, `+`, `-`, `*`, `/`, `%` など）
- **複合代入演算子**（10種類完全サポート）
  - 算術複合代入: `+=`, `-=`, `*=`, `/=`, `%=`
  - ビット演算複合代入: `&=`, `|=`, `^=`
  - シフト演算複合代入: `<<=`, `>>=`
  - 配列要素への複合代入: `arr[i] += 5`, `arr[i*2+1] *= (x+y)`
- **インクリメント・デクリメント**（前置・後置両方完全対応）
  - 前置: `++variable`, `--variable`
  - 後置（文として）: `variable++;`, `variable--;`

### 入出力機能 ✅
- **print関数**: printf風フォーマット指定子対応
  - `%d`: 整数 (tiny, short, int)
  - `%lld`: 長整数 (long)  
  - `%s`: 文字列 (string)
  - `%c`: 文字 (char)
  - `%%`: パーセント記号のエスケープ

### エラーハンドリング・デバッグ ✅
- **多言語対応エラーメッセージ**: 英語・日本語でのエラー表示
- **包括的な型範囲チェック**: 全整数型で自動範囲チェック
- **詳細なデバッグ機能**: `--debug`（英語）、`--debug-ja`（日本語）オプション
- **UTF-8文字列処理**: 日本語を含む文字列の適切な処理

### テストフレームワーク ✅
- **統合テスト**: 1116個の包括的テストケース（全機能カバレッジ）
- **単体テスト**: 26個のモジュール別詳細テスト
- **自動テスト実行**: `make test`で全テスト実行
- **カテゴリ別テスト実行時間計測**: パフォーマンス分析機能
- **100%テスト成功率**: 完全な品質保証体制
- **多次元配列処理テスト**: typedef配列関数戻り値の完全検証

## 実装状況

### ✅ 完成機能
- プリミティブ型（tiny, short, int, long, string, char, bool）
- 変数宣言・初期化・配列（複数変数同時宣言対応）
- 関数定義・呼び出し
- 制御構造（if/else, for, while, break, continue, return）
- **演算子（算術、比較、論理、代入、複合代入、インクリメント）**
  - **10種類の複合代入演算子**: `+=`, `-=`, `*=`, `/=`, `%=`, `&=`, `|=`, `^=`, `<<=`, `>>=`
  - **前置・後置インクリメント/デクリメント**: `++var`, `--var`, `var++`, `var--`
  - **配列要素複合代入**: `arr[index] += value`
  - **自己代入機能**: 基本的な自己代入、配列自己代入、ビット演算自己代入
- **構造体システム**（完全実装）
  - 基本構造体定義・使用
  - 構造体リテラル初期化（名前付き・位置指定）
  - 構造体配列メンバー（個別代入・配列リテラル代入）
  - **定数サイズ配列メンバー**: `tiny SIZE = 3; int[SIZE] arr;` の完全対応
  - **sync_struct_members_from_direct_access**: 構造体メンバー同期エンジンの修正完了
  - 構造体の配列（構造体配列リテラル初期化）
- **Interface/Implシステム**（完全実装） 🆕
  - プリミティブ型、構造体型、typedef型へのメソッド実装
  - インターフェース変数による型の抽象化
  - 再帰的typedefの独立した実装（`typedef int INT; typedef INT INT2;`）
  - 実装存在チェック付きの型安全な代入
  - 多重インターフェース実装サポート
  - デバッグ機能とエラーハンドリング
- **Union型システム**（完全実装）
  - TypeScript風のUnion型構文とセマンティクス
  - リテラル値、基本型、カスタム型、構造体、配列Union対応
  - 混合Union型と厳密な型検証
  - **Union型文字列処理**: 再帰的既存文字列実装との完全統合
  - **Union型複合代入演算子**: 構造体メンバーでの完全対応（`obj.union_member += value`）
  - 再帰的typedef互換性チェック
  - 包括的エラーハンドリング（15種類のテストケース）
- **関数実行最適化**: 複合代入での重複実行問題修正（`p(test) += 1`が1回のみ実行）
- ストレージ修飾子（const, static）
- 標準出力（print, printf風フォーマット）
- 包括的テストフレームワーク（統合テスト1116個、単体テスト26個）
- 再帰下降パーサーによる構文解析
- **多次元配列関数戻り値処理**: typedef配列の完全対応

### 🚧 将来の拡張予定
- **浮動小数点数型**: `float`, `double`のサポート
- **enum**: 列挙型（部分実装済み）
- **interface/trait**: 抽象化機能
- **ジェネリクス・テンプレート**: 型パラメータ化機能
- **標準ライブラリ**: 文字列操作、数学関数、ファイルIO等の拡充

## ディレクトリ構成

```
src/
├── frontend/          # フロントエンド（字句・構文解析）
│   ├── recursive_parser/   # 再帰下降パーサー
│   │   ├── recursive_lexer.cpp/h    # 字句解析
│   │   └── recursive_parser.cpp/h   # 構文解析・AST構築
│   ├── help_messages.cpp/h  # ヘルプメッセージ
│   └── main.cpp      # メインプログラム
├── backend/          # バックエンド（実行エンジン）
│   ├── interpreter.cpp/h     # ASTインタープリター
│   ├── error_handler.cpp/h   # エラー処理
│   ├── evaluator/           # 式評価エンジン
│   │   └── expression_evaluator.cpp/h
│   ├── executor/            # 文実行エンジン
│   │   └── statement_executor.cpp/h
│   └── output/              # 出力処理
│       └── output_manager.cpp/h
├── common/           # 共通モジュール
│   ├── ast.h         # ASTノード定義
│   ├── type_utils.cpp/h     # 型関連ユーティリティ
│   ├── debug.h              # デバッグ機能定義
│   ├── debug_impl.cpp       # デバッグ実装
│   ├── debug_messages.cpp/h # デバッグメッセージ
│   ├── io_interface.cpp/h   # I/Oインターフェース
│   ├── type_alias.cpp/h     # 型エイリアス
│   ├── utf8_utils.cpp/h     # UTF-8処理
│   └── cb_config.cpp/h      # 設定管理
└── platform/         # プラットフォーム固有
    ├── native/       # ネイティブ実行環境
    │   └── native_stdio_output.cpp
    └── baremetal/    # ベアメタル環境
        └── baremetal_uart_output.cpp

cgen/                 # Cコード生成器（将来拡張）
└── cgen_main.cpp     # トランスパイラー本体

tests/
├── unit/             # 単体テスト（26テスト）
│   ├── framework/    # テストフレームワーク
│   ├── backend/      # バックエンドテスト
│   └── common/       # 共通モジュールテスト
├── integration/      # 統合テスト（.hppファイル）
└── cases/            # テストケース（.cbファイル）
    ├── array_literal/  # 配列リテラルテスト
    ├── arithmetic/     # 算術演算テスト
    ├── loop/           # ループ制御テスト（break/continue）
    ├── multiple_var_decl/ # 複数変数宣言テスト
    ├── string/         # 文字列処理テスト
    ├── printf/         # printf機能テスト
    └── ...            # その他機能別テスト

docs/                 # ドキュメント
└── spec.md           # 言語仕様書

sample/               # サンプルコード
stdlib/               # 標準ライブラリ
lib/                  # ライブラリモジュール
modules/              # モジュールファイル
Makefile             # ビルド設定
```

## ビルド方法

### インタープリター版（メイン）
```sh
make
```

### デバッグ版
```sh
make debug-build
```

### 全体ビルド（インタープリター + テスト）
```sh
make all
```

### クリーンビルド
```sh
make clean
make
```

## テスト方法

### 全テスト実行（推奨）
統合テスト（1116テスト）と単体テスト（26テスト）を全て実行：
```sh
make test
```

### 統合テスト（Integration Test）
実際の.cbファイルをインタープリターで実行し、出力やエラーを検証：
```sh
make integration-test
```

### 単体テスト（Unit Test）
型ごと・機能ごとにASTノード生成と評価ロジックのテスト：
```sh
make unit-test
```

### デバッグ付きテスト
```sh
make debug-build-test
```

統合テストでは`tests/integration/`以下の.hppファイルが`tests/cases/`以下の.cbファイルを使用して包括的なテストを実行します。

## 実行方法

### インタープリター
```sh
# 基本実行
./main sample/fibonacci.cb

# デバッグモード（英語）
./main --debug sample/fibonacci.cb

# デバッグモード（日本語）
./main --debug-ja sample/fibonacci.cb
```

## サンプルコード例

### 複数変数宣言
```cb
int main() {
    // 複数変数の同時宣言
    int a, b, c;
    int x = 10, y = 20, z = 30;
    
    // 配列の複数宣言
    int[5] arr1, arr2;
    string[3] names = ["Alice", "Bob", "Charlie"];
    
    print("x: %d, y: %d, z: %d", x, y, z);
    return 0;
}
```

### ループ制御（break/continue）
```cb
int main() {
    // continue文でスキップ
    for (int i = 1; i <= 10; i++) {
        if (i % 2 == 0) {
            continue;  // 偶数をスキップ
        }
        print("奇数: %d", i);
    }
    
    // break文でループ脱出
    for (int j = 1; j <= 100; j++) {
        if (j > 5) {
            break;  // 5を超えたら終了
        }
        print("値: %d", j);
    }
    
    return 0;
}
```

### 複合代入演算子とインクリメント
```cb
int main() {
    // 複合代入演算子
    int a = 10;
    a += 5;     // a = a + 5 → 15
    a -= 3;     // a = a - 3 → 12
    a *= 2;     // a = a * 2 → 24
    a /= 4;     // a = a / 4 → 6
    a %= 5;     // a = a % 5 → 1
    
    // ビット演算複合代入
    int b = 12; // 1100
    b &= 10;    // b = b & 10 → 8 (1000)
    b |= 3;     // b = b | 3 → 11 (1011)
    b ^= 5;     // b = b ^ 5 → 14 (1110)
    
    // シフト演算複合代入
    int c = 4;
    c <<= 2;    // c = c << 2 → 16
    c >>= 3;    // c = c >> 3 → 2
    
    // 配列要素への複合代入
    int[5] arr = [1, 2, 3, 4, 5];
    arr[0] += 10;        // arr[0] = 11
    arr[1] *= arr[2];    // arr[1] = 2 * 3 = 6
    
    // インクリメント・デクリメント
    int d = 5;
    ++d;        // 前置インクリメント → 6
    d--;        // 後置デクリメント → 5
    
    print("最終結果: a=%d, b=%d, c=%d, d=%d", a, b, c, d);
    return 0;
}
```

### char型と文字リテラル
```cb
int main() {
    char letter = 'A';
    char newline = '\n';
    char tab = '\t';
    
    print("文字: %c", letter);
    print("改行: %c", newline);
    print("タブ: %c", tab);
    
    return 0;
}
```

### printf風フォーマット出力
```cb
int main() {
    int age = 25;
    string name = "太郎";
    char grade = 'A';
    
    print("名前: %s, 年齢: %d歳, 成績: %c", name, age, grade);
    print("パーセント記号: %%");
    
    return 0;
}
```

### FizzBuzz
```cb
int main() {
    for (int i = 1; i <= 15; i++) {  // 後置インクリメント使用
        if ((i % 3 == 0) && (i % 5 == 0))
            print("FizzBuzz");
        else if (i % 3 == 0)
            print("Fizz");
        else if (i % 5 == 0)
            print("Buzz");
        else
            print("%d", i);
    }
    return 0;
}
```

### 構造体定数サイズ配列メンバー（修正済み機能）
```cb
// 定数サイズ配列メンバーの個別代入とprintf出力
tiny SIZE = 3;

struct Point {
    int[SIZE] arr;  // 定数サイズ配列メンバー
    int x, y;
};

int main() {
    Point s;
    
    // 定数サイズ配列メンバーに個別代入
    s.arr[0] = 100;
    s.arr[1] = 200;  
    s.arr[2] = 300;
    s.x = 50;
    s.y = 75;
    
    // 個別アクセス（従来から動作）
    println("Individual access: %d, %d, %d", s.arr[0], s.arr[1], s.arr[2]);
    
    // printf内での配列アクセス（修正により正常動作）
    println("Direct printf: [%d, %d, %d]", s.arr[0], s.arr[1], s.arr[2]);
    
    // 通常メンバーも正常動作
    println("Other members: x=%d, y=%d", s.x, s.y);
    
    // 出力:
    // Individual access: 100, 200, 300
    // Direct printf: [100, 200, 300]  ← 修正前は [0, 0, 0] だった
    // Other members: x=50, y=75
    
    return 0;
}
```

### Union型複合代入演算子（新機能）
```cb
typedef Status = 200 | 404 | 500;
typedef Uni = int | string;

struct TestStruct {
    Status code;
    Uni value;
    int count;
};

int main() {
    TestStruct test = {
        code: 200,
        value: 5,      // Union型に int を代入
        count: 10
    };
    
    // Union型メンバーへの複合代入演算子
    test.value *= 3;   // Union型 int メンバーの複合代入: 5 * 3 = 15
    test.count += 5;   // 通常の複合代入: 10 + 5 = 15
    
    println("After compound assignment: value=%d, count=%d", test.value, test.count);
    
    // Union型の型変更
    test.value = "Hello";  // int → string に変更
    println("After type change: value=%s", test.value);
    
    // 再び数値に変更して複合代入
    test.value = 7;
    test.value += 3;       // Union型での加算複合代入: 7 + 3 = 10
    
    println("Final: value=%d", test.value);
    
    // 出力:
    // After compound assignment: value=15, count=15
    // After type change: value=Hello
    // Final: value=10
    
    return 0;
}
```

### 関数実行回数最適化（修正済み機能）
```cb
int call_count = 0;

int p(int x) {
    call_count++;
    println("Function p called, count: %d, value: %d", call_count, x);
    return x;
}

int main() {
    call_count = 0;
    int test = 5;
    
    println("Before: test=%d, call_count=%d", test, call_count);
    
    // 修正前: p(test) が2回実行されていた
    // 修正後: p(test) が正確に1回のみ実行される
    test = p(test) + 1;
    
    println("After: test=%d, call_count=%d", test, call_count);
    
    if (call_count == 1) {
        println("✓ Test passed: Function called exactly once");
    } else {
        println("✗ Test failed: Function called %d times, expected 1", call_count);
    }
    
    // 出力:
    // Before: test=5, call_count=0
    // Function p called, count: 1, value: 5
    // After: test=6, call_count=1
    // ✓ Test passed: Function called exactly once
    
    return 0;
}
```

### 多次元配列戻り値処理（新機能）
```cb
// typedef配列の多次元配列関数戻り値（修正済み機能）
typedef Matrix2D = int[2][2];

Matrix2D create_matrix() {
    Matrix2D result;
    result[0][0] = 1;
    result[0][1] = 2;
    result[1][0] = 3; 
    result[1][1] = 4;
    return result;
}

void print_matrix(Matrix2D matrix) {
    println("Matrix (2x2):");
    for (int i = 0; i < 2; i++) {
        printf("Row %d : [ ", i);
        for (int j = 0; j < 2; j++) {
            printf("%d", matrix[i][j]);
            if (j < 1) printf(", ");
        }
        println(" ]");
    }
}

int main() {
    // 修正前: 境界エラーが発生していた
    // 修正後: 全4要素が正確に転送・処理される
    Matrix2D matrix = create_matrix();
    
    print_matrix(matrix);
    
    // 出力:
    // Matrix (2x2):
    // Row 0 : [ 1, 2 ]
    // Row 1 : [ 3, 4 ]  ← 修正前はここで境界エラー
    
    return 0;
}
```

**修正内容**:
- **Variable Manager**: `ret.int_array_3d[0][0]`のみ処理していた問題を修正し、多次元配列の全要素展開を実装
- **Statement Executor**: ReturnException処理での多次元配列判定・次元情報設定を追加
- **Array Manager**: multidim_array_values配列の境界チェック強化
- **型判定ロジック**: typedef配列名と実際の配列構造から多次元配列を正確に識別

typedef StringOrInt = string | int;
typedef Response = "success" | "error" | "pending";

int main() {
    // Union型での文字列代入と出力
    StringOrInt data = "Hello World";
    println("String value: %s", data);  // 修正により正常出力
    
    // リテラル型での文字列処理
    Response status = "success";
    println("Status: %s", status);      // 修正により正常出力
    
    // 数値から文字列への変更
    data = 42;
    println("Numeric value: %d", data);
    
    // 再び文字列に変更（再帰的文字列実装使用）
    data = "Converted back";
    println("Back to string: %s", data); // 修正により正常出力
    
    // Union型の文字列比較処理
    Response status1 = "success";
    Response status2 = "error";
    
    println("Comparison test:");
    println("status1 (%s) != status2 (%s): different values", status1, status2);
    
    status2 = "success";
    println("status1 (%s) == status2 (%s): same values", status1, status2);
    
    // 出力:
    // String value: Hello World
    // Status: success
    // Numeric value: 42
    // Back to string: Converted back
    // Comparison test:
    // status1 (success) != status2 (error): different values
    // status1 (success) == status2 (success): same values
    
    return 0;
}
```
```cb
// リテラル値Union - 特定の値のみ許可
typedef HttpStatus = 200 | 404 | 500;
typedef Direction = "up" | "down" | "left" | "right";

// 基本型Union - 複数の基本型を組み合わせ
typedef NumericValue = int | long | string;
typedef BoolOrString = bool | string;

// カスタム型Union - 定義したtypedefを組み合わせ
typedef UserID = int;
typedef ProductID = string;
typedef ID = UserID | ProductID;

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
typedef ArrayUnion = int[5] | string[3];

// 混合Union - リテラル値と型を組み合わせ
typedef MixedUnion = 42 | int | string;

int main() {
    // リテラル値Unionの使用
    HttpStatus status = 200;  // OK: 許可されたリテラル値
    // HttpStatus invalid = 301;  // エラー: 許可されていない値
    
    // 基本型Unionの使用
    NumericValue value1 = 42;      // int値
    NumericValue value2 = "test";  // string値
    // NumericValue invalid = true; // エラー: bool型は許可されていない
    
    // カスタム型Unionの使用（再帰的typedef対応）
    UserID user_id = 12345;
    ID general_id = user_id;       // OK: UserID → ID は互換性あり
    
    // 構造体Unionの使用
    User alice = {id: 1, name: "Alice"};
    Entity entity = alice;         // OK: User → Entity
    
    // 配列Unionの使用
    int[5] numbers = [1, 2, 3, 4, 5];
    ArrayUnion arr_union = numbers; // OK: int[5] → ArrayUnion
    
    print("Union型システムが正常に動作しています");
    print("status: %d, value1: %d", status, value1);
    
    return 0;
}
```

### Union型エラーハンドリング
```cb
typedef RestrictedUnion = int | string;  // boolは許可されない

int main() {
    // 正常なケース
    RestrictedUnion valid1 = 42;
    RestrictedUnion valid2 = "hello";
    
    // エラーケース（実行時エラー）
    bool flag = true;
    // RestrictedUnion invalid = flag;  // エラー: bool型は許可されていない
    
    print("Union型の型安全性が保証されています");
    return 0;
}
```

### 構造体システム（完全実装）
```cb
// 基本構造体定義
struct Point {
    int x;
    int y;
    string label;
}

struct Rectangle {
    Point top_left;
    Point bottom_right;  // ❌ ネストした構造体は未サポート
    int width;
    int height;
    string name;
}

int main() {
    // 構造体リテラル初期化
    Point p1 = {x: 10, y: 20, label: "Origin"};     // 名前付き初期化
    Point p2 = {30, 40, "Target"};                  // 位置指定初期化
    
    // 構造体配列
    Point[3] points = [
        {x: 0, y: 0, label: "Start"},
        {10, 10, "Middle"},
        {x: 20, y: 20, label: "End"}
    ];
    
    // 構造体メンバーアクセス
    print("Point %s: (%d, %d)", p1.label, p1.x, p1.y);
    
    // 構造体配列メンバー
    Rectangle rect;
    rect.name = "Sample Rectangle";
    rect.width = 100;
    rect.height = 50;
    
    // 配列要素のメンバーアクセス
    for (int i = 0; i < 3; i++) {
        print("Point %d: %s (%d, %d)", i, points[i].label, 
              points[i].x, points[i].y);
    }
    
    return 0;
}
```

### ダイクストラ法アルゴリズム（構造体活用）
```cb
// エッジ（辺）を表す構造体
struct Edge {
    int to;     // 接続先のノード
    int weight; // エッジの重み
};

const int INF = 999999;
const int MAX_NODES = 6;

int node_count = 6;
int[6] distances;         // 各ノードへの最短距離
bool[6] visited;          // 訪問済みフラグ
Edge[20] edges;           // エッジ配列
int[36] adjacency_matrix; // 隣接行列

void dijkstra(int start) {
    distances[start] = 0;
    
    for (int count = 0; count < node_count; count++) {
        int current = find_min_distance_node();
        if (current == -1) break;
        
        visited[current] = true;
        
        for (int neighbor = 0; neighbor < node_count; neighbor++) {
            int weight = adjacency_matrix[current * MAX_NODES + neighbor];
            
            if (!visited[neighbor] && weight != INF) {
                int new_distance = distances[current] + weight;
                if (new_distance < distances[neighbor]) {
                    distances[neighbor] = new_distance;
                }
            }
        }
    }
}

int main() {
    // グラフの初期化とエッジ追加
    init_graph();
    add_edge(0, 1, 2);
    add_edge(0, 3, 1);
    // ...
    
    dijkstra(0);  // ノード0からの最短距離を計算
    
    for (int i = 0; i < node_count; i++) {
        println("Node %d: distance = %d", i, distances[i]);
    }
    
    return 0;
}
```

### 配列とループ
```cb
int main() {
    int[5] numbers = [10, 20, 30, 40, 50];
    
    // 複合代入演算子を使った配列操作
    for (int i = 0; i < 5; i++) {
        numbers[i] *= 2;  // 各要素を2倍
        print("numbers[%d] = %d", i, numbers[i]);
    }
    
    return 0;
}
```

### 文字列配列
```cb
int main() {
    string[3] messages = ["Hello", "World", "Cb言語"];
    
    for (int i = 0; i < 3; i++) {
        print("メッセージ %d: %s", i, messages[i]);
    }
    
    return 0;
}
```

### 関数定義
```cb
int fibonacci(int n) {
    if (n <= 1)
        return n;
    return fibonacci(n - 1) + fibonacci(n - 2);
}

int main() {
    for (int i = 0; i < 10; i++) {
        print("fibonacci(%d) = %d", i, fibonacci(i));
    }
    return 0;
}
```

### const修飾子
```cb
int main() {
    const int MAX_SIZE = 100;
    const string MESSAGE = "Hello, Cb!";
    
    int[MAX_SIZE] buffer;  // const値を配列サイズに使用
    
    print("%s (配列サイズ: %d)", MESSAGE, MAX_SIZE);
    
    return 0;
}
```

### Interface/Implシステムの使用例 🆕

```cb
// インターフェースの定義
interface Printable {
    string toString();
    int getSize();
};

// Typedef型の定義
typedef int MyInt;
typedef int[5] IntArray;

// プリミティブ型への実装
impl Printable for MyInt {
    string toString() {
        return "MyInt value";
    }
    
    int getSize() {
        return 1;
    }
};

// 配列型への実装
impl Printable for IntArray {
    string toString() {
        return "IntArray[5]";
    }
    
    int getSize() {
        return 5;
    }
};

int main() {
    // 異なる型でも同じインターフェースを使用
    MyInt mi = 42;
    IntArray arr = [1, 2, 3, 4, 5];
    
    // インターフェース変数による抽象化
    Printable p1 = mi;
    Printable p2 = arr;
    
    // 統一的なメソッド呼び出し
    println("MyInt: %s (size: %d)", p1.toString(), p1.getSize());
    println("IntArray: %s (size: %d)", p2.toString(), p2.getSize());
    
    return 0;
}
```

### 再帰的Typedef独立性 🆕

```cb
typedef int INT;
typedef INT INT2;
typedef INT2 INT3;

// INT3にのみPrintableを実装
impl Printable for INT3 {
    string toString() {
        return "INT3 implementation";
    }
    
    int getSize() {
        return 333;
    }
};

int main() {
    int original = 100;   // Printableなし
    INT int1 = 200;       // Printableなし
    INT2 int2 = 300;      // Printableなし
    INT3 int3 = 400;      // Printableあり
    
    // これは成功する
    Printable p3 = int3;
    println("INT3: %s", p3.toString());
    
    // これらはエラーになる
    // Printable p_orig = original; // Error: No impl found for interface 'Printable' with type 'int'
    // Printable p1 = int1;        // Error: No impl found for interface 'Printable' with type 'INT'
    // Printable p2 = int2;        // Error: No impl found for interface 'Printable' with type 'INT2'
    
    return 0;
}
```

### 型不整合エラー（英語）
```
$ ./main --debug test_error.cb
Error: Array 'mixed' element 1: int type expected but string type provided
```

### 型不整合エラー（日本語）
```
$ ./main --debug-ja test_error.cb
エラー: 配列'mixed'の要素1: int型が期待されましたがstring型が渡されました
```

### char型範囲外エラー
```
$ ./main --debug test_char_error.cb
Error: char type value out of range (0-255): 300
```

## 技術仕様

### 開発言語・ツール
- **C++17**: メイン実装言語
- **再帰下降パーサー**: 字句・構文解析
- **Make**: ビルドシステム

### アーキテクチャ
- **フロントエンド**: 字句解析 → 構文解析 → AST生成（再帰下降パーサー）
- **バックエンド**: インタープリター実行エンジン
- **Union型システム**: TypeScript風の高度な型検証エンジン
- **Interface/Implシステム**: 型安全なメソッド実装システム
- **多言語サポート**: 英語・日本語でのエラーメッセージ・デバッグ情報
- **UTF-8対応**: 日本語を含む文字列の適切な処理
- **モジュール化設計**: 機能別ディレクトリ構成

### テストカバレッジ
- **963個の統合テストケース**: 全機能の動作検証
- **26個の単体テスト**: モジュール別詳細テスト
- **型安全性テスト**: 境界値・型不整合の検出確認
- **構造体機能テスト**: リテラル初期化、配列メンバー、構造体配列の完全検証
- **自己代入テスト**: 基本、配列、ビット演算による自己代入パターン
- **国際化テスト**: 多言語エラーメッセージの検証
- **自動テストフレームワーク**: `make test`で完全自動化
- **パフォーマンス計測**: カテゴリ別実行時間測定機能

## 開発・貢献

### 開発環境セットアップ
```sh
# 必要な依存関係をインストール
# Ubuntu/Debian:
sudo apt-get install build-essential

# macOS:
xcode-select --install

# プロジェクトのクローン・ビルド
git clone <repository-url>
cd Cb
make all
make test
```

### デバッグ方法
- `--debug`: 英語デバッグ情報
- `--debug-ja`: 日本語デバッグ情報  
- `CB_DEBUG_MODE=1`: 環境変数でのデバッグ有効化

## 注意事項・制限事項

- **C++17以降推奨**: modern C++の機能を利用
- **整数型範囲チェック**: 自動的に検出しエラー終了
- **文字型範囲**: char型は0-255の範囲（ASCII互換）
- **UTF-8文字列**: 内部的にUTF-8で処理（表示は環境依存）
- **メモリ管理**: スマートポインタ使用、手動メモリ管理なし
- **関数オーバーロード**: 未サポート
- **例外処理構文**: 未サポート（`try`/`catch`なし）
- **配列**: 静的サイズのみサポート（動的配列は将来実装予定）

## ドキュメント

詳細な言語仕様については以下を参照してください：
- **基本言語仕様**: [docs/spec.md](docs/spec.md)
- **Interface/Implシステム**: [docs/interface_system.md](docs/interface_system.md)

---

*このプロジェクトは継続的に開発中です。バグ報告や機能提案はIssuesまでお願いします。*
