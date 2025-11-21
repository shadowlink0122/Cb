# Cb言語 実装優先度リスト

**最終更新**: 2025-11-21
**現在の統合テスト成功率**: ~58% (推定: 493/849)
**優先度1（メモリ管理）**: ✅ 完了（new/delete演算子実装済み）
**優先度2（関数ポインタ）**: ✅ 95%実装（直接宣言✅、戻り値✅、配列✅、配列要素直接呼出し❌）
**優先度3（ポインタ配列）**: ✅ 部分完了（型解析修正済み、関数渡し未対応）
**優先度5（Option/Result）**: ✅ Match式パターンマッチング完全実装

このドキュメントは、統合テスト結果を基に、実装が必要な機能の優先度をまとめたものです。
async/await機能は最後に実装します。

---

## 🔴 優先度1: 基本的なメモリ管理（緊急）

### 現状の問題
- コンパイラがSegmentation Fault 11でクラッシュする
- メモリ関連の基本機能が未実装

### 必要な機能
1. `new`演算子の実装
   - 型推論付きメモリ確保
   - コンストラクタ呼び出し
2. `delete`演算子の実装
   - デストラクタ呼び出し
   - 二重deleteの検出
3. `malloc`/`free`のサポート
4. `memcpy`のサポート
5. メモリ安全性チェック
   - Use-after-delete検出
   - ダングリングポインタ検出
   - メモリリーク検出

### テストケース（13件）
```
memory/test_new_delete_basic.cb
memory/test_new_delete_sizeof.cb
memory/test_memcpy_basic.cb
memory/test_memcpy_comprehensive.cb
memory/test_array_access.cb
memory/test_generic_memory_integration.cb
memory/test_memory_edge_cases.cb
memory/test_sizeof_advanced.cb
memory/errors/double_delete.cb
memory/errors/use_after_delete.cb
memory/errors/dangling_pointer_return.cb
memory/errors/delete_uninitialized.cb
memory/errors/memory_leak_detection.cb
stdlib/collections/test_array_double.cb (Segfault)
stdlib/collections/test_array_double_direct.cb (Segfault)
stdlib/collections/test_double_ptr.cb (Segfault)
```

### 実装方針
- HIRに`New`と`Delete`ノードを追加
- C++コード生成で`new`/`delete`に変換
- `malloc`はFFI経由で実装

---

## 🟠 優先度2: 関数ポインタの完全実装

### 現状の問題
- ✅ typedefを使った関数ポインタは動作する
- ✅ 直接宣言は実装済み (v0.14.0)
- ✅ 戻り値としての使用が実装済み (v0.14.0)
- ✅ 配列宣言が実装済み (v0.14.0)
- ✅ 直接呼び出し構文の問題も修正済み
- 🟡 配列要素の直接呼び出し（operations[0](10, 5)）は未対応

### 必要な機能
1. ✅ 関数ポインタの直接宣言
   ```cb
   int (*func)(int, int) = &add;  // 実装済み！
   ```
2. ✅ 関数ポインタの戻り値（v0.14.0で実装完了）
   ```cb
   int (*get_operation())(int, int) {  // 実装済み！
       return &add;  // 動作確認済み
   }
   ```
3. ✅ 関数ポインタの配列（v0.14.0で実装完了）
   ```cb
   int (*funcs[3])(int, int) = {&add, &sub, &mul};  // 実装済み！
   ```
4. 関数ポインタの比較演算
   ```cb
   if (func1 == func2) { ... }
   ```
5. アドレス表示とデバッグ

### テストケース（6件）
```
function_pointer/test_basic_typedef.cb
function_pointer/test_callback.cb
function_pointer/test_multiple_pointers.cb
function_pointer/test_return_function_pointer.cb
function_pointer/test_pointer_address_comparison.cb
function_pointer/test_pointer_address_print.cb
```

### 実装方針
- パーサーで関数ポインタ構文を完全サポート
- 型システムに関数ポインタ型を統合
- HIRで関数ポインタ型を明示的に扱う

---

## 🟠 優先度3: ポインタ機能の拡張

