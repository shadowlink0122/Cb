# Queue Generic Support Implementation Plan

**Status**: Planning  
**Date**: 2025-01-11  
**Target Version**: v0.12.0  

## Overview

現在、`Queue`はint型に特化した実装になっていますが、将来的には任意の型（構造体、インターフェース、typedefなど）をサポートするジェネリック`Queue<T>`に拡張する必要があります。

## Current Limitation

### 1. Interface Parser Does Not Support Generics

現在のinterface parserは以下の構文をサポートしていません:

```cb
// ❌ 現在サポートされていない
export interface QueueOps<T> {
    void enqueue(T value);
    T dequeue();
}
```

エラーメッセージ:
```
error: Expected '{' after interface name
export interface QueueOps<T> {
                         ^
```

### 2. Current Workaround

現在は以下のようにint型に特化した実装を使用しています:

```cb
// ✅ 現在の実装
export interface QueueOps {
    void enqueue(int value);
    int dequeue();
}
```

## Implementation Plan

### Phase 1: Interface Parser Generic Support

**Priority**: High  
**Effort**: Medium  

#### Required Changes

1. **InterfaceParser::parseInterfaceDeclaration()** の拡張
   - struct parserと同様のジェネリクス構文解析を追加
   - `<T>`, `<T, U>` などの型パラメータをサポート
   - interface定義にtype_paramsフィールドを追加

   ```cpp
   // src/frontend/recursive_parser/parsers/interface_parser.cpp
   ASTNode *InterfaceParser::parseInterfaceDeclaration() {
       parser_->consume(TokenType::TOK_INTERFACE, "Expected 'interface'");
       
       std::string interface_name = parser_->current_token_.value;
       parser_->advance();
       
       // NEW: ジェネリクス型パラメータの解析
       std::vector<std::string> type_params;
       if (parser_->check(TokenType::TOK_LT)) {
           type_params = parseTypeParameters();
       }
       
       parser_->consume(TokenType::TOK_LBRACE,
                        "Expected '{' after interface name");
       // ... 残りの実装
   }
   ```

2. **InterfaceDefinition** の拡張
   - type_paramsフィールドを追加
   - ジェネリック型の検証ロジック

3. **Interpreter側の対応**
   - interface登録時にtype_paramsを保持
   - impl宣言時の型パラメータマッチング検証

### Phase 2: Queue<T> Implementation

**Priority**: High  
**Effort**: Low  

#### Required Changes

1. **stdlib/collections/queue.cb** の更新
   
   ```cb
   // 構造体定義をジェネリック化
   export struct Queue<T> {
       int capacity;
       int length;
       int front;
       int rear;
       void* data;  // T型配列
   };
   
   // インターフェースをジェネリック化
   export interface QueueOps<T> {
       void init(int initial_capacity);
       void enqueue(T value);
       T dequeue();
       T peek();
       // ... その他のメソッド
   }
   ```

2. **型特殊化の実装**
   
   ```cb
   // int型特殊化
   impl QueueOps<int> for Queue<int> {
       void enqueue(int value) {
           array_set_int(self.data, self.rear, value);
           // ...
       }
       
       int dequeue() {
           int value = array_get_int(self.data, self.front);
           // ...
           return value;
       }
   }
   
   // string型特殊化
   impl QueueOps<string> for Queue<string> {
       void enqueue(string value) {
           array_set_string(self.data, self.rear, value);
           // ...
       }
       
       string dequeue() {
           string value = array_get_string(self.data, self.front);
           // ...
           return value;
       }
   }
   
   // 構造体型特殊化（汎用版）
   // TODO: 真のジェネリクス実装が必要
   ```

### Phase 3: Auto Type Inference

**Priority**: Medium  
**Effort**: High  

#### Motivation

現在、dequeue()の戻り値型を明示的に指定する必要があります:

```cb
Queue q;
int val = q.dequeue();  // ✅ 現在
```

これは不便で、特にジェネリック型の場合、型推論がないと冗長になります:

```cb
Queue<Point> point_queue;
Point p = point_queue.dequeue();  // 型を明示する必要がある
```

#### Proposed Syntax

```cb
Queue<Point> point_queue;
auto p = point_queue.dequeue();  // 型推論で自動的にPointと推論
```

#### Implementation Requirements

1. **Lexer**: `auto`キーワードの追加
   - TokenType::TOK_AUTO

2. **Parser**: auto型宣言の解析
   - variable declaration parserでautoを認識
   - 右辺の式から型を推論するためのプレースホルダーを設定

3. **Interpreter**: 型推論エンジン
   - 右辺の式を評価して型を決定
   - ジェネリック型のインスタンス化情報から型を抽出
   - 変数に正しい型を割り当て

4. **Type System**: 型推論ロジック
   - メソッド呼び出しの戻り値型を追跡
   - ジェネリック型パラメータの具体化を解決
   - ネストしたジェネリック型のサポート（例: `Queue<Vector<int, SystemAllocator>>`）

#### Example Implementation

