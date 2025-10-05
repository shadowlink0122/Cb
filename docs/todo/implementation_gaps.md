# 実装状況と考慮不足機能のまとめ

**最終更新**: 2025年10月6日  
**バージョン**: v0.9.0+（関数ポインタ追加）

---

## 🎯 概要

v0.9.0時点での実装済み機能と、考慮不足・未実装機能を体系的にまとめたドキュメントです。

---

## ✅ 実装済み機能の確認

### 1. ポインタシステム ✅

#### 基本機能
- ✅ ポインタ宣言と初期化: `int* ptr = &value;`
- ✅ デリファレンス: `*ptr` による読み書き
- ✅ ポインタ演算: `ptr++`, `ptr--`, `ptr + n`, `ptr - n`
- ✅ 16進数アドレス表示: `0x[hex]` 形式
- ✅ ポインタ再代入: `ptr = &other;`

#### 構造体ポインタ
- ✅ 構造体ポインタ: `struct Point* ptr = &p;`
- ✅ デリファレンス構文: `(*ptr).member`
- ✅ アロー演算子: `ptr->member`
- ✅ ネストした構造体ポインタ: `ptr->nested.value`, `(*ptr).nested->value`

#### Interface/Implポインタ
- ✅ Interfaceポインタ宣言: `Shape* ptr = &rect;`
- ✅ ポリモーフィックメソッド呼び出し: `(*ptr).area()`, `ptr->area()`
- ✅ implブロック内での`self`使用: `self.member`
- ✅ implメソッド内でのポインタ操作

#### ポインタ配列
- ✅ ポインタの配列: `int* ptrs[10];`
- ✅ 各要素への個別代入: `ptrs[0] = &arr[0];`
- ✅ ポインタ配列要素の間接参照: `*ptrs[i]`

### 2. ネストした構造体 ✅

- ✅ 多階層ネスト: 3階層以上のネスト構造
- ✅ ネストした構造体リテラル初期化: `{center: {x: 10, y: 20}, radius: 5}`
- ✅ チェーンアクセス: `obj.nested.member`
- ✅ ポインタ経由のネストアクセス: `ptr->nested.member`

### 3. enum型 ✅

- ✅ enum定義: `enum Color { RED = 0, GREEN, BLUE };`
- ✅ スコープアクセス: `Color::RED`
- ✅ typedef enum: `typedef enum Color { ... } Color;`
- ✅ enum値の比較

### 4. 浮動小数点数 ✅

- ✅ float型: 32bit単精度
- ✅ double型: 64bit倍精度
- ✅ 四則演算・比較演算
- ✅ float/double配列
- ✅ 構造体メンバーとしてのfloat/double

### 5. 多次元配列 ✅

- ✅ 2次元配列: `int[3][4] matrix;`
- ✅ 3次元配列: `int[2][3][4] cube;`
- ✅ 多次元配列リテラル初期化: `[[1,2],[3,4]]`
- ✅ 多次元配列の関数戻り値
- ✅ typedef多次元配列: `typedef Matrix2D = int[2][2];`

### 6. private修飾子 ✅

- ✅ 構造体メンバーへのprivate指定: `private int balance;`
- ✅ implメソッドへのprivate指定: `private int helper() { ... }`
- ✅ privateメンバーのアクセス制御

### 7. static変数 ✅

- ✅ 関数内static変数: `static int counter = 0;`
- ✅ static変数のスコープ管理（関数ごとに独立）
- ✅ static const組み合わせ

---

## ❌ 未実装・考慮不足機能

### 1. 🔴 **重要** impl内でのstatic変数

**状態**: ❌ 未実装

#### 問題点
現在、implブロック内でstatic変数を宣言することができません。

#### 期待される仕様
```c++
struct Counter {
    int value;
};

impl Helper for Counter {
    static int shared_counter = 0;  // ❌ 未サポート
    
    void increment() {
        self.value++;
        shared_counter++;  // すべてのCounterインスタンスで共有
    }
    
    int get_total() {
        return shared_counter;
    }
};
```

