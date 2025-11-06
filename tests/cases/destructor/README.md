# Destructor Tests

デストラクタ機能のテストスイート

## テスト概要

このディレクトリには、Cb言語のデストラクタ機能に関するテストが含まれています。

### テストケース

1. **test_basic.cb** - 基本的なデストラクタ機能
   - 非ジェネリック構造体のデストラクタ
   - スコープ終了時の自動呼び出し
   - メモリ解放の確認

2. **test_generic.cb** - ジェネリック構造体のデストラクタ
   - ジェネリック型パラメータを持つ構造体のデストラクタ
   - トレイト境界付きジェネリクス型のデストラクタ
   - 複数のジェネリックパラメータを持つ構造体

3. **test_scope.cb** - スコープとデストラクタ
   - ネストしたスコープでのデストラクタ呼び出し順序
   - 複数の変数が同じスコープで破棄される場合
   - LIFO順（後入れ先出し）の動作確認

4. **test_vector_destructor.cb** - Vector構造体での実例
   - メモリ管理を伴う実用的なデストラクタ
   - new/deleteとの統合
   - 自動メモリ解放の検証

## 実行方法

### 個別テストの実行

```bash
./main tests/cases/destructor/test_basic.cb
./main tests/cases/destructor/test_generic.cb
./main tests/cases/destructor/test_scope.cb
./main tests/cases/destructor/test_vector_destructor.cb
```

### 統合テストの実行

```bash
make test
```

## 期待される動作

- スコープを抜ける時にデストラクタが自動的に呼ばれること
- デストラクタ内で`self`が正しく設定されていること
- 複数の変数がある場合、宣言の逆順（LIFO）でデストラクタが呼ばれること
- ジェネリック型に対してもデストラクタが正しく動作すること
- メモリリークが発生しないこと

## 実装の詳細

### 構文

```cb
// 非ジェネリック構造体のデストラクタ
impl StructName {
    ~self() {
        // クリーンアップ処理
    }
}

// ジェネリック構造体のデストラクタ
impl StructName<T, A: Trait> {
    ~self() {
        // クリーンアップ処理
    }
}
```

### 重要な注意事項

1. **スコープの作成**: Cbでは複合文 `{}` がスコープを作成する（v0.11.0で実装完了）
   - ✅ スコープを作成する構文: `{}`, `if`, `while`, `for`, 関数本体
   - デストラクタはスコープ終了時（`}`）に自動的に呼ばれる

2. **値渡しの制限**: Cbの構造体は値渡しのため、メソッド内での変更が呼び出し元に反映されない

3. **デストラクタの呼び出しタイミング**: スコープ終了時（右波括弧 `}` に到達した時）

4. **型名のマングリング**: ジェネリック型は内部で `StructName_TypeParam1_TypeParam2` のようにマングルされる

### スコープの例

```cb
// ✅ 裸のブロックでもスコープが作成される
void test() {
    println("Start");
    {
        Item item;  // このitemはブロック終了時に破棄される
        println("In block");
    }  // ← デストラクタがここで呼ばれる
    println("End");
}

// ✅ if文でもスコープが作成される
void test() {
    println("Start");
    if (true) {
        Item item;  // このitemはif文のスコープ終了時に破棄される
        println("In if block");
    }  // ← デストラクタがここで呼ばれる
    println("End");
}
```

## 関連ドキュメント

- [デストラクタ実装完了レポート](../../../docs/features/constructor_destructor.md)
- [メモリ管理機能](../../../docs/features/memory_management_complete.md)
