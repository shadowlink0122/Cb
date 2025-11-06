# インターフェース境界テストスイート

## 概要

Cb言語のインターフェース境界機能のテストケース集です。ジェネリック型パラメータに対して複数のインターフェース制約を設定する機能と、メソッド名の衝突検出をテストします。

## 基本構文

```cb
// 単一インターフェース境界
struct Container<T, A: Allocator> {
    A allocator;
}

// 複数インターフェース境界
struct Container<T, A: Allocator + Clone + Debug> {
    A allocator;
}

// 複数の型パラメータでそれぞれ異なる境界
struct MultiContainer<K: Clone + Debug, V, A: Allocator + Clone> {
    K* keys;
    V* values;
    A allocator;
}
```

## テストファイル

### 1. test_multiple_bounds_per_param.cb
型パラメータごとの複数インターフェース境界のテスト

**テスト内容**:
- 単一型パラメータに複数境界 (`A: Allocator + Clone`)
- 3つの境界を持つ型パラメータ (`A: Allocator + Clone + Debug`)
- 複数の型パラメータでそれぞれ異なる境界
- パース成功の確認

**期待される動作**:
- 構文解析が成功すること
- 型パラメータと境界の対応が正しく保存されること

**実行方法**:
```bash
./main tests/cases/interface_bounds/test_multiple_bounds_per_param.cb
```

**期待される出力**:
```
Multiple interface bounds per parameter test: Parsing OK
```

---

### 2. test_function_multiple_bounds.cb
関数でのジェネリック複数境界のテスト

**テスト内容**:
- ジェネリック関数での複数境界 (`T: Clone + Debug`)
- 複数の型パラメータでそれぞれ異なる境界
- 関数定義のパース成功確認

**期待される動作**:
- 関数の型パラメータに複数境界を設定できること
- 構文解析が成功すること

**実行方法**:
```bash
./main tests/cases/interface_bounds/test_function_multiple_bounds.cb
```

**期待される出力**:
```
Generic function multiple bounds test: Parsing OK
```

---

### 3. test_enum_multiple_bounds.cb
列挙型でのジェネリック複数境界のテスト

**テスト内容**:
- 列挙型での複数境界 (`T: Clone + Debug`)
- 複数の型パラメータでそれぞれ異なる境界
- 列挙型定義のパース成功確認

**期待される動作**:
- 列挙型の型パラメータに複数境界を設定できること
- 構文解析が成功すること

**実行方法**:
```bash
./main tests/cases/interface_bounds/test_enum_multiple_bounds.cb
```

**期待される出力**:
```
Generic enum multiple bounds test: Parsing OK
```

---

### 4. test_conflict_methods.cb
インターフェース境界でのメソッド名衝突検出テスト（エラーケース）

**テスト内容**:
- 複数のインターフェースが同じメソッド名を定義
- 型パラメータが両方のインターフェースを要求 (`A: Allocator + Resettable`)
- 両インターフェースが `reset()` メソッドを持つ
- ダイヤモンド問題の検出

**期待される動作**:
- implブロック登録時にメソッド名の衝突を検出
- 明確なエラーメッセージを表示
- プログラムの実行が失敗すること（exit code != 0）

**実行方法**:
```bash
./main tests/cases/interface_bounds/test_conflict_methods.cb
```

**期待されるエラー**:
```
Error: Method name conflict: method 'reset' is already defined in impl 'Allocator' for type 'DummyAllocator'. Cannot redefine in impl 'Resettable'.
```

---

### 5. test_duplicate_impl_methods.cb
同一型への複数implでのメソッド名衝突検出テスト（エラーケース）

**テスト内容**:
- 同じ型に対して複数のimplブロックを定義
- 異なるインターフェースだが同じメソッド名を実装
- implブロック登録時の衝突検出

**期待される動作**:
- 2つ目のimplブロック登録時にメソッド名の衝突を検出
- 明確なエラーメッセージを表示
- プログラムの実行が失敗すること（exit code != 0）

**実行方法**:
```bash
./main tests/cases/interface_bounds/test_duplicate_impl_methods.cb
```

