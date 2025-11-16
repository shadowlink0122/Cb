# Generic Struct Array Tests - Comprehensive Test Suite

このディレクトリには、ジェネリック構造体配列の包括的なテストスイートが含まれています。

## 📋 テストファイル概要

### 1. test_generic_struct_array_comprehensive.cb
**目的**: ジェネリック構造体内の配列機能の包括的なテスト

**テスト内容**:
- Container<T> 構造体で固定サイズ配列を含むケース
- Matrix<T> 構造体で複数の配列フィールドを持つケース  
- Wrapper<T> でネストされたジェネリック構造体
- Pair<T, U> で複数の型パラメータと配列の組み合わせ
- ループ操作、コピー操作、更新操作
- 境界アクセス、ゼロ初期化のテスト

**統計**:
- テストセクション: 10
- コード行数: 251行
- アサーション数: 30+

**主要な機能テスト**:
```cb
struct Container<T> {
    T[5] items;
    int count;
};

Container<int> int_container;
Container<long> long_container;
Container<string> str_container;
```

### 2. test_array_of_generic_structs.cb
**目的**: ジェネリック構造体の配列（配列要素として使用）のテスト

**テスト内容**:
- Box<int>[3], Box<long>[4], Box<string>[3] 等の配列宣言
- Pair<int, long>[2] のような複数型パラメータの配列
- Item<T> でメタデータを含む複雑な構造体の配列
- ループによる初期化と検証
- 配列要素間のコピー操作
- 要素のスワップ、最大値検索などのアルゴリズム

**統計**:
- テストセクション: 10
- コード行数: 213行
- アサーション数: 25+

**主要な機能テスト**:
```cb
Box<int>[3] int_boxes;
Pair<int, long>[2] pairs;
Item<int>[3] items;
```

### 3. test_generic_functions_with_arrays.cb
**目的**: ジェネリック関数と配列の連携テスト

**テスト内容**:
- identity<T>(T value) のような基本的なジェネリック関数
- Container<T> を扱うジェネリック関数
- Box<T> make_box<T>(T value) のような構造体を返す関数
- max<T>(T a, T b) のような比較関数
- 配列要素に対するジェネリック関数の適用
- チェーン操作と複雑なシナリオ

**統計**:
- テストセクション: 10
- コード行数: 208行
- アサーション数: 20+

**主要な機能テスト**:
```cb
T identity<T>(T value);
Box<T> make_box<T>(T value);
T max<T>(T a, T b);
```

## 🎯 テストカバレッジ

### 型の組み合わせ
- ✅ `int` 型との組み合わせ
- ✅ `long` 型との組み合わせ  
- ✅ `string` 型との組み合わせ
- ✅ 複数型パラメータ (`Pair<T, U>`)

### 操作のカバレッジ
- ✅ 配列要素の読み書き
- ✅ ループによる反復処理
- ✅ 配列要素のコピー
- ✅ 境界値アクセス
- ✅ 更新・インクリメント操作
- ✅ スワップ操作
- ✅ 検索・比較操作
- ✅ ジェネリック関数との連携

### 構造パターン
- ✅ 単一配列フィールド (`T[N]`)
- ✅ 複数配列フィールド (Matrix型)
- ✅ ネストされたジェネリック構造体
- ✅ 配列の配列 (`Box<T>[N]`)
- ✅ メタデータを含む複合構造

## 🔧 実行方法

### 個別実行
```bash
# Test 1: 構造体内の配列
./main tests/cases/generics/test_generic_struct_array_comprehensive.cb

# Test 2: 構造体の配列
./main tests/cases/generics/test_array_of_generic_structs.cb

# Test 3: ジェネリック関数と配列
./main tests/cases/generics/test_generic_functions_with_arrays.cb
```

### Integration Test実行
```bash
make integration-test
```

Integration testでは以下のテストが実行されます:
- Test 25: Generic Struct Array Comprehensive
- Test 26: Array of Generic Structs  
- Test 27: Generic Functions with Arrays

## 📊 テスト結果

全てのテストが正常に動作し、以下の結果が得られています:

```
Test 1: ✅ 10/10 sections passed
Test 2: ✅ 10/10 sections passed
Test 3: ✅ 10/10 sections passed
─────────────────────────────────
Total:  ✅ 30/30 sections passed
```

## 🏗️ アーキテクチャ

### ファイル構成
```
tests/cases/generics/
├── test_generic_struct_array_comprehensive.cb  (251行)
├── test_array_of_generic_structs.cb            (213行)
└── test_generic_functions_with_arrays.cb       (208行)

tests/integration/generics/
└── test_generics.hpp                           (更新: Test 25-27追加)
```

### Integration Test Framework
各テストは `test_generics.hpp` に統合され、以下をチェックします:
- 実行成功（exit code 0）
- 期待される出力メッセージ
- 各テストセクションの完了確認
- アサーションの成功確認

## 💡 使用例

### Example 1: Generic Container with Array
```cb
struct Container<T> {
    T[5] items;
    int count;
};

Container<int> c;
c.items[0] = 42;
c.items[1] = 84;
c.count = 2;
```

### Example 2: Array of Generic Structs
```cb
struct Box<T> {
    T value;
};

Box<int>[3] boxes;
boxes[0].value = 10;
boxes[1].value = 20;
boxes[2].value = 30;
```

### Example 3: Generic Functions
```cb
Box<T> make_box<T>(T value) {
    Box<T> result;
    result.value = value;
    return result;
}

Box<int> my_box = make_box<int>(42);
```

## 🔍 主要な検証項目

1. **型の正確性**: 各型パラメータが正しくインスタンス化される
2. **メモリ安全性**: 配列の境界チェックと初期化
3. **操作の正確性**: 読み書き、コピー、更新が期待通りに動作
4. **相互運用性**: ジェネリック構造体と関数が正しく連携
5. **パフォーマンス**: ループ操作が効率的に実行される

## 📝 今後の拡張可能性

- [ ] 動的配列との組み合わせ
- [ ] より深いネスト構造（3層以上）
- [ ] ジェネリック構造体のポインタ配列
- [ ] ジェネリック enum との組み合わせ
- [ ] マルチスレッド環境でのテスト

## 📚 関連ドキュメント

- [ジェネリクス基本仕様](../../../docs/generics/)
- [配列の仕様](../../../docs/arrays/)
- [Integration Test Framework](../../integration/framework/)

## 🎉 まとめ

この包括的なテストスイートにより、Cbプログラミング言語のジェネリック構造体配列機能が、
以下の点で十分にテストされ、実用レベルで動作することが確認されています:

- ✅ 構造体内での配列の使用
- ✅ 配列要素としてのジェネリック構造体
- ✅ ジェネリック関数との連携
- ✅ 複数の型パラメータのサポート
- ✅ 実践的なアルゴリズムの実装

**合計672行のテストコード**で、**30のテストセクション**と**75以上のアサーション**により、
堅牢なジェネリック配列機能が保証されています。