### 現状の問題
- 基本的なポインタ（`*`, `&`）は動作する
- ✅ ポインタ配列の型解析修正済み (`int*[3]` → `std::array<int*, 3>`)
- ⚠️ ポインタ配列の関数パラメータ渡しが未対応
- ダブルポインタが未対応

### 必要な機能
1. ダブルポインタ（`int**`）
   - ポインタのポインタ
   - 多段階の間接参照
2. ポインタ配列
   ```cb
   int* array[10];  // ポインタの配列
   ```
3. 配列のポインタ
   ```cb
   int (*ptr)[10];  // 配列へのポインタ
   ```
4. ポインタ演算の完全実装
   - 加算・減算
   - 配列アクセスとの関係
5. void*の完全サポート
   - 型キャスト
   - 安全性チェック
6. ポインタのフォーマット出力
   - アドレス表示（16進数）
7. 再帰的構造体のポインタ
   ```cb
   struct Node {
       int value;
       Node* next;
   }
   ```

### テストケース（23件）
```
pointer/test_double_pointer_basic.cb
pointer/test_pointer_array_edge_cases.cb
pointer/test_pointer_array_function_arg.cb
pointer/test_pointer_array_function_return.cb
pointer/test_pointer_array_loop_assign.cb
pointer/test_pointer_array_nested.cb
pointer/test_pointer_format.cb
pointer/test_deep_nested_struct.cb
pointer/test_recursive_struct.cb
pointer/test_struct_pointer_members.cb
pointer/test_pointer_boundary_comprehensive.cb
pointer/test_pointer_return_comprehensive.cb
pointer/test_ptr_array_struct_arrow.cb
pointer/test_ptr_comprehensive.cb
pointer/test_declaration_init_comprehensive.cb
pointer/test_advanced_pointer_features.cb
pointer/test_deref_incdec.cb
pointer/test_enum_pointer_basic.cb
pointer/test_enum_pointer_function.cb
pointer/test_float_double_pointer_function.cb
pointer/test_impl_pointer_function.cb
pointer/test_impl_with_pointers.cb
pointer/void_ptr_comprehensive.cb
```

### 実装方針
- HIRでポインタの次元を管理
- 型システムでポインタの階層を追跡
- C++コード生成で適切な型を出力

---

## 🟡 優先度4: 参照型（Reference）の完全実装

### 現状の問題
- 基本的な参照パラメータは動作する
- 戻り値としての参照が未対応

### 必要な機能
1. 参照の戻り値
   ```cb
   int& get_value() {
       return ref_to_value;
   }
   ```
2. 右辺値参照の型制約
   - プリミティブ型のムーブエラー検出
3. 参照のライフタイム管理
   - ダングリング参照の検出
4. const参照の完全サポート

### テストケース（3件）
```
reference/test_reference_return.cb
reference/test_simple_ref.cb (Abort trap)
reference/test_simple_reference_return.cb
rvalue_reference/type_restriction.cb
```

### 実装方針
- HIRで参照型の戻り値をサポート
- ライフタイム解析の基礎実装
- エラーチェックの強化

---

## 🟡 優先度5: エラー処理（Option/Result型）

### 現状の問題
- Option/Result型が未実装
- `?`演算子が未実装

### 必要な機能
1. `Option<T>`型の実装
   ```cb
   Option<int> find_value(string key) {
       if (found) {
           return Some(value);
       }
       return None;
   }
   ```
2. `Result<T, E>`型の実装
   ```cb
   Result<int, string> divide(int a, int b) {
       if (b == 0) {
           return Err("Division by zero");
       }
       return Ok(a / b);
   }
   ```
3. `?`演算子（エラー伝播）
   ```cb
   int value = get_result()?;
   ```
4. パターンマッチングとの統合
   ```cb
   match result {
       Ok(value) => println(value),
       Err(e) => println("Error: {e}")
   }
   ```

### テストケース（17件）
```
builtin_types/option_basic.cb
builtin_types/result_basic.cb
error_propagation/test_question_basic_option.cb
error_propagation/test_question_basic_result.cb
error_propagation/test_question_comprehensive.cb
error_propagation/test_question_operator_option.cb
error_propagation/test_question_operator_result.cb
error_propagation/test_result_propagation.cb
error_propagation/test_simple_propagation.cb
error_propagation/test_without_propagation.cb
pattern_matching/match_option_basic.cb
pattern_matching/match_result_basic.cb
pattern_matching/match_result_debug.cb
stdlib/std/option.cb
stdlib/std/result.cb
async/test_async_result_basic.cb
async/test_option_async_integration.cb
```

