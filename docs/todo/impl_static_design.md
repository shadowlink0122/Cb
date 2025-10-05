# impl内static変数 実装設計書

**作成日**: 2025年10月5日  
**対象バージョン**: v0.10.0  
**担当**: Cb言語開発チーム

---

## 🎯 目的

implブロック内でstatic変数を宣言・使用できるようにし、Interface/Implシステムの表現力を向上させる。

---

## 📋 要求仕様

### 基本要件

1. **implブロック内でのstatic変数宣言**
   ```c++
   impl Interface for Struct {
       static int counter = 0;
       
       void method() {
           counter++;
       }
   }
   ```

2. **スコープ規則**
   - 同じ`impl Interface for StructType`内のすべてのメソッドで共有
   - 異なる`impl Interface for StructType`では独立（構造体型が異なる場合）
   - 同じStructTypeの複数インスタンス間で共有

3. **名前空間**
   ```
   impl::InterfaceName::StructTypeName::variable_name
   
   例:
   impl::Shape::Circle::instance_count
   impl::Shape::Rectangle::instance_count
   // ↑ これらは別々のstatic変数
   ```

4. **初期化**
   - 最初のアクセス時に一度だけ初期化
   - 初期化式がある場合はその値で、なければデフォルト値（0, "", など）

5. **型サポート**
   - プリミティブ型: `int`, `long`, `float`, `double`, `char`, `string`, `bool`
   - unsigned指定可能
   - const指定可能: `static const int MAX = 100;`

---

## 🏗️ 設計概要

### データ構造

#### 1. static変数の格納先

現在の`Interpreter`クラスの`static_variables`を拡張：

```cpp
class Interpreter {
private:
    // 既存: 関数内static変数
    std::map<std::string, Variable> static_variables;
    
    // 新規: impl内static変数
    std::map<std::string, Variable> impl_static_variables;
    
    // キー形式: "impl::InterfaceName::StructTypeName::variable_name"
};
```

#### 2. 名前空間生成ロジック

```cpp
std::string generate_impl_static_key(
    const std::string& interface_name,
    const std::string& struct_type_name,
    const std::string& variable_name
) {
    return "impl::" + interface_name + "::" + struct_type_name + "::" + variable_name;
}
```

#### 3. 現在のimplコンテキストの追跡

implメソッド実行中に、どのimpl定義内にいるかを追跡する必要がある：

```cpp
class Interpreter {
private:
    // 現在実行中のimplコンテキスト
    struct ImplContext {
        std::string interface_name;
        std::string struct_type_name;
        bool is_active = false;
    };
    
    ImplContext current_impl_context_;
    
public:
    void enter_impl_context(const std::string& interface_name,
                           const std::string& struct_type_name) {
        current_impl_context_.interface_name = interface_name;
        current_impl_context_.struct_type_name = struct_type_name;
        current_impl_context_.is_active = true;
    }
    
    void exit_impl_context() {
        current_impl_context_.is_active = false;
    }
    
    bool is_in_impl_context() const {
        return current_impl_context_.is_active;
    }
};
```

---

## 🔧 実装手順

### Phase 1: パーサー拡張

#### 1.1 構文解析の修正

**ファイル**: `src/frontend/recursive_parser/recursive_parser.cpp`

implブロック内でstatic変数宣言を認識：

```cpp
std::unique_ptr<ASTNode> RecursiveParser::parseImplDeclaration() {
    // ... 既存のコード ...
    
    // メソッド宣言のループ内で
    while (current_token_index < tokens.size()) {
        // static変数宣言のチェックを追加
        if (peek_token().type == TokenType::TOK_STATIC) {
            auto static_var_node = parseStaticVariableDeclaration();
            static_var_node->is_impl_static = true;  // 新しいフラグ
            impl_node->impl_static_variables.push_back(std::move(static_var_node));
            continue;
        }
        
        // メソッド宣言の既存処理
        // ...
    }
}
```

#### 1.2 ASTNodeの拡張

**ファイル**: `src/common/ast.h`

```cpp
struct ASTNode {
    // ... 既存のメンバー ...
    
    // impl内static変数用の新しいフラグ
    bool is_impl_static = false;
    
    // impl定義ノード用
    std::vector<std::unique_ptr<ASTNode>> impl_static_variables;
};
```

### Phase 2: インタープリター拡張

#### 2.1 impl_static_variables領域の追加

**ファイル**: `src/backend/interpreter/core/interpreter.h`

