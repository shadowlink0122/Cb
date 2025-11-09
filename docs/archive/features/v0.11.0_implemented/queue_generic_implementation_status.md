# Queue Generic Support - Current Status and Future Plan

**Date**: 2025-01-11  
**Status**: ✅ Current Implementation Complete, 📋 Future Plan Documented  

## Summary

Queueライブラリのジェネリクス対応について調査・実装を行いました。現在はint型特化版として実装されており、完全なジェネリック対応は将来の機能拡張として計画されています。

## Current Implementation

### ✅ Completed

1. **Queue (int specialization)**
   - File: `stdlib/collections/queue.cb`
   - Type: `Queue` (int型特化版)
   - Features:
     * 動的メモリ管理（new/delete）
     * 循環バッファ
     * 自動リサイズ
     * デストラクタによる自動メモリ解放
   - All tests passing: ✅

2. **Test Coverage**
   - Integration tests: 3463/3463 ✅
   - Stdlib tests (C++): 25/25 ✅
   - Stdlib tests (Cb): 4/4 ✅
   - Files:
     * `tests/cases/stdlib/collections/test_queue_import.cb`
     * `tests/cases/stdlib/collections/test_simple_import.cb`
     * `tests/cases/stdlib/collections/test_selective_import.cb`
     * `tests/cases/stdlib/collections/test_advanced_selective.cb`
     * `tests/stdlib/collections/test_queue.hpp`

3. **Documentation**
   - ✅ Implementation plan: `docs/todo/queue_generic_support.md`
   - ✅ Auto type inference design: `docs/features/auto_type_inference.md`
   - ✅ Code comments with future generics notes

## Technical Limitation

### Interface Parser Does Not Support Generics

**Root Cause**: 
```cpp
// src/frontend/recursive_parser/parsers/interface_parser.cpp
ASTNode *InterfaceParser::parseInterfaceDeclaration() {
    parser_->consume(TokenType::TOK_INTERFACE, "Expected 'interface'");
    
    std::string interface_name = parser_->current_token_.value;
    parser_->advance();
    
    // ❌ No support for <T> here
    parser_->consume(TokenType::TOK_LBRACE,
                     "Expected '{' after interface name");
    // ...
}
```

**Error When Attempting Generics**:
```cb
// ❌ This fails to parse
export interface QueueOps<T> {
    void enqueue(T value);
    T dequeue();
}
```

Error message:
```
error: Expected '{' after interface name
export interface QueueOps<T> {
                         ^
```

**Current Workaround**:
```cb
// ✅ Works (int specialization)
export interface QueueOps {
    void enqueue(int value);
    int dequeue();
}
```

## Future Roadmap

### Phase 1: Interface Generic Support (Priority: High)

**Effort**: Medium (2-3 weeks)

#### Changes Required

1. **Parser Updates**
   - Add generic syntax parsing to `InterfaceParser`
   - Similar to `StructParser` generic support
   - Parse `<T>`, `<T, U>`, etc.

2. **AST Updates**
   - Add `type_params` field to `InterfaceDefinition`
   - Store generic type parameter names

3. **Interpreter Updates**
   - Register generic interfaces
   - Match impl declarations with generic interface signatures
   - Validate type parameter usage

#### Expected Outcome

```cb
// ✅ After Phase 1
export interface QueueOps<T> {
    void enqueue(T value);
    T dequeue();
    T peek();
}
```

### Phase 2: Queue<T> Generic Implementation (Priority: High)

**Effort**: Low (1 week)

#### Changes Required

1. **Queue Definition**
   ```cb
   export struct Queue<T> {
       int capacity;
       int length;
       int front;
       int rear;
       void* data;  // T型配列
   };
   ```