### 実装方針
- 標準ライブラリとしてOption/Resultを実装
- `?`演算子は構文糖衣として実装
- パターンマッチングで自動展開

---

## 🟡 優先度6: ラムダ式

### 現状の問題
- ラムダ式が全く実装されていない

### 必要な機能
1. 基本的なラムダ構文
   ```cb
   var add = |x, y| x + y;
   ```
2. 型推論
   ```cb
   var square = |x| x * x;  // 型を自動推論
   ```
3. キャプチャ
   ```cb
   int base = 10;
   var add_base = |x| x + base;  // 値キャプチャ
   ```
4. 参照キャプチャ
   ```cb
   var increment = |&x| { x = x + 1; };
   ```
5. 即座実行
   ```cb
   int result = (|x| x * 2)(5);  // => 10
   ```

### テストケース（8件）
```
lambda/basic/basic.cb
lambda/basic/assignment.cb
lambda/basic/multiple_params.cb
lambda/basic/void_return.cb
lambda/comprehensive/comprehensive.cb
lambda/comprehensive/simple.cb
lambda/immediate_invocation/immediate_invocation.cb
lambda/immediate_invocation/chain_invocation.cb
lambda/debug/debug.cb
async/test_async_lambda_basic.cb
async/test_async_lambda_complex.cb
async/test_async_lambda_params.cb
```

### 実装方針
- HIRに`Lambda`式を追加
- C++のラムダ式に変換
- キャプチャリストを明示的に管理

---

## 🟢 優先度7: デフォルト引数

### 現状の問題
- デフォルト引数が未実装

### 必要な機能
1. 関数のデフォルト引数
   ```cb
   void greet(string name = "World") {
       println("Hello, {name}!");
   }
   ```
2. 構造体メンバーのデフォルト値
   ```cb
   struct Point {
       int x = 0;
       int y = 0;
   }
   ```
3. 複数のデフォルト引数
4. 型推論との統合

### テストケース（11件）
```
default_args/test_default_args_basic.cb
default_args/test_default_args_types.cb
default_args/test_default_args_array.cb
default_args/test_default_args_struct.cb
default_args/test_default_args_const.cb
default_args/test_default_args_error1.cb
default_args/test_default_args_error2.cb
default_member/test_default_all_types.cb
default_member/test_default_member_implicit_assign.cb
default_member/test_default_impl.cb
default_member/test_bool_fix.cb
default_member/test_default_array_pointer.cb
default_member/test_suite.cb
```

### 実装方針
- ASTにデフォルト値を保存
- HIRで関数オーバーロードとして展開
- C++のデフォルト引数に変換

---

## 🟢 優先度8: 破棄変数（Discard Variable）

### 現状の問題
- 基本的な`_`は動作するが、エラーチェックが不十分

### 必要な機能
1. 複数の`_`変数
   ```cb
   var (_, _, value) = get_triple();
   ```
2. エラーチェックの強化
   - `_`の読み取り禁止
   - `_`の再代入禁止
   - `_`を引数に渡すことを禁止

### テストケース（5件）
```
discard_variable/basic/multiple.cb
discard_variable/error/read_discard.cb
discard_variable/error/reassign_discard.cb
discard_variable/error/return_discard.cb
discard_variable/error/use_in_array.cb
```

### 実装方針
- パーサーで`_`を特別扱い
- セマンティック解析で使用を禁止
- HIRでは無視

---

## 🟢 優先度9: Enum機能の拡張

### 現状の問題
- 基本的なenumは動作する
- 負の値、大きな値、関連値が未対応

### 必要な機能
1. 負の値のサポート
   ```cb
   enum Status {
       ERROR = -1,
       OK = 0,
       SUCCESS = 1
   }
   ```
2. 64bit値のサポート
   ```cb
   enum BigValue {
       MAX = 9223372036854775807
   }
   ```
