# Auto Type Inference

**Status**: Design  
**Date**: 2025-01-11  
**Target Version**: v0.12.0  

## Overview

C++やRust、C#のような現代的な言語にある`auto`（またはそれに相当する機能）による型推論をCbに追加します。これにより、特にジェネリック型を使用する際のコードの冗長性を大幅に削減できます。

## Motivation

### Problem: Verbose Type Declarations

現在、変数宣言時に型を明示的に指定する必要があります:

```cb
// 冗長な型宣言
Queue q;
int value = q.dequeue();

Vector<int, SystemAllocator> vec;
vec.push(10);
int item = vec.get(0);
```

ジェネリック型の場合、さらに冗長になります:

```cb
// 将来のQueue<T>での例
Queue<Point> point_queue;
Point p = point_queue.dequeue();  // Point型を繰り返し書く必要がある

Queue<Vector<int, SystemAllocator>> nested_queue;
Vector<int, SystemAllocator> vec = nested_queue.dequeue();  // 非常に長い！
```

### Solution: Auto Type Inference

`auto`キーワードで右辺の式から型を推論:

```cb
Queue q;
auto value = q.dequeue();  // int と推論

Vector<int, SystemAllocator> vec;
auto item = vec.get(0);  // int と推論
```

ジェネリック型でもシンプルに:

```cb
Queue<Point> point_queue;
auto p = point_queue.dequeue();  // Point と推論

Queue<Vector<int, SystemAllocator>> nested_queue;
auto vec = nested_queue.dequeue();  // Vector<int, SystemAllocator> と推論
```

## Design

### Syntax

```cb
auto variable_name = expression;
```

- `auto`: 新しいキーワード（予約語として追加）
- `variable_name`: 変数名
- `expression`: 型推論の対象となる式

### Constraints

1. **初期化必須**: autoは必ず初期化式が必要

   ```cb
   auto x = 10;     // ✅ OK
   auto y;          // ❌ エラー: auto requires initialization
   ```

2. **単一宣言のみ**: 複数変数の同時宣言は不可

   ```cb
   auto x = 10, y = 20;  // ❌ エラー: multiple auto declarations not supported
   ```

3. **再代入での型変更不可**: 型は宣言時に固定

   ```cb
   auto x = 10;    // int と推論
   x = 20;         // ✅ OK (int → int)
   x = "hello";    // ❌ エラー: type mismatch (int vs string)
   ```

4. **推論不可能な場合はエラー**:

   ```cb
   auto x = unknown_function();  // ❌ エラー: cannot infer type
   ```

### Type Inference Rules

#### Rule 1: Literal Types

リテラルから直接型を推論:

```cb
auto i = 42;           // int
auto f = 3.14;         // float
auto s = "hello";      // string
auto b = true;         // bool
auto p = nullptr;      // void*
```

#### Rule 2: Variable Types

変数の型をそのまま継承:

```cb
int x = 10;
auto y = x;    // int

Point p;
auto q = p;    // Point
```

#### Rule 3: Function Return Types

関数の戻り値型から推論:

```cb
int get_value() { return 42; }
auto val = get_value();  // int

string get_name() { return "Cb"; }
auto name = get_name();  // string
```

#### Rule 4: Method Return Types

メソッドの戻り値型から推論（ジェネリック対応）:

```cb
Vector<int, SystemAllocator> vec;
auto item = vec.get(0);  // int

Queue<Point> queue;
auto p = queue.dequeue();  // Point
```

#### Rule 5: Generic Instantiation

ジェネリック型のインスタンスから推論:

```cb
Vector<int, SystemAllocator> vec;
auto vec2 = vec;  // Vector<int, SystemAllocator>
```

#### Rule 6: Expression Types

式の評価結果の型から推論:

```cb
auto sum = 10 + 20;           // int
auto product = 3.5 * 2.0;     // float
auto concat = "Hello" + " World";  // string（将来サポート）
```

### Type Deduction Algorithm

