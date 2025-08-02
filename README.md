# Cb (C-flat) 自作コンパイラ

## 概要

C++で作成した静的型付きの簡易片付け言語（静的型付きインタプリタ）です。
読み方はシーフラット(C++,C#があるのであえて逆を行ってみました)

flex(lex)とbison(yacc)を利用し、AST（抽象構文木）を構築し、C++でASTを逐次実行します。

## 特徴

- 静的型付き言語（型は宣言時に決定され、型違反は実行時にエラーとなります）
- 整数型の明示的な型宣言: tiny (int8_t), short (int16_t), int (int32_t), long (int64_t)
    - 例: `int a = 123;`, `long b = 1234567890123;`
- Cライクな制御構造: if/else, else if, for, while, break, ブロックなし単文もサポート
- Cライクな演算子優先順位（&&, ||, ==, !=, <, >, +, -, *, /, % など）
- Cライクな自己代入演算子（`+=`, `-=`, `*=`, `/=`, `%=`）とインクリメント・デクリメント（`++`, `--`、前置・後置）に対応
- すべての整数型で型ごとの範囲チェックを自動で行い、範囲外は実行時エラー
- printによる標準出力
- サンプル・テストケースが豊富

## 追加予定機能

- 型
    - bool（部分実装済み）
    - 浮動小数点数
    - 文字列（部分実装済み）
- 構造体
- 関数定義の拡張（引数・戻り値型推論や多戻り値など）
- ポインタ
- interface/trait
- 標準ライブラリの拡充

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

### ユニットテスト
型ごと・機能ごとにASTノード＋evalによる評価ロジックの単体テストを行っています。

```sh
make unit-test
```

`tests/unit/` 以下にassign/cross_type/boundary などのテストがあり、ASTノードを直接生成し `eval` で評価します。

### 結合テスト（integration test）
実際の.cbファイル（サンプルコード）をmainで解釈・実行し、出力やエラーを検証します。

```sh
make integration-test
```

`tests/integration/` 以下に各種制御構造（if/else, for, while, break, 演算子優先順位など）のテストがあり、`tests/cases/` 以下の.cbファイルを使って動作検証します。

デバッグ出力は `CB_DEBUG_MODE=1` で有効、未設定時はデフォルトで無効です。

## 実行方法

```sh
./main sample/test1.cb
```

## サンプルコード例

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

## 注意事項
- C++11以降推奨（ASTノードの初期化やauto等を利用しています）。
- C++11未満の環境ではMakefileのCFLAGSに`-std=c++03`などを指定してください。
- 生成物や一時ファイルは`.gitignore`で管理されています。
- 変数のインクリメント・デクリメントや自己代入演算子も型ごとの範囲チェックが自動で行われます。範囲外の値になると即座にエラー終了します。