3. 関連値付きenum（ADT）
   ```cb
   enum Result {
       Ok(int),
       Err(string)
   }
   ```
4. enumのコピーセマンティクス
5. enumのtypedef

### テストケース（8件）
```
enum/negative_values.cb
enum/large_values.cb
enum/test_enum_copy_semantics.cb
enum/test_nested_enum_associated.cb
enum/array_index.cb
typedef/test_enum_typedef_basic.cb
typedef/test_enum_typedef_comprehensive.cb
typedef/test_enum_typedef_functions.cb
typedef/test_enum_typedef_separated.cb
```

### 実装方針
- HIRでenum値の範囲を拡張
- 関連値はstd::variantで実装
- typedefはC++のusing句に変換

---

## 🟢 優先度10: グローバル変数と静的変数

### 現状の問題
- ローカルな静的変数は動作する
- グローバル変数と配列が未対応

### 必要な機能
1. グローバル変数
   ```cb
   int global_counter = 0;
   
   void increment() {
       global_counter += 1;
   }
   ```
2. グローバル配列
   ```cb
   int global_array[100];
   ```
3. 静的変数の初期化順序
4. 関数間でのグローバル変数共有

### テストケース（7件）
```
global_vars/basic.cb
global_vars/array_share.cb
global_array/basic.cb
global_array/function_access.cb
global_array/types.cb
static_variables/basic_static.cb
static_variables/static_integration.cb
```

### 実装方針
- HIRでグローバルスコープを管理
- C++のグローバル変数に変換
- 初期化順序を保証

---

## 🟢 優先度11: ジェネリック関数

### 現状の問題
- ジェネリック構造体は動作する
- ジェネリック関数が未実装

### 必要な機能
1. 関数のジェネリックパラメータ
   ```cb
   fn identity<T>(value: T) -> T {
       return value;
   }
   ```
2. 複数の型パラメータ
   ```cb
   fn pair<T, U>(first: T, second: U) -> Pair<T, U> {
       return {first, second};
   }
   ```
3. swap関数
   ```cb
   fn swap<T>(a: &T, b: &T) {
       T temp = a;
       a = b;
       b = temp;
   }
   ```

### テストケース（6件）
```
generics/function_basic.cb
generics/function_comprehensive.cb
generics/function_multiple_params.cb
generics/function_swap.cb
generics/function_with_struct.cb
generics/test_multiple_type_params.cb
```

### 実装方針
- パーサーで関数のジェネリックをサポート
- HIRでテンプレート展開
- C++テンプレート関数に変換

---

## 🟢 優先度12: FFI（Foreign Function Interface）の拡張

### 現状の問題
- FFI宣言のパースは動作する
- 実際の関数呼び出しが未対応

### 必要な機能
1. double戻り値の処理
2. void戻り値の処理
3. 複数モジュールのサポート
4. 数学関数の統合
   - `sqrt`, `pow`, `sin`, `cos`, `tan`

### テストケース（8件）
```
ffi/double_return.cb
ffi/void_return.cb
ffi/int_functions.cb
ffi/math_functions.cb
ffi/trigonometric.cb
ffi/multi_module.cb
ffi/module_namespace.cb
ffi/test_ffi_basic.cb
```

### 実装方針
- FFI関数のC++宣言を生成
- 名前空間でモジュールを分離
- リンク時に実際の関数を解決

---

## 🟢 優先度13: モジュールシステム

### 現状の問題
- import文のパースは動作する
- 実際のモジュール解決が未実装

### 必要な機能
1. 基本的なimport/export
   ```cb
   // math.cb
   export fn add(a: int, b: int) -> int {
       return a + b;
   }
   
   // main.cb
   import math;
   int result = math.add(1, 2);
   ```
2. 修飾名での呼び出し
3. 複数モジュールの統合
4. 循環依存の検出

### テストケース（11件）
```
import_export/test_basic_import_export.cb
import_export/test_qualified_call.cb
import_export/test_multiple_modules.cb
import_export/test_integration.cb
import_export/test_duplicate_import.cb
import_export/test_import_const.cb
import_export/test_import_constructor.cb
import_export/test_module_with_helper.cb
import_export/test_module_helper.cb
module_functions/test_module_functions.cb
module_functions/test_simple_module_functions.cb (✓)
```

