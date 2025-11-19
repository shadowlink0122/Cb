# Interface値型実装設計書

**日付**: 2025年11月19日
**ステータス**: 設計完了、実装未完了
**関連**: Interface/Implの多態性実装

## 概要

現在、Interface/Implはポインタベース（vtable）で実装されています：
```cb
Rectangle rect = {10, 5};
Shape* shape = &rect;  // ポインタのみ
println(shape->area());
```

この設計書では、値型としてinterfaceを使用できるようにする実装方法を説明します：
```cb
Rectangle rect = {10, 5};
Shape shape = rect;  // 値型での代入
println(shape.area());  // ドット演算子
```

## ハイブリッドアプローチ

ポインタベースと値型の両方をサポート：
- **ポインタベース**: `Shape* s = &rect;` → vtableを使用（既存実装）
- **値型ベース**: `Shape s = rect;` → 型消去を使用（新規実装）

## 型消去パターン（Type Erasure）

### 基本概念

型消去は、具体的な型情報を隠蔽しながら、インターフェースを介して操作を実行する技法です。Rustのtrait objectやC++の`std::any`、`std::function`で使用されています。

### C++での実装例

```cpp
// Interface定義（純粋仮想関数）
class Shape {
public:
    virtual ~Shape() = default;
    virtual int area() = 0;
    virtual int perimeter() = 0;
};

// 値型インターフェース（型消去版）
class Shape_Value {
private:
    // 内部コンセプト（純粋仮想）
    struct Concept {
        virtual int area() = 0;
        virtual int perimeter() = 0;
        virtual std::unique_ptr<Concept> clone() const = 0;
        virtual ~Concept() = default;
    };

    // テンプレート実装（具体型を保持）
    template<typename T>
    struct Model : Concept {
        T data;

        Model(T d) : data(std::move(d)) {}

        int area() override {
            return data.area();
        }

        int perimeter() override {
            return data.perimeter();
        }

        std::unique_ptr<Concept> clone() const override {
            return std::make_unique<Model<T>>(data);
        }
    };

    std::unique_ptr<Concept> ptr_;

public:
    // コンストラクタ（任意の型から構築）
    template<typename T>
    Shape_Value(T obj)
        : ptr_(std::make_unique<Model<T>>(std::move(obj))) {}

    // コピーコンストラクタ
    Shape_Value(const Shape_Value& other)
        : ptr_(other.ptr_ ? other.ptr_->clone() : nullptr) {}

    // ムーブコンストラクタ
    Shape_Value(Shape_Value&& other) = default;

    // コピー代入演算子
    Shape_Value& operator=(const Shape_Value& other) {
        if (this != &other) {
            ptr_ = other.ptr_ ? other.ptr_->clone() : nullptr;
        }
        return *this;
    }

    // ムーブ代入演算子
    Shape_Value& operator=(Shape_Value&& other) = default;

    // メソッド呼び出し
    int area() {
        return ptr_->area();
    }

    int perimeter() {
        return ptr_->perimeter();
    }
};
```

### Cbでの使用例

```cb
struct Rectangle {
    int width;
    int height;
};

struct Circle {
    int radius;
};

interface Shape {
    int area();
    int perimeter();
}

impl Shape for Rectangle {
    int area() {
        return self.width * self.height;
    }
    int perimeter() {
        return 2 * (self.width + self.height);
    }
}

impl Shape for Circle {
    int area() {
        return 3 * self.radius * self.radius;  // π ≈ 3
    }
    int perimeter() {
        return 6 * self.radius;  // 2π ≈ 6
    }
}

int main() {
    Rectangle rect = {10, 5};
    Circle circ = {7};

    // 値型として使用
    Shape shape1 = rect;
    Shape shape2 = circ;

    println(shape1.area());      // 50
    println(shape2.area());      // 147

    // ポインタベースも併用可能（ハイブリッド）
    Shape* shape_ptr = &rect;
    println(shape_ptr->area());  // 50

    return 0;
}
```

## 実装手順

### 1. HIRノードの拡張

**ファイル**: `src/backend/ir/hir/hir_node.h`

HIRInterfaceに値型フラグを追加：
```cpp
struct HIRInterface {
    // 既存フィールド
    std::string name;
    std::vector<MethodSignature> methods;
    std::vector<std::string> generic_params;
    SourceLocation location;

    // 新規フィールド
    bool generate_value_type = true;  // デフォルトで値型も生成
};
```

### 2. C++コード生成の拡張

**ファイル**: `src/backend/codegen/hir_to_cpp.cpp`

#### 2.1 Interface生成の拡張

`generate_interfaces()` 関数を修正：

```cpp
void HIRToCpp::generate_interfaces(const std::vector<HIRInterface> &interfaces) {
    for (const auto &interface : interfaces) {
        // 既存のポインタベースinterface生成
        generate_pointer_interface(interface);

        // 値型interface生成
        if (interface.generate_value_type) {
            generate_value_interface(interface);
        }
    }
}
```

