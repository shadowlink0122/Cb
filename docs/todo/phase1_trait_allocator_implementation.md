# Phase 1: Interface ベースアロケータの実装

**作成日**: 2025年10月27日  
**ブランチ**: feature/trait-allocator  
**対象**: v0.11.0 Phase 1 - メモリ管理基盤  
**優先度**: 🔴 最優先（すべての機能の基礎）

---

## 📋 概要

既存の **interface/impl 機能** を使って、アロケータシステムと動的配列（Vector<T>）を実装します。これは、Cb言語のメモリ管理とコレクションの基盤となります。

**重要**: トレイトは実装しません。既存の `interface/impl` 機能を使用します。

---

## 🎯 実装する機能

### Phase 1-1: インターフェース境界（Interface Bounds）（1週間）

**既存の機能を確認・拡張**:
1. ✅ `interface` 定義構文（既存）
2. ✅ `impl Interface for Type` 構文（既存）
3. 🆕 **インターフェース境界**: `struct Vector<T, A: Allocator>`
4. 🆕 **静的ディスパッチ**: `A.allocate()` の解決

**必要な拡張**:
- ジェネリクス型パラメータにインターフェース境界を追加
- コンパイル時のインターフェース実装チェック
- 型パラメータ経由のメソッド呼び出し

---

### Phase 1-2: アロケータ実装（1週間）

1. **Allocatorインターフェース**
   - `std/core/allocator.cb`
   - allocate/deallocate メソッド定義

2. **SystemAllocator**
   - `std/allocator/system.cb`
   - malloc/freeのラッパー
   - OS環境用

3. **BumpAllocator**
   - `std/allocator/bump.cb`
   - バンプポインタ方式
   - ベアメタル環境用

4. **GlobalAllocator**
   - デフォルトアロケータの設定
   - cb_config.jsonで切り替え

---

### Phase 1-3: Vector<T, A>実装（1週間）

1. **Vector構造体**
   - `std/collections/vector.cb`
   - `Vector<T, A: Allocator>`
   - data, length, capacity

2. **基本メソッド**
   - new(), push(), pop()
   - operator[](int index)
   - length(), capacity()

3. **メモリ管理**
   - 自動リサイズ
   - デストラクタで自動解放
   - コピー/ムーブセマンティクス

4. **型エイリアス**
   - `using Vec<T> = Vector<T, GlobalAllocator>`

---

## 📁 実装するファイル

### 言語機能（コンパイラ）- インターフェース境界の拡張

```
src/
├── common/
│   └── ast.h                    # AST拡張
│       - interface定義（既存）
│       - impl定義（既存）
│       - 🆕 INTERFACE_BOUND    # インターフェース境界
│
├── frontend/
│   └── recursive_parser/
│       └── parsers/
│           └── type_parser.cpp  # 型パーサー拡張
│               - 🆕 インターフェース境界のパース
│               - 例: <T, A: Allocator>
│
└── backend/
    └── interpreter/
        └── evaluator/
            └── interface_evaluator.cpp  # インターフェース評価
                - 🆕 インターフェース境界チェック
                - 🆕 型パラメータ経由のメソッド呼び出し
```

### 標準ライブラリ（新規作成）

```
stdlib/
├── core/
│   └── allocator.cb             # Allocatorインターフェース定義
│
├── allocator/
│   ├── system.cb                # SystemAllocator
│   ├── bump.cb                  # BumpAllocator
│   └── global.cb                # GlobalAllocator設定
│
└── collections/
    └── vector.cb                # Vector<T, A>
```

---

## 🔧 実装詳細

### 1. 既存のinterface/impl機能の確認

```cb
// sample/dijkstra_struct.cb から確認

interface PriorityQueue {
    void push(int node_id, int distance);
    int pop();
    bool is_empty();
    int size();
    void clear();
}

struct MinHeapPQ {
    int dummy;
}

impl PriorityQueue for MinHeapPQ {
    void push(int node_id, int distance) {
        // 実装
    }
    
    int pop() {
        // 実装
    }
    
    // ... その他のメソッド
}
```

**確認事項**:
- ✅ interface定義は動作している
- ✅ impl ... for ... は動作している
- 🆕 インターフェース境界 `<A: Interface>` が必要

### 2. インターフェース境界のパース（新規実装）

```cpp
// src/frontend/recursive_parser/parsers/type_parser.cpp

ParsedTypeInfo TypeParser::parse_generic_type() {
    // Vector<T, A: Allocator>
    //          ^^^^^^^^^^^^^ インターフェース境界
    
    ParsedTypeInfo type;
    type.base_type = current_token().value;
    advance();
    
    if (current_token().type == TOK_LT) {
        advance();
        
        while (true) {
            std::string param_name = current_token().value;
            advance();
            
            GenericParam param;
            param.name = param_name;
            
            // インターフェース境界のチェック
            if (current_token().type == TOK_COLON) {
                advance();
                param.interface_bound = current_token().value;
                advance();
            }
            
            type.generic_params.push_back(param);
            
            if (current_token().type != TOK_COMMA) break;
            advance();
        }
        
        expect(TOK_GT);
    }
    
    return type;
}
```

