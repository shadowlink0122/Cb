# Type Cast Implementation Design

**Date**: 2025/10/27  
**Feature**: Type Casting for Cb Language  
**Priority**: Week 2 Day 2  
**Status**: Design Phase

## Overview

Implement type casting to enable conversion between pointer types, especially `void*` to typed pointers like `int*`, which is essential for Vector data storage.

## Syntax Options

### Option 1: C-style Cast (Recommended)
```cb
int* typed_ptr = (int*)void_ptr;
char* char_ptr = (char*)generic_ptr;
```

**Pros**:
- Familiar to C/C++ developers
- Clear and explicit
- Matches existing C documentation

**Cons**:
- Parentheses can be confused with function calls

### Option 2: Rust-style Cast
```cb
int* typed_ptr = void_ptr as int*;
char* char_ptr = generic_ptr as char*;
```

**Pros**:
- More readable
- No ambiguity with function calls
- Modern syntax

**Cons**:
- Different from C/C++
- May confuse C developers

### Decision: Implement Option 1 First (C-style)

**Reason**: Cb already uses C-like syntax, and users expect C compatibility.

## Use Cases

### Use Case 1: Vector Data Storage

**Current (placeholder)**:
```cb
void vector_push_int_system(Vector<int, SystemAllocator>& vec, int value) {
    // Can't store actual data
    vec.length = vec.length + 1;
}
```

**With Cast**:
```cb
void vector_push_int_system(Vector<int, SystemAllocator>& vec, int value) {
    int* data = (int*)vec.data;  // Cast void* to int*
    data[vec.length] = value;     // Store actual value
    vec.length = vec.length + 1;
}
```

### Use Case 2: Memory Allocator Return

```cb
void* generic_ptr = allocator.allocate(100);
int* int_array = (int*)generic_ptr;  // Cast to typed pointer
int_array[0] = 42;
```

### Use Case 3: Generic Data Structures

```cb
struct Node {
    void* data;
    int data_size;
};

void node_set_int(Node& node, int value) {
    int* typed_data = (int*)node.data;
    *typed_data = value;
}
```

## Implementation Plan

### Phase 1: AST Extension ✅

**File**: `src/common/ast.h`

Add new AST node type:
```cpp
enum class ASTNodeType {
    // ... existing types ...
    AST_CAST_EXPR,  // Type cast expression (type)expr
    // ... rest ...
};
```

Add cast information to ASTNode:
```cpp
struct ASTNode {
    // ... existing fields ...
    
    // For AST_CAST_EXPR
    std::string cast_target_type;    // Target type name (e.g., "int*")
    TypeInfo cast_type_info;         // Parsed type info
    ASTNode* cast_expr;              // Expression to cast
};
```

### Phase 2: Lexer Extension (if needed)

**File**: `src/frontend/lexer/lexer.cpp`

Check if parentheses are already handled:
- `(` → TOKEN_LPAREN
- `)` → TOKEN_RPAREN
- `*` → TOKEN_ASTERISK

**Result**: No changes needed (tokens already exist)

### Phase 3: Parser Extension ⚪

**File**: `src/frontend/recursive_parser/recursive_parser.cpp`

Add cast parsing in `parsePrimaryExpression()`:

```cpp
ASTNode* RecursiveParser::parsePrimaryExpression() {
    // Check for cast: (type)expr
    if (current_token_.type == TOKEN_LPAREN) {
        size_t saved_pos = current_token_index_;
        
        // Try to parse as cast
        eat(TOKEN_LPAREN);
        
        // Check if this looks like a type
        if (isType()) {
            std::string type_str = parseType();
            eat(TOKEN_RPAREN);
            
            // Parse the expression to cast
            ASTNode* expr = parsePrimaryExpression();
            
            // Create cast node
            ASTNode* cast_node = new ASTNode();
            cast_node->node_type = ASTNodeType::AST_CAST_EXPR;
            cast_node->cast_target_type = type_str;
            cast_node->cast_expr = expr;
            cast_node->line_number = current_token_.line;
            
            return cast_node;
        } else {
            // Not a cast, restore and parse as grouped expression
            current_token_index_ = saved_pos;
            current_token_ = tokens_[current_token_index_];
            // Fall through to normal parsing
        }
    }
    
    // ... rest of primary expression parsing ...
}
```

Helper function:
```cpp
bool RecursiveParser::isType() {
    // Check if current token sequence is a type
    return current_token_.type == TOKEN_IDENTIFIER ||
           current_token_.value == "int" ||
           current_token_.value == "char" ||
           current_token_.value == "float" ||
           current_token_.value == "void" ||
           // ... other types
           ;
}
```

### Phase 4: Type Checker ⚪

**File**: `src/backend/interpreter/managers/types/*.cpp`

Add type checking for casts:

```cpp
bool TypeChecker::checkCastValid(TypeInfo from_type, TypeInfo to_type) {
    // Allow pointer to pointer casts
    if (isPointerType(from_type) && isPointerType(to_type)) {
        return true;  // All pointer casts allowed (like C)
    }
    
    // Allow void* to any pointer
    if (from_type == TYPE_VOID_PTR) {
        return isPointerType(to_type);
    }
    
    // Allow any pointer to void*
    if (to_type == TYPE_VOID_PTR) {
        return isPointerType(from_type);
    }
    
    // Numeric conversions
    if (isNumericType(from_type) && isNumericType(to_type)) {
        return true;
    }
    
    return false;
}
```

