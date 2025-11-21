# デバッグモードガイド / Debug Mode Guide

## 概要 / Overview

Cbコンパイラv0.14.0には、HIR生成とC++コード生成の詳細なデバッグメッセージ機能が追加されました。
これにより、コンパイラの内部動作を詳しく観察でき、ポインタ実装などの開発時に役立ちます。

The Cb compiler v0.14.0 includes detailed debug messages for HIR generation and C++ code generation.
This allows you to observe the internal workings of the compiler in detail, which is useful during development of pointer implementations and other features.

## デバッグモードの使用方法 / How to Use Debug Mode

### コンパイル時にデバッグを有効化 / Enable Debug During Compilation

```bash
# 英語のデバッグメッセージ / English debug messages
./cb compile --debug <file.cb>

# 日本語のデバッグメッセージ / Japanese debug messages
./cb compile --debug-ja <file.cb>
```

### デバッグメッセージのカテゴリ / Debug Message Categories

#### 1. HIR生成関連 / HIR Generation
- `[HIR_GENERATION_START]` - HIR生成開始
- `[HIR_STRUCT_PROCESSING]` - 構造体処理中
- `[HIR_STRUCT_ADDED]` - 構造体追加完了
- `[HIR_FUNC_PROCESSING]` - 関数処理中
- `[HIR_FUNC_ADDED]` - 関数追加完了
- `[HIR_TYPE_POINTER]` - ポインタ型検出
- `[HIR_GENERATION_COMPLETE]` - HIR生成完了

#### 2. C++コード生成関連 / C++ Code Generation
- `[CODEGEN_CPP_START]` - C++コード生成開始
- `[CODEGEN_CPP_PROGRAM]` - プログラム全体の生成
- `[CODEGEN_CPP_STRUCT_START]` - 構造体生成開始
- `[CODEGEN_CPP_STRUCT_COMPLETE]` - 構造体生成完了
- `[CODEGEN_CPP_FUNC_START]` - 関数生成開始
- `[CODEGEN_CPP_FUNC_COMPLETE]` - 関数生成完了
- `[CODEGEN_CPP_COMPLETE]` - C++コード生成完了（行数付き）

#### 3. ポインタ操作関連 / Pointer Operations
- `[CODEGEN_CPP_PTR]` - ポインタ型生成
  - `Generating pointer type for: <type>` - ポインタ型生成開始
  - `Generated pointer type: <type>*` - ポインタ型生成完了
  - `Const pointer: <type>* const` - constポインタ
  - `Pointer to const: const <type>*` - constへのポインタ
  - `Address-of: &<expr>` - アドレス演算子
  - `Dereference: *<expr>` - デリファレンス
  - `Arrow operator: <obj>-><member>` - アロー演算子
  - `Pointer arithmetic: <ptr> <op> <value>` - ポインタ算術演算

#### 4. ステートメント生成 / Statement Generation
- `[CODEGEN_CPP_STMT]` - ステートメント生成
  - `Variable declaration: <type> <name>` - 変数宣言
  - `Assignment: <name> = ...` - 代入文
  - `If statement` - if文
  - `While loop` - whileループ
  - `For loop` - forループ
  - `Return statement` - return文
  - `Block` - ブロック

#### 5. 式生成 / Expression Generation
- `[CODEGEN_CPP_EXPR]` - 式生成
  - `Literal: <value>` - リテラル
  - `Variable: <name>` - 変数参照
  - `Binary op: <op>` - 二項演算
  - `Unary op: <op>` - 単項演算
  - `Function call: <func>` - 関数呼び出し
  - `Method call: <obj>.<method>` - メソッド呼び出し
  - `Member access: .<member>` - メンバアクセス
  - `Array access: [<index>]` - 配列アクセス

## 実例 / Example

### テストプログラム / Test Program

```cb
// test_pointer_debug.cb
struct Point {
    int x;
    int y;
};

void test_pointers() {
    int x = 42;
    int* ptr = &x;
    int value = *ptr;
    println("Value:", value);
    
    Point p;
    p.x = 10;
    p.y = 20;
    Point* pptr = &p;
    println("Point x:", pptr->x);
}

int main() {
    test_pointers();
    return 0;
}
```

### デバッグ出力例 / Debug Output Example