```
function inferType(expression):
    if expression is LITERAL:
        return literalType(expression)
    
    if expression is VARIABLE:
        return variableType(expression)
    
    if expression is FUNCTION_CALL:
        signature = lookupFunction(expression.name)
        return signature.return_type
    
    if expression is METHOD_CALL:
        receiver = expression.receiver
        receiver_type = inferType(receiver)
        
        if receiver_type.isGeneric():
            // ジェネリック型の場合、具体化された型を取得
            method_sig = lookupMethod(receiver_type, expression.method_name)
            return resolveGenericReturnType(method_sig, receiver_type)
        else:
            method_sig = lookupMethod(receiver_type, expression.method_name)
            return method_sig.return_type
    
    if expression is BINARY_OP:
        left_type = inferType(expression.left)
        right_type = inferType(expression.right)
        return commonType(left_type, right_type)
    
    return TYPE_UNKNOWN
```

## Implementation Plan

### Phase 1: Lexer & Parser

#### 1.1 Lexer Changes

**File**: `src/frontend/recursive_parser/recursive_lexer.cpp`

```cpp
// autoキーワードの追加
{"auto", TokenType::TOK_AUTO},
```

**Token Type**: 
```cpp
enum class TokenType {
    // ... existing tokens
    TOK_AUTO,
    // ...
};
```

#### 1.2 Parser Changes

**File**: `src/frontend/recursive_parser/parsers/variable_declaration_parser.cpp`

```cpp
ASTNode* VariableDeclarationParser::parseVariableDeclaration() {
    bool is_auto = false;
    std::string type_str;
    
    // autoキーワードのチェック
    if (parser_->check(TokenType::TOK_AUTO)) {
        is_auto = true;
        parser_->advance();
    } else {
        // 通常の型解析
        type_str = parser_->parseType();
    }
    
    std::string var_name = parser_->current_token_.value;
    parser_->advance();
    
    if (is_auto) {
        // auto の場合、初期化式が必須
        if (!parser_->check(TokenType::TOK_ASSIGN)) {
            parser_->error("auto requires initialization");
            return nullptr;
        }
        
        parser_->advance();  // '='をスキップ
        
        // 初期化式を解析
        ASTNode* init_expr = parser_->parseExpression();
        
        // AUTO_VARIABLE_DECLARATION ノードを作成
        return createAutoVariableDeclaration(var_name, init_expr);
    }
    
    // 通常の変数宣言処理
    // ...
}
```

**AST Node**:
```cpp
struct AutoVariableDeclaration {
    std::string variable_name;
    ASTNode* init_expression;
    TypeInfo inferred_type;  // 推論された型（Interpreter側で設定）
};
```

### Phase 2: Type Inference Engine

#### 2.1 Type Inference Core

**File**: `src/backend/interpreter/core/type_inference.cpp`

```cpp
TypeInfo TypeInferenceEngine::inferTypeFromExpression(
    Interpreter* interp, 
    ASTNode* expr
) {
    if (!expr) {
        return TYPE_UNKNOWN;
    }
    
    switch (expr->type) {
        case NODE_INTEGER_LITERAL:
            return TYPE_INT;
        
        case NODE_FLOAT_LITERAL:
            return TYPE_FLOAT;
        
        case NODE_STRING_LITERAL:
            return TYPE_STRING;
        
        case NODE_BOOL_LITERAL:
            return TYPE_BOOL;
        
        case NODE_NULLPTR_LITERAL:
            return createPointerType(TYPE_VOID);
        
        case NODE_IDENTIFIER: {
            // 変数の型を取得
            auto var = interp->lookupVariable(expr->value);
            return var.type;
        }
        
        case NODE_FUNCTION_CALL: {
            // 関数の戻り値型を取得
            auto func = interp->lookupFunction(expr->function_name);
            return func.return_type;
        }
        
        case NODE_MEMBER_ACCESS: {
            return inferMethodReturnType(interp, expr);
        }
        
        case NODE_BINARY_EXPRESSION: {
            return inferBinaryExpressionType(interp, expr);
        }
        
        // ... その他の式タイプ
        
        default:
            return TYPE_UNKNOWN;
    }
}
```

#### 2.2 Generic Type Resolution

**File**: `src/backend/interpreter/core/type_inference.cpp`

