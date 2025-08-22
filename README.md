# Cb (C-flat) 自作プログラミング言語・コンパイラ

## 概要

C++で作成した静的型付きの簡易プログラミング言語です。
読み方はシーフラット(C++, C#があるのであえて逆を行ってみました)

flex(lex)とbison(yacc)を利用してAST（抽象構文木）を構築し、C++でASTを逐次実行するインタープリター、またはCコードに変換するトランスパイラーとして動作します。

## 特徴

### 型システム
- **静的型付き言語**（型は宣言時に決定され、型チェックはパース時・実行時に行われます）
- **整数型**: `tiny` (int8_t), `short` (int16_t), `int` (int32_t), `long` (int64_t)
- **文字列型**: UTF-8対応の文字列処理
- **配列型**: 各型の配列をサポート（例: `int[]`, `string[]`）
- **配列リテラル**: `{1, 2, 3}` 形式での初期化と包括的な型チェック
- **bool型**: 真偽値の扱い
- **const修飾子**: 定数宣言のサポート

### 制御構造
- **Cライクな制御構造**: `if`/`else`, `else if`, `for`, `while`, `break`, `return`
- **ブロック文とブロックなし単文**の両方をサポート
- **関数定義と呼び出し**

### 演算子
- **Cライクな演算子優先順位**（`&&`, `||`, `==`, `!=`, `<`, `>`, `+`, `-`, `*`, `/`, `%` など）
- **自己代入演算子**（`+=`, `-=`, `*=`, `/=`, `%=`）
- **インクリメント・デクリメント**（`++`, `--`、前置・後置両方対応）

### エラーハンドリング・デバッグ
- **多言語対応エラーメッセージ**: 英語・日本語でのエラー表示
- **包括的な型範囲チェック**: 全整数型で自動範囲チェック
- **詳細なデバッグ機能**: `--debug`（英語）、`--debug-ja`（日本語）オプション
- **UTF-8文字列処理**: 日本語を含む文字列の適切な処理

### 出力形式
- **インタープリター**: 直接実行
- **トランスパイラー**: Cコードへの変換（cgen）

## 今後の拡張予定

- **浮動小数点数型**: `float`, `double`のサポート
- **構造体・クラス**: カスタムデータ型の定義
- **ポインタ・参照**: メモリ管理機能
- **interface/trait**: 抽象化機能の拡張
- **標準ライブラリ**: 文字列操作、数学関数、ファイルIO等の拡充
- **ジェネリクス・テンプレート**: 型パラメータ化機能

## ディレクトリ構成

```
src/
├── frontend/          # フロントエンド（字句・構文解析）
│   ├── lexer.l       # 字句解析（flex）
│   ├── parser.y      # 構文解析・AST構築（bison）
│   ├── parser_utils.cpp/h  # パーサー支援関数
│   ├── debug.h       # デバッグ機能定義
│   ├── debug_messages.cpp/h  # 多言語エラーメッセージ
│   └── main.cpp      # メインプログラム
├── backend/          # バックエンド（実行エンジン）
│   ├── interpreter.cpp/h  # ASTインタープリター
│   └── codegen.h     # コード生成インターface
├── common/           # 共通モジュール
│   ├── ast.h         # ASTノード定義
│   └── type_utils.cpp/h  # 型関連ユーティリティ
└── ast/              # AST関連の追加定義
    └── ast.h         # 拡張AST定義

cgen/                 # Cコード生成器
└── cgen_main.cpp     # トランスパイラー本体

tests/
├── unit/             # ユニットテスト
├── integration/      # 統合テスト（.hppファイル）
└── cases/            # テストケース（.cbファイル）
    ├── array_literal/  # 配列リテラルテスト
    ├── arithmetic/     # 算術演算テスト
    ├── string/         # 文字列処理テスト
    └── ...            # その他機能別テスト

sample/               # サンプルコード
Makefile             # ビルド設定
```

## ビルド方法

### インタープリター版
```sh
make
```

### トランスパイラー版（Cb → C変換）
```sh
make cgen
```

### デバッグ版
```sh
make debug-build
```

### 全体ビルド
```sh
make all
```

## テスト方法

### ユニットテスト
型ごと・機能ごとにASTノード生成と評価ロジックの単体テストを実行します。

```sh
make unit-test
```

### 統合テスト（Integration Test）
実際の.cbファイルをインタープリターで実行し、出力やエラーを検証します。

```sh
make integration-test
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

### トランスパイラー（Cb → C変換）
```sh
# Cコードに変換
./cgen_main sample/fibonacci.cb output.c

# 変換後のCコードをコンパイル・実行
gcc output.c -o output
./output
```

## サンプルコード例

### FizzBuzz
```cb
int main() {
    for (int i = 1; i <= 15; i = i + 1) {
        if ((i % 3 == 0) && (i % 5 == 0))
            print "FizzBuzz";
        else if (i % 3 == 0)
            print "Fizz";
        else if (i % 5 == 0)
            print "Buzz";
        else
            print i;
    }
    return 0;
}
```

### 配列とループ
```cb
int main() {
    int[] numbers = {10, 20, 30, 40, 50};
    
    for (int i = 0; i < 5; i++) {
        print numbers[i];
    }
    
    return 0;
}
```

### 文字列配列
```cb
int main() {
    string[] messages = {"Hello", "World", "Cb言語"};
    
    for (int i = 0; i < 3; i++) {
        print messages[i];
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
        print fibonacci(i);
    }
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

## 技術仕様

### 開発言語・ツール
- **C++17**: メイン実装言語
- **flex**: 字句解析器生成
- **bison**: 構文解析器生成
- **Make**: ビルドシステム

### アーキテクチャ
- **フロントエンド**: 字句解析 → 構文解析 → AST生成
- **バックエンド**: インタープリター実行 または Cコード生成
- **多言語サポート**: 英語・日本語でのエラーメッセージ・デバッグ情報
- **UTF-8対応**: 日本語を含む文字列の適切な処理

### テストカバレッジ
- **40+個の統合テストケース**: 全機能の動作検証
- **型安全性テスト**: 境界値・型不整合の検出確認
- **国際化テスト**: 多言語エラーメッセージの検証

## 貢献・開発

### 開発環境セットアップ
```sh
# 必要な依存関係をインストール
# Ubuntu/Debian:
sudo apt-get install flex bison build-essential

# macOS:
brew install flex bison

# プロジェクトのクローン・ビルド
git clone <repository-url>
cd Cb
make all
make integration-test
```

### デバッグ方法
- `--debug`: 英語デバッグ情報
- `--debug-ja`: 日本語デバッグ情報  
- `CB_DEBUG_MODE=1`: 環境変数でのデバッグ有効化

## 注意事項・制限事項

- **C++17以降推奨**: modern C++の機能を利用
- **整数型オーバーフロー**: 自動的に検出しエラー終了
- **UTF-8文字列**: 内部的にUTF-8で処理（表示は環境依存）
- **メモリ管理**: スマートポインタ使用、手動メモリ管理なし
- **関数オーバーロード**: 未サポート
- **例外処理構文**: 未サポート（`try`/`catch`なし）
