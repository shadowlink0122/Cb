# Cb v0.14.0 統合テスト失敗分析とHIR実装ガイド

**作成日**: 2025-11-22
**対象バージョン**: v0.14.0
**テスト実行**: `make integration-test-unified-compiler`

## 📊 テスト結果サマリー

- **総テスト数**: 856
- **成功**: 495 (57.8%)
- **失敗**: 361 (42.2%)

## 🎯 失敗テストの分類方針

### 意図的な失敗（Expected Failures）
以下のパターンに該当するテストは**失敗することが期待されている**ため、修正不要：
- `*_error.cb` - エラー検出テスト
- `*_fail_*.cb` - 失敗パターンテスト
- `error_*.cb` - エラーケーステスト
- `ng.cb` / `*/error/*` - ネガティブテスト

### 修正が必要な失敗
上記パターンに該当しないテストで、実装すべき機能を示すもの

---

## 🔴 未実装機能（HIR実装が必要）

### 1. **Pattern Matching & Match式** 🔥 優先度: 高

#### 影響範囲
- `pattern_matching/*` (21テスト中15失敗)
- `error_handling/*` (全失敗)
- `error_propagation/*` (ほぼ全失敗)
- `builtin_types/option_basic.cb`
- `builtin_types/result_basic.cb`

#### 必要な実装

```cb
// 1. match式の基本構文
match (value) {
    Pattern1 => expression1,
    Pattern2 => expression2,
}

// 2. パターンバインディング
match (opt_value) {
    Some(x) => { /* xを使用 */ }
    None => { /* ... */ }
}

match (result) {
    Ok(value) => { /* valueを使用 */ }
    Err(error) => { /* errorを使用 */ }
}

// 3. ワイルドカードパターン
match (value) {
    1 => "one",
    2 => "two",
    _ => "other"
}
```

#### HIR実装要件
- [ ] `HIR::Match` ノードの作成
- [ ] パターンマッチングの型チェック
- [ ] バインディング変数のスコープ管理
- [ ] 網羅性チェック（exhaustiveness checking）
- [ ] ワイルドカードパターン `_` のサポート
- [ ] ネストされたmatchのサポート

---

### 2. **Option<T> / Result<T, E> Builtin Types** 🔥 優先度: 高

#### 影響範囲
- `builtin_types/*` (10テスト中6失敗)
- `error_handling/*`
- `error_propagation/*`
- `pattern_matching/*`

#### 必要な実装

```cb
// Option<T> - インタプリタ起動時に自動登録
Option<int> some_val = Option<int>::Some(42);
Option<int> none_val = Option<int>::None;

// Result<T, E> - インタプリタ起動時に自動登録
Result<int, string> ok_val = Result<int, string>::Ok(42);
Result<int, string> err_val = Result<int, string>::Err("error");

// RuntimeError builtin型
Result<int, RuntimeError> result = try_operation();
```

#### HIR実装要件
- [ ] `Option<T>` enum定義の組み込み登録
- [ ] `Result<T, E>` enum定義の組み込み登録
- [ ] `RuntimeError` 型の定義
- [ ] ジェネリック enum のインスタンス化
- [ ] `::Some()`, `::None`, `::Ok()`, `::Err()` コンストラクタ
- [ ] これらの型がユーザー定義で再定義されないようなチェック

---

### 3. **Error Handling Keywords** 🔥 優先度: 高

#### 影響範囲
- `error_handling/basic.cb`
- `error_propagation/*`

#### 必要な実装

```cb
// 1. try キーワード - ポインタデリファレンスを安全に
Result<int, RuntimeError> safe_deref(int* ptr) {
    return try *ptr;  // nullptrならErr、さもなくばOk
}

// 2. checked キーワード - 配列アクセスを安全に
Result<int, RuntimeError> checked_access(int[] arr, int idx) {
    return checked arr[idx];  // 境界外ならErr
}

// 3. ? オペレータ - エラー伝播
Result<int, string> operation()? {
    int value = risky_operation()?;  // Errなら早期リターン
    return Ok(value);
}
```

#### HIR実装要件
- [ ] `try` キーワードのパース
- [ ] `try` 式の型推論（常に `Result<T, RuntimeError>` を返す）
- [ ] nullptrチェックとResult変換
- [ ] `checked` キーワードのパース
- [ ] 配列境界チェックとResult変換
- [ ] `?` オペレータのパース
- [ ] `?` による早期リターン（return伝播）の実装
- [ ] エラー型の互換性チェック

