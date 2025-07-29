# Cb (C-flat) 自作コンパイラ

## 概要
C++で作成した簡易コンパイラです。
読み方はシーフラット(C++,C#があるのであえて逆を行ってみました)

flex(lex)とbison(yacc)を利用し、AST（抽象構文木）を構築し、C++でASTを逐次実行します。

## 追加予定機能

- 型
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

## 実行方法

```sh
./main sample/test1.cb
```

## サンプルコード例

```
a = 1 + 2;
b = a * 3;
print a;
print b;
```

## 注意事項
- C++11未満の環境ではMakefileのCFLAGSに`-std=c++03`などを指定してください。
- 生成物や一時ファイルは`.gitignore`で管理されています。