#### 2.2 ポインタベースInterface生成

```cpp
void HIRToCpp::generate_pointer_interface(const HIRInterface &interface) {
    // 既存の実装（純粋仮想関数を持つ抽象クラス）
    emit_line("// Interface (pointer-based): " + interface.name);

    if (!interface.generic_params.empty()) {
        emit("template<");
        for (size_t i = 0; i < interface.generic_params.size(); i++) {
            if (i > 0) emit(", ");
            emit("typename " + interface.generic_params[i]);
        }
        emit(">\n");
    }

    emit_line("class " + interface.name + " {");
    emit_line("public:");
    increase_indent();

    emit_line("virtual ~" + interface.name + "() = default;");
    emit_line("");

    for (const auto &method : interface.methods) {
        emit("virtual ");
        emit(generate_type(method.return_type));
        emit(" " + method.name + "(");

        for (size_t i = 0; i < method.parameters.size(); i++) {
            if (i > 0) emit(", ");
            const auto &param = method.parameters[i];
            if (param.is_const) emit("const ");
            emit(generate_type(param.type));
            emit(" " + param.name);
        }

        emit(") = 0;\n");
    }

    decrease_indent();
    emit_line("};");
    emit_line("");
}
```

#### 2.3 値型Interface生成（型消去）

```cpp
void HIRToCpp::generate_value_interface(const HIRInterface &interface) {
    std::string value_class_name = interface.name + "_Value";

    emit_line("// Interface (value-based, type erasure): " + interface.name);

    // テンプレートパラメータ
    if (!interface.generic_params.empty()) {
        emit("template<");
        for (size_t i = 0; i < interface.generic_params.size(); i++) {
            if (i > 0) emit(", ");
            emit("typename " + interface.generic_params[i]);
        }
        emit(">\n");
    }

    emit_line("class " + value_class_name + " {");
    emit_line("private:");
    increase_indent();

    // Concept（内部インターフェース）
    emit_line("struct Concept {");
    increase_indent();

    for (const auto &method : interface.methods) {
        emit("virtual ");
        emit(generate_type(method.return_type));
        emit(" " + method.name + "(");

        for (size_t i = 0; i < method.parameters.size(); i++) {
            if (i > 0) emit(", ");
            const auto &param = method.parameters[i];
            if (param.is_const) emit("const ");
            emit(generate_type(param.type));
            emit(" " + param.name);
        }

        emit(") = 0;\n");
    }

    emit_line("virtual std::unique_ptr<Concept> clone() const = 0;");
    emit_line("virtual ~Concept() = default;");

    decrease_indent();
    emit_line("};");
    emit_line("");

    // Model（テンプレート実装）
    emit_line("template<typename T>");
    emit_line("struct Model : Concept {");
    increase_indent();

    emit_line("T data;");
    emit_line("");
    emit_line("Model(T d) : data(std::move(d)) {}");
    emit_line("");

    for (const auto &method : interface.methods) {
        emit(generate_type(method.return_type));
        emit(" " + method.name + "(");

        for (size_t i = 0; i < method.parameters.size(); i++) {
            if (i > 0) emit(", ");
            const auto &param = method.parameters[i];
            if (param.is_const) emit("const ");
            emit(generate_type(param.type));
            emit(" " + param.name);
        }

        emit(") override {\n");
        increase_indent();

        emit("return data." + method.name + "(");
        for (size_t i = 0; i < method.parameters.size(); i++) {
            if (i > 0) emit(", ");
            emit(method.parameters[i].name);
        }
        emit(");\n");

        decrease_indent();
        emit_line("}");
    }

    emit_line("");
    emit_line("std::unique_ptr<Concept> clone() const override {");
    increase_indent();
    emit_line("return std::make_unique<Model<T>>(data);");
    decrease_indent();
    emit_line("}");

    decrease_indent();
    emit_line("};");
    emit_line("");

    // メンバ変数
    emit_line("std::unique_ptr<Concept> ptr_;");
    emit_line("");

    decrease_indent();
    emit_line("public:");
    increase_indent();

    // コンストラクタ
    emit_line("template<typename T>");
    emit_line(value_class_name + "(T obj)");
    increase_indent();
    emit_line(": ptr_(std::make_unique<Model<T>>(std::move(obj))) {}");
    decrease_indent();
    emit_line("");

    // コピーコンストラクタ
    emit_line(value_class_name + "(const " + value_class_name + "& other)");
    increase_indent();
    emit_line(": ptr_(other.ptr_ ? other.ptr_->clone() : nullptr) {}");
    decrease_indent();
    emit_line("");

    // ムーブコンストラクタ
    emit_line(value_class_name + "(" + value_class_name + "&& other) = default;");
    emit_line("");

    // コピー代入演算子
    emit_line(value_class_name + "& operator=(const " + value_class_name + "& other) {");
    increase_indent();
    emit_line("if (this != &other) {");
    increase_indent();
    emit_line("ptr_ = other.ptr_ ? other.ptr_->clone() : nullptr;");
    decrease_indent();
    emit_line("}");
    emit_line("return *this;");
    decrease_indent();
    emit_line("}");
    emit_line("");

    // ムーブ代入演算子
    emit_line(value_class_name + "& operator=(" + value_class_name + "&& other) = default;");
    emit_line("");

    // メソッド
    for (const auto &method : interface.methods) {
        emit(generate_type(method.return_type));
        emit(" " + method.name + "(");

        for (size_t i = 0; i < method.parameters.size(); i++) {
            if (i > 0) emit(", ");
            const auto &param = method.parameters[i];
            if (param.is_const) emit("const ");
            emit(generate_type(param.type));
            emit(" " + param.name);
        }

        emit(") {\n");
        increase_indent();

        emit("return ptr_->" + method.name + "(");
        for (size_t i = 0; i < method.parameters.size(); i++) {
            if (i > 0) emit(", ");
            emit(method.parameters[i].name);
        }
        emit(");\n");

        decrease_indent();
        emit_line("}");
    }

    decrease_indent();
    emit_line("};");
    emit_line("");
}
```