**期待されるエラー**:
```
Error: Method name conflict: method 'reset' is already defined in impl 'Allocator' for type 'MyType'. Cannot redefine in impl 'Resettable'.
```

---

### 6. test_no_conflict_different_types.cb
異なる型への実装での非衝突テスト（正常ケース）

**テスト内容**:
- 同じインターフェースを異なる型に実装
- 同じメソッド名だが異なる型なので衝突しない
- 正常に動作することを確認

**期待される動作**:
- 異なる型への同名メソッド実装は許可されること
- エラーが発生しないこと
- プログラムが正常終了すること（exit code == 0）

**実行方法**:
```bash
./main tests/cases/interface_bounds/test_no_conflict_different_types.cb
```

**期待される出力**:
```
No conflict for different types test: OK
```

---

## 全テスト実行

```bash
# 正常系テスト（exit code == 0が期待される）
./main tests/cases/interface_bounds/test_multiple_bounds_per_param.cb
./main tests/cases/interface_bounds/test_function_multiple_bounds.cb
./main tests/cases/interface_bounds/test_enum_multiple_bounds.cb
./main tests/cases/interface_bounds/test_no_conflict_different_types.cb

# エラーケーステスト（exit code != 0が期待される）
./main tests/cases/interface_bounds/test_conflict_methods.cb
./main tests/cases/interface_bounds/test_duplicate_impl_methods.cb
```

---

## インターフェース境界のルール

### 1. 構文
```cb
// 基本形式
<TypeParameter>: <Interface1> + <Interface2> + ...

// 例
T: Clone
A: Allocator + Clone
K: Clone + Debug + Display
```

### 2. 適用可能な場所
- 構造体の型パラメータ
- 関数の型パラメータ
- 列挙型の型パラメータ

### 3. 制約
- 型引数はすべての指定されたインターフェースを実装していなければならない
- 複数のインターフェースが同じメソッド名を定義している場合はエラー（ダイヤモンド問題）
- 同じ型に対する複数のimplブロックで同じメソッド名を実装した場合はエラー

---

## メソッド名衝突の検出

### ケース1: インターフェース境界による衝突
```cb
interface Allocator {
    void reset();  // 共通メソッド
}

interface Resettable {
    void reset();  // 衝突！
}

// エラー: 両インターフェースが同じメソッド名を持つ
struct Container<T, A: Allocator + Resettable> {
    A allocator;
}
```

### ケース2: implブロックによる衝突
```cb
struct MyType {
    int value;
}

impl Allocator for MyType {
    void reset() { }
}

// エラー: MyTypeはすでにreset()を実装している
impl Resettable for MyType {
    void reset() { }
}
```

### ケース3: 正常（異なる型への実装）
```cb
struct TypeA { int value; }
struct TypeB { int data; }

// OK: 異なる型なので衝突しない
impl Resettable for TypeA {
    void reset() { }
}

impl Resettable for TypeB {
    void reset() { }
}
```

---

## エラーメッセージのフォーマット

### インターフェース境界による衝突
```
Error: Method name conflict: method '<method_name>' is defined in multiple interfaces (<Interface1>, <Interface2>) required by type parameter '<param>' in '<Struct<...>>'
```

### implブロックによる衝突
```
Error: Method name conflict: method '<method_name>' is already defined in impl '<Interface1>' for type '<Type>'. Cannot redefine in impl '<Interface2>'.
```

---

## 実装ステータス

- [x] データ構造の更新（`std::unordered_map<std::string, std::vector<std::string>>`）
- [x] パーサーの更新（構造体、関数、列挙型）
- [x] インターフェース境界の検証
- [x] インターフェース境界によるメソッド衝突検出
- [x] implブロックによるメソッド衝突検出
- [x] テストケースの作成
- [x] エラーメッセージの実装
- [ ] Integration testの作成
- [ ] ドキュメントの更新

---

## 参考

- 言語仕様: `docs/spec.md`
- アーキテクチャドキュメント: `docs/architecture.md`
- BNF: `docs/BNF.md`