---

### 4. **配列リテラル構文** 🔥 優先度: 高

#### 影響範囲
- `func/simple_array_return.cb`
- `enum/array_index.cb`
- `floating_point/functions_and_arrays.cb`
- その他多数

#### 必要な実装

```cb
// 1次元配列リテラル
int[] arr = [1, 2, 3, 4, 5];
int[5] fixed_arr = [10, 20, 30, 40, 50];

// 多次元配列リテラル
int[2][3] matrix = [
    [1, 2, 3],
    [4, 5, 6]
];

// 動的サイズ配列
int[] dynamic = [1, 2, 3];  // サイズ推論
```

#### HIR実装要件
- [ ] 配列リテラル `[expr1, expr2, ...]` のパース
- [ ] 1次元配列リテラルの型推論
- [ ] 多次元配列リテラルのパース
- [ ] ネストされた配列リテラルの型チェック
- [ ] 動的サイズ配列 `T[]` のサポート（現状は固定サイズのみ）
- [ ] 配列リテラルからのサイズ推論
- [ ] 配列リテラルの初期化コード生成

---

### 5. **Default Member (暗黙的代入)** 🔥 優先度: 中

#### 影響範囲
- `default_member/*` (7テスト中6失敗)

#### 必要な実装

```cb
struct IntWrapper {
    default int value;  // デフォルトメンバー
};

void main() {
    IntWrapper w = {42};
    println(w);        // 42 (デフォルトメンバーの値を出力)
    w = 100;          // valueに暗黙的代入
    println(w);        // 100
    println(w.value);  // 100 (明示的アクセスも可能)
}
```

#### HIR実装要件
- [ ] `default` キーワードのパース（構造体メンバーに対して）
- [ ] 構造体に対する暗黙的代入演算子のオーバーロード
- [ ] デフォルトメンバーの型チェック（1つのみ許可）
- [ ] `println(struct)` でデフォルトメンバーを出力
- [ ] 全型（int, float, double, bool等）のサポート

---

### 6. **Impl Static Variables** 🔥 優先度: 中

#### 影響範囲
- `impl_static/*` (6テスト中3失敗)

#### 必要な実装

```cb
interface Counter {
    int increment();
};

struct Point { int x; };

impl Counter for Point {
    static int shared_counter = 0;  // impl内static変数

    int increment() {
        shared_counter = shared_counter + 1;
        return shared_counter;
    }
}
```

#### HIR実装要件
- [ ] impl内での `static` 変数宣言のパース
- [ ] impl static変数の初期化
- [ ] impl static変数のスコープ管理（impl全体で共有）
- [ ] static変数へのアクセス生成
- [ ] static変数の寿命管理（プログラム全体）

---

### 7. **Default Arguments with Const** 🔥 優先度: 低

#### 影響範囲
- `default_args/test_default_args_const.cb`

#### 必要な実装

```cb
const int DEFAULT_WIDTH = 800;
const string DEFAULT_TITLE = "Window";

void create_window(int w = DEFAULT_WIDTH, string title = DEFAULT_TITLE) {
    println("Window: " + title);
}
```

#### HIR実装要件
- [ ] デフォルト引数でのconst変数参照
- [ ] const変数の評価タイミング（コンパイル時 vs 実行時）
- [ ] 文字列連結 `+` オペレータ（これも未実装の可能性）

---

### 8. **Enum拡張機能** 🔥 優先度: 低

#### 影響範囲
- `enum/array_index.cb`
- `enum/large_values.cb`
- `enum/negative_values.cb`

#### 必要な実装

```cb
// 配列インデックスとしてenumを使用
enum Job { a = 0, b, c }
int[5] arr = [1, 2, 3, 4, 5];
int val = arr[Job::c];  // arr[2]

// 負の値
enum Signed { neg = -5, zero = 0, pos = 5 }

// 大きな値
enum Large { big = 1000000 }
```

#### HIR実装要件
- [ ] enum値を配列インデックスとして使用時の型変換
- [ ] enum値の負の数サポート
- [ ] enum値の大きな数サポート（long範囲）

---

### 9. **Const Safety拡張** 🔥 優先度: 低

#### 影響範囲
- `const_array/*` (4テスト中3失敗 - すべてerrorパターン)
- `const_parameters/*` (6テスト中3失敗 - すべてerrorパターン)
- `const_variables/*` (4テスト中3失敗 - すべてerrorパターン)
- `const_pointer_safety/*` (7テスト中6失敗)