### 実装方針
- ファイル単位でモジュールを管理
- コンパイル時にすべてのモジュールを読み込み
- C++の名前空間に変換

---

## 🟢 優先度14: constパラメータ

### 現状の問題
- const変数は動作する
- constパラメータが未対応

### 必要な機能
1. constパラメータの読み取り
   ```cb
   fn process(const int value) {
       println(value);  // OK
       value = 10;      // Error
   }
   ```
2. const違反のエラーチェック
3. const配列パラメータ
   ```cb
   fn sum(const int[] array, int size) -> int {
       // ...
   }
   ```

### テストケース（6件）
```
const_parameters/const_param_read_ok.cb (Abort trap)
const_parameters/const_param_reassign_error.cb
const_parameters/const_param_compound_error.cb
const_parameters/const_array_param_error.cb
const_parameters/const_all_types_ok.cb (Abort trap)
const_parameters/const_mixed_params_ok.cb (Abort trap)
```

### 実装方針
- パラメータにconst修飾子を追加
- セマンティック解析でconst違反を検出
- HIRでconst情報を保持

---

## 🟢 優先度15: 文字列機能の拡張

### 現状の問題
- 基本的な文字列は動作する
- 文字列補間が不完全

### 必要な機能
1. 文字列補間の高度な機能
   ```cb
   int x = 10;
   println("Value: {x}");
   ```
2. エスケープシーケンス
   ```cb
   println("Line1\nLine2\tTab");
   ```
3. 配列要素へのアクセス
   ```cb
   int[] arr = {1, 2, 3};
   println("First: {arr[0]}");
   ```
4. 構造体メンバーへのアクセス
   ```cb
   Point p = {10, 20};
   println("Point: ({p.x}, {p.y})");
   ```

### テストケース（11件）
```
string_interpolation/test_basic.cb (Abort trap)
string_interpolation/test_array_access.cb (Abort trap)
string_interpolation/test_member_access.cb (Abort trap)
string_interpolation/test_types.cb (Abort trap)
string_interpolation/test_expressions.cb (Abort trap)
string_interpolation/test_edge_cases.cb (Abort trap)
string_interpolation/test_escape.cb
string_interpolation/test_escape_simple.cb
string_interpolation/expression_evaluation.cb
string_interpolation/practical_examples.cb
string_interpolation/advanced_features.cb
string_array/test_string_array_basic.cb (Abort trap)
string_array/test_string_array_const.cb (Abort trap)
string_array/test_string_array_struct.cb (Abort trap)
```

### 実装方針
- パーサーで`{}`内の式を解析
- 実行時に式を評価して文字列に変換
- C++のstd::formatまたは独自実装

---

## 🔵 優先度16（最後）: Async/Await完全実装

### 現状の問題
- 基本的なasync/awaitは動作する
- 高度な機能が未実装

### 必要な機能
1. `sleep()`関数
   ```cb
   async fn task() {
       await sleep(1000);  // 1秒待機
   }
   ```
2. タイムアウト機能
   ```cb
   Result<int, string> result = await timeout(async_task(), 5000);
   ```
3. 非同期インターフェース
   ```cb
   interface AsyncHandler {
       async fn handle() -> Result<int, string>;
   }
   ```
4. 非同期ラムダ
   ```cb
   var async_task = async || {
       await sleep(100);
       return 42;
   };
   ```
5. Resultとの統合
   ```cb
   Result<int, string> result = await fetch_data()?;
   ```
6. yield機能の拡張
   ```cb
   async fn generator() {
       yield 1;
       yield 2;
       yield 3;
   }
   ```

