# Const Pointer Safety Tests

このディレクトリには、const変数のアドレスを非constポインタに代入することを防ぐ安全機能のテストが含まれています。

## 機能概要

const変数のアドレスを非constポインタに代入しようとすると、**実行時エラー**が発生します。これにより、constの制約を迂回してデータを変更する危険なコードを検出できます。

## テストケース

### 正常系テスト（4ファイル）

1. **`test_correct_usage.cb`** - 基本的な正しい使い方
   ```cb
   const int x = 42;
   const int* ptr = &x;  // ✅ OK
   ```

2. **`test_comprehensive.cb`** - 単一ポインタの包括的テスト
   - const変数 → const pointer
   - 非const変数 → 非const pointer
   - 非const変数 → const pointer

3. **`test_double_pointer.cb`** - ダブルポインタの基本テスト
   ```cb
   const int* ptr1 = &x;
   const int** ptr2 = &ptr1;  // ✅ OK
   ```

4. **`test_double_pointer_comprehensive.cb`** - ダブルポインタの包括的テスト

### エラー検出テスト（3ファイル）

1. **`error_assign_const_to_nonconst.cb`** - const変数→非constポインタエラー
   ```cb
   const int x = 42;
   int* ptr;
   ptr = &x;  // ❌ Error
   ```

2. **`error_double_pointer.cb`** - const T*→T**エラー
   ```cb
   const int* ptr1 = &x;
   int** ptr2 = &ptr1;  // ❌ Error
   ```

3. **`error_const_pointer_address.cb`** - T* const→T**エラー
   ```cb
   int* const ptr1 = &x;
   int** ptr2 = &ptr1;  // ❌ Error
   ```

## 検出される違反パターン

### 1. const変数のアドレス → 非constポインタ

```cb
const int x = 42;
int* ptr = &x;  // ❌ Error
```

**エラーメッセージ**:
```
Error: Cannot assign address of const variable 'x' to non-const pointer 'ptr'. 
Use 'const int*' instead of 'int*'
```

### 2. const T*のアドレス → T**

```cb
const int* ptr1 = &x;
int** ptr2 = &ptr1;  // ❌ Error
```

**エラーメッセージ**:
```
Error: Cannot assign address of pointer to const (const T*) 'ptr1' to non-const double pointer 'ptr2'. 
The pointee should be 'const T**', not 'T**'
```

### 3. T* constのアドレス → T**

```cb
int* const ptr1 = &x;
int** ptr2 = &ptr1;  // ❌ Error
```

**エラーメッセージ**:
```
Error: Cannot assign address of const pointer (T* const) 'ptr1' to non-const double pointer 'ptr2'. 
Use 'const' qualifier appropriately
```

## 実装

### 実装ファイル

- ✅ **`src/backend/interpreter/executors/assignments/simple_assignment.cpp`** 
  - ポインタ代入時のconst安全性チェック（完全実装）
  - 行範囲: 785-830

- ⚠️ **`src/backend/interpreter/executors/declarations/variable_declaration.cpp`**
  - 宣言+初期化時のconst安全性チェック（コード実装済み、実行パス調査中）
  - 行範囲: 229-275, 630-685

### 統合テスト

統合テストフレームワークで自動実行：

```bash
make test
```

出力例：
```
=== Const Pointer Safety Tests ===

--- Correct Usage Tests ---
[✓] test_correct_usage passed (9.429ms)
[✓] test_comprehensive passed (8.991ms)
[✓] test_double_pointer passed (9.204ms)
[✓] test_double_pointer_comprehensive passed (8.877ms)

--- Error Detection Tests ---
[✓] test_error_assign_const_to_nonconst passed (8.622ms)
[✓] test_error_double_pointer passed (8.586ms)
[✓] test_error_const_pointer_address passed (8.689ms)
=== All Const Pointer Safety Tests Passed ===

[integration-test] ✅ PASS: Const Pointer Safety Tests (25 tests)
```

## 実装ステータス

- ✅ **ポインタ代入時のconst安全性チェック** - 完全動作
- ✅ **3つの違反パターンを検出**
- ✅ **ダブルポインタ対応**
- ✅ **明確なエラーメッセージ**
- ⚠️ **宣言+初期化パス** - コード実装済み（実行パス調査中）

## 詳細ドキュメント

詳細は `docs/spec.md`の「Const Pointer Safety」セクションを参照してください。

---

**バージョン**: v0.9.1  
**実装日**: 2025年1月  
**テストステータス**: ✅ 完全動作（代入パス）