#### 必要な実装

```cb
// const配列要素への代入エラー検出
const int[3] arr = [1, 2, 3];
arr[0] = 10;  // ERROR

// constパラメータへの代入エラー検出
void func(const int x) {
    x = 10;  // ERROR
}

// const文字列要素への代入エラー検出
const string s = "hello";
s[0] = 'H';  // ERROR
```

#### HIR実装要件
- [ ] const配列要素への代入検出
- [ ] constパラメータへの再代入検出
- [ ] const文字列要素への代入検出
- [ ] const複合代入（`+=`, etc）の検出
- [ ] constポインタの高度な安全性チェック

**注**: これらはほとんどが `*_error.cb` パターンなので、エラー検出機能の実装

---

### 10. **Constructor/Move Semantics** 🔥 優先度: 低

#### 影響範囲
- `constructor/move_basic_test.cb`
- `constructor/lvalue_ref_test.cb`
- `constructor/copy_vs_move_test.cb`

#### 必要な実装

```cb
// Move constructor
struct Resource {
    int* data;

    // Move constructor
    Resource(Resource&& other) {
        self.data = other.data;
        other.data = nullptr;
    }
}

// lvalue reference
void process(Resource& res) { /* ... */ }
```

#### HIR実装要件
- [ ] rvalue reference `T&&` の完全サポート
- [ ] move constructorの自動生成
- [ ] copy vs move の選択ロジック
- [ ] プリミティブ型のmoveエラー検出

---

### 11. **Interface拡張機能** 🔥 優先度: 中

#### 影響範囲
- `interface/*` (40テスト中26失敗)
- `interface_bounds/*` (20テスト中5失敗)

#### 必要な実装

多くの高度な機能が不足：
- 配列型のインターフェース実装
- enum型のインターフェース実装
- union型のインターフェース実装
- ポインタ型のself
- arrow演算子によるself member access
- 多次元配列のインターフェース
- privateフィールドとインターフェース
- 複雑な型引数

#### HIR実装要件
詳細な調査が必要（個別テストを確認）

---

### 12. **Generics拡張機能** 🔥 優先度: 中

#### 影響範囲
- `generics/*` (50テスト中20失敗)

#### 必要な実装

```cb
// 関数ジェネリクス
T max<T>(T a, T b) {
    return a > b ? a : b;
}

// Enumジェネリクス（既に部分サポート）
enum MyOption<T> {
    Some(T),
    None
}

// より複雑なネスト
Option<Result<int, string>> nested;
```

#### HIR実装要件
- [ ] 関数ジェネリクスのパース
- [ ] 関数ジェネリクスの型推論
- [ ] ジェネリック関数のインスタンス化
- [ ] enumジェネリクスの完全サポート
- [ ] より複雑なネストされたジェネリクス
- [ ] ジェネリック関数と配列の組み合わせ

---

### 13. **Async/Await機能** 🔥 優先度: 低（将来機能）

#### 影響範囲
- `async/*` (85テスト中71失敗)

#### 必要な実装

```cb
async int fetch_data() {
    yield;
    return 42;
}

void main() {
    Future<int> future = fetch_data();
    int result = await future;
}
```

#### HIR実装要件
**注**: これは大規模な機能セットで、現在は実装途中
- [ ] `async` 関数のパース
- [ ] `await` 式のパース
- [ ] `yield` 文のパース
- [ ] `Future<T>` 型のサポート
- [ ] タスクキューの実装
- [ ] イベントループの実装
- [ ] async/awaitのコード変換

---

### 14. **Module System (Import/Export)** 🔥 優先度: 中

#### 影響範囲
- `import_export/*` (19テスト中16失敗)

#### 必要な実装

```cb
// module_a.cb
export struct Point { int x; int y; }
export int add(int a, int b) { return a + b; }

// main.cb
import { Point, add } from "module_a";

void main() {
    Point p = {1, 2};
    int result = add(3, 4);
}
```

#### HIR実装要件
- [ ] `export` キーワードのパース
- [ ] `import` 文のパース
- [ ] モジュール解決
- [ ] シンボルのエクスポート/インポート
- [ ] 名前空間管理
- [ ] 循環依存の検出

---

### 15. **FFI (Foreign Function Interface)** 🔥 優先度: 低

#### 影響範囲
- `ffi/*` (11テスト中7失敗)

#### 必要な実装

