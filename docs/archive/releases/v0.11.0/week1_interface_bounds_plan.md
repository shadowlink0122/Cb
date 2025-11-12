# Week 1: インターフェース境界の実装

**期間**: 2025年10月28日 - 11月1日（5日間）  
**目標**: ジェネリクス型パラメータにインターフェース境界を追加

---

## 📋 現状確認

### ✅ 既存の実装

1. **interface/impl機能** (完全実装済み)
   ```cb
   interface PriorityQueue {
       void push(int node_id, int distance);
       int pop();
   }
   
   impl PriorityQueue for MinHeapPQ {
       void push(int node_id, int distance) { /* 実装 */ }
       int pop() { /* 実装 */ }
   }
   ```

2. **ジェネリクス機能** (v0.11.0 Part 1で実装済み)
   ```cb
   struct Box<T> {
       T value;
   }
   ```
   - `type_parameters` フィールド存在
   - `type_arguments` フィールド存在

### 🆕 実装が必要な機能

**インターフェース境界** - 型パラメータにインターフェース制約を追加
```cb
struct Vector<T, A: Allocator> {
    //          ^^^^^^^^^^^^^ これを実装する
    T* data;
    int length;
}
```

---

## 🎯 実装ステップ

### Day 1: AST拡張（2025/10/28）

#### 1-1. ASTNode構造体の拡張

**ファイル**: `src/common/ast.h`

```cpp
// 既存のASTNode構造体に追加
class ASTNode {
    // ... 既存のフィールド ...
    
    // ジェネリクス関連（v0.11.0既存）
    bool is_generic = false;
    std::vector<std::string> type_parameters;     // 既存
    std::vector<std::string> type_arguments;       // 既存
    
    // 🆕 インターフェース境界（v0.11.0 Phase 1追加）
    std::unordered_map<std::string, std::string> interface_bounds;
    // 例: {"A" => "Allocator", "B" => "Iterator"}
    // type_parameters[i] に対するinterface制約
};
```

**変更内容**:
- `interface_bounds` フィールド追加
- 型パラメータ名 → インターフェース名のマッピング

#### 1-2. 構造体定義の例

実装後のASTイメージ:
```
struct Vector<T, A: Allocator> { ... }

↓ ASTNode

node->type_parameters = ["T", "A"]
node->interface_bounds = {
    {"A", "Allocator"}
}
```

---

### Day 2: パーサー拡張（2025/10/29）

#### 2-1. 型パラメータのパース拡張

**ファイル**: `src/frontend/recursive_parser/parsers/struct_parser.cpp`

**現在の実装**:
```cpp
// struct Box<T> のパース
if (current_token_.type == TOK_LT) {
    // 型パラメータをパース
    type_parameters.push_back(current_token_.value);
}
```

**拡張後**:
```cpp
// struct Vector<T, A: Allocator> のパース
if (current_token_.type == TOK_LT) {
    advance();
    
    while (true) {
        // 型パラメータ名
        std::string param_name = current_token_.value;
        advance();
        
        type_parameters.push_back(param_name);
        
        // インターフェース境界のチェック
        if (current_token_.type == TOK_COLON) {
            advance();
            std::string interface_name = current_token_.value;
            advance();
            
            // interface_boundsに追加
            interface_bounds[param_name] = interface_name;
        }
        
        if (current_token_.type != TOK_COMMA) break;
        advance();
    }
    
    expect(TOK_GT);
}
```

#### 2-2. 対象ファイル

- `src/frontend/recursive_parser/parsers/struct_parser.cpp`
  - `parseStructDeclaration()` 修正
- `src/frontend/recursive_parser/parsers/enum_parser.cpp`
  - `parseEnumDeclaration()` 修正（将来的に）

---

### Day 3: インターフェース境界の検証（2025/10/30）

#### 3-1. 型チェックの実装

**ファイル**: `src/backend/interpreter/evaluator/type_checker.cpp` (新規作成)