### 3. インターフェース境界の検証

```cpp
// src/backend/interpreter/evaluator/interface_evaluator.cpp

bool InterfaceEvaluator::check_interface_bounds(
    const ParsedTypeInfo& type,
    const std::string& interface_name
) {
    // 型がインターフェースを実装しているかチェック
    
    // インターフェース実装テーブルを検索
    auto key = std::make_pair(type.base_type, interface_name);
    if (interface_impl_table_.find(key) == interface_impl_table_.end()) {
        throw std::runtime_error(
            "Type '" + type.base_type + 
            "' does not implement interface '" + interface_name + "'"
        );
    }
    
    return true;
}

ASTNode* InterfaceEvaluator::resolve_interface_method(
    const std::string& type_name,
    const std::string& interface_name,
    const std::string& method_name
) {
    // インターフェースメソッドを解決（静的ディスパッチ）
    auto key = std::make_pair(type_name, interface_name);
    auto& methods = interface_impl_table_[key];
    
    for (auto* method : methods) {
        if (method->name == method_name) {
            return method;
        }
    }
    
    return nullptr;
}
```

---

### 3. Vector<T, A>の実装

```cb
// stdlib/collections/vector.cb

import std.core.allocator;

struct Vector<T, A: Allocator> {
    T* data;
    int length;
    int capacity;
}

impl Vector<T, A> {
    Vector<T, A> new() {
        Vector<T, A> v;
        v.data = nullptr;
        v.length = 0;
        v.capacity = 0;
        return v;
    }
    
    void push(T value) {
        if (self.length >= self.capacity) {
            self.resize();
        }
        self.data[self.length] = value;
        self.length = self.length + 1;
    }
    
    T pop() {
        if (self.length == 0) {
            // エラー処理
            panic("pop from empty vector");
        }
        self.length = self.length - 1;
        return self.data[self.length];
    }
    
    T operator[](int index) {
        if (index < 0 || index >= self.length) {
            panic("index out of bounds");
        }
        return self.data[index];
    }
    
    int length() {
        return self.length;
    }
    
    int capacity() {
        return self.capacity;
    }
    
    bool is_empty() {
        return self.length == 0;
    }
    
    void resize() {
        int new_capacity = self.capacity == 0 ? 4 : self.capacity * 2;
        
        // Aはアロケータ型（静的ディスパッチ）
        T* new_data = A.allocate(sizeof(T) * new_capacity);
        
        // 既存データをコピー
        for (int i = 0; i < self.length; i = i + 1) {
            new_data[i] = self.data[i];
        }
        
        if (self.data != nullptr) {
            A.deallocate(self.data);
        }
        
        self.data = new_data;
        self.capacity = new_capacity;
    }
    
    ~Vector() {
        // デストラクタ: 自動的にメモリ解放
        if (self.data != nullptr) {
            A.deallocate(self.data);
            self.data = nullptr;
        }
    }
}
```

---

### 4. アロケータ実装

#### Allocatorトレイト

```cb
// stdlib/core/allocator.cb

trait Allocator {
    void* allocate(int size);
    void deallocate(void* ptr);
}
```

#### SystemAllocator

```cb
// stdlib/allocator/system.cb

import std.core.allocator;

struct SystemAllocator {}

impl Allocator for SystemAllocator {
    void* allocate(int size) {
        return malloc(size);
    }
    
    void deallocate(void* ptr) {
        free(ptr);
    }
}
```

#### BumpAllocator

```cb
// stdlib/allocator/bump.cb

import std.core.allocator;

struct BumpAllocator {
    static char* heap_start;
    static char* heap_end;
    static char* current;
}

impl BumpAllocator {
    void init(char* heap, int size) {
        BumpAllocator.heap_start = heap;
        BumpAllocator.heap_end = heap + size;
        BumpAllocator.current = heap;
    }
    
    void reset() {
        BumpAllocator.current = BumpAllocator.heap_start;
    }
}

impl Allocator for BumpAllocator {
    void* allocate(int size) {
        int aligned_size = (size + 7) & ~7;
        char* ptr = BumpAllocator.current;
        char* new_current = ptr + aligned_size;
        
        if (new_current > BumpAllocator.heap_end) {
            panic("Out of memory");
        }
        
        BumpAllocator.current = new_current;
        return ptr;
    }
    
    void deallocate(void* ptr) {
        // バンプアロケータは個別解放しない
    }
}
```

---

## 🧪 テスト計画

### Phase 1-1: インターフェース境界（10テスト）