```cb
// C言語の関数を呼び出し
ffi {
    double sin(double x);
    double cos(double x);
}

void main() {
    double result = sin(3.14);
}
```

#### HIR実装要件
- [ ] `ffi` ブロックのパース
- [ ] 外部関数宣言の処理
- [ ] 外部関数呼び出しのコード生成
- [ ] 型変換（Cb型 ↔ C型）
- [ ] ライブラリリンク

---

### 16. **Union型拡張** 🔥 優先度: 低

#### 影響範囲
- `union/*` (17テスト中7失敗)
- `float_double_unsigned/union_types.cb`

#### 必要な実装

```cb
// カスタム型のunion
union Value {
    MyStruct s;
    MyEnum e;
}

// float/doubleのunion
union Number {
    float f;
    double d;
}
```

#### HIR実装要件
- [ ] カスタム型（struct/enum）のunionサポート
- [ ] float/doubleのunionサポート
- [ ] union型のエラー検出（型不一致等）

---

### 17. **Typedef拡張** 🔥 優先度: 低

#### 影響範囲
- `typedef/*` (19テスト中6失敗)

#### 必要な実装

```cb
// Enumのtypedef
typedef MyEnum AliasEnum;

// 構造体のtypedef（より複雑なケース）
typedef Result<int, string> IntResult;

// 配列のtypedef
typedef int[10] IntArray;
```

#### HIR実装要件
- [ ] enumのtypedefサポート
- [ ] 複雑なジェネリック型のtypedef
- [ ] typedefの重複定義エラー検出（一部実装済み）
- [ ] 配列のtypedefサポート

---

### 18. **Pointer関連のSegfault修正** 🔥 優先度: 高（バグ修正）

#### 影響範囲
- `pointer/test_recursive_struct.cb` - Segmentation fault
- `pointer/test_typedef_recursive.cb` - Segmentation fault
- `func/integration_func.cb` - Segmentation fault
- `struct/test_nested_member_assignment.cb` - Segmentation fault
- その他多数

#### 必要な対応

これらはコンパイラのバグの可能性が高い。

#### HIR実装要件
- [ ] 再帰的構造体のポインタ処理を調査
- [ ] Segfaultの原因を特定（おそらく型解決やコード生成バグ）
- [ ] テストケースでデバッグ
- [ ] 修正とリグレッションテスト

---

### 19. **String Interpolation拡張** 🔥 優先度: 低

#### 影響範囲
- `string_interpolation/*` (18テスト中5失敗)

#### 必要な実装

```cb
// エスケープ処理
string s = "Value: \{not_interpolated}";  // \{でエスケープ

// より高度な式評価
string s = "{complex_expression()}";
```

#### HIR実装要件
- [ ] `\{` エスケープシーケンスのサポート
- [ ] より複雑な式の補間サポート

---

### 20. **Float/Double/Unsigned拡張** 🔥 優先度: 低

#### 影響範囲
- `floating_point/functions_and_arrays.cb`
- `unsigned/boundary_overflow_long.cb`
- `unsigned/struct_interface.cb`

#### 必要な実装

多次元配列リテラルと関数戻り値（配列リテラル機能に依存）

#### HIR実装要件
配列リテラル機能の実装後に自動的に解決される可能性が高い

---

### 21. **Stdlib (Standard Library)** 🔥 優先度: 中

#### 影響範囲
- `stdlib/collections/*` (多数失敗)
- `stdlib/std/*` (多数失敗)

#### 必要な実装

```cb
// Vector
import { Vector } from "std/collections";
Vector<int> vec;
vec.push(42);

// Map
import { Map } from "std/collections";
Map<string, int> map;

// String
import { String } from "std/string";
String s = String::from("hello");

// Time
import { Time } from "std/time";
```

#### HIR実装要件
これらはほとんどがモジュールシステム（import/export）に依存
モジュールシステム実装後に対応可能

---

### 22. **その他の細かい機能**

#### Discard Variable
- `discard_variable/error/reassign_discard.cb`
- `discard_variable/error/use_in_array.cb`

#### Static Variables
- `static_variables/basic_static.cb`
- `static_variables/static_integration.cb`

#### Global Variables
- `global_vars/redeclare.cb` - 重複宣言エラー検出

#### Preprocessor
- `preprocessor/syntax_highlight_test.cb`

---

## 📋 実装優先順位マトリクス