```cpp
bool TypeChecker::check_interface_bound(
    const std::string& type_name,
    const std::string& interface_name,
    const std::vector<ImplDefinition>& impl_definitions
) {
    // 型がインターフェースを実装しているかチェック
    for (const auto& impl_def : impl_definitions) {
        if (impl_def.struct_name == type_name &&
            impl_def.interface_name == interface_name) {
            return true;  // 実装が見つかった
        }
    }
    
    return false;  // 実装が見つからない
}
```

#### 3-2. ジェネリック型の実体化時にチェック

```cpp
// Vector<int, SystemAllocator> の実体化時

// 型引数 = ["int", "SystemAllocator"]
// interface_bounds = {{"A", "Allocator"}}

for (const auto& [param_name, interface_name] : interface_bounds) {
    // "A" に対応する型引数 "SystemAllocator" を取得
    std::string concrete_type = get_type_argument_for_parameter(param_name);
    
    // SystemAllocator が Allocator を実装しているかチェック
    if (!check_interface_bound(concrete_type, interface_name, impl_definitions_)) {
        throw std::runtime_error(
            "Type '" + concrete_type + 
            "' does not implement interface '" + interface_name + "'"
        );
    }
}
```

---

### Day 4: 型パラメータ経由のメソッド呼び出し（2025/10/31）

#### 4-1. 静的ディスパッチの実装

**問題**:
```cb
struct Vector<T, A: Allocator> {
    void resize() {
        T* new_data = A.allocate(sizeof(T) * capacity);
        //            ^^^^^^^^^^^ これを解決する必要がある
    }
}
```

**解決策**:
```cpp
// A.allocate() の解析時

// 1. "A" が型パラメータであることを確認
if (is_type_parameter("A")) {
    // 2. "A" のインターフェース境界を取得
    std::string interface_name = get_interface_bound("A");  // "Allocator"
    
    // 3. Vector<int, SystemAllocator> の場合、"A" = "SystemAllocator"
    std::string concrete_type = get_concrete_type_for_parameter("A");
    
    // 4. impl Allocator for SystemAllocator から allocate メソッドを探す
    ASTNode* method = find_impl_method(concrete_type, interface_name, "allocate");
    
    // 5. メソッドを呼び出す（静的ディスパッチ）
    return call_method(method, args);
}
```

#### 4-2. メソッド解決の実装

**ファイル**: `src/backend/interpreter/evaluator/method_resolver.cpp` (新規)

```cpp
ASTNode* MethodResolver::resolve_interface_method(
    const std::string& type_parameter_name,    // "A"
    const std::string& method_name,             // "allocate"
    const std::unordered_map<std::string, std::string>& type_param_map,  // {"A" => "SystemAllocator"}
    const std::unordered_map<std::string, std::string>& interface_bounds, // {"A" => "Allocator"}
    const std::vector<ImplDefinition>& impl_definitions
) {
    // 1. 型パラメータの実際の型を取得
    auto it = type_param_map.find(type_parameter_name);
    if (it == type_param_map.end()) {
        throw std::runtime_error("Type parameter '" + type_parameter_name + "' not found");
    }
    std::string concrete_type = it->second;  // "SystemAllocator"
    
    // 2. インターフェース境界を取得
    auto bound_it = interface_bounds.find(type_parameter_name);
    if (bound_it == interface_bounds.end()) {
        throw std::runtime_error("No interface bound for type parameter '" + type_parameter_name + "'");
    }
    std::string interface_name = bound_it->second;  // "Allocator"
    
    // 3. impl定義を探す
    for (const auto& impl_def : impl_definitions) {
        if (impl_def.struct_name == concrete_type &&
            impl_def.interface_name == interface_name) {
            // 4. メソッドを探す
            for (const auto* method : impl_def.methods) {
                if (method->name == method_name) {
                    return method;
                }
            }
        }
    }
    
    throw std::runtime_error(
        "Method '" + method_name + "' not found in interface '" + 
        interface_name + "' for type '" + concrete_type + "'"
    );
}
```