### テストケース（52件）
```
async/test_async_sleep.cb
async/test_sleep_simple.cb
async/test_sleep_concurrent.cb
async/test_timeout_basic.cb
async/test_timeout_compile.cb
async/test_timeout_comprehensive.cb
async/test_timeout_concurrent.cb
async/test_timeout_multiple.cb
async/test_timeout_sequential.cb
async/test_timeout_types.cb
async/test_timeout_chained.cb
async/test_timeout_edge_cases.cb
async/future_features/test_timeout_question_operator.cb
async/future_features/test_timeout_zero.cb
async/phase2_async_interface.cb
async/test_interface_basic.cb
async/test_interface_concurrent.cb
async/test_interface_self.cb
async/test_async_lambda_basic.cb
async/test_async_lambda_complex.cb
async/test_async_lambda_params.cb
async/test_async_result_basic.cb
async/test_async_result_integration.cb
async/test_async_result_minimal.cb
async/test_async_result_propagation.cb
async/test_async_simple_result.cb
async/comprehensive_async_result.cb
async/test_result_only.cb
async/test_result_construct.cb
async/test_async_question_operator.cb
async/integration_async_syntax.cb
async/test_yield_basic.cb
async/test_yield_edge_cases.cb
async/test_yield_state.cb
async/test_task_queue_basic.cb (Abort trap)
async/test_task_queue_comprehensive.cb (Abort trap)
async/test_task_queue.cb (Abort trap)
async/test_async_loop.cb
async/test_async_nested.cb
async/test_async_recursive.cb
async/phase2_for_loop_fairness.cb
async/phase2_nested_function_fairness.cb
async/phase2_recursive_fairness.cb
async/phase2_task_timing_test.cb
async/phase2_unawaited_exit_test.cb
async/phase2_while_loop_fairness.cb
async/test_async_cooperative.cb
async/test_async_complex_patterns.cb
async/test_async_variant_check.cb
async/test_async_generic_interface_comprehensive.cb
async/test_nested_generic_non_async.cb
async/test_no_vardecl.cb
builtin_types/sleep_test.cb
```

### 実装方針
- イベントループの拡張
- タイマー機能の追加
- コルーチン実装の完成

---

## テスト統計サマリー

| 優先度 | カテゴリ | 失敗テスト数 | 推定成功率向上 |
|--------|----------|--------------|----------------|
| 🔴 P1 | メモリ管理 | 16件 | +3.5% |
| 🟠 P2 | 関数ポインタ | 6件 | +1.4% |
| 🟠 P3 | ポインタ拡張 | 23件 | +5.4% |
| 🟡 P4 | 参照型 | 4件 | +0.9% |
| 🟡 P5 | Option/Result | 17件 | +4.0% |
| 🟡 P6 | ラムダ式 | 12件 | +2.8% |
| 🟢 P7 | デフォルト引数 | 13件 | +3.0% |
| 🟢 P8 | 破棄変数 | 5件 | +1.2% |
| 🟢 P9 | Enum拡張 | 9件 | +2.1% |
| 🟢 P10 | グローバル/静的 | 7件 | +1.6% |
| 🟢 P11 | ジェネリック関数 | 6件 | +1.4% |
| 🟢 P12 | FFI拡張 | 8件 | +1.9% |
| 🟢 P13 | モジュール | 11件 | +2.6% |
| 🟢 P14 | constパラメータ | 6件 | +1.4% |
| 🟢 P15 | 文字列拡張 | 14件 | +3.3% |
| 🔵 P16 | Async/Await | 52件 | +12.2% |

**合計失敗テスト数**: 209件（その他230件は別カテゴリ）

---

## 実装ロードマップ

### フェーズ1: 安定化（P1-P3）
- メモリ管理の実装
- 関数ポインタの完全サポート
- ポインタ機能の拡張
- **目標**: クラッシュをゼロにする

### フェーズ2: コア機能（P4-P6）
- 参照型の完全実装
- Option/Result型の実装
- ラムダ式の実装
- **目標**: 現代的なエラーハンドリング

### フェーズ3: 利便性向上（P7-P10）
- デフォルト引数
- 破棄変数の完全サポート
- Enum機能の拡張
- グローバル/静的変数の完全サポート
- **目標**: 使いやすさの向上

### フェーズ4: 高度な機能（P11-P15）
- ジェネリック関数
- FFI拡張
- モジュールシステム
- constパラメータ
- 文字列機能拡張
- **目標**: 言語機能の完成

### フェーズ5: 非同期処理（P16）
- Async/Awaitの完全実装
- **目標**: 非同期処理の完成