#### 実装要件
- `impl Interface for StructA` と `impl Interface for StructB` では、それぞれ**異なる**static変数を持つ
- `StructA`の複数インスタンス間では、**同じ**static変数を共有する
- static変数の名前空間: `impl::InterfaceName::StructTypeName::variable_name`

#### 実装の難しさ
- 現在のstatic変数は関数スコープのみ (`function_name::variable_name`)
- implスコープの概念を追加する必要がある
- impl定義ごとにstatic変数領域を確保する必要がある

---

### 2. 🟡 **中** 多次元配列へのポインタ

**状態**: ❌ 未実装

#### 問題点
多次元配列の要素や行へのポインタを取得できません。

```c++
int[3][4] matrix = [[1,2,3,4], [5,6,7,8], [9,10,11,12]];

// ❌ 未サポート: 行へのポインタ
int[4]* row_ptr = &matrix[0];  // コンパイルエラー

// ❌ 未サポート: 特定要素へのポインタ（多次元の場合）
int* elem_ptr = &matrix[1][2];  // 構文解析エラー

// ✅ 動作: 1次元配列の要素へのポインタ
int[12] flat = [1,2,3,4,5,6,7,8,9,10,11,12];
int* ptr = &flat[5];  // OK
```

#### 実装要件
- 行ポインタの型: `int[4]*` (4要素の配列へのポインタ)
- 要素ポインタの型: `int*` (通常のintポインタ)
- ポインタ演算: `row_ptr++` で次の行へ移動
- デリファレンス: `(*row_ptr)[2]` で行内の要素アクセス

---

### 3. 🟡 **中** 構造体配列メンバーの関数戻り値代入

**状態**: ⚠️ 部分的サポート

#### 問題点
構造体メンバーが配列の場合、関数戻り値を直接代入できません。

```c++
struct Container {
    int[5] data;
};

int[5] create_array() {
    return [1, 2, 3, 4, 5];
}

int main() {
    Container c;
    
    // ❌ エラー: 関数戻り値の配列メンバー代入
    c.data = create_array();
    
    // ✅ 回避策: ループで代入
    int[5] temp = create_array();
    for (int i = 0; i < 5; i++) {
        c.data[i] = temp[i];
    }
    
    // ✅ 動作: 初期化時の指定
    Container c2 = { data: [1, 2, 3, 4, 5] };
    
    return 0;
}
```

#### 実装要件
- 構造体メンバー配列への関数戻り値の直接代入
- メモリコピーの最適化
- 型チェックの強化（サイズ一致確認）

---

### 4. 🟡 **中** 多次元配列の関数戻り値からメンバー代入

**状態**: ❌ 未実装

#### 問題点
多次元配列を返す関数の結果を構造体メンバーに代入できません。

```c++
struct Matrix {
    int[2][2] data;
};

int[2][2] create_matrix() {
    return [[1, 2], [3, 4]];
}

int main() {
    Matrix m;
    
    // ❌ エラー: Multi-dimensional function return member array assignment not supported
    m.data = create_matrix();
    
    // ✅ 回避策: 個別代入
    m.data[0][0] = 1; m.data[0][1] = 2;
    m.data[1][0] = 3; m.data[1][1] = 4;
    
    return 0;
}
```

---

### 5. 🟡 **中** ポインタの配列（配列要素としてのポインタ型）

**状態**: ✅ 基本実装済み（高度な操作は未検証）

#### 確認が必要な機能
```c++
// ✅ 基本: ポインタの配列宣言
int* ptrs[10];

// ✅ 基本: 個別要素への代入
int a = 10, b = 20;
ptrs[0] = &a;
ptrs[1] = &b;

// ⚠️ 未検証: ポインタ配列のループ操作
for (int i = 0; i < 10; i++) {
    ptrs[i] = &arr[i];
}

// ⚠️ 未検証: ポインタ配列の関数引数
void process(int** ptr_array, int size) {
    // ...
}

// ⚠️ 未検証: 構造体メンバーとしてのポインタ配列
struct PointerContainer {
    int* ptrs[10];  // これは動作するか？
};
```