2. **Type Specializations**
   ```cb
   // int型特殊化
   impl QueueOps<int> for Queue<int> {
       void enqueue(int value) { /* int用の実装 */ }
       int dequeue() { /* int用の実装 */ }
   }
   
   // string型特殊化
   impl QueueOps<string> for Queue<string> {
       void enqueue(string value) { /* string用の実装 */ }
       string dequeue() { /* string用の実装 */ }
   }
   
   // 構造体型特殊化
   impl QueueOps<Point> for Queue<Point> {
       void enqueue(Point value) { /* Point用の実装 */ }
       Point dequeue() { /* Point用の実装 */ }
   }
   ```

3. **Test Updates**
   - Update all test files to use `Queue<int>`
   - Add tests for `Queue<string>`
   - Add tests for `Queue<struct>`

#### Expected Outcome

```cb
// ✅ After Phase 2
Queue<int> int_queue;
Queue<string> str_queue;
Queue<Point> point_queue;
```

### Phase 3: Auto Type Inference (Priority: Medium)

**Effort**: High (3-4 weeks)

#### Motivation

現在の問題:
```cb
Queue<Point> queue;
Point p = queue.dequeue();  // 型を繰り返し書く必要がある
```

改善後:
```cb
Queue<Point> queue;
auto p = queue.dequeue();  // 型推論で自動的にPointと推論
```

#### Changes Required

1. **Lexer**
   - Add `auto` keyword (TokenType::TOK_AUTO)

2. **Parser**
   - Parse `auto variable_name = expression;`
   - Create AUTO_VARIABLE_DECLARATION node

3. **Type Inference Engine**
   - Infer type from literal expressions
   - Infer type from function/method return types
   - Resolve generic type parameters
   - Handle nested generics

4. **Interpreter**
   - Declare variable with inferred type
   - Validate type consistency
   - Error handling for unresolvable types

#### Expected Outcome

```cb
// ✅ After Phase 3
Queue<int> int_queue;
auto val = int_queue.dequeue();  // int

Vector<int, SystemAllocator> vec;
auto item = vec.get(0);  // int

Queue<Vector<int, SystemAllocator>> nested;
auto vec2 = nested.dequeue();  // Vector<int, SystemAllocator>
```

### Phase 4: True Generic Implementation (Priority: Low)

**Effort**: Very High (6-8 weeks)

#### Two Approaches

**A. Template Expansion (C++ Style)**
- Pros: Best performance
- Cons: Code size increase, complex implementation

**B. Type Erasure (Java Style)**
- Pros: Simpler implementation, smaller code size
- Cons: Slight performance overhead

#### Recommended: Type Erasure with Runtime Type Info

```cb
// Internal implementation (hidden from users)
struct GenericQueue {
    void* data;
    TypeInfo element_type;  // Runtime type information
    
    void enqueue_generic(void* value) {
        // Dispatch based on type info
        switch (element_type.base_type) {
            case TYPE_INT:
                array_set_int(data, rear, *(int*)value);
                break;
            case TYPE_STRING:
                array_set_string(data, rear, (string*)value);
                break;
            case TYPE_STRUCT:
                memcpy(array_get_ptr(data, rear), value, element_type.size);
                break;
        }
    }
}
```

## Testing Strategy

### Regression Tests

All existing tests continue to pass:
- ✅ Integration tests: 3463/3463
- ✅ Stdlib tests (C++): 25/25
- ✅ Stdlib tests (Cb): 4/4

### New Tests (After Each Phase)

**Phase 1: Interface Generics**
- [ ] Parse `interface Name<T>`
- [ ] Parse `interface Name<T, U>`
- [ ] Parse generic method signatures
- [ ] Register generic interface definitions
- [ ] Match impl with generic interface

**Phase 2: Queue<T>**
- [ ] `Queue<int>` works
- [ ] `Queue<string>` works
- [ ] `Queue<Point>` works (custom struct)
- [ ] Destructor works for all types
- [ ] Resize works for all types

**Phase 3: Auto Type Inference**
- [ ] `auto` with literals
- [ ] `auto` with function returns
- [ ] `auto` with method returns
- [ ] `auto` with generic types
- [ ] Error handling (missing init, type mismatch, etc.)

**Phase 4: True Generics**
- [ ] Generic implementation works for any type
- [ ] Performance benchmarks
- [ ] Memory safety tests
- [ ] Edge cases (recursive types, etc.)