### 3. 型推論の拡張

**ファイル**: `src/backend/ir/hir/hir_generator.cpp`

変数宣言で値型interfaceを検出：

```cpp
// AST_VAR_DECL処理
if (node->type_info == TYPE_STRUCT) {
    // 型がinterfaceかチェック
    bool is_interface = false;
    for (const auto &iface : interfaces) {
        if (iface.name == node->type_name) {
            is_interface = true;
            break;
        }
    }

    if (is_interface) {
        // 値型interfaceの型を設定
        stmt.var_type.kind = HIRType::TypeKind::Interface;
        stmt.var_type.name = node->type_name + "_Value";
    }
}
```

### 4. 使用例の生成

変数宣言 `Shape shape = rect;` は次のC++コードを生成：

```cpp
Shape_Value CB_HIR_shape = Rectangle{10, 5};
```

メソッド呼び出し `shape.area()` は：

```cpp
CB_HIR_shape.area()
```

## 制約事項とトレードオフ

### メモリオーバーヘッド

- **ポインタベース**: ポインタ1つ分（8バイト）
- **値型（型消去）**: `unique_ptr` + vtable ≈ 16-24バイト

### パフォーマンス

- **ポインタベース**:
  - 間接参照1回
  - 仮想関数呼び出し1回

- **値型（型消去）**:
  - 間接参照2回（`unique_ptr` → `Concept` → 実装）
  - 仮想関数呼び出し1回

### コンパイル時間

値型interfaceはテンプレートを多用するため、コンパイル時間が増加します。

### バイナリサイズ

各具体型に対して `Model<T>` がインスタンス化されるため、バイナリサイズが増加します。

## 実装の優先度

1. **Phase 1**: ポインタベースinterface（✅ 完了）
2. **Phase 2**: 構造体リテラル（✅ 完了）
3. **Phase 3**: 値型interface（⏸ 保留）
4. **Phase 4**: ハイブリッドアプローチの最適化

## 参考資料

- [Rust Trait Objects](https://doc.rust-lang.org/book/ch17-02-trait-objects.html)
- [C++ Type Erasure](https://www.modernescpp.com/index.php/c-core-guidelines-type-erasure-with-templates)
- [Sean Parent's "Inheritance Is The Base Class of Evil"](https://www.youtube.com/watch?v=bIhUE5uUFOA)

## 実装チェックリスト

- [ ] HIRInterfaceに`generate_value_type`フラグを追加
- [ ] `generate_value_interface()`関数を実装
- [ ] 型推論で値型interfaceを検出
- [ ] 変数宣言のコード生成を拡張
- [ ] メソッド呼び出しのコード生成を拡張
- [ ] テストケースの作成
  - [ ] 単一interfaceの値型テスト
  - [ ] 複数interfaceの値型テスト
  - [ ] ポインタベースと値型の混在テスト
  - [ ] ジェネリックinterfaceの値型テスト
- [ ] パフォーマンステスト
- [ ] ドキュメント更新

## まとめ

値型interfaceは型消去パターンを使用して実装します。これにより：

✅ **利点**:
- 値型として使える直感的な構文
- スタック割り当てが可能
- Rustのtrait objectと同様のセマンティクス

❌ **欠点**:
- 実装が複雑
- パフォーマンスオーバーヘッド
- バイナリサイズの増加

現時点では、ポインタベースinterfaceが実装されており、実用上十分です。値型interfaceは将来の拡張として、必要性に応じて実装することを推奨します。