```
[CODEGEN_CPP_START]
[CODEGEN_CPP_PROGRAM] Generating program (structs: 1, functions: 2)
[CODEGEN_CPP_STRUCT_START] Generating struct: Point (fields: 2)
[CODEGEN_CPP_STRUCT_COMPLETE] ✓ Struct generated: Point

[CODEGEN_CPP_FUNC_START] Generating function: test_pointers (params: 0)
[CODEGEN_CPP_STMT]   Variable declaration: int x
[CODEGEN_CPP_PTR] Generating pointer type for: int
[CODEGEN_CPP_PTR]   Generated pointer type: int*
[CODEGEN_CPP_STMT]   Variable declaration: int* ptr
[CODEGEN_CPP_PTR]   Address-of: &x
[CODEGEN_CPP_PTR]   Dereference: *ptr
[CODEGEN_CPP_STMT]   Variable declaration: int value

[CODEGEN_CPP_STMT]   Variable declaration: Point p
[CODEGEN_CPP_PTR] Generating pointer type for: Point
[CODEGEN_CPP_PTR]   Generated pointer type: Point*
[CODEGEN_CPP_STMT]   Variable declaration: Point* pptr
[CODEGEN_CPP_PTR]   Arrow operator: pptr->x

[CODEGEN_CPP_FUNC_COMPLETE] ✓ Function generated: test_pointers (statements: 10)
[CODEGEN_CPP_COMPLETE] === C++ Code Generation Completed (lines: 122) ===
```

## デバッグメッセージのフィルタリング / Filtering Debug Messages

特定のメッセージだけを表示したい場合は、grepを使用します:

To display only specific messages, use grep:

```bash
# ポインタ関連のメッセージのみ表示
# Show only pointer-related messages
./cb compile --debug test.cb 2>&1 | grep "CODEGEN_CPP_PTR"

# HIR生成のメッセージのみ表示
# Show only HIR generation messages
./cb compile --debug test.cb 2>&1 | grep "HIR_"

# 構造体生成のメッセージのみ表示
# Show only struct generation messages
./cb compile --debug test.cb 2>&1 | grep "STRUCT"
```

## ポインタ実装の検証 / Verifying Pointer Implementation

デバッグモードを使用してポインタ実装を検証する手順:

Steps to verify pointer implementation using debug mode:

1. **ポインタ型の生成確認** / Verify pointer type generation
   - `[CODEGEN_CPP_PTR] Generated pointer type` メッセージを確認
   - 正しいポインタ型（`int*`, `Point*`など）が生成されているか

2. **アドレス演算子の確認** / Verify address-of operator
   - `[CODEGEN_CPP_PTR] Address-of: &<var>` メッセージを確認
   - `&`演算子が正しく適用されているか

3. **デリファレンスの確認** / Verify dereference operator
   - `[CODEGEN_CPP_PTR] Dereference: *<var>` メッセージを確認
   - `*`演算子が正しく適用されているか

4. **アロー演算子の確認** / Verify arrow operator
   - `[CODEGEN_CPP_PTR] Arrow operator: <ptr>-><member>` メッセージを確認
   - `->`演算子が正しく生成されているか

5. **生成されたC++コードの確認** / Verify generated C++ code
   - `-cpp <dir>` オプションで生成されたC++コードを確認
   - ポインタ操作が正しいC++コードになっているか

## トラブルシューティング / Troubleshooting

### デバッグメッセージが表示されない
Debug messages not showing up

**原因**: `--debug`フラグが指定されていない
**Cause**: `--debug` flag not specified

**解決方法**: コンパイル時に`--debug`または`--debug-ja`を追加
**Solution**: Add `--debug` or `--debug-ja` when compiling

### ポインタ型生成メッセージが重複して表示される
Pointer type generation messages appear multiple times

**原因**: 型解決が複数回行われている（正常動作）
**Cause**: Type resolution occurs multiple times (normal behavior)

**説明**: 同じポインタ型が異なるコンテキストで複数回生成されることは正常です
**Explanation**: It's normal for the same pointer type to be generated multiple times in different contexts

## まとめ / Summary

デバッグモードを活用することで:
- コンパイラの内部動作を理解できる
- ポインタ実装の正確性を検証できる
- バグの特定が容易になる
- 新機能の開発がスムーズになる

By utilizing debug mode:
- Understand the internal workings of the compiler
- Verify the accuracy of pointer implementations
- Easily identify bugs
- Smoothly develop new features

---

**バージョン / Version**: Cb Compiler v0.14.0
**最終更新 / Last Updated**: 2025-11-19
