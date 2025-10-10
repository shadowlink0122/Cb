# const * const 型安全性の完全実装計画

## 📋 現状の問題

### 🔴 重大な型安全性の欠陥

現在の実装では、`const * const`型の情報が**関数パラメータと戻り値で失われる**ため、Rustの`Pin<&T>`相当の不変性保証が破られてしまいます。

#### 問題1: パラメータでのconst情報損失

```cb
void modify_through_param(int* ptr) {
    *ptr = 100;  // 非constポインタなので変更可能
}

void main() {
    int x = 42;
    const int* const ptr = &x;  // const * const型（値・ポインタ両方不変）
    
    // ❌ 型安全性違反: const * constを非constポインタパラメータに渡せてしまう
    modify_through_param(ptr);  // エラーになるべきだが通ってしまう
    
    println(x);  // 100 - 不変のはずの値が変更された！
}
```

**実行結果**: `100` （本来エラーであるべき）

#### 問題2: 戻り値でのconst情報損失

```cb
int global_x = 42;

const int* const get_const_pointer() {
    const int* const ptr = &global_x;
    return ptr;  // const * constを返すはず
}

void main() {
    // ❌ 型安全性違反: const * const戻り値を非constポインタで受け取れてしまう
    int* ptr = get_const_pointer();  // エラーになるべきだが通ってしまう
    
    *ptr = 200;  // constのはずの値を変更できてしまう
    println(*ptr);  // 200
}
```

**実行結果**: `200` （本来エラーであるべき）

### 影響範囲

- ✅ **ローカル変数**: `const * const`は正しく動作（v0.9.2で実装済み）
- ❌ **関数パラメータ**: const情報が失われる
- ❌ **関数戻り値**: const情報が失われる
- ❌ **関数ポインタ**: 未実装

## 🎯 実装目標

### Rust Pin<&T>相当の不変性保証

Rustの`Pin<&T>`は以下を保証します:

```rust
let x = 42;
let pin: Pin<&i32> = Pin::new(&x);

// ✅ 読み取りは可能
println!("{}", *pin);

// ❌ 値の変更は不可能（コンパイルエラー）
// *pin = 100;  // error: cannot assign to data in a `&` reference

// ❌ ムーブも不可能（コンパイルエラー）
// let pin2 = pin;  // error: `Pin<&T>` does not implement `Copy`
```

**Cb言語での同等実装**:

```cb
const int* const ptr = &x;

// ✅ 読み取りは可能
println(*ptr);

// ❌ 値の変更は不可能（v0.9.2実装済み）
// *ptr = 100;  // Error: Cannot modify value through pointer to const

// ❌ ポインタ再代入は不可能（v0.9.2実装済み）
// ptr = &y;  // Error: Cannot reassign const pointer

// ❌ 非constポインタへの暗黙変換は不可能（未実装）
// int* ptr2 = ptr;  // エラーになるべき

// ❌ 非constパラメータへの渡し方は不可能（未実装）
// modify(ptr);  // エラーになるべき
```

## 🔧 実装計画

### Phase 1: 型情報の拡張

#### Variable構造体の拡張

```cpp
struct Variable {
    // 既存フィールド
    bool is_pointer = false;
    bool is_pointee_const = false;  // const T* (値がconst)
    bool is_pointer_const = false;  // T* const (ポインタがconst)
    int pointer_depth = 0;
    
    // 🆕 追加フィールド
    std::vector<bool> pointee_const_levels;   // 各ポインタレベルのconst情報
    std::vector<bool> pointer_const_levels;   // 各ポインタレベルの自身const情報
    
    // 例: const int* const* ptr の場合
    // pointer_depth = 2
    // pointee_const_levels = {true, false}  // 最内側のintがconst
    // pointer_const_levels = {true, false}  // 最内側のポインタがconst
};
```

#### ASTNodeの拡張

```cpp
struct ASTNode {
    // 既存フィールド
    bool is_const = false;
    bool is_pointer = false;
    int pointer_depth = 0;
    
    // 🆕 追加フィールド
    std::vector<bool> pointee_const_at_level;
    std::vector<bool> pointer_const_at_level;
};
```

