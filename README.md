# Cb (C-flat) 自作コンパイラ

## 概要

C++で作成した静的型付きの簡易片付け言語（静的型付きインタプリタ）です。
読み方はシーフラット(C++,C#があるのであえて逆を行ってみました)

flex(lex)とbison(yacc)を利用し、AST（抽象構文木）を構築し、C++でASTを逐次実行します。

## 特徴

- 静的型付き言語（型は宣言時に決定され、型違反は実行時にエラーとなります）
- 整数型の明示的な型宣言をサポート
    - tiny (int8_t), short (int16_t), int (int32_t), long (int64_t)
    - 例: `int a = 123;`, `long b = 1234567890123;`

## 追加予定機能

- 型
    - bool
    - 浮動小数点数
    - 文字列
- ループ
- 分岐
- 構造体
- 関数定義
    - ⭐️main関数から実行されること
- ポインタ
- interface
- trait

## ディレクトリ構成

- `src/lexer.l` : 字句解析（flex）
- `src/parser.y` : 構文解析・AST構築（bison）
- `src/ast.h` : ASTノード定義
- `src/eval.cpp`, `src/eval.h` : ASTの評価・実行
- `src/main.cpp` : メインプログラム
- `Makefile` : ビルド用
- `sample/` : サンプルコード

## ビルド方法

```sh
make
```

## テスト方法

ユニットテストは型ごとに分割されており、ASTノード＋evalによる評価ロジックの単体テストを行っています。

```sh
make unit-test
```

テストは `tests/unit/` 以下に型ごと・機能ごとに分割されており、assign/cross_type/boundary などはASTノードを直接生成し `eval` で評価します。


デバッグ出力は `CB_DEBUG_MODE=1` で有効、未設定時はデフォルトで無効です。

### テスト構成例

- `tests/unit/assign/` : 代入文の型ごとテスト
- `tests/unit/cross_type/` : 型変換のテスト
- `tests/unit/boundary/` : 各型の境界値テスト
- それぞれ `test_*.cpp` でASTノードを生成し `eval` で評価

## 実行方法

```sh
./main sample/test1.cb
```

## サンプルコード例

```cb
int a = 1 + 2;
long b = a * 10000000000;
print a;
print b;
```

## 注意事項
- C++11以降推奨（ASTノードの初期化やauto等を利用しています）。
- C++11未満の環境ではMakefileのCFLAGSに`-std=c++03`などを指定してください。
- 生成物や一時ファイルは`.gitignore`で管理されています。