| 優先度 | 機能 | 影響テスト数 | 実装難易度 | 推奨順序 |
|-------|------|-------------|-----------|---------|
| 🔴 **最優先** | 配列リテラル | 50+ | 中 | 1 |
| 🔴 **最優先** | Pattern Matching | 40+ | 高 | 2 |
| 🔴 **最優先** | Option<T>/Result<T,E> | 40+ | 高 | 3 |
| 🔴 **最優先** | Error Handling (try/checked/?) | 20+ | 高 | 4 |
| 🔴 **最優先** | Segfault修正 | 10+ | ? | 5 |
| 🟡 **高** | Generics拡張 | 20 | 高 | 6 |
| 🟡 **高** | Interface拡張 | 26 | 中-高 | 7 |
| 🟡 **高** | Module System | 16 | 高 | 8 |
| 🟡 **高** | Default Member | 6 | 中 | 9 |
| 🟢 **中** | Impl Static | 3 | 低-中 | 10 |
| 🟢 **中** | Stdlib | 20+ | 中 | 11 |
| ⚪ **低** | Async/Await | 71 | 極高 | 12 |
| ⚪ **低** | Constructor/Move | 5 | 中 | 13 |
| ⚪ **低** | FFI | 7 | 中 | 14 |
| ⚪ **低** | その他拡張 | 10 | 低-中 | 15 |

---

## 🎯 推奨実装ロードマップ

### Phase 1: 基礎機能（v0.14.0 → v0.15.0）
1. **配列リテラル** - 多くの機能の基盤
2. **Segfault修正** - 安定性向上
3. **Pattern Matching基礎** - match式の基本構文
4. **Option<T>/Result<T,E>** - Builtin型として登録

### Phase 2: エラーハンドリング（v0.15.0 → v0.16.0）
5. **try/checked/? キーワード** - エラー処理の完成
6. **Default Member** - 構造体の利便性向上

### Phase 3: 型システム拡張（v0.16.0 → v0.17.0）
7. **Generics拡張** - 関数ジェネリクス等
8. **Interface拡張** - より複雑なインターフェース機能
9. **Impl Static** - impl内static変数

### Phase 4: モジュールシステム（v0.17.0 → v0.18.0）
10. **Import/Export** - モジュールシステムの実装
11. **Stdlib** - 標準ライブラリの整備

### Phase 5: 高度な機能（v0.18.0+）
12. **Constructor/Move Semantics** - 完全なムーブセマンティクス
13. **FFI** - C言語連携
14. **Async/Await** - 非同期処理（長期プロジェクト）

---

## 📝 各テストカテゴリの詳細分析

### Const関連
- **総テスト数**: 28
- **成功**: 11
- **失敗**: 17
  - **意図的な失敗**: 16 (error/ngパターン)
  - **要修正**: 1 (`const_pointer_safety`の一部)

**結論**: ほぼ問題なし。エラー検出機能として正常動作。

---

### Builtin Types
- **総テスト数**: 10
- **成功**: 2 (`option_simple.cb`, `result_simple.cb`)
- **失敗**: 8
  - **意図的な失敗**: 2 (`error_redefine_*`)
  - **要修正**: 6 (Option/Result/Matchの未実装)

**結論**: Pattern MatchingとBuiltin型の実装が必要。

---

### Constructor
- **総テスト数**: 12
- **成功**: 7
- **失敗**: 5
  - **意図的な失敗**: 1 (`primitive_move_error_test`)
  - **要修正**: 4 (Move semantics関連)

**結論**: Move semanticsの実装が必要（優先度低）。

---

### Default Args
- **総テスト数**: 7
- **成功**: 4
- **失敗**: 3
  - **意図的な失敗**: 2 (`error1`, `error2`)
  - **要修正**: 1 (`test_default_args_const`)

**結論**: Const変数をデフォルト引数に使用する機能が必要。

---

### Default Member
- **総テスト数**: 7
- **成功**: 1
- **失敗**: 6
  - **意図的な失敗**: 0
  - **要修正**: 6

**結論**: Default member機能の実装が必要。

---

### Enum
- **総テスト数**: 18
- **成功**: 8
- **失敗**: 10
  - **意図的な失敗**: 3 (`error_*`)
  - **要修正**: 7 (配列インデックス、負の値、大きな値等)

**結論**: Enumの拡張機能が必要。

---

### Error Handling / Pattern Matching
- **総テスト数**: 約45
- **成功**: 約5
- **失敗**: 約40
  - **意図的な失敗**: 0
  - **要修正**: 40

