# Day 4: 型パラメータメソッド解決の設計

## 概要

Week 1 Day 4では、型パラメータ経由でインターフェースメソッドを呼び出すための設計を行います。

## 目標構文

```cb
interface Allocator {
    void* allocate(int size);
    void deallocate(void* ptr);
}

struct SystemAllocator {};

impl Allocator for SystemAllocator {
    void* allocate(int size) {
        return malloc(size);
    }
    void deallocate(void* ptr) {
        free(ptr);
    }
}

struct Vector<T, A: Allocator> {
    T* data;
    int capacity;
    
    void resize(int new_capacity) {
        // 型パラメータAを通じてallocateメソッドを呼び出す
        void* new_data = A.allocate(sizeof(T) * new_capacity);
        //               ^^^^^^^^^^^ これを実装する
    }
}
```

## 解決プロセス

### 1. 構文解析時

`A.allocate(...)`を解析した際:

```cpp
// ASTNode構造:
// - type: AST_MEMBER_ACCESS
// - left->name: "A"  (型パラメータ名)
// - name: "allocate" (メソッド名)
// - is_type_parameter_access: true (新フラグ)
```

### 2. 型解決時

インスタンス化された構造体内で`A.allocate()`に遭遇:

```cpp
// Vector<int, SystemAllocator>の場合:
// 1. "A"が型パラメータであることを確認
// 2. 型パラメータのバインディングを取得: A -> SystemAllocator
// 3. インターフェース境界を取得: A -> Allocator
// 4. implを検索: impl Allocator for SystemAllocator
// 5. メソッドを解決: SystemAllocator::allocate (via Allocator impl)
```

### 3. 実行時

```cpp
// 静的ディスパッチで直接呼び出し
call_impl_method("SystemAllocator", "Allocator", "allocate", args);
```

## 実装計画

### Phase 1: AST拡張 (Day 4-1)

**追加フィールド**:
```cpp
// src/common/ast.h
class ASTNode {
    // ...
    bool is_type_parameter_access = false;  // A.method() 形式か
    std::string type_parameter_context;     // どの構造体/関数内か
};
```

### Phase 2: パーサー拡張 (Day 4-2)

**メンバーアクセスの識別**:
```cpp
// src/frontend/recursive_parser/parsers/expression_parser.cpp
ASTNode* parseMemberAccess() {
    // ...
    if (isTypeParameter(left->name)) {
        node->is_type_parameter_access = true;
        node->type_parameter_context = current_generic_context;
    }
}
```

### Phase 3: 型解決器 (Day 4-3)

**新ファイル**: `src/backend/interpreter/evaluator/type_parameter_resolver.cpp`

```cpp
class TypeParameterResolver {
public:
    // 型パラメータ経由のメソッド呼び出しを解決
    const ASTNode* resolve_type_parameter_method(
        const std::string& param_name,      // "A"
        const std::string& method_name,     // "allocate"
        const StructDefinition& struct_def  // Vector_int_SystemAllocator
    );
    
private:
    // 型パラメータの具体的な型を取得
    std::string get_concrete_type(
        const std::string& param_name,
        const std::map<std::string, std::string>& bindings
    );
    
    // インターフェース境界を取得
    std::string get_interface_bound(
        const std::string& param_name,
        const std::unordered_map<std::string, std::string>& bounds
    );
    
    // implメソッドを検索
    const ASTNode* find_impl_method(
        const std::string& struct_name,
        const std::string& interface_name,
        const std::string& method_name
    );
};
```

### Phase 4: 評価器統合 (Day 4-4)

```cpp
// src/backend/interpreter/evaluator/access/member.cpp
int64_t Evaluator::evaluate_member_access(const ASTNode* node) {
    if (node->is_type_parameter_access) {
        // 型パラメータ経由のメソッドアクセス
        return evaluate_type_parameter_method_call(node);
    }
    // 通常のメンバーアクセス
    // ...
}

int64_t Evaluator::evaluate_type_parameter_method_call(const ASTNode* node) {
    // 1. 現在のコンテキストから構造体定義を取得
    const StructDefinition* struct_def = get_current_struct_definition();
    
    // 2. 型パラメータを解決
    TypeParameterResolver resolver;
    const ASTNode* impl_method = resolver.resolve_type_parameter_method(
        node->left->name,  // "A"
        node->name,        // "allocate"
        *struct_def
    );
    
    // 3. implメソッドを呼び出し
    return call_method(impl_method, node->arguments);
}
```

## コンテキスト管理

### ジェネリック構造体のメソッド内