### Phase 2: 関数パラメータでの型チェック

#### 実装箇所

`src/backend/interpreter/evaluator/functions/call_impl.cpp`

#### 型チェックロジック

```cpp
// パラメータ渡し時の型チェック関数
bool check_pointer_parameter_compatibility(
    const Variable* arg_var,        // 実引数の変数
    const ASTNode* param_node) {    // 仮引数のASTノード
    
    if (!arg_var->is_pointer || !param_node->is_pointer) {
        return true;  // ポインタ以外は既存のチェックで処理
    }
    
    // ポインタ深度が一致するか
    if (arg_var->pointer_depth != param_node->pointer_depth) {
        return false;
    }
    
    // 各レベルでconst修飾が適合するかチェック
    for (int level = 0; level < arg_var->pointer_depth; level++) {
        // ルール: const → non-const は禁止、non-const → const は許可
        
        // 値のconst性チェック (const T* → T* は禁止)
        bool arg_pointee_const = arg_var->pointee_const_levels[level];
        bool param_pointee_const = param_node->pointee_const_at_level[level];
        if (arg_pointee_const && !param_pointee_const) {
            // constな値を指すポインタを、非constな値を指すポインタに渡すのは禁止
            return false;
        }
        
        // ポインタ自身のconst性チェック (T* const → T* は禁止)
        bool arg_pointer_const = arg_var->pointer_const_levels[level];
        bool param_pointer_const = param_node->pointer_const_at_level[level];
        if (arg_pointer_const && !param_pointer_const) {
            // constポインタを非constポインタに渡すのは禁止
            return false;
        }
    }
    
    return true;
}
```

#### エラーメッセージ

```cpp
throw std::runtime_error(
    "Type mismatch: cannot pass 'const int* const' to parameter of type 'int*'\n"
    "  Argument type: " + format_pointer_type(arg_var) + "\n"
    "  Parameter type: " + format_pointer_type(param_node) + "\n"
    "  Note: const qualification cannot be discarded"
);
```

### Phase 3: 関数戻り値での型チェック

#### 実装箇所

`src/backend/interpreter/evaluator/functions/call_impl.cpp` (return文処理)
`src/backend/interpreter/executors/assignments/pointer.cpp` (代入時)

#### 型チェックロジック

```cpp
// 戻り値の型チェック関数
bool check_return_type_compatibility(
    const Variable* return_var,     // return文の変数
    const ASTNode* func_node) {     // 関数定義のASTノード
    
    if (!return_var->is_pointer || !func_node->is_pointer) {
        return true;  // ポインタ以外は既存のチェックで処理
    }
    
    // 同様の型チェックロジック（パラメータと同じ）
    // ...
}

// 代入時の型チェック関数
bool check_assignment_compatibility(
    const Variable* source_var,     // 代入元
    const Variable* target_var) {   // 代入先
    
    if (!source_var->is_pointer || !target_var->is_pointer) {
        return true;
    }
    
    // const修飾の互換性チェック
    // const → non-const は禁止
    // ...
}
```

### Phase 4: テストケースの作成

#### 正常系テスト

```cb
// test_const_pointer_parameter_ok.cb

// パラメータに渡す型の互換性テスト
void read_const_pointer(const int* ptr) {
    println(*ptr);
}

void read_const_const_pointer(const int* const ptr) {
    println(*ptr);
}

void main() {
    int x = 42;
    const int* ptr1 = &x;
    const int* const ptr2 = &x;
    
    // ✅ OK: const int* → const int*
    read_const_pointer(ptr1);
    
    // ✅ OK: const int* const → const int*
    read_const_pointer(ptr2);
    
    // ✅ OK: const int* const → const int* const
    read_const_const_pointer(ptr2);
    
    // ✅ OK: int* → const int* (non-const → const は安全)
    int* ptr3 = &x;
    read_const_pointer(ptr3);
    
    println("All parameter passing tests passed!");
}
```

#### エラー系テスト

```cb
// error_const_pointer_parameter_mismatch.cb

void modify_through_pointer(int* ptr) {
    *ptr = 100;
}

void main() {
    int x = 42;
    const int* const ptr = &x;
    
    // ❌ Error: const int* const → int* は禁止
    modify_through_pointer(ptr);
}
```