### Phase 5: Interpreter/Evaluator ⚪

**File**: `src/backend/interpreter/evaluator/expression_evaluator.cpp`

Add cast evaluation:

```cpp
TypedValue Interpreter::evaluateCast(ASTNode* node) {
    // Evaluate the expression to cast
    TypedValue value = evaluateExpression(node->cast_expr);
    
    // Parse target type
    TypeInfo target_type = parseTypeString(node->cast_target_type);
    
    // Perform cast
    TypedValue result;
    result.type = target_type;
    
    // For pointer casts, just copy the pointer value
    if (isPointerType(value.type) && isPointerType(target_type)) {
        result.ptr_value = value.ptr_value;
        return result;
    }
    
    // For numeric casts
    if (isNumericType(value.type) && isNumericType(target_type)) {
        return convertNumericType(value, target_type);
    }
    
    throw InterpreterError("Invalid cast");
}
```

### Phase 6: Testing ⚪

Create comprehensive test cases.

## Implementation Challenges

### Challenge 1: Disambiguating Cast vs Grouped Expression

**Problem**:
```cb
(int)x   // Cast
(x)      // Grouped expression
(x + y)  // Grouped expression
```

**Solution**: Lookahead to check if token after `(` is a type name.

### Challenge 2: Pointer Type Parsing

**Problem**:
```cb
(int*)ptr      // Simple pointer cast
(int**)ptr     // Double pointer cast
(MyStruct*)ptr // Struct pointer cast
```

**Solution**: Extend type parser to handle pointer types fully.

### Challenge 3: Runtime Pointer Conversion

**Problem**: void* and int* are both represented as pointers, but need type information.

**Solution**: Store type info in TypedValue, pointer value remains the same.

## Type System Changes

### Current Type System
```cpp
enum TypeInfo {
    TYPE_INT,
    TYPE_CHAR,
    TYPE_VOID,
    TYPE_VOID_PTR,  // void*
    TYPE_INT_PTR,   // int*
    TYPE_CHAR_PTR,  // char*
    // ... more pointer types
};
```

### Extension Needed
```cpp
// May need generic pointer type with base type
struct PointerTypeInfo {
    TypeInfo base_type;  // What it points to
    int indirection;     // Number of *'s
};
```

## Safety Considerations

### Unsafe Casts (Like C)

Cb will follow C's philosophy: **trust the programmer**

```cb
void* ptr = nullptr;
int* int_ptr = (int*)ptr;  // Allowed but dangerous
*int_ptr = 42;              // Runtime error (segfault)
```

**No runtime checks** - same as C for performance.

### Safe Patterns

```cb
// 1. Check before cast
void* ptr = allocator.allocate(sizeof(int));
if (ptr != nullptr) {
    int* int_ptr = (int*)ptr;
    *int_ptr = 42;
}

// 2. Know what you're casting
Vector<int, SystemAllocator> vec;
// vec.data is known to be int* underneath
int* data = (int*)vec.data;
```

## Testing Strategy

### Test 1: Basic Pointer Cast
```cb
void* void_ptr = nullptr;
int* int_ptr = (int*)void_ptr;
char* char_ptr = (char*)void_ptr;
```

### Test 2: Cast with Actual Data
```cb
void* generic = nullptr;  // Future: actual allocation
int* typed = (int*)generic;
// typed[0] = 42;  // Future: actual access
```

### Test 3: Cast in Function
```cb
void process(void* data) {
    int* values = (int*)data;
    // Use values
}
```

### Test 4: Cast with Vector
```cb
Vector<int, SystemAllocator> vec;
int* data = (int*)vec.data;
```

### Test 5: Error Cases
```cb
// These should produce parse errors
int x = (int)  ;      // Missing expression
int* p = (int;        // Missing )
int* p = int*)ptr;    // Missing (
```

## Timeline

### Day 2: Cast Implementation (Today)
- ✅ Design document (this file)
- ⚪ AST extension
- ⚪ Parser implementation
- ⚪ Basic tests

### Day 3: Integration
- ⚪ Type checker integration
- ⚪ Interpreter/evaluator support
- ⚪ Comprehensive tests

### Day 4: Vector Integration
- ⚪ Update vector_push to use cast
- ⚪ Update vector_pop to use cast
- ⚪ Actual data storage tests

## Success Criteria

1. ✅ Parser can parse `(type)expr` syntax
2. ✅ Type checker validates cast operations
3. ✅ Interpreter correctly converts types
4. ✅ Vector operations use casts for data storage
5. ✅ All tests pass

## Future Enhancements

### Phase 2 Features (v0.12.0+)

1. **Rust-style Cast**
   ```cb
   int* ptr = void_ptr as int*;
   ```

2. **Checked Cast**
   ```cb
   int* ptr = try_cast<int*>(void_ptr);  // Returns nullptr on failure
   ```

3. **Type Alias Cast**
   ```cb
   typedef int* IntPtr;
   IntPtr p = (IntPtr)void_ptr;
   ```

## References

- C cast: `(type)expr`
- C++ cast: `static_cast<type>(expr)`, `reinterpret_cast<type>(expr)`
- Rust cast: `expr as type`
- Go: No explicit cast, uses type assertions

## Decision Summary

**Syntax**: C-style `(type)expr`  
**Safety**: Unsafe (like C, trust programmer)  
**Scope**: Pointer casts first, numeric casts later  
**Timeline**: Day 2-3 of Week 2

---

**Status**: ✅ Design Complete  
**Next**: Implement AST extension