```cpp
TypeInfo TypeInferenceEngine::inferMethodReturnType(
    Interpreter* interp,
    ASTNode* member_access
) {
    // レシーバーの型を取得
    TypeInfo receiver_type = inferTypeFromExpression(
        interp, 
        member_access->receiver
    );
    
    if (receiver_type == TYPE_UNKNOWN) {
        return TYPE_UNKNOWN;
    }
    
    std::string method_name = member_access->member_name;
    
    // メソッドシグネチャを取得
    MethodSignature sig = interp->lookupMethod(receiver_type, method_name);
    
    // ジェネリック型の場合、型パラメータを解決
    if (receiver_type.is_generic) {
        return resolveGenericReturnType(sig, receiver_type);
    }
    
    return sig.return_type;
}

TypeInfo TypeInferenceEngine::resolveGenericReturnType(
    const MethodSignature& sig,
    const TypeInfo& receiver_type
) {
    // 例: Vector<int, SystemAllocator>.get(0) の場合
    //   sig.return_type = "T" (型パラメータ)
    //   receiver_type.type_args = ["int", "SystemAllocator"]
    //   -> "int" を返す
    
    if (sig.return_type.is_type_parameter) {
        // 型パラメータの位置を取得
        int param_index = findTypeParameterIndex(
            receiver_type.base_type,
            sig.return_type.name
        );
        
        if (param_index >= 0 && param_index < receiver_type.type_args.size()) {
            return receiver_type.type_args[param_index];
        }
    }
    
    return sig.return_type;
}
```

### Phase 3: Interpreter Integration

#### 3.1 Auto Variable Declaration Handler

**File**: `src/backend/interpreter/managers/variables/declaration.cpp`

```cpp
void VariableManager::declareAutoVariable(
    Interpreter* interp,
    ASTNode* auto_decl_node
) {
    std::string var_name = auto_decl_node->variable_name;
    ASTNode* init_expr = auto_decl_node->init_expression;
    
    // 初期化式から型を推論
    TypeInfo inferred_type = interp->type_inference_engine.inferTypeFromExpression(
        interp,
        init_expr
    );
    
    if (inferred_type == TYPE_UNKNOWN) {
        interp->error(
            "Cannot infer type for variable '" + var_name + "'"
        );
        return;
    }
    
    // 推論された型で変数を宣言
    interp->declareVariable(var_name, inferred_type);
    
    // 初期化式を評価して値を設定
    Value init_value = interp->evaluateExpression(init_expr);
    interp->assignVariable(var_name, init_value);
    
    // デバッグ出力
    if (interp->debug_mode) {
        std::cout << "[Auto] Variable '" << var_name 
                  << "' inferred as type: " 
                  << typeInfoToString(inferred_type) << "\n";
    }
}
```

### Phase 4: Error Handling

#### 4.1 Common Errors

1. **Missing Initialization**:
   ```cb
   auto x;  // ❌
   ```
   Error: `auto requires initialization`

2. **Type Cannot Be Inferred**:
   ```cb
   auto x = some_unknown_function();  // ❌
   ```
   Error: `Cannot infer type for variable 'x'`

3. **Type Mismatch on Reassignment**:
   ```cb
   auto x = 10;
   x = "hello";  // ❌
   ```
   Error: `Type mismatch: cannot assign 'string' to 'int'`

4. **Multiple Declarations**:
   ```cb
   auto x = 10, y = 20;  // ❌
   ```
   Error: `Multiple auto declarations not supported`

## Examples

### Example 1: Simple Types

```cb
void test_auto_simple() {
    auto i = 42;           // int
    auto f = 3.14;         // float
    auto s = "Hello";      // string
    auto b = true;         // bool
    
    println("i={i}, f={f}, s={s}, b={b}");
}
```

### Example 2: Function Returns

```cb
int compute() {
    return 100;
}

void test_auto_function() {
    auto result = compute();  // int
    println("Result: {result}");
}
```

### Example 3: Method Returns

```cb
void test_auto_method() {
    Vector<int, SystemAllocator> vec;
    vec.init(5);
    vec.push(10);
    vec.push(20);
    
    auto item = vec.get(0);  // int
    auto len = vec.get_length();  // int
    
    println("Item: {item}, Length: {len}");
}
```

### Example 4: Queue with Auto

```cb
void test_queue_auto() {
    Queue q;
    q.init(5);
    q.enqueue(100);
    q.enqueue(200);
    
    auto val1 = q.dequeue();  // int
    auto val2 = q.peek();     // int
    
    println("val1={val1}, val2={val2}");
}
```

### Example 5: Generic Queue (Future)

