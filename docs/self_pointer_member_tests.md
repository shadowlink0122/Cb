# Self Pointer Member Access Test Documentation

このドキュメントは、構造体のポインタメンバーとselfアクセスに関する包括的なテストをまとめたものです。

## テスト概要

### 実装済み・動作確認済みのパターン

#### 1. Arrow Operator Assignment (`ptr->member = value`)
**ファイル**: `test_arrow_assignment_comprehensive.cb`

**テストケース**:
- ✅ Test 1: 直接ポインタ代入 (`ptr->x = 100`)
- ✅ Test 2: ネストしたポインタメンバー代入 (`Point* ptr = c.data; ptr->x = 500`)
- ✅ Test 3: 同一オブジェクトへの複数ポインタアクセス
- ✅ Test 4: 関数パラメータとしてのポインタ (`void modify_point(Point* ptr)`)
- ✅ Test 5: ポインタチェーン (複数の Container が同じ Point を指す)
- ✅ Test 6: 式を使った代入 (`ptr->x = 10 + 20`)
- ✅ Test 7: 自己代入 (`ptr->x = ptr->x + 100`)

**結果**: 全7件成功 ✅

---

#### 2. Self with Pointer Member (`self.ptr->field`)
**ファイル**: `test_self_pointer_member_comprehensive.cb`

**テストケース**:
- ✅ Test 1: selfを通じたポインタメンバーの読み取り (`return self.data->x`)
- ✅ Test 2: selfを通じたポインタメンバーの書き込み (`self.data->x = newX`)
- ✅ Test 3: 両座標の同時変更
- ✅ Test 4: ポインタメンバーを使った計算 (`return self.data->x + self.data->y`)
- ✅ Test 5: インクリメント操作 (`self.data->x = self.data->x + 1`)
- ✅ Test 6: 値の倍化 (`self.data->x = self.data->x * 2`)
- ✅ Test 7: selfの直接フィールドとポインタメンバーの混合 (`return self.id + self.data->x`)

**結果**: 全7件成功 ✅

---

#### 3. Self Member Arrow Field Access (`self.member->field`)
**ファイル**: `test_self_member_arrow_field.cb`

**テストケース**:
- ✅ Test 1: self.data->x の読み取り
- ✅ Test 2: self.data->x の書き込み
- ✅ Test 3: self.data->x + self.data->y の計算

**結果**: 全3件成功 ✅

---

#### 4. Comprehensive Self Pointer Member Tests
**ファイル**: `test_self_pointer_comprehensive.cb`

**構造**: 
```cb
struct Rectangle {
    Point* topLeft;
    Point* bottomRight;
};
```

**テストケース**:
- ✅ Test 1: 4つの角すべての読み取り (`self.topLeft->x`, `self.topLeft->y`, `self.bottomRight->x`, `self.bottomRight->y`)
- ✅ Test 2: 4つの角すべての書き込み
- ✅ Test 3: 複数のポインタメンバーを使った計算 (幅、高さ、面積)
- ✅ Test 4: 複雑な操作 - 移動 (`moveBy`)
  - 4つの self.ptr->field への同時代入
- ✅ Test 5: 複雑な操作 - スケーリング (`scale`)
  - 式を使った self.ptr->field の計算
- ✅ Test 6: スケーリング後の寸法検証

**結果**: 全6件成功 ✅

---

## 未サポート・今後の実装が必要なパターン

### 1. Self Member Method Call (`self.ptr->method()`)
**パターン**: `self.data->getX()`

**現状**: ❌ サポートされていない

**エラーメッセージ**:
```
[METHOD_LOOKUP_FAIL] receiver='self.data' type='pointer' method='getX'
Error: Undefined function: getX
```

**理由**: 
- `self.data` はポインタ型として認識される
- ポインタが指す型 (Point) のメソッドルックアップが行われていない
- メソッド呼び出しの解決で、`self.data` の実体の型を取得して impl を探す必要がある

**必要な実装**:
- メソッド呼び出し時に、レシーバーがポインタの場合、ポインタが指す型のメソッドを探す
- `self.data` (Point*) → Point のメソッドを探す

---

### 2. Pointer Type Struct Impl (`impl Interface for Point*`)
**パターン**: 
```cb
impl IPoint for Point* {
    int getX() {
        return self->x;  // self自体がPoint*
    }
}
```

**現状**: ❌ パーサーレベルでサポートされていない

**エラーメッセージ**:
```
Error: Expected '{' after type name in impl declaration
```

**理由**: 
- パーサーが `impl Interface for Type*` の構文をサポートしていない
- 現在は `impl Interface for Type` のみサポート

**必要な実装**:
- パーサーで `for Type*` を許可
- self をポインタ型として扱う (`self->member` をサポート)
- メソッド呼び出しで `ptr->method()` を `(*ptr).method()` として処理

---

### 3. Dereference Member Assignment (`(*self.ptr).field = value`)
**パターン**: `(*self.data).x = 100`

**現状**: ⚠️ 読み取りは成功、書き込みは失敗

**テスト結果**:
- ✅ 読み取り: `return (*self.data).x` → 成功
- ❌ 書き込み: `(*self.data).x = newX` → 失敗 (元の変数に反映されない)

**理由**:
- `execute_member_assignment` が `(*pointer).member` パターンの代入を処理していない
- `member_access->left` がデリファレンスノードの場合のハンドリングが不足