---

## 次のアクション

1. **優先度1（メモリ管理）から実装を開始**
   - `new`/`delete`演算子
   - メモリ安全性チェック
   
2. **各機能の実装前にテストを確認**
   - テストケースを読んで期待動作を理解
   - 最小限の実装で最大の効果を狙う

3. **統合テストを継続的に実行**
   - 各実装後に成功率を確認
   - リグレッションを防ぐ

---

**このドキュメントは実装の進捗に応じて更新されます。**

---

## 実装進捗ログ

### 2025-11-21

#### ✅ 完了: Match式パターンマッチング（優先度5）
- Option/Result型のパターンマッチング完全実装
- 変数バインディング対応（Some(x), Ok(val), Err(e)）
- HIR→C++ if-else chainへの変換実装
- テスト: 複数のmatchテストが動作確認済み

#### ✅ 完了: メモリ管理 - new/delete演算子（優先度1）
- `new T[size]`配列メモリ確保の実装
- 配列サイズのHIR伝播修正
- 小数点リテラル（double_value）の正しい処理
- `array_set_double`/`array_get_double`組み込み関数追加
- テスト: test_array_double.cb ✅ PASSED

### 2025-11-19

#### ✅ 完了: HIRでのポインタ対応
- 変数ポインタ（`&`, `*`）の実装完了
- 関数ポインタのtypedef対応完了
- テスト: `tests/test_pointer_basic.cb` ✅ PASSED
- テスト: `tests/test_function_pointer.cb` ✅ PASSED

#### 🔄 進行中: メモリ管理（優先度1）
**問題**: `new`/`delete`演算子でSegmentation Fault発生
**原因**: HIR生成で型情報が正しく処理されていない
**次のステップ**: 
1. `new`式のデバッグを継続
2. `generate_type`での型解決を修正
3. 配列の`new`サポート

**保留理由**: 時間を考慮し、より基礎的な機能を先に完成させる

#### 次の実装予定
1. ポインタ機能の拡張（優先度3）
   - ダブルポインタ（`int**`）
   - ポインタ配列
2. 関数ポインタの完全実装（優先度2）
   - 直接宣言（typedefなし）
   - 戻り値としての使用


---

## 実装進捗ログ（更新）

### 2025-11-19 午後

#### ✅ 完了・動作確認済み

**基本的なポインタ操作**:
- ✅ 変数のアドレス取得 (`&x`)
- ✅ 間接参照 (`*ptr`)
- ✅ ポインタ経由の値変更
- ✅ 構造体ポインタとアロー演算子 (`p->x`)
- ✅ ダブルポインタ (`int**`) - 完全動作
- ✅ 多段階間接参照 (`**pp`)

**関数ポインタ（typedef版）**:
- ✅ typedefを使った関数ポインタ定義
- ✅ 関数ポインタ経由の呼び出し
- ✅ 関数ポインタのパラメータ渡し
- ✅ 関数ポインタの変数代入

**実行成功したテスト**:
```bash
./cb compile tests/test_pointer_basic.cb && ./tests/test_pointer_basic.o
# Output: ✅ All tests passed

./cb compile tests/test_function_pointer.cb && ./tests/test_function_pointer.o  
# Output: ✅ Function pointer tests passed

./cb compile /tmp/test_double_ptr.cb && /tmp/test_double_ptr.o
# Output: ✅ Double pointer working correctly
```

#### ❌ 未完了・問題あり

**メモリ管理**:
- ❌ `new` 演算子 - Segmentation Fault
  - 原因: HIR生成で型情報が不完全
  - 状況: 基本型（`new int`）は一部動作するが不安定
- ❌ `delete` 演算子 - `new`に依存
- ❌ 配列の動的確保 (`new int[10]`)

**ポインタ配列**:
- ❌ `int*[3]` 構文 - 型解決エラー
  - 原因: `std::array<int, 3>`として誤解釈（正: `std::array<int*, 3>`）
  - パーサーで配列とポインタの組み合わせが未対応

**関数ポインタ（typedef なし）**:
- ❌ 直接宣言 `int (*func)(int, int)`
- ❌ 戻り値としての関数ポインタ
- ❌ 関数ポインタの配列

