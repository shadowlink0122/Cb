# Discard Variable Tests

このディレクトリには、discard変数（`_`）機能のテストケースが含まれています。

## 概要

Discard変数は、使用しない値を明示的に破棄するための特殊な変数です。
変数名として`_`を使用すると、その変数は宣言・代入はできますが、読み込むことはできません。

## テストケース

### ✅ Success Tests (basic/)

成功するべきテストケース（正常系）

1. **basic.cb** - 基本的なdiscard変数の使用
   - 複数の型でdiscard変数を宣言
   - 正常に実行完了

2. **function_return.cb** - 関数戻り値の破棄
   - 関数の戻り値をdiscard変数に代入
   - 関数は実行されるが戻り値は使用されない

3. **multiple.cb** - 複数のdiscard変数
   - 同じスコープ内で複数のdiscard変数を宣言
   - それぞれ独立して動作

### ❌ Error Tests (error/)

エラーになるべきテストケース（異常系）

1. **read_discard.cb** - discard変数の読み込み
   - discard変数を別の変数に代入しようとする
   - エラー: "Cannot reference discard variable '_'"

2. **use_in_expression.cb** - 式内でのdiscard変数使用
   - discard変数を計算式で使用しようとする
   - エラー: "Cannot reference discard variable '_'"

3. **pass_as_argument.cb** - 関数引数としての使用
   - discard変数を関数の引数として渡そうとする
   - エラー: "Cannot reference discard variable '_'"

4. **print_discard.cb** - discard変数の出力
   - discard変数をprintlnで出力しようとする
   - エラー: "Cannot reference discard variable '_'"

5. **reassign_discard.cb** - discard変数への再代入
   - discard変数に新しい値を代入しようとする
   - エラー: パーサーレベルで"Invalid assignment target"

6. **use_in_array.cb** - 配列リテラル内での使用
   - discard変数を配列の初期化リストで使用しようとする
   - エラー: パーサーレベルでエラー

7. **return_discard.cb** - discard変数のreturn
   - 関数内でdiscard変数を返り値として返そうとする
   - エラー: "Cannot reference discard variable '_'"

## 実行方法

### 個別テスト実行

```bash
# 成功テスト
./main tests/cases/discard_variable/basic/basic.cb

# エラーテスト
./main tests/cases/discard_variable/error/read_discard.cb
```

### 統合テスト実行

```bash
# 統合テストフレームワークから実行
./test_integration
```

## 期待される動作

### 許可される操作
- ✅ discard変数の宣言
- ✅ discard変数への代入（初期化時）
- ✅ 複数のdiscard変数の宣言

### 禁止される操作
- ❌ discard変数の読み込み
- ❌ discard変数の参照（式、関数引数、return等）
- ❌ discard変数への再代入

## 設計意図

Discard変数は以下の用途で使用されます：

1. **不要な戻り値の破棄** - 関数の副作用のみが必要な場合
2. **タプル展開での無視** - 将来実装予定のタプル機能で、一部の値を無視
3. **コードの意図を明示** - 値を使用しないことを明確に示す

## 実装詳細

- **トークン**: `TOK_UNDERSCORE`
- **ASTノード**: `AST_DISCARD_VARIABLE`
- **エラーチェック**: evaluator.cpp内で`AST_DISCARD_VARIABLE`の参照時にエラー
- **内部名**: 各discard変数には一意の内部名が割り当てられる（`_discard_N`形式）
