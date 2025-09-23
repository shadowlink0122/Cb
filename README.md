# Cb (C-flat) プログラミング言語

## 概要

C++で作成した静的型付きプログラミング言語です。
読み方は「シーフラット」（C++, C#があるので敢えて逆を行ってみました）

再帰下降パーサーを使用してAST（抽象構文木）を構築し、C++でASTを逐次実行するインタープリターとして動作します。

## 特徴

### 型システム ✅
- **静的型付き言語** - 型は宣言時に決定、型チェックはパース時・実行時
- **整数型**: `tiny` (8bit), `short` (16bit), `int` (32bit), `long` (64bit)
- **文字列型**: UTF-8対応の文字列処理 (`string`)
- **文字型**: ASCII文字型 (`char`) - 0-255の範囲をサポート
- **論理型**: 真偽値型 (`bool`)
- **配列型**: 各型の静的配列をサポート（例: `int[10]`, `string[5]`）
- **配列リテラル**: `[1, 2, 3]` 形式での初期化と包括的な型チェック
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
- **自己代入演算子**（`+=`, `-=`, `*=`, `/=`, `%=`）
- **インクリメント・デクリメント**（`++`, `--`、前置・後置両方対応）

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
- **統合テスト**: 130個の包括的テストケース
- **単体テスト**: 26個のモジュール別詳細テスト
- **自動テスト実行**: `make test`で全テスト実行

## 実装状況

### ✅ 完成機能
- プリミティブ型（tiny, short, int, long, string, char, bool）
- 変数宣言・初期化・配列（複数変数同時宣言対応）
- 関数定義・呼び出し
- 制御構造（if/else, for, while, break, continue, return）
- 演算子（算術、比較、論理、代入、インクリメント）
- ストレージ修飾子（const, static）
- 標準出力（print, printf風フォーマット）
- 包括的テストフレームワーク

### 🚧 将来の拡張予定
- **浮動小数点数型**: `float`, `double`のサポート
- **構造体・クラス**: カスタムデータ型の定義
- **typedef**: 型エイリアス機能
- **enum**: 列挙型
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
統合テスト（130テスト）と単体テスト（26テスト）を全て実行：
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
    for (int i = 1; i <= 15; i = i + 1) {
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

### 配列とループ
```cb
int main() {
    int[5] numbers = [10, 20, 30, 40, 50];
    
    for (int i = 0; i < 5; i++) {
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

## エラーメッセージ例

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
- **多言語サポート**: 英語・日本語でのエラーメッセージ・デバッグ情報
- **UTF-8対応**: 日本語を含む文字列の適切な処理
- **モジュール化設計**: 機能別ディレクトリ構成

### テストカバレッジ
- **130個の統合テストケース**: 全機能の動作検証
- **26個の単体テスト**: モジュール別詳細テスト
- **型安全性テスト**: 境界値・型不整合の検出確認
- **国際化テスト**: 多言語エラーメッセージの検証
- **自動テストフレームワーク**: `make test`で完全自動化

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

詳細な言語仕様については[docs/spec.md](docs/spec.md)を参照してください。

---

*このプロジェクトは継続的に開発中です。バグ報告や機能提案はIssuesまでお願いします。*