期待エラー:
```
Error: Type mismatch in function call to 'modify_through_pointer'
  Argument type: const int* const
  Parameter type: int*
  Note: cannot discard const qualifier
```

#### 戻り値テスト

```cb
// test_const_pointer_return_ok.cb

int global_x = 42;

const int* const get_const_pointer() {
    const int* const ptr = &global_x;
    return ptr;
}

void main() {
    // ✅ OK: const int* const → const int* const
    const int* const ptr1 = get_const_pointer();
    println(*ptr1);
    
    // ✅ OK: const int* const → const int* (ポインタconstを削除は安全)
    const int* ptr2 = get_const_pointer();
    println(*ptr2);
    
    println("All return value tests passed!");
}
```

```cb
// error_const_pointer_return_mismatch.cb

int global_x = 42;

const int* const get_const_pointer() {
    const int* const ptr = &global_x;
    return ptr;
}

void main() {
    // ❌ Error: const int* const → int* は禁止
    int* ptr = get_const_pointer();
    *ptr = 100;
}
```

### Phase 5: 多重ポインタでの型チェック

```cb
// test_multilevel_const_pointer.cb

int global_x = 42;

void read_double_pointer(const int** pptr) {
    println(**pptr);
}

void main() {
    int* ptr = &global_x;
    const int** pptr = &ptr;
    
    // ✅ OK: 型が完全一致
    read_double_pointer(pptr);
    
    // ❌ Error: int** → const int** は危険（共変性の問題）
    int** pptr2 = &ptr;
    // read_double_pointer(pptr2);  // コンパイルエラーになるべき
}
```

## 📊 実装優先順位

### 🔴 Phase 1: 型情報拡張（v0.10.0）

- Variable/ASTNode構造体の拡張
- パーサーでのconst情報解析強化
- 優先度: **HIGHEST**
- 工数: 2-3日

### 🔴 Phase 2: パラメータ型チェック（v0.10.0）

- 関数呼び出し時の型互換性チェック
- エラーメッセージの実装
- 優先度: **HIGHEST**
- 工数: 2-3日

### 🟡 Phase 3: 戻り値型チェック（v0.10.0）

- return文での型チェック
- 代入時の型チェック
- 優先度: **HIGH**
- 工数: 1-2日

### 🟢 Phase 4-5: テストと多重ポインタ（v0.10.0）

- 包括的テストスイート作成
- 多重ポインタでのエッジケース処理
- 優先度: **MEDIUM**
- 工数: 2-3日

**合計工数**: 7-11日（約2週間）

## 🎓 技術的背景

### C++での型システム

C++ではconst修飾は型システムの一部として厳密にチェックされます:

```cpp
int x = 42;
const int* const ptr = &x;

void modify(int* p) { *p = 100; }

// ❌ コンパイルエラー: const int* const → int* は禁止
modify(ptr);  // error: invalid conversion from 'const int*' to 'int*'
```

### Rustでの型システム

Rustでは借用チェッカーが同様の保証を提供:

```rust
let mut x = 42;
let ptr: &i32 = &x;  // immutable reference

fn modify(p: &mut i32) { *p = 100; }

// ❌ コンパイルエラー: &i32 → &mut i32 は禁止
modify(ptr);  // error: mismatched types
```

### Cb言語での目標

C++/Rustと同等の型安全性を実現:

```cb
int x = 42;
const int* const ptr = &x;

void modify(int* p) { *p = 100; }

// ❌ 実行時エラー: const int* const → int* は禁止
modify(ptr);  // Error: Type mismatch
```

## 📝 まとめ

現在の`const * const`実装は**ローカル変数でのみ**Rust Pin相当の不変性を保証していますが、関数の境界（パラメータ・戻り値）で型情報が失われています。

v0.10.0で完全実装することで:
- ✅ Rust `Pin<&T>`と同等の不変性保証
- ✅ C++と同等の型安全性
- ✅ メモリ安全性の大幅向上
- ✅ バグの早期発見

が実現されます。