```cpp
// 現在実行中のジェネリックコンテキストを追跡
struct GenericContext {
    std::string struct_name;                                    // "Vector_int_SystemAllocator"
    std::vector<std::string> type_parameters;                  // ["T", "A"]
    std::map<std::string, std::string> type_bindings;          // {"T": "int", "A": "SystemAllocator"}
    std::unordered_map<std::string, std::string> interface_bounds;  // {"A": "Allocator"}
};

// Interpreterにスタックとして保持
std::vector<GenericContext> generic_context_stack_;
```

## テストケース

### Test 1: 基本的なメソッド呼び出し

```cb
interface Allocator {
    void* allocate(int size);
}

struct SystemAllocator {};

impl Allocator for SystemAllocator {
    void* allocate(int size) {
        println("allocate(%d)", size);
        return nullptr;
    }
}

struct Box<T, A: Allocator> {
    void create() {
        void* ptr = A.allocate(sizeof(T));  // 型パラメータ経由
    }
}

void main() {
    Box<int, SystemAllocator> box;
    box.create();  // "allocate(4)" が出力されるはず
}
```

### Test 2: 複数の型パラメータ

```cb
interface Allocator {
    void* allocate(int size);
}

interface Logger {
    void log(int message);
}

struct DebugAllocator {};

impl Allocator for DebugAllocator {
    void* allocate(int size) {
        return nullptr;
    }
}

impl Logger for DebugAllocator {
    void log(int message) {
        println("log: %d", message);
    }
}

struct Container<T, A: Allocator, L: Logger> {
    void setup() {
        L.log(100);           // Logger経由
        void* p = A.allocate(10);  // Allocator経由
    }
}

void main() {
    Container<int, DebugAllocator, DebugAllocator> c;
    c.setup();
}
```

### Test 3: エラーケース

```cb
struct NoLogger {};

// impl Logger for NoLogger がない

struct Bad<T, L: Logger> {
    void test() {
        L.log(1);  // エラー: NoLoggerはLoggerを実装していない
    }
}

void main() {
    Bad<int, NoLogger> b;  // 型チェックでエラー
}
```

## 実装の優先順位

Week 1の残り時間を考慮して、以下の順序で実装:

### 優先度: HIGH (Day 4で実装)
- ✅ 設計文書の作成 (このファイル)
- ⚪ AST拡張: `is_type_parameter_access`フラグ
- ⚪ テストケースの作成

### 優先度: MEDIUM (Week 2で実装)
- パーサー拡張: 型パラメータアクセスの識別
- TypeParameterResolver の実装
- 評価器統合

### 優先度: LOW (Week 3で実装)
- ジェネリック関数での型パラメータメソッド呼び出し
- 最適化: キャッシング

## 制約事項

### Week 1での制約

1. **構造体メソッドは未サポート**: Cbはまだ構造体メソッド(`struct { void method() {} }`)をサポートしていない
2. **implメソッドのみ**: impl定義内のメソッドのみ呼び出し可能
3. **静的コンテキストのみ**: インスタンスメソッドではなく静的メソッドとして実装

### 回避策

Week 1では以下のアプローチで検証:

```cb
// 構造体メソッドの代わりにグローバル関数を使用
void vector_resize<T, A: Allocator>(int capacity) {
    // 将来的にA.allocate()を呼び出す
}
```

## 次のステップ (Week 2以降)

1. **Week 2 Day 1-2**: TypeParameterResolver実装
2. **Week 2 Day 3-4**: 評価器統合とテスト
3. **Week 2 Day 5**: 最適化とドキュメント更新
4. **Week 3**: Vector<T, A>の完全実装

## 参考実装

### Rust風の実装イメージ

```rust
// Rustでは以下のように書ける:
struct Vec<T, A: Allocator> {
    fn resize(&mut self, new_cap: usize) {
        let ptr = A::alloc(new_cap * size_of::<T>());
        //        ^^^^^^^^ 型パラメータ経由の関連関数呼び出し
    }
}
```

### C++風の実装イメージ

```cpp
// C++では以下のように書ける:
template<typename T, typename A>
    requires Allocator<A>
struct Vec {
    void resize(size_t new_cap) {
        void* ptr = A::allocate(new_cap * sizeof(T));
        //          ^^^^^^^^^^^^ テンプレートパラメータ経由の静的メソッド呼び出し
    }
};
```

---

**文書作成日**: 2025/10/27  
**作成者**: Week 1 Day 4 実装チーム  
**ステータス**: 設計完了、実装準備中