---

### 6. � **スコープ外** 配列要素へのポインタ構文糖衣 `ptr[n]`

**状態**: ⛔ スコープ外（今回は実装しない）

#### 理由
C言語の `ptr[n]` 構文（`*(ptr + n)` の糖衣構文）は便利ですが、以下の理由により今回のスコープから除外します：

1. 既存の明示的なポインタ演算で十分に機能する
2. パーサーの複雑度が増す
3. 他の優先度の高い機能を先に実装すべき

```c++
int[5] arr = [10, 20, 30, 40, 50];
int* ptr = &arr[0];

// ⛔ スコープ外: 配列風アクセス
// int value = ptr[2];  // 実装しない

// ✅ 推奨: 明示的なポインタ演算
int* ptr2 = ptr + 2;
int value = *ptr2;
```

---

### 7. 🟢 **低** const ポインタ

**状態**: ❌ 未実装

#### 問題点
ポインタ自体やポイント先の不変性を保証できません。

```c++
int value = 42;
int other = 100;

// ❌ 未サポート: ポイント先が定数
const int* ptr1 = &value;
// *ptr1 = 10;  // エラー: 値を変更できない
ptr1 = &other;  // OK: ポインタ自体は変更可能

// ❌ 未サポート: ポインタ自体が定数
int* const ptr2 = &value;
*ptr2 = 10;     // OK: 値は変更可能
// ptr2 = &other;  // エラー: ポインタを変更できない

// ❌ 未サポート: 両方が定数
const int* const ptr3 = &value;
// *ptr3 = 10;     // エラー
// ptr3 = &other;  // エラー
```

---

### 8. 🟢 **低** 多重ポインタ（ポインタのポインタ）

**状態**: ❌ 未実装

```c++
int value = 42;
int* ptr = &value;
int** pptr = &ptr;  // ❌ 未サポート

// ❌ 2レベル間接参照
**pptr = 100;
```

---

### 9. 🟢 **低** 参照型 `int&`

**状態**: ✅ **実装済み**

```c++
int value = 10;
int& ref = value;  // ✅ サポート済み

ref = 20;  // valueが20になる

void increment(int& val) {  // ✅ 参照引数サポート
    val++;
}

// 参照戻り値もサポート
int& get_global() {
    return global_var;
}
```

**テスト**: `tests/cases/reference/` で包括的にテスト済み

---

### 10. 🟢 **低** 動的メモリ管理 (`new`/`delete`)

**状態**: ❌ 未実装

```c++
// ❌ 未サポート: 動的確保
Point* p = new Point;
p->x = 10;
delete p;

// ❌ 未サポート: 配列の動的確保
int* arr = new int[10];
delete[] arr;
```

---

### 11. 🟢 **低** スマートポインタ

**状態**: ❌ 未実装

```c++
// ❌ 未サポート: 自動メモリ管理
unique_ptr<Point> p = make_unique<Point>();
shared_ptr<Point> sp = make_shared<Point>();
```

---

### 12. 🟢 **低** `nullptr` リテラル

**状態**: ✅ **実装済み**

```c++
int* ptr = nullptr;  // ✅ サポート済み

// nullptrチェック
if (ptr == nullptr) {
    // ...
}

// 従来の方法も引き続きサポート
int* ptr2 = 0;  // ✅ ゼロ初期化も可能
```

**機能**:
- `nullptr`キーワード認識
- ポインタ初期化・代入
- `nullptr`との比較演算
- `nullptr == 0` (true)

**テスト**: `tests/cases/pointer/test_nullptr.cb` で検証済み

---

### 13. 🟢 **低** 関数ポインタ

**状態**: ✅ **実装済み**