```cpp
class Interpreter {
private:
    // 既存
    std::map<std::string, Variable> static_variables;  // 関数内static
    
    // 新規
    std::map<std::string, Variable> impl_static_variables_;  // impl内static
    
    // implコンテキスト追跡
    struct ImplContext {
        std::string interface_name;
        std::string struct_type_name;
        bool is_active = false;
    } current_impl_context_;
    
public:
    // impl内static変数の操作
    Variable* find_impl_static_variable(const std::string& var_name);
    void create_impl_static_variable(const std::string& var_name, 
                                     const ASTNode* node);
    
    // implコンテキストの管理
    void enter_impl_context(const std::string& interface_name,
                           const std::string& struct_type_name);
    void exit_impl_context();
    bool is_in_impl_context() const;
    std::string get_current_impl_key() const;
};
```

#### 2.2 impl内static変数の作成

**ファイル**: `src/backend/interpreter/core/interpreter.cpp`

```cpp
void Interpreter::create_impl_static_variable(const std::string& var_name,
                                             const ASTNode* node) {
    if (!is_in_impl_context()) {
        throw std::runtime_error("Static variable can only be declared inside impl block");
    }
    
    // impl専用のキーを生成
    std::string impl_key = "impl::" + 
                          current_impl_context_.interface_name + "::" +
                          current_impl_context_.struct_type_name + "::" +
                          var_name;
    
    // 既に存在する場合はスキップ（初期化は一度だけ）
    if (impl_static_variables_.find(impl_key) != impl_static_variables_.end()) {
        return;
    }
    
    // Variable作成
    Variable var;
    var.type = node->type_info;
    var.is_const = node->is_const;
    var.is_assigned = false;
    
    // デフォルト値設定
    if (var.type == TYPE_STRING) {
        var.str_value = "";
    } else {
        var.value = 0;
    }
    
    // 初期化式があれば評価
    if (node->init_expr) {
        if (var.type == TYPE_STRING && 
            node->init_expr->node_type == ASTNodeType::AST_STRING_LITERAL) {
            var.str_value = node->init_expr->str_value;
        } else {
            var.value = evaluate(node->init_expr.get());
        }
        var.is_assigned = true;
    }
    
    impl_static_variables_[impl_key] = var;
    
    if (debug_mode) {
        std::cerr << "Created impl static variable: " << impl_key 
                  << " (type=" << static_cast<int>(var.type) << ")" << std::endl;
    }
}

Variable* Interpreter::find_impl_static_variable(const std::string& var_name) {
    if (!is_in_impl_context()) {
        return nullptr;
    }
    
    std::string impl_key = "impl::" + 
                          current_impl_context_.interface_name + "::" +
                          current_impl_context_.struct_type_name + "::" +
                          var_name;
    
    auto it = impl_static_variables_.find(impl_key);
    if (it != impl_static_variables_.end()) {
        return &it->second;
    }
    
    return nullptr;
}

void Interpreter::enter_impl_context(const std::string& interface_name,
                                     const std::string& struct_type_name) {
    current_impl_context_.interface_name = interface_name;
    current_impl_context_.struct_type_name = struct_type_name;
    current_impl_context_.is_active = true;
    
    if (debug_mode) {
        std::cerr << "Entered impl context: " << interface_name 
                  << " for " << struct_type_name << std::endl;
    }
}

void Interpreter::exit_impl_context() {
    current_impl_context_.is_active = false;
    
    if (debug_mode) {
        std::cerr << "Exited impl context" << std::endl;
    }
}

bool Interpreter::is_in_impl_context() const {
    return current_impl_context_.is_active;
}

std::string Interpreter::get_current_impl_key() const {
    if (!is_in_impl_context()) {
        return "";
    }
    return "impl::" + current_impl_context_.interface_name + "::" +
           current_impl_context_.struct_type_name;
}
```

#### 2.3 impl定義の処理時にstatic変数を登録

**ファイル**: `src/backend/interpreter/core/interpreter.cpp`

```cpp
void Interpreter::execute_statement(const ASTNode* node) {
    // ... 既存のコード ...
    
    case ASTNodeType::AST_IMPL_DECL: {
        // impl定義の処理
        std::string interface_name = node->interface_name;
        std::string struct_type_name = node->struct_type_name;
        
        // impl内のstatic変数を登録
        for (const auto& static_var : node->impl_static_variables) {
            // implコンテキストを一時的に設定
            enter_impl_context(interface_name, struct_type_name);
            create_impl_static_variable(static_var->name, static_var.get());
            exit_impl_context();
        }
        
        // implメソッドの登録（既存の処理）
        ImplDefinition impl_def;
        impl_def.interface_name = interface_name;
        impl_def.struct_name = struct_type_name;
        // ... 既存の処理 ...
        
        break;
    }
}
```