#### 🔧 必要な修正

1. **ポインタ配列の型解決**（優先）
   - ファイル: `src/frontend/recursive_parser/parsers/type_parser.cpp`
   - `T*[N]`を`std::array<T*, N>`として正しく解釈
   
2. **`new`のSegfault修正**（優先）
   - ファイル: `src/backend/ir/hir/hir_generator.cpp`
   - `AST_NEW_EXPR`の型情報推論を改善
   
3. **関数ポインタの直接宣言**（中）
   - パーサーで`(*func)`構文をサポート

#### 📊 統計

- 実装開始時の成功率: 48.2% (408/847)
- 新規動作確認: ダブルポインタ、関数ポインタ（typedef）
- 推定成功率向上: +2.5% (約21テスト)
- 残課題: メモリ管理（16テスト）、ポインタ配列（5テスト）

---

## 実装ログ

### 2025-11-21: Match式パターンマッチングの完全実装 (優先度5)
- **実装内容**:
  - ✅ Match式がOption/Result型で完全に動作
  - ✅ 変数バインディングの実装（`Some(val)` → 自動的に変数`val`が使える）
  - ✅ ネストしたパターンマッチングのサポート
- **修正したバグ**:
  - AST→HIRの変換でmatch_exprフィールドの欠落を修正
  - 変数バインディングでCB_HIR_プレフィックスの追加漏れを修正
  - 一時変数カウンタの宣言漏れを修正
- **成功率の改善**: 推定 +5テスト分

### 2025-11-21: メモリ管理実装 (優先度1)
- **実装内容**:
  - ✅ new/delete演算子の完全実装
  - ✅ 配列newの修正（`new double[5]`のサイズ保持）
  - ✅ float/doubleリテラルの精度保持修正
  - ✅ array_set_double/array_get_doubleヘルパー関数の追加
- **修正したバグ**:
  - HIRでnew_array_sizeが転送されない問題を修正
  - floatリテラルがintに変換される問題を修正
  - test_array_double.cbのセグフォルトを解消
- **成功率の改善**: 推定 +13テスト分

### 2025-11-21: 関数ポインタほぼ完全実装 (優先度2)
- **実装内容**:
  - ✅ `int (*func)(int, int) = &add;` 形式の直接宣言をサポート
  - ✅ statement_parser.cppでパターン `(*identifier)` を検出し関数ポインタとして解析
  - ✅ HIRへの変換とC++コード生成も正常に動作（基本宣言のみ）
  - ✅ test_basic_typedef.cbが正常に動作
  - ✅ 関数ポインタ戻り値型のパーサー実装完了 `int (*func(params))(int, int)`
  - ✅ ASTノードに`is_function_pointer_return`フラグ追加
  - ✅ 関数ポインタ戻り値型のHIR変換実装完了
  - ✅ 関数ポインタ戻り値型のC++コード生成実装完了
  - ✅ 変数宣言での関数ポインタ型推論修正
- **実装ファイル**:
  - src/frontend/recursive_parser/parsers/statement_parser.cpp: 関数ポインタ直接宣言と戻り値型の解析
  - src/common/ast.h: is_function_pointer_returnフラグ追加
  - src/backend/ir/hir/hir_decl_type_converter.cpp: 関数ポインタ戻り値のHIR変換
  - src/backend/codegen/codegen_declarations.cpp: 関数ポインタ戻り値の特殊構文生成
  - src/backend/codegen/codegen_types.cpp: ポインタから関数への特殊処理
  - src/backend/ir/hir/hir_stmt_converter.cpp: 関数ポインタ変数宣言の型処理
  - ✅ 関数ポインタ配列の宣言サポート実装 `int (*funcs[3])(int, int)`
  - ✅ 直接関数ポインタ呼び出し構文の修正（`getOperation(3)(6, 7)`が動作）
  - ✅ 関数ポインタ配列のHIR変換とC++コード生成
- **残作業**:
  - 🟡 配列要素の直接呼び出し `operations[0](10, 5)` のサポート
  - ❌ 関数ポインタの比較演算子サポート
- **成功率の改善**: ~8テスト分（関数ポインタテストがほぼ動作）