```cb
void test_generic_queue_auto() {
    Queue<Point> point_queue;
    point_queue.init(5);
    
    Point p1;
    p1.x = 10;
    p1.y = 20;
    point_queue.enqueue(p1);
    
    auto p2 = point_queue.dequeue();  // Point
    println("Point: ({p2.x}, {p2.y})");
}
```

### Example 6: Nested Generics (Future)

```cb
void test_nested_generics_auto() {
    Queue<Vector<int, SystemAllocator>> queue;
    queue.init(3);
    
    Vector<int, SystemAllocator> vec1;
    vec1.init(5);
    vec1.push(1);
    vec1.push(2);
    
    queue.enqueue(vec1);
    
    auto vec2 = queue.dequeue();  // Vector<int, SystemAllocator>
    auto item = vec2.get(0);      // int
    
    println("Item: {item}");
}
```

## Testing

### Test Cases

#### TC1: Basic Auto Inference
```cb
void test_auto_basic() {
    auto i = 10;
    assert(i == 10);
    
    auto s = "test";
    assert(s == "test");
}
```

#### TC2: Auto with Functions
```cb
int get_int() { return 42; }

void test_auto_function() {
    auto val = get_int();
    assert(val == 42);
}
```

#### TC3: Auto with Methods
```cb
void test_auto_method() {
    Vector<int, SystemAllocator> vec;
    vec.init(3);
    vec.push(100);
    
    auto item = vec.get(0);
    assert(item == 100);
}
```

#### TC4: Auto Error Cases
```cb
void test_auto_errors() {
    // ❌ Missing initialization
    // auto x;
    
    // ❌ Type mismatch
    auto y = 10;
    // y = "string";  // Should error
}
```

#### TC5: Auto with Queue
```cb
void test_auto_queue() {
    Queue q;
    q.init(5);
    q.enqueue(123);
    
    auto val = q.dequeue();
    assert(val == 123);
}
```

### Test File Location

**File**: `tests/cases/auto_type_inference.cb`

```cb
// Auto Type Inference Tests

void test_auto_literals() {
    auto i = 42;
    auto f = 3.14;
    auto s = "hello";
    auto b = true;
    println("✅ Auto literals work");
}

void test_auto_queue() {
    Queue q;
    q.init(5);
    q.enqueue(100);
    
    auto val = q.dequeue();
    assert(val == 100);
    println("✅ Auto with Queue works");
}

void main() {
    println("=== Auto Type Inference Tests ===");
    test_auto_literals();
    test_auto_queue();
    println("✅ All auto tests passed!");
}
```

## Compatibility

### Backward Compatibility

- `auto`は新しいキーワードなので、既存コードへの影響なし
- 既存の明示的型宣言は引き続き動作
- 段階的な移行が可能

### Future Enhancements

1. **const auto**: 
   ```cb
   const auto x = 10;  // immutable
   ```

2. **auto& / auto***: 参照とポインタのサポート
   ```cb
   auto& ref = some_variable;
   auto* ptr = &some_variable;
   ```

3. **decltype**: C++スタイルの型推論
   ```cb
   decltype(expression) var = ...;
   ```

4. **Template Type Deduction**: 
   ```cb
   template<T>
   void func(T value) {
       auto x = value;  // T と推論
   }
   ```

## Related Work

### Similar Features in Other Languages

- **C++11**: `auto` keyword
- **C# 3.0**: `var` keyword
- **Rust**: Type inference with `let`
- **Go**: `:=` operator
- **TypeScript**: Type inference
- **Swift**: Type inference with `let` / `var`

### Differences from C++

1. Cbの`auto`は常にvalue semantics（C++はreference可）
2. 単一宣言のみ（C++は複数宣言可）
3. 型推論はシンプル（C++のdecltypeやdecltype(auto)はない）

## Timeline

- **Phase 1**: Lexer & Parser - 1 week
- **Phase 2**: Type Inference Engine - 2 weeks
- **Phase 3**: Interpreter Integration - 1 week
- **Phase 4**: Testing & Documentation - 3-5 days

**Total**: 4-5 weeks

## References

- [Queue Generic Support](../todo/queue_generic_support.md)
- [Type Inference Algorithm](../architecture/interpreter_structure.md)
- [BNF Grammar](../BNF.md)