1. **test_interface_bound_basic.cb** - 基本的なインターフェース境界
2. **test_interface_bound_check.cb** - 境界チェック
3. **test_interface_method_call.cb** - インターフェースメソッド呼び出し
4. **test_interface_multiple_impl.cb** - 複数のインターフェース実装
5. **test_interface_generic.cb** - ジェネリクス + インターフェース
6. **test_interface_bound_error.cb** - インターフェース境界エラー検出
7. **test_interface_static_dispatch.cb** - 静的ディスパッチ検証
8. **test_interface_complex.cb** - 複雑なインターフェース使用
9. **test_interface_inheritance.cb** - インターフェース継承（将来）
10. **test_interface_nested.cb** - ネストしたインターフェース境界

### Phase 1-2: アロケータ（10テスト）

11. **test_allocator_trait.cb** - Allocatorトレイト
12. **test_system_allocator.cb** - SystemAllocator
13. **test_bump_allocator.cb** - BumpAllocator
14. **test_allocator_switch.cb** - アロケータ切り替え
15. **test_allocator_custom.cb** - カスタムアロケータ
16. **test_allocator_stress.cb** - ストレステスト
17. **test_allocator_oom.cb** - メモリ不足ハンドリング
18. **test_allocator_alignment.cb** - アラインメント
19. **test_allocator_leak.cb** - メモリリーク検出
20. **test_allocator_bare_metal.cb** - ベアメタル環境

### Phase 1-3: Vector<T, A>（15テスト）

21. **test_vector_basic.cb** - 基本的な使用
22. **test_vector_push_pop.cb** - push/pop
23. **test_vector_index.cb** - インデックスアクセス
24. **test_vector_resize.cb** - 自動リサイズ
25. **test_vector_destructor.cb** - デストラクタ
26. **test_vector_system_alloc.cb** - SystemAllocator使用
27. **test_vector_bump_alloc.cb** - BumpAllocator使用
28. **test_vector_generic.cb** - ジェネリック型
29. **test_vector_struct.cb** - 構造体の格納
30. **test_vector_nested.cb** - ネストしたVector
31. **test_vector_empty.cb** - 空のVector操作
32. **test_vector_large.cb** - 大量データ
33. **test_vector_copy.cb** - コピー
34. **test_vector_move.cb** - ムーブ
35. **test_vector_type_alias.cb** - 型エイリアス

---

## 📅 実装スケジュール（3週間）

### Week 1: インターフェース境界（5日）

**Day 1-2: パーサー拡張**
- [ ] インターフェース境界のパース (`<A: Allocator>`)
- [ ] GenericParam構造体拡張（interface_boundフィールド追加）
- [ ] 型パラメータの検証

**Day 3-4: インターフェース境界チェック**
- [ ] インターフェース実装テーブルの確認
- [ ] 型パラメータ経由のメソッド呼び出し解決
- [ ] 静的ディスパッチの実装

**Day 5: テスト**
- [ ] 10個のテスト作成・実行
- [ ] 既存のinterface/implテストとの互換性確認

---

### Week 2: アロケータ（5日）

**Day 6-7: Allocatorトレイト + SystemAllocator**
- [ ] std/core/allocator.cb
- [ ] std/allocator/system.cb
- [ ] 基本テスト

**Day 8-9: BumpAllocator + GlobalAllocator**
- [ ] std/allocator/bump.cb
- [ ] GlobalAllocator設定
- [ ] ベアメタルテスト

**Day 10: テスト**
- [ ] 10個のテスト作成・実行

---

### Week 3: Vector<T, A>（5日）

**Day 11-13: Vector実装**
- [ ] Vector<T, A>構造体
- [ ] 基本メソッド (new, push, pop, [])
- [ ] resize実装
- [ ] デストラクタ

**Day 14: 型エイリアス**
- [ ] using Vec<T> = Vector<T, GlobalAllocator>
- [ ] 簡潔な使用例

**Day 15: テストと統合**
- [ ] 15個のテスト作成・実行
- [ ] ドキュメント更新

---

## 🎯 完了基準

### 必須要件
1. ✅ インターフェース境界構文が動作する (`<A: Allocator>`)
2. ✅ インターフェース境界チェックが機能する
3. ✅ SystemAllocator/BumpAllocatorが動作する
4. ✅ Vector<T, A>が動作する
5. ✅ 35個のテストがすべてパス
6. ✅ OS環境とベアメタル環境の両方で動作
7. ✅ 既存のinterface/implテストに影響なし

### 次のステップへの準備
- ✅ Queue<T, A>を実装できる
- ✅ Stack<T, A>を実装できる
- ✅ HashMap<K, V, A>を実装できる
- ✅ イベントループをVectorベースで実装できる

---

## 📊 依存関係

```
Phase 1-1: トレイト機能
    ↓
Phase 1-2: アロケータ実装
    ↓
Phase 1-3: Vector<T, A>実装
    ↓
Phase 2: Queue<T, A>実装
    ↓
Phase 3: イベントループ実装
```

---

**作成者**: GitHub Copilot  
**レビュアー**: shadowlink0122  
**最終更新**: 2025年10月27日  
**ブランチ**: feature/trait-allocator  
**ステータス**: 実装準備完了  
**期間**: 3週間（2025/10/28 - 2025/11/17）