**結論**: Match、Option、Result、try/checkedの実装が最優先。

---

### Generics
- **総テスト数**: 50
- **成功**: 30
- **失敗**: 20
  - **意図的な失敗**: 0
  - **要修正**: 20 (関数ジェネリクス等)

**結論**: 基本機能は動作。高度な機能の実装が必要。

---

### Interface
- **総テスト数**: 60
- **成功**: 34
- **失敗**: 26
  - **意図的な失敗**: 約8 (`error_*`)
  - **要修正**: 約18

**結論**: 基本機能は動作。配列/enum/unionのインターフェース実装等が必要。

---

### Pointer
- **総テスト数**: 60
- **成功**: 52
- **失敗**: 8
  - **意図的な失敗**: 0
  - **Segfault**: 2
  - **要修正**: 6

**結論**: ほぼ動作。Segfaultの修正が必要。

---

### Async
- **総テスト数**: 85
- **成功**: 14
- **失敗**: 71
  - **Segfault**: 多数
  - **要修正**: 大部分

**結論**: 現在実装途中の大規模機能。将来対応。

---

### Module System (Import/Export)
- **総テスト数**: 19
- **成功**: 3
- **失敗**: 16
  - **要修正**: 16

**結論**: モジュールシステムの実装が必要。

---

### Stdlib
- **総テスト数**: 約30
- **成功**: 5
- **失敗**: 25
  - **要修正**: 25 (ほとんどimport依存)

**結論**: モジュールシステム実装後に対応可能。

---

## 🔧 HIR実装ガイドライン

### 1. 配列リテラルの実装例

```rust
// HIR Node
pub enum HIR {
    // ...
    ArrayLiteral {
        elements: Vec<Box<HIR>>,
        element_type: Type,
        dimensions: Vec<usize>,
    },
}

// Type inference
fn infer_array_literal(elements: &[HIR]) -> Type {
    // 1. 全要素の型をチェック
    // 2. 統一された型を推論
    // 3. 多次元の場合は再帰的に処理
}

// Code generation
fn generate_array_literal(elements: &[HIR]) -> String {
    // Stack allocation or heap allocation
    // Initialize elements
}
```

### 2. Pattern Matchingの実装例

```rust
// HIR Node
pub enum HIR {
    Match {
        scrutinee: Box<HIR>,
        arms: Vec<MatchArm>,
    },
}

pub struct MatchArm {
    pattern: Pattern,
    guard: Option<Box<HIR>>,
    body: Box<HIR>,
}

pub enum Pattern {
    Wildcard,
    Binding(String),
    Constructor {
        name: String,
        fields: Vec<Pattern>,
    },
    Literal(Literal),
}

// Exhaustiveness checking
fn check_exhaustiveness(patterns: &[Pattern], scrutinee_type: &Type) -> Result<()> {
    // アルゴリズム実装
}
```

### 3. Option<T>/Result<T,E>の実装例

```rust
// Builtin type registration
fn register_builtin_types(env: &mut Environment) {
    // Option<T>
    let option_def = EnumDef {
        name: "Option".to_string(),
        type_params: vec!["T".to_string()],
        variants: vec![
            Variant { name: "Some".to_string(), fields: vec![Type::Generic("T")] },
            Variant { name: "None".to_string(), fields: vec![] },
        ],
    };
    env.register_builtin_enum(option_def);

    // Result<T, E>
    let result_def = EnumDef {
        name: "Result".to_string(),
        type_params: vec!["T".to_string(), "E".to_string()],
        variants: vec![
            Variant { name: "Ok".to_string(), fields: vec![Type::Generic("T")] },
            Variant { name: "Err".to_string(), fields: vec![Type::Generic("E")] },
        ],
    };
    env.register_builtin_enum(result_def);

    // RuntimeError
    let runtime_error_def = /* ... */;
    env.register_builtin_type(runtime_error_def);
}
```

---

## 📚 参考リンク

- Cbドキュメント: `docs/`
- テストケース: `tests/cases/`
- HIR実装: `src/hir/`
- 型システム: `src/types/`

---

## ✅ チェックリスト

各機能実装時のチェック項目：

- [ ] HIRノードの定義
- [ ] パーサーの実装
- [ ] 型チェックの実装
- [ ] コード生成の実装
- [ ] エラーメッセージの実装
- [ ] テストケースの追加
- [ ] ドキュメントの更新
- [ ] リグレッションテストの実行

---

**このドキュメントは継続的に更新されます**