---

### Day 5: テストとデバッグ（2025/11/1）

#### 5-1. 基本テスト

**test_interface_bound_basic.cb**:
```cb
interface Allocator {
    void* allocate(int size);
    void deallocate(void* ptr);
}

struct SystemAllocator {}

impl Allocator for SystemAllocator {
    void* allocate(int size) {
        return malloc(size);
    }
    
    void deallocate(void* ptr) {
        free(ptr);
    }
}

struct Box<T, A: Allocator> {
    T value;
}

int main() {
    Box<int, SystemAllocator> box;
    box.value = 42;
    println("OK");
    return 0;
}
```

#### 5-2. メソッド呼び出しテスト

**test_interface_method_call.cb**:
```cb
// ... 上記のinterface/implは同じ ...

struct Container<T, A: Allocator> {
    T* data;
}

impl Container<T, A> {
    void allocate_data() {
        self.data = A.allocate(sizeof(T));  // 静的ディスパッチ
    }
    
    void free_data() {
        A.deallocate(self.data);
    }
}

int main() {
    Container<int, SystemAllocator> c;
    c.allocate_data();
    c.free_data();
    println("OK");
    return 0;
}
```

#### 5-3. エラーチェックテスト

**test_interface_bound_error.cb**:
```cb
interface Allocator {
    void* allocate(int size);
}

struct SystemAllocator {}
// ★ Allocatorを実装していない

struct Box<T, A: Allocator> {
    T value;
}

int main() {
    // エラー: SystemAllocator は Allocator を実装していない
    Box<int, SystemAllocator> box;
    return 0;
}
```

期待されるエラーメッセージ:
```
Error: Type 'SystemAllocator' does not implement interface 'Allocator'
```

---

## 📊 進捗チェックリスト

### Day 1: AST拡張
- [ ] `interface_bounds` フィールド追加
- [ ] コンストラクタ修正
- [ ] コンパイル確認

### Day 2: パーサー拡張
- [ ] struct_parser.cpp 修正
- [ ] `<T, A: Allocator>` 構文のパース
- [ ] パースのテスト

### Day 3: 型チェック
- [ ] TypeChecker クラス作成
- [ ] `check_interface_bound()` 実装
- [ ] ジェネリック実体化時のチェック

### Day 4: メソッド解決
- [ ] MethodResolver クラス作成
- [ ] `resolve_interface_method()` 実装
- [ ] 静的ディスパッチの実装

### Day 5: テスト
- [ ] 10個のテストケース作成
- [ ] すべてのテストがパス
- [ ] エラーメッセージの確認

---

## 🎯 完了基準

1. ✅ `struct Vector<T, A: Allocator>` 構文がパースできる
2. ✅ インターフェース境界がASTに保存される
3. ✅ 型引数がインターフェースを実装していない場合エラー
4. ✅ `A.allocate()` のような型パラメータ経由のメソッド呼び出しが動作
5. ✅ 10個のテストがすべてパス

---

## 📁 作成・修正するファイル

### 修正
- `src/common/ast.h` - `interface_bounds` フィールド追加
- `src/frontend/recursive_parser/parsers/struct_parser.cpp` - パース拡張

### 新規作成
- `src/backend/interpreter/evaluator/type_checker.cpp`
- `src/backend/interpreter/evaluator/type_checker.h`
- `src/backend/interpreter/evaluator/method_resolver.cpp`
- `src/backend/interpreter/evaluator/method_resolver.h`

### テスト
- `tests/cases/interface_bounds/test_*.cb` (10ファイル)

---

**作成者**: GitHub Copilot  
**レビュアー**: shadowlink0122  
**最終更新**: 2025年10月27日  
**ブランチ**: feature/trait-allocator  
**ステータス**: 実装準備完了
