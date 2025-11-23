# Interface/Impl vtable実装完了レポート

**日付**: 2025年11月19日
**実装者**: Claude Code
**ステータス**: ✅ 完了

## 実装内容

Interface/Implの多態性をvtableアプローチ（ポインタベース）で実装しました。

## 実装した機能

### 1. 構造体リテラルのサポート

**問題**: 位置ベース初期化 `{10, 5}` が動作しない

**原因**: HIR生成時に `node->arguments` を処理していなかった

**修正箇所**:
- `src/backend/ir/hir/hir_generator.cpp:893-910`
- `src/backend/codegen/hir_to_cpp.cpp:523-550`

**修正内容**:
```cpp
// HIR生成で位置ベース初期化をサポート
for (const auto &arg : node->arguments) {
    expr.field_values.push_back(convert_expr(arg.get()));
}
```

**使用例**:
```cb
struct Point {
    int x;
    int y;
};

int main() {
    Point p = {10, 20};  // ✅ 動作する
    println(p.x);  // 10
    println(p.y);  // 20
    return 0;
}
```

### 2. Interface/Implの仮想関数実装

**問題**: implメソッドに `virtual` と `override` が付いていない

**修正箇所**:
- `src/backend/codegen/hir_to_cpp.cpp:560-579`

**修正内容**:
各implに対して対応するinterfaceを検索し、メソッドに `virtual` と `override` を追加：

```cpp
// 各implに対して対応するinterfaceを見つける
for (const auto *impl_ptr : struct_impls) {
    const HIRInterface *interface_ptr = nullptr;
    if (current_program && !impl_ptr->interface_name.empty()) {
        std::string interface_base = impl_ptr->interface_name;
        // ... interfaceを検索
    }

    // virtualとoverrideを追加
    if (!impl_ptr->interface_name.empty() && interface_method) {
        emit("virtual ");
    }
}
```

**生成されるC++コード**:
```cpp
struct Rectangle : public Shape {
    int width;
    int height;

    virtual int area() override;
    virtual int perimeter() override;
};
```

### 3. Interfaceを実装する構造体の初期化コンストラクタ

**問題**: interfaceを継承すると集成体初期化ができない

**原因**: C++では基底クラスを持つと集成体ではなくなる

**修正箇所**:
- `src/backend/codegen/hir_to_cpp.cpp:529-550`

**修正内容**:
Interfaceを実装する構造体に、フィールドを受け取るコンストラクタを自動生成：

```cpp
if (!implemented_interfaces.empty() && !struct_def.fields.empty()) {
    emit(struct_def.name + "(");
    for (size_t i = 0; i < struct_def.fields.size(); i++) {
        if (i > 0) emit(", ");
        const auto &field = struct_def.fields[i];
        emit(generate_type(field.type) + " _" + field.name);
    }
    emit(")");
    emit(" : ");
    for (size_t i = 0; i < struct_def.fields.size(); i++) {
        if (i > 0) emit(", ");
        const auto &field = struct_def.fields[i];
        emit(field.name + "(_" + field.name + ")");
    }
    emit(" {}\n");
}
```

**生成されるC++コード**:
```cpp
struct Rectangle : public Shape {
    int width;
    int height;

    // Default constructor
    Rectangle() = default;

    // Field initialization constructor
    Rectangle(int _width, int _height) : width(_width), height(_height) {}

    virtual int area() override;
    virtual int perimeter() override;
};
```

### 4. 複数Interfaceのサポート

**テスト**: 1つの構造体が複数のinterfaceを実装

**動作確認**:
```cb
struct Circle {
    int radius;
};

interface Shape {
    int area();
};

interface Printable {
    void display();
};

impl Shape for Circle {
    int area() {
        return 3 * self.radius * self.radius;
    }
};

impl Printable for Circle {
    void display() {
        println("Circle");
    }
};

int main() {
    Circle c = {5};

    Shape* s = &c;
    println(s->area());  // 75

    Printable* p = &c;
    p->display();  // "Circle"

    return 0;
}
```