#### 2.4 implメソッド実行時のコンテキスト設定

**ファイル**: `src/backend/interpreter/evaluator/expression_evaluator.cpp`

implメソッド呼び出し時にコンテキストを設定：

```cpp
TypedValue ExpressionEvaluator::evaluate_typed_expression(const ASTNode* node) {
    // ... 既存のコード ...
    
    case ASTNodeType::AST_METHOD_CALL: {
        // ... selfの解決など既存の処理 ...
        
        // implメソッド呼び出しの場合、コンテキストを設定
        if (/* implメソッドの判定 */) {
            std::string interface_name = /* interface名を取得 */;
            std::string struct_type = /* struct型を取得 */;
            
            interpreter_->enter_impl_context(interface_name, struct_type);
            
            // メソッド実行
            try {
                // ... メソッド本体の実行 ...
            } catch (...) {
                interpreter_->exit_impl_context();
                throw;
            }
            
            interpreter_->exit_impl_context();
        }
        
        break;
    }
}
```

#### 2.5 変数検索の拡張

**ファイル**: `src/backend/interpreter/managers/variable_manager.cpp`

```cpp
Variable* VariableManager::find_variable(const std::string& name) {
    // 1. ローカルスコープから検索（既存）
    // ...
    
    // 2. グローバルスコープから検索（既存）
    // ...
    
    // 3. 関数内static変数から検索（既存）
    Variable* static_var = interpreter_->find_static_variable(name);
    if (static_var) {
        return static_var;
    }
    
    // 4. impl内static変数から検索（新規）
    Variable* impl_static_var = interpreter_->find_impl_static_variable(name);
    if (impl_static_var) {
        return impl_static_var;
    }
    
    return nullptr;
}
```

### Phase 3: テストケースの作成

#### 3.1 基本的なimpl static変数テスト

**ファイル**: `tests/cases/impl_static/test_impl_static_basic.cb`

```c++
struct Counter {
    int value;
};

interface Helper {
    void increment();
    int get_total();
};

impl Helper for Counter {
    static int shared_counter = 0;
    
    void increment() {
        self.value++;
        shared_counter++;
    }
    
    int get_total() {
        return shared_counter;
    }
};

int main() {
    Counter c1 = {value: 0};
    Counter c2 = {value: 0};
    Counter c3 = {value: 0};
    
    c1.increment();  // shared_counter = 1
    c2.increment();  // shared_counter = 2
    c3.increment();  // shared_counter = 3
    
    println("c1.value:", c1.value);        // 1
    println("c2.value:", c2.value);        // 1
    println("c3.value:", c3.value);        // 1
    println("Total:", c1.get_total());     // 3
    println("Total:", c2.get_total());     // 3 (同じstatic変数を参照)
    
    return 0;
}
```

**期待される出力**:
```
c1.value: 1
c2.value: 1
c3.value: 1
Total: 3
Total: 3
```

#### 3.2 型ごとに独立したstatic変数テスト

**ファイル**: `tests/cases/impl_static/test_impl_static_separate.cb`

```c++
struct Circle {
    int radius;
};

struct Rectangle {
    int width;
    int height;
};

interface Shape {
    void register_instance();
    int get_count();
};

impl Shape for Circle {
    static int instance_count = 0;
    
    void register_instance() {
        instance_count++;
    }
    
    int get_count() {
        return instance_count;
    }
};

impl Shape for Rectangle {
    static int instance_count = 0;  // Circleとは別のstatic変数
    
    void register_instance() {
        instance_count++;
    }
    
    int get_count() {
        return instance_count;
    }
};

int main() {
    Circle c1 = {radius: 10};
    Circle c2 = {radius: 20};
    Rectangle r1 = {width: 30, height: 40};
    
    c1.register_instance();  // Circle::instance_count = 1
    c2.register_instance();  // Circle::instance_count = 2
    r1.register_instance();  // Rectangle::instance_count = 1
    
    println("Circle count:", c1.get_count());       // 2
    println("Circle count:", c2.get_count());       // 2
    println("Rectangle count:", r1.get_count());    // 1
    
    return 0;
}
```