**必要な実装**:
- `statement_executor.cpp` の `execute_member_assignment` で:
  - `member_access->left->node_type == AST_DEREFERENCE` のケースを追加
  - デリファレンスされたポインタが指す実体のメンバーに書き込む
  - `sync_individual_member_from_struct` を呼び出して個別変数と同期

---

## サポート状況まとめ

| パターン | 構文 | 読み取り | 書き込み | メソッド呼び出し | 状態 |
|---------|------|---------|---------|----------------|------|
| Arrow operator | `ptr->field` | ✅ | ✅ | - | 完全サポート |
| Self + Arrow | `self.ptr->field` | ✅ | ✅ | - | 完全サポート |
| Dereference | `(*ptr).field` | ✅ | ❌ | - | 部分サポート |
| Self + Dereference | `(*self.ptr).field` | ✅ | ❌ | - | 部分サポート |
| Arrow method | `ptr->method()` | - | - | ✅ | 完全サポート (通常変数) |
| Self + Arrow method | `self.ptr->method()` | - | - | ❌ | 未サポート |
| Pointer struct impl | `impl I for T*` | - | - | - | 未サポート |

---

## 実装の技術詳細

### Arrow Assignment の仕組み

1. **execute_arrow_assignment** (statement_executor.cpp)
   - `ptr->member = value` を処理
   - ポインタをデリファレンスして構造体変数を取得
   - `struct_members` に値を書き込む
   - **sync_individual_member_from_struct** を呼び出し

2. **sync_individual_member_from_struct** (interpreter.cpp)
   - `struct_var` のアドレスから変数名を逆引き
   - 全スコープ (グローバル、ローカル、static) を探索
   - 対応する個別変数 (`var_name.member_name`) を更新
   - 個別変数システムとの同期を保証

### Self Setup と Writeback

1. **Self Setup** (expression_evaluator.cpp, evaluate_method_call)
   - メソッド呼び出し時に `self.*` 変数を作成
   - ポインタメンバーの場合、ポインタ値をコピー
   - ネストした構造体メンバーも再帰的に作成

2. **Self Writeback** (expression_evaluator.cpp)
   - メソッド終了時に `self.*` の変更を receiver に書き戻す
   - 全 `self.*` 変数をループして `receiver.*` にコピー
   - ポインタメンバーの場合、arrow assignment 経由で更新が既に反映されている

---

## テスト統計

- **総テスト数**: 2163件 (統合テスト)
- **成功率**: 100% ✅
- **新規追加テスト**: 23件
  - Arrow Assignment: 7件
  - Self Pointer Member: 7件
  - Self Member Arrow Field: 3件
  - Self Pointer Comprehensive: 6件

---

## 推奨される使用パターン

### ✅ 推奨: Arrow Operator
```cb
impl IContainer for Container {
    int getDataX() {
        return self.data->x;  // 推奨
    }
    
    void setDataX(int newX) {
        self.data->x = newX;  // 推奨
    }
}
```

### ⚠️ 非推奨: Dereference (書き込み未サポート)
```cb
impl IContainer for Container {
    int getDataX() {
        return (*self.data).x;  // 読み取りのみ OK
    }
    
    void setDataX(int newX) {
        (*self.data).x = newX;  // ❌ 書き込み未サポート
    }
}
```

### ❌ 未サポート: メソッド呼び出し
```cb
impl IContainer for Container {
    int getDataX() {
        return self.data->getX();  // ❌ 未サポート
    }
}
```

---

## 今後の改善提案

### 優先度: 高
1. **Self Member Method Call サポート**
   - `self.ptr->method()` の実装
   - メソッドルックアップでポインタの指す型を解決

2. **Dereference Assignment 修正**
   - `(*self.ptr).field = value` の書き込みサポート
   - `execute_member_assignment` で AST_DEREFERENCE ケースを追加

### 優先度: 中
3. **Pointer Struct Impl サポート**
   - `impl Interface for Type*` のパーサー対応
   - self をポインタとして扱う (`self->member`)

### 優先度: 低
4. **エラーメッセージ改善**
   - 未サポートパターンでより明確なエラーメッセージ
   - 例: "self.ptr->method() is not yet supported. Use self.ptr->field instead."

---

## 関連ファイル

### テストファイル
- `tests/cases/interface/test_arrow_assignment_comprehensive.cb`
- `tests/cases/interface/test_self_pointer_member_comprehensive.cb`
- `tests/cases/interface/test_self_member_arrow_field.cb`
- `tests/cases/interface/test_self_pointer_comprehensive.cb`
- `tests/cases/interface/test_self_member_deref_field.cb` (部分サポート)
- `tests/cases/interface/test_self_member_method_call.cb` (未サポート)
- `tests/cases/interface/test_pointer_struct_impl.cb` (未サポート)

### 実装ファイル
- `src/backend/interpreter/executor/statement_executor.cpp`
  - `execute_arrow_assignment`
  - `execute_member_assignment`
- `src/backend/interpreter/core/interpreter.cpp`
  - `sync_individual_member_from_struct`
- `src/backend/interpreter/evaluator/expression_evaluator.cpp`
  - Self setup (evaluate_method_call)
  - Self writeback

### 統合テスト
- `tests/integration/interface/interface_tests.hpp`
  - `test_arrow_assignment_comprehensive`
  - `test_self_pointer_member_comprehensive`
  - `test_self_member_arrow_field`
  - `test_self_pointer_comprehensive`