## Migration Strategy

### Backward Compatibility

現在の`Queue`（int特化版）は将来も維持されます:

1. **Immediate** (現在):
   ```cb
   Queue q;  // int特化版
   ```

2. **After Phase 1-2** (v0.12.0):
   ```cb
   Queue q;            // ✅ 引き続き動作（int特化版）
   Queue<int> q2;      // ✅ 新しい構文（同じ動作）
   Queue<string> q3;   // ✅ string版も利用可能
   ```

3. **After Phase 3** (v0.13.0):
   ```cb
   Queue q;
   auto val = q.dequeue();  // ✅ auto推論サポート
   ```

4. **After Phase 4** (v0.14.0):
   ```cb
   Queue<T> q;  // ✅ 真のジェネリクス
   ```

### Deprecation Timeline

- **v0.11.0**: 現在のQueue（int特化版）
- **v0.12.0**: Queue<T>サポート追加、旧Queueは非推奨警告
- **v0.13.0**: auto推論サポート
- **v0.14.0**: 真のジェネリクス、旧Queueは削除

## Related Documents

- [Queue Generic Support Implementation Plan](../todo/queue_generic_support.md)
- [Auto Type Inference Design](../features/auto_type_inference.md)
- [Vector Generic Implementation](../features/void_ptr_summary.md)
- [Interface System](../BNF.md)

## Lessons Learned

### What Worked Well

1. **Int Specialization Approach**
   - 現在の実装は完全に動作
   - テストカバレッジは十分
   - ユーザーは今すぐQueueを使用可能

2. **Documentation First**
   - 将来の計画を明確にドキュメント化
   - 実装の優先順位が明確
   - チーム（またはコミュニティ）が貢献しやすい

3. **Incremental Development**
   - 段階的なアプローチで複雑さを管理
   - 各フェーズで独立してテスト可能
   - ユーザーは早期に機能を利用開始

### Challenges Encountered

1. **Interface Parser Limitation**
   - ジェネリクスサポートがないことを発見
   - 回避策として型特化版を実装
   - 将来の拡張性を確保

2. **Auto Type Inference Complexity**
   - ジェネリック型からの型推論は非自明
   - 慎重な設計が必要
   - C++やRustの実装を参考にする

3. **True Generics Trade-offs**
   - パフォーマンス vs 実装の複雑さ
   - Type erasureアプローチを選択
   - ベンチマークで検証が必要

## User Experience Goals

### Before (Current)

```cb
// ✅ 現在のQueue（int特化版）
Queue q;
q.init(5);
q.enqueue(10);
int val = q.dequeue();
```

### After Phase 1-2 (Queue<T>)

```cb
// ✅ 型安全なQueue
Queue<int> int_queue;
Queue<string> str_queue;
Queue<Point> point_queue;

int_queue.enqueue(10);
str_queue.enqueue("hello");
point_queue.enqueue(Point{10, 20});
```

### After Phase 3 (Auto)

```cb
// ✅ 簡潔な型推論
Queue<int> queue;
queue.enqueue(100);
auto val = queue.dequeue();  // 型推論で自動的にint
```

### After Phase 4 (True Generics)

```cb
// ✅ 完全なジェネリクス
Queue<T> create_queue<T>() {
    Queue<T> q;
    q.init(10);
    return q;
}

auto int_queue = create_queue<int>();
auto str_queue = create_queue<string>();
```

## Conclusion

現在の`Queue`実装は完全に動作し、全てのテストがパスしています。将来のジェネリクス対応とauto型推論の計画も明確に文書化されています。段階的なアプローチにより、ユーザーは今すぐQueueを使用でき、将来的にはより強力なジェネリック機能を利用できるようになります。

**Next Steps**:
1. Interface parserのジェネリクスサポートを実装（Phase 1）
2. Queue<T>に移行（Phase 2）
3. Auto型推論を実装（Phase 3）
4. 真のジェネリクス実装を検討（Phase 4）