```cpp
// src/backend/interpreter/core/type_inference.cpp

TypeInfo Interpreter::inferTypeFromExpression(ASTNode* expr) {
    if (expr->type == NODE_MEMBER_ACCESS) {
        // メソッド呼び出しの場合
        if (expr->isMethodCall()) {
            // レシーバーの型を取得
            TypeInfo receiver_type = getReceiverType(expr);
            
            // メソッドの戻り値型を取得
            std::string method_name = expr->method_name;
            MethodSignature sig = lookupMethod(receiver_type, method_name);
            
            // ジェネリック型の場合、具体化された型を返す
            if (receiver_type.isGeneric()) {
                return resolveGenericReturnType(sig, receiver_type);
            }
            
            return sig.return_type;
        }
    }
    
    // その他の式タイプ...
    return TYPE_UNKNOWN;
}
```

### Phase 4: True Generic Implementation

**Priority**: Low  
**Effort**: Very High  

#### Two Approaches

##### Approach A: Template Expansion (C++ Style)

- コンパイル時に各型特殊化のコードを生成
- パフォーマンスは最高だが、コードサイズが増加
- 実装難易度: 高

##### Approach B: Type Erasure (Java Style)

- ランタイムで型情報を保持
- void*とメタデータを使用
- パフォーマンスは少し低下するが、コードサイズは小さい
- 実装難易度: 中

#### Recommended Approach

**Type Erasure with Runtime Type Information**を推奨:

```cb
// 内部実装（ユーザーには見えない）
struct GenericQueue {
    void* data;
    TypeInfo element_type;  // ランタイム型情報
    
    void enqueue_generic(void* value) {
        // 型情報に基づいて適切な操作を実行
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

## Testing Plan

### Phase 1 Tests

- [ ] Interface parser can parse `interface Name<T>`
- [ ] Interface parser can parse `interface Name<T, U>`
- [ ] Interface parser can parse generic method signatures
- [ ] Interpreter correctly registers generic interface definitions
- [ ] impl declarations can match generic interfaces

### Phase 2 Tests

- [ ] `Queue<int>` works correctly
- [ ] `Queue<string>` works correctly
- [ ] `Queue<Point>` works with custom structs
- [ ] Destructor correctly frees memory for all types
- [ ] Resize works for all types

### Phase 3 Tests

- [ ] `auto val = queue.dequeue()` infers correct type
- [ ] `auto` works with nested generics
- [ ] Error handling for unresolvable types
- [ ] Type checking still works with auto

### Phase 4 Tests

- [ ] Generic implementation works for any type
- [ ] Performance benchmarks (vs specialized versions)
- [ ] Memory safety tests
- [ ] Edge cases (recursive types, etc.)

## Migration Path

### Step 1: Add Interface Generic Support
- Update parser
- Update interpreter
- All existing code continues to work

### Step 2: Migrate Queue to Generic
- Add `Queue<int>` as the first specialization
- Update all tests to use `Queue<int>`
- Deprecate old `Queue` (but keep for backward compatibility)

### Step 3: Add Auto Type Inference
- Add `auto` keyword
- Update type inference engine
- Optional feature - users can still use explicit types

### Step 4: Implement True Generics
- Choose implementation strategy
- Implement runtime type system
- Migrate all specialized implementations to unified generic implementation

## Timeline

- **Phase 1**: 2-3 weeks
  - Parser changes: 1 week
  - Interpreter changes: 1 week
  - Testing: 3-5 days

- **Phase 2**: 1 week
  - Queue<T> migration: 2-3 days
  - Type specializations: 2-3 days
  - Testing: 2 days

- **Phase 3**: 3-4 weeks
  - Lexer/Parser changes: 3-5 days
  - Type inference engine: 2 weeks
  - Testing & debugging: 1 week

- **Phase 4**: 6-8 weeks
  - Design review: 1 week
  - Implementation: 4-5 weeks
  - Testing & optimization: 2 weeks

## Related Documents

- [Generic Collections Design](generic_collections_design.md) (To be created)
- [Auto Type Inference](auto_type_inference.md) (To be created)
- [Vector Implementation](../features/void_ptr_summary.md)
- [Interface System](../BNF.md)

## Current Status

- ✅ Queue works correctly for int type
- ✅ All stdlib tests pass (25/25)
- ✅ Integration tests pass (3463/3463)
- ❌ Interface generic syntax not supported in parser
- ❌ Auto type inference not implemented
- ❌ True generic implementation not started

## Notes

1. **Priority**: Interface generic supportを最優先
   - これがないとQueue<T>の適切な実装ができない
   - 他のコレクション（Stack, Set, Map等）も同じ問題を抱える

2. **Backward Compatibility**: 
   - 既存の`Queue`（int特化版）は維持
   - 新しい`Queue<T>`と共存可能
   - マイグレーションは段階的に実施

3. **User Experience**:
   - auto型推論があることでジェネリクスの使い勝手が大幅に向上
   - しかし、必須ではない（明示的な型宣言も可能）

4. **Performance Considerations**:
   - Type erasure approach でも十分なパフォーマンス
   - 重要な場合は特殊化版を提供可能（例: Queue<int>の最適化版）