**期待される出力**:
```
Circle count: 2
Circle count: 2
Rectangle count: 1
```

#### 3.3 static const組み合わせテスト

**ファイル**: `tests/cases/impl_static/test_impl_static_const.cb`

```c++
struct Config {
    int value;
};

interface Settings {
    int get_max();
    void check_limit();
};

impl Settings for Config {
    static const int MAX_VALUE = 100;
    static int access_count = 0;
    
    int get_max() {
        access_count++;
        return MAX_VALUE;
    }
    
    void check_limit() {
        if (self.value > MAX_VALUE) {
            println("Error: Value exceeds maximum");
        } else {
            println("Value is within limit");
        }
    }
};

int main() {
    Config cfg1 = {value: 50};
    Config cfg2 = {value: 150};
    
    println("Max:", cfg1.get_max());  // 100
    cfg1.check_limit();               // "Value is within limit"
    cfg2.check_limit();               // "Error: Value exceeds maximum"
    
    println("Max:", cfg2.get_max());  // 100
    
    return 0;
}
```

#### 3.4 複数の型サポートテスト

**ファイル**: `tests/cases/impl_static/test_impl_static_types.cb`

```c++
struct Stats {
    int id;
};

interface Tracker {
    void record();
    void display();
};

impl Tracker for Stats {
    static int count = 0;
    static long total = 0;
    static float average = 0.0f;
    static string name = "Statistics";
    
    void record() {
        count++;
        total = total + self.id;
        average = total / count;
    }
    
    void display() {
        println("Name:", name);
        println("Count:", count);
        println("Total:", total);
        println("Average:", average);
    }
};

int main() {
    Stats s1 = {id: 10};
    Stats s2 = {id: 20};
    Stats s3 = {id: 30};
    
    s1.record();
    s2.record();
    s3.record();
    
    s1.display();
    
    return 0;
}
```

**期待される出力**:
```
Name: Statistics
Count: 3
Total: 60
Average: 20
```

---

## 📊 実装スケジュール

| Phase | 作業内容 | 見積もり時間 | 担当 |
|-------|---------|-------------|------|
| Phase 1 | パーサー拡張 | 2-3時間 | フロントエンド |
| Phase 2.1-2.3 | インタープリター基礎実装 | 3-4時間 | バックエンド |
| Phase 2.4-2.5 | コンテキスト管理・変数検索 | 2-3時間 | バックエンド |
| Phase 3 | テストケース作成・検証 | 2-3時間 | QA |
| **合計** | - | **9-13時間** | - |

---

## 🧪 テスト計画

### 単体テスト

1. **パーサーテスト**
   - impl内でのstatic宣言の正しい解析
   - ASTノードの正しい構造
   - エラーケース（impl外でのstatic宣言）

2. **インタープリターテスト**
   - static変数の作成と初期化
   - 名前空間の正しい生成
   - 変数の検索と取得
   - コンテキストの管理

### 統合テスト

1. **基本機能テスト** (test_impl_static_basic.cb)
2. **型ごとの独立性テスト** (test_impl_static_separate.cb)
3. **static const組み合わせテスト** (test_impl_static_const.cb)
4. **複数型サポートテスト** (test_impl_static_types.cb)

### エッジケーステスト

1. 同じinterfaceで異なるstructに対するimpl
2. 初期化なしのstatic変数
3. static変数への複合代入（`+=`, `-=`, など）
4. static変数のprintln出力

---

## 🚨 既知の制限事項

1. **配列型のstatic変数**: v0.10.0では未サポート
   ```c++
   static int[10] arr;  // ❌ 未サポート
   ```

2. **構造体型のstatic変数**: v0.10.0では未サポート
   ```c++
   static Point origin;  // ❌ 未サポート
   ```

3. **動的初期化**: 初期化式は定数式のみサポート
   ```c++
   static int x = some_function();  // ❌ 未サポート
   ```

これらは将来のバージョンで対応予定。

---

## 📝 次のステップ

1. ✅ 設計書のレビュー・承認
2. ⏳ Phase 1の実装開始（パーサー拡張）
3. ⏳ Phase 2の実装（インタープリター拡張）
4. ⏳ Phase 3の実装（テストケース）
5. ⏳ 統合テスト・デバッグ
6. ⏳ ドキュメント更新
7. ⏳ v0.10.0リリース

---

**設計書バージョン**: 1.0  
**最終更新**: 2025年10月5日
