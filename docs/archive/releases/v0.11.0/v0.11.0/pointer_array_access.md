# Pointer Array Access (ポインタ配列アクセス)

**Version**: v0.11.0 Week 2 Day 3  
**Status**: ✅ Completed  
**Date**: 2025-01-XX

## 概要

配列へのポインタを使用して、ポインタ演算なしで配列要素にアクセスできる機能を実装しました。`ptr[index]`構文を使用して、すべてのプリミティブ型（int、float、double）と構造体配列にアクセスできます。

## 機能

### 1. プリミティブ型ポインタ配列アクセス

#### Int型配列
```cb
int[3] arr = {10, 20, 30};
int* ptr = &arr[0];

// 読み取り
println("%d", ptr[0]);  // 出力: 10
println("%d", ptr[1]);  // 出力: 20
println("%d", ptr[2]);  // 出力: 30

// 書き込み
ptr[0] = 100;
println("%d", arr[0]);  // 出力: 100（元の配列が変更される）
```

#### Float/Double型配列
```cb
float[3] farr = {1.5, 2.5, 3.5};
float* fptr = &farr[0];

// 読み取り（型情報が保持される）
println("%f", fptr[0]);  // 出力: 1.500000
println("%f", fptr[1]);  // 出力: 2.500000

// 書き込み
fptr[0] = 10.5;
println("%f", farr[0]);  // 出力: 10.500000

// Double型も同様に動作
double[3] darr = {1.5, 2.5, 3.5};
double* dptr = &darr[0];
println("%f", dptr[0]);  // 出力: 1.500000
```

### 2. 構造体ポインタ配列のアローアクセス

```cb
struct Point {
    int x;
    int y;
};

Point[3] points;
points[0].x = 10;
points[0].y = 20;
points[1].x = 30;
points[1].y = 40;

Point* ptr = &points[0];

// ptr[index]->member で読み取り
println("%d", ptr[0]->x);  // 出力: 10
println("%d", ptr[0]->y);  // 出力: 20
println("%d", ptr[1]->x);  // 出力: 30
println("%d", ptr[1]->y);  // 出力: 40

// ptr[index]->member で書き込み
ptr[0]->x = 100;
ptr[0]->y = 200;
ptr[1]->x = 300;

// 元の配列が変更される
println("%d", points[0].x);  // 出力: 100
println("%d", points[0].y);  // 出力: 200
println("%d", points[1].x);  // 出力: 300
```

## 実装の詳細

### 型保持システム

float/double型のポインタ配列アクセスでは、`ReturnException`メカニズムを使用して型情報を保持します：

1. **読み取り操作** (`array.cpp`)：
   - `ptr[index]`を評価すると、`ReturnException`で型情報付きの値を返す
   - `TYPE_FLOAT`または`TYPE_DOUBLE`フラグが設定される
   - `evaluate_typed_expression`で`ReturnException`をキャッチし、`TypedValue`に変換

2. **書き込み操作** (`simple_assignment.cpp`)：
   - 右辺を評価し、`ReturnException`から`TYPE_FLOAT/DOUBLE`を検出
   - `is_floating`フラグを設定
   - ポインタの`pointer_base_type`を確認し、適切な`assign_array_element_float`を呼び出す

### 構造体アクセスの実装

#### 読み取り (`special.cpp`)
```cpp
// evaluate_arrow_access内でReturnExceptionをキャッチ
try {
    ptr_value = evaluate_expression_func(node->left.get());
} catch (const ReturnException &ret) {
    if (ret.is_struct) {
        // 構造体からメンバーを取得して返す
        Variable member_var = get_struct_member_func(ret.struct_value, member_name);
        return member_var.value;
    }
}
```

#### 書き込み (`member_assignment.cpp`)
```cpp
// execute_arrow_assignment内でReturnExceptionをキャッチ
try {
    ptr_value = interpreter.evaluate(arrow_access->left.get());
    struct_var = reinterpret_cast<Variable *>(ptr_value);
} catch (const ReturnException &ret) {
    if (ret.is_struct) {
        // メタデータから配列名を取得
        // "points[0]"のような要素名を生成
        // 元の配列要素変数を取得して更新
    }
}
```

### ポインタメタデータ

構造体配列の書き込み時、元の配列要素を特定するために`PointerMetadata`の`array_name`フィールドを使用：

```cpp
struct PointerMetadata {
    std::string array_name;  // 配列変数の名前（例: "points"）
    size_t element_index;    // 要素インデックス（例: 0）
    // ...
};
```

## テストケース

### test_ptr_array_primitives.cb
- Int型ポインタ配列の読み取り・書き込み
- Float型ポインタ配列の読み取り・書き込み
- Double型ポインタ配列の読み取り・書き込み
- 元の配列が正しく更新されることを確認

### test_ptr_array_struct_arrow.cb
- 構造体ポインタ配列の読み取り（`ptr[0]->x`）
- 構造体ポインタ配列の書き込み（`ptr[0]->x = 100`）
- 元の構造体配列要素が正しく更新されることを確認

## 制限事項

1. **多次元配列**: 現在は1次元配列のみサポート
2. **ポインタ演算**: `ptr + 1`のような演算はサポートされていない（`ptr[1]`を使用）
3. **直接ポインタ**: メタデータポインタのみ完全サポート（直接Variable*ポインタは一部制限あり）

## ファイル変更

### 主要な変更
- `src/backend/interpreter/evaluator/access/array.cpp`: float/double型の`ReturnException`対応
- `src/backend/interpreter/executors/assignments/simple_assignment.cpp`: 
  - `ReturnException`からの型情報抽出
  - ポインタ型の`pointer_base_type`チェック
- `src/backend/interpreter/evaluator/access/special.cpp`: 
  - `evaluate_arrow_access`での`ReturnException`ハンドリング
- `src/backend/interpreter/executors/assignments/member_assignment.cpp`:
  - `execute_arrow_assignment`での構造体配列アクセス対応
  - メタデータからの配列名取得

### 関連ファイル
- `src/backend/interpreter/core/pointer_metadata.h`: `array_name`フィールド（既存）
- `src/backend/interpreter/core/interpreter.h`: `pointer_base_type`フィールド（既存）

## パフォーマンス

- **読み取り**: `ReturnException`のオーバーヘッドあり（型情報保持のため必要）
- **書き込み**: 型チェックの追加オーバーヘッドあり
- **構造体アクセス**: 配列要素名の文字列生成によるオーバーヘッドあり

通常の使用では問題ないレベルですが、ホットパスでの最適化が必要な場合は直接配列アクセス（`arr[index]`）を推奨。

## 今後の拡張

1. **多次元配列サポート**: `int[3][4] arr; int* ptr = &arr[0][0]; ptr[5]`
2. **ポインタ演算**: `ptr + offset`、`ptr++`、`ptr--`
3. **const correctness**: `const int* ptr`の書き込み禁止
4. **境界チェック**: デバッグモードでの範囲外アクセス検出

## まとめ

Week 2 Day 3で実装したポインタ配列アクセス機能により、Cbはより柔軟な配列操作をサポートするようになりました：

✅ Int型ポインタ配列: 完全サポート  
✅ Float/Double型ポインタ配列: 型保持システムで完全サポート  
✅ 構造体ポインタ配列: `ptr[index]->member`パターンで完全サポート  
✅ 読み取り・書き込み: すべての型で動作  
✅ 元の配列更新: すべてのケースで正しく動作  

この機能により、C言語スタイルのポインタ操作とCbの安全性を組み合わせた、強力な配列アクセスが可能になりました。
