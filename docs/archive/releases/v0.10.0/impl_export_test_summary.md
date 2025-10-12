# Import/Export Impl Test Suite

このドキュメントは、`impl`（コンストラクタ・デストラクタ、およびインターフェース実装）のexport/import機能のテストスイートについて説明します。

## テストファイル

### 1. test_import_constructor.cb
**目的**: コンストラクタとインターフェース実装の包括的なテスト

**テスト内容**:
- ✅ **Test 1**: コンストラクタ呼び出し (`Rectangle(10, 20)`)
- ✅ **Test 2**: implメソッドの使用 (`getArea()`, `display()`)
- ✅ **Test 3**: メンバーへの直接アクセス (`rect.width`, `rect.height`)
- ✅ **Test 4**: 複数のインスタンス作成
- ✅ **Test 5**: インスタンスの独立性確認
- ⏸️ **Test 6**: デフォルトコンストラクタ（関数オーバーロード未実装のためスキップ）
- ✅ **Test 7**: エッジケース（0x0、1x1の矩形）

**検証項目**:
```cb
// コンストラクタが正しく呼ばれる
Rectangle rect = Rectangle(10, 20);

// implメソッドが動作する
rect.display();  // impl Shape for Rectangle
int area = rect.getArea();

// メンバーアクセスが動作する
assert(rect.width == 10);
assert(rect.height == 20);

// 複数インスタンスが独立している
Rectangle rect2 = Rectangle(5, 8);
assert(rect.width == 10);  // 変わっていない
assert(rect2.width == 5);
```

### 2. test_impl_types.cb
**目的**: 異なる種類のimplのexportをテスト

**テスト内容**:
- ✅ **Test 1**: 構造体定義のインポート
- ✅ **Test 2**: `impl Rectangle`（コンストラクタ）のインポート
- ✅ **Test 3**: `impl Shape for Rectangle`（インターフェース実装）のインポート
- ✅ **Test 4**: インターフェース定義の可用性確認
- ⏸️ **Test 5**: 複数実装（将来的なテスト）

**検証項目**:
```cb
// 構造体定義がインポートされる
Rectangle rect = Rectangle(100, 50);

// コンストラクタ（impl Rectangle）が動作
assert(rect.width == 100);

// インターフェースメソッド（impl Shape for Rectangle）が動作
int area = rect.getArea();
assert(area == 5000);
```

### 3. test_simple_constructor.cb
**目的**: 基本的なコンストラクタインポートの動作確認

**テスト内容**:
- ✅ コンストラクタの呼び出し
- ✅ 構造体メンバーへのアクセス

**検証項目**:
```cb
Rectangle rect = Rectangle(10, 20);
assert(rect.width == 10);
assert(rect.height == 20);
```

## モジュールファイル

### modules/shapes.cb
**エクスポート内容**:

1. **インターフェース定義**:
```cb
export interface Shape {
    int getArea();
    void display();
};
```

2. **構造体定義**:
```cb
export struct Rectangle {
    int width;
    int height;
};
```

3. **コンストラクタ・デストラクタ** (`impl Rectangle`):
```cb
export impl Rectangle {
    self(int w, int h) { /* パラメータ付きコンストラクタ */ }
    // self() { /* デフォルトコンストラクタ - オーバーロード未実装のためコメントアウト */ }
    ~self() { /* デストラクタ */ }
}
```

4. **インターフェース実装** (`impl Shape for Rectangle`):
```cb
export impl Shape for Rectangle {
    int getArea() { return self.width * self.height; }
    void display() { println("Rectangle: ", self.width, " x ", self.height); }
};
```

## 実装の詳細

### Export処理
- `src/frontend/recursive_parser/parsers/statement_parser.cpp`:
  - `export impl`を処理する際に`is_exported`フラグを設定

### Import処理
- `src/backend/interpreter/core/interpreter.cpp`:
  - `AST_IMPL_DECL`を処理する際:
    - コンストラクタを`struct_constructors_`に登録
    - コンストラクタを関数テーブルにも登録（`Rectangle()`として呼び出し可能に）
    - デストラクタを`struct_destructors_`に登録
    - インターフェースメソッドを`handle_impl_declaration()`経由で登録

### コンストラクタ呼び出し
- `src/backend/interpreter/evaluator/functions/call_impl.cpp`:
  - コンストラクタ呼び出し時に`self`変数を作成
  - 構造体定義から`self`のメンバーを初期化

## 既知の制限事項

### 1. 関数オーバーロード未サポート
**問題**: 同じ名前の関数が複数定義できない
```cb
// これは動作しない（最後のコンストラクタのみが有効）
export impl Rectangle {
    self(int w, int h) { /* ... */ }
    self() { /* デフォルトコンストラクタ */ }  // これが優先される
}
```

**回避策**: 現在は1つのコンストラクタシグネチャのみを使用

### 2. 自動デストラクタ未サポート
**問題**: デストラクタは自動的に呼ばれない
```cb
{
    Rectangle rect = Rectangle(10, 20);
    // スコープ終了時にデストラクタは呼ばれない
}
```

**回避策**: 現在はデストラクタ定義のみサポート（将来の実装のための準備）

### 3. まとめてexport未サポート
**問題**: `export { ... }`構文はまだ実装されていない
```cb
// これは未サポート
export {
    Rectangle,
    impl Rectangle,
    impl Shape for Rectangle,
};
```

**回避策**: 個別に`export`を付ける
```cb
export struct Rectangle { ... };
export impl Rectangle { ... };
export impl Shape for Rectangle { ... };
```

## テスト結果

### 統合テストカバレッジ
- **総テスト数**: 2867
- **成功**: 2867 (100%)
- **失敗**: 0

### Import/Exportテスト
- `test_import_export_impl`: ✅ PASS
- `test_import_export_impl_types`: ✅ PASS
- `test_import_export_simple_constructor`: ✅ PASS

## まとめ

### 実装済み機能
✅ `export impl Rectangle { self(...) }`（コンストラクタ）のexport/import
✅ `export impl Shape for Rectangle { ... }`（インターフェース実装）のexport/import
✅ インポートしたコンストラクタの呼び出し
✅ インポートしたimplメソッドの呼び出し
✅ 複数インスタンスの作成と独立性
✅ 構造体メンバーへの直接アクセス

### 今後の課題
⏸️ 関数オーバーロード（複数のコンストラクタシグネチャ）
⏸️ 自動デストラクタ呼び出し（RAII）
⏸️ まとめてexport (`export { ... }`) 構文
⏸️ インターフェース型変数のサポート