**生成されるC++コード**:
```cpp
struct Circle : public Shape, public Printable {
    int radius;

    Circle() = default;
    Circle(int _radius) : radius(_radius) {}

    virtual int area() override;
    virtual void display() override;
};
```

## 使用方法

### 基本的な使用例

```cb
struct Rectangle {
    int width;
    int height;
};

interface Shape {
    int area();
    int perimeter();
};

impl Shape for Rectangle {
    int area() {
        return self.width * self.height;
    }

    int perimeter() {
        return 2 * (self.width + self.height);
    }
};

int main() {
    // 構造体リテラルで初期化
    Rectangle rect = {10, 5};

    // ポインタベースの多態性
    Shape* shape = &rect;
    println(shape->area());      // 50
    println(shape->perimeter()); // 30

    return 0;
}
```

### 参照を使った例

```cb
int main() {
    Rectangle rect = {10, 5};

    // 参照でも使える
    Shape& shape = rect;
    println(shape.area());      // 50
    println(shape.perimeter()); // 30

    return 0;
}
```

## 技術的詳細

### vtableアプローチ

C++の仮想関数テーブル（vtable）を使用した実装：

1. **Interface** → 純粋仮想関数を持つ抽象クラス
2. **Struct** → interfaceを継承
3. **Impl** → 仮想関数をオーバーライド
4. **実行時**: vtableを介して動的ディスパッチ

### メモリレイアウト

```
Rectangle インスタンス:
┌─────────────────┐
│ vtable pointer  │ ← Shape vtable を指す
├─────────────────┤
│ width           │
├─────────────────┤
│ height          │
└─────────────────┘

Shape vtable:
┌─────────────────┐
│ area()          │ → Rectangle::area()
├─────────────────┤
│ perimeter()     │ → Rectangle::perimeter()
└─────────────────┘
```

### パフォーマンス

- **メモリオーバーヘッド**: vtableポインタ1つ分（8バイト）
- **関数呼び出しコスト**: 1回の間接参照 + 1回の仮想関数呼び出し
- **最適化**: devirtualization（コンパイラが最適化可能）

## テスト結果

✅ 基本的なinterface/impl
✅ 複数interface実装
✅ 構造体リテラル
✅ ポインタ経由のメソッド呼び出し
✅ 参照経由のメソッド呼び出し
✅ ジェネリックパラメータ（Future<T>など）

## 既知の制限

1. **値型として使えない**: `Shape shape = rect;` は未サポート
   - → 値型interfaceの実装が必要（型消去パターン）
   - → 設計書: `docs/interface_value_type_design.md`

2. **FFI呼び出しとの変数名衝突**: 短い変数名（3文字以下）はFFI呼び出しと誤認される
   - 回避策: 4文字以上の変数名を使用

3. **構造体リテラルの型名付き構文**: `Point{10, 20}` は未サポート
   - 現在: `{10, 20}` のみサポート

## ファイル変更一覧

| ファイル | 変更内容 |
|---------|---------|
| `src/backend/ir/hir/hir_generator.cpp` | 位置ベース初期化のサポート |
| `src/backend/codegen/hir_to_cpp.cpp` | virtual/override追加、初期化コンストラクタ生成 |
| `docs/interface_value_type_design.md` | 値型interface設計書（新規） |
| `docs/interface_impl_vtable_implementation.md` | このドキュメント（新規） |

## 次のステップ

1. **値型interfaceの実装**: 型消去パターンを使用（設計書参照）
2. **ジェネリックinterfaceのテスト**: より複雑なケースでの検証
3. **パフォーマンステスト**: 実際のアプリケーションでの測定
4. **ドキュメント**: ユーザー向けガイドの作成

## まとめ

Interface/Implのvtableアプローチによる実装が完了しました。これにより：

✅ C++の仮想関数を活用した効率的な多態性
✅ 構造体リテラルによる簡潔な初期化
✅ 複数interfaceの実装サポート
✅ 既存のC++エコシステムとの互換性

実用的なポリモーフィズムが利用可能になり、Cbの表現力が大幅に向上しました。