```c++
int add(int a, int b) { return a + b; }

// ✅ 関数ポインタ宣言と初期化
int* op = &add;

// ✅ 関数ポインタ呼び出し（2つの形式）
int result1 = op(5, 3);      // 形式1: 暗黙的呼び出し
int result2 = (*op)(5, 3);   // 形式2: 明示的デリファレンス

// ✅ 関数ポインタを返す関数
int* getOperation(int code) {
    if (code == 1) return &add;
    return &subtract;
}

// ✅ チェーン呼び出し
int result = getOperation(3)(6, 7);

// ✅ コールバック関数
void process(int a, int b, int* callback) {
    return callback(a, b);
}

// ✅ アドレス比較
int* fp1 = &add;
int* fp2 = &add;
if (fp1 == fp2) { }  // 同じ関数を指す

// ✅ アドレス表示
println(op);  // 0x12345678 (16進数)
```

**機能**:
- 関数ポインタの宣言、初期化、再代入
- 2つの呼び出し形式（暗黙的・明示的）
- 関数ポインタを返す関数
- チェーン呼び出し（戻り値の直接呼び出し）
- コールバック関数パターン
- ポインタのアドレス比較と表示

**テスト**: `tests/cases/function_pointer/` および `tests/integration/pointer/pointer_tests.hpp` で包括的にテスト済み

---

### 14. 🟢 **低** `void*` 汎用ポインタ

**状態**: ❌ 未実装

```c++
void* generic_ptr = &some_value;  // ❌ 未サポート
int* int_ptr = (int*)generic_ptr;
```

---

## 📊 優先度まとめ

### 🔴 高優先度（v0.10.0で検討）

1. **impl内でのstatic変数** - Interface/Implシステムの完成度向上
2. **多次元配列へのポインタ** - ポインタシステムの完全性向上

### 🟡 中優先度（v0.10.0で検討）

3. **構造体配列メンバーの関数戻り値代入** - 利便性向上
4. **多次元配列の関数戻り値からメンバー代入** - 利便性向上
5. **ポインタ配列の高度な操作** - 検証と強化
7. **const ポインタ** - 型安全性向上
8. **多重ポインタ** - 高度な機能

### 🟢 低優先度（v1.0.0以降で検討）

10. **動的メモリ管理** - メモリ管理の柔軟性
11. **スマートポインタ** - 安全なメモリ管理
14. **void* 汎用ポインタ** - 型安全性を犠牲にした柔軟性

### 🔵 スコープ外（今回は実装しない）

6. **配列要素へのポインタ構文糖衣** `ptr[n]` - 明示的なポインタ演算で代替可能

### ✅ 実装済み

9. **参照型** `int&` - 完全実装済み
12. **nullptr リテラル** - 完全実装済み
13. **関数ポインタ** - 完全実装済み（v0.9.0）

---

## 🔍 検証が必要な実装済み機能

以下の機能は基本的に実装済みですが、エッジケースや複雑な組み合わせの検証が不十分です：

1. **ポインタ配列のループ操作**
2. **ポインタ配列の関数引数・戻り値**
3. **構造体メンバーとしてのポインタ配列**
4. **impl内でのポインタメンバー変数**
5. **ネストした構造体の深い階層（5階層以上）**
6. **float/double配列へのポインタ**
7. **enum配列へのポインタ**

---

## 📝 次のアクションアイテム

### v0.10.0に向けて

1. **impl内static変数の設計**
   - static変数の名前空間設計: `impl::Interface::Struct::varname`
   - 実装方法の検討: `std::map<std::string, std::map<std::string, Variable>>`
   - テストケースの作成

2. **多次元配列ポインタの実装**
   - 行ポインタの型システム拡張
   - ポインタ演算の多次元対応
   - テストケースの作成

3. **既存機能の包括的テスト**
   - ポインタ配列の全操作パターン
   - エッジケースの洗い出し

---

**ドキュメント作成日**: 2025年10月5日  
**対象バージョン**: v0.9.0  
**次回更新予定**: v0.10.0リリース時
