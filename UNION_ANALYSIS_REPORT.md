# Cb Union Type Analysis Report

## Executive Summary
The Cb language implements a TypeScript-like union type system that allows combining:
- Literal values (numbers, strings, booleans, characters)
- Basic types (int, long, bool, string, char)
- Custom types (typedef aliases, struct types)
- Array types (fixed-size arrays of any compatible type)
- Mixed combinations of the above

The implementation is partially present in the AST but lacks complete HIR and C++ code generation support.

---

## 1. UNION SYNTAX AND FEATURES

### 1.1 Syntax Overview
Union types use the pipe (`|`) operator in typedef declarations:
```cb
typedef UnionName = Type1 | Type2 | Type3 | ...;
```

### 1.2 Supported Union Categories

#### A) Literal Value Unions
Values: Integer literals, string literals, boolean literals, character literals
```cb
typedef StatusCode = 200 | 404 | 500;
typedef Flag = true | false;
typedef Grade = 'A' | 'B' | 'C' | 'D' | 'F';
typedef LogLevel = "DEBUG" | "INFO" | "WARN" | "ERROR";
```

#### B) Basic Type Unions
Combinations of primitive types
```cb
typedef NumericType = int | long;
typedef PrimitiveType = int | bool | char;
typedef ValueType = string | int;
```

#### C) Custom Type Unions
Unions of typedef'd types and custom types
```cb
typedef MyInt = int;
typedef MyString = string;
typedef CustomUnion = MyInt | MyString;
```

#### D) Struct Type Unions
Unions of struct types
```cb
struct Point { int x, y; };
struct Person { string name; int age; };
typedef EntityUnion = Point | Person;
```

#### E) Array Type Unions
Unions of array types with specific sizes
```cb
typedef ArrayUnion = int[3] | bool[2];
typedef NumberArrays = int[3] | int[5];
```

#### F) Mixed/Complex Unions
Combinations of all the above categories
```cb
typedef MegaUnion = 999 | "special" | true | int | MyInt | Point | int[2];
typedef ComplexUnion = UserName | Rectangle | bool[3];
```

### 1.3 Variable Declaration and Assignment
Union variables are declared with their union type name:
```cb
StatusCode status = 200;           // Literal assignment
NumericType num = 1000000;          // Type inference
CustomUnion custom = customInt;     // Custom type assignment
EntityUnion entity = origin;        // Struct assignment
ArrayUnion array = numbers;         // Array assignment
```

### 1.4 Key Features
- Type inference based on assigned value
- Assignment validation (runtime checks expected)
- Support for reassignment with different valid types
- Access to struct/array members through union variables
- String interpolation in println() with union variables

---

## 2. TEST CASES OVERVIEW (16 total test files)

### Positive Test Cases (10)
1. **type_union.cb** - Basic primitive type unions (int, long, bool, char, string)
2. **literal_union.cb** - Literal value unions (number, bool, char, string literals)
3. **struct_union.cb** - Struct type unions with field access
4. **array_union.cb** - Array type unions with element access
5. **custom_union.cb** - Custom typedef unions
6. **mixed_union.cb** - Complex mixed unions (literals + types + structs + arrays)
7. **string_processing.cb** - String manipulation with union types
8. **comprehensive.cb** - All union categories in one test (38 test items)
9. **struct_union_compound_assignment.cb** - Compound assignment operations

### Error/Validation Test Cases (6)
1. **error_invalid_literal.cb** - Invalid literal value assignment
2. **error_type_mismatch.cb** - Type mismatch in assignment
3. **error_undefined_type.cb** - Using undefined types in union
4. **error_struct_type.cb** - Invalid struct type in union
5. **error_custom_type.cb** - Invalid custom typedef in union
6. **error_multiple.cb** - Multiple validation errors
7. **error_array_type.cb** - Invalid array type specification

---

## 3. CURRENT IMPLEMENTATION STATUS

### 3.1 AST Level (ast.h)
STATUS: FULLY IMPLEMENTED

**Existing Structures:**
```cpp
struct UnionValue {
    TypeInfo value_type;      // Type of the value
    int64_t int_value;        // Integer/char value storage
    std::string string_value; // String value storage
    bool bool_value;          // Boolean value storage
};

struct UnionDefinition {
    std::string name;
    std::vector<UnionValue> allowed_values;      // Literal values
    std::vector<TypeInfo> allowed_types;         // Basic types (int, long, etc.)
    std::vector<std::string> allowed_custom_types;  // Struct/typedef names
    std::vector<std::string> allowed_array_types;   // Array type signatures
    
    bool has_literal_values;
    bool has_type_values;
    bool has_custom_types;
    bool has_array_types;
};
```

**Helper Methods:**
- `is_literal_union()` - Check if union contains only literals
- `is_type_union()` - Check if union contains only basic types
- `is_custom_type_union()` - Check if union contains only custom types
- `is_array_type_union()` - Check if union contains only arrays
- `is_mixed_union()` - Check if union combines multiple categories
- `is_literal_value_allowed()` - Validate literal value
- `is_type_allowed()` - Validate type membership

**ASTNodeType enum includes:**
```cpp
TYPE_UNION = 15  // Union type support
```

### 3.2 HIR Level (hir_node.h and hir_generator.cpp)
STATUS: MINIMAL/NOT IMPLEMENTED

**Current HIRTypedef Structure:**
```cpp
struct HIRTypedef {
    std::string name;
    HIRType target_type;      // Only stores a single target type
    SourceLocation location;
};
```

**ISSUE:** 
The current HIRTypedef only stores a single `HIRType`, but unions need to represent multiple allowed types. This architecture cannot express union constraints.

**Missing Components:**
1. No `HIRUnion` structure to represent union types
2. No union type kind in `HIRType::TypeKind` enum:
   - Currently: Unknown, Void, Tiny, Short, Int, Long, Char, String, Bool, Float, Double, Struct, Enum, Interface, Pointer, Reference, Array, Nullptr, Function, Generic, Optional, Result
   - **Missing: Union type kind**

3. No conversion logic in `hir_generator.cpp`:
   - No handling of `AST_TYPEDEF` with union definitions
   - No extraction of union constraints into HIR representation
   - typedef conversion completely bypassed for unions

### 3.3 Code Generation Level (hir_to_cpp.cpp)
STATUS: INCOMPLETE

**Current generate_typedefs() function:**
```cpp
void HIRToCpp::generate_typedefs(const std::vector<HIRTypedef> &typedefs) {
    for (const auto &typedef_def : typedefs) {
        emit("using " + typedef_def.name + " = ");
        emit(generate_type(typedef_def.target_type));  // Only single type!
        emit(";\n");
    }
}
```

**ISSUE:**
Generates C++ type aliases with only ONE type, but unions need constraint validation.

**Missing Features:**
1. No validation wrapper generation for union types
2. No runtime type checking for union assignments
3. No support for mixed union code generation
4. No handling of literal value unions
5. No support for union member access patterns

**Expected C++ Output Pattern (not implemented):**
For `typedef StatusCode = 200 | 404 | 500;`

Option 1 - Class wrapper with validation:
```cpp
class StatusCode {
private:
    int value_;
public:
    StatusCode(int v) : value_(v) {
        if (v != 200 && v != 404 && v != 500) {
            throw std::invalid_argument("Invalid StatusCode value");
        }
    }
    operator int() const { return value_; }
};
using StatusCode = /* C++ concept or validator class */;
```

Option 2 - Type alias with constexpr validation:
```cpp
using StatusCode = int;  // With compile-time or runtime validation
```

---

## 4. DETAILED ANALYSIS

### 4.1 What Works
1. AST parsing of union syntax in typedefs
2. Representation of union constraints in UnionDefinition
3. Classification of union types (literal, type, custom, array, mixed)
4. Value validation logic in UnionDefinition helpers

### 4.2 What's Broken/Missing

#### A) HIR Generation
- **Problem:** typedef with union definitions ignored during AST->HIR conversion
- **Root cause:** No case in generate() switch for AST_TYPEDEF nodes
- **Impact:** Union type information lost before reaching codegen
- **Fix needed:** 
  1. Add `HIRUnion` structure to hir_node.h
  2. Add union type kind to `HIRType::TypeKind` enum
  3. Add typedef conversion logic to hir_generator.cpp
  4. Store union definitions in HIRProgram

#### B) Code Generation
- **Problem:** generate_typedefs() only handles single types
- **Root cause:** No understanding of union semantics in C++ codegen
- **Impact:** Union types generate as bare type aliases without validation
- **Fix needed:**
  1. Detect union types in generate_typedefs()
  2. Generate appropriate C++ representation:
     - For literal unions: class wrapper with value validation
     - For type unions: using alias (constraint in semantic analysis)
     - For struct unions: variant wrapper or polymorphic pattern
     - For array unions: polymorphic array handler
  3. Generate validation methods for assignments
  4. Handle union variable access patterns

#### C) Type System Integration
- **Problem:** No constraint tracking through type system
- **Root cause:** HIRType doesn't have union constraint information
- **Impact:** No validation at assignment time
- **Fix needed:**
  1. Store union definition reference in HIRType when it's a union
  2. Implement validation in generated C++ code

### 4.3 Architecture Gap Analysis

```
┌─────────────────────────────────────────────────────────┐
│                    AST Level                             │
│  UnionDefinition fully specified with all constraints   │
└──────────────────────┬──────────────────────────────────┘
                       │
                       ▼ (BROKEN - No conversion)
┌─────────────────────────────────────────────────────────┐
│                    HIR Level                             │
│  Missing:                                               │
│  - HIRUnion structure                                   │
│  - Union type kind in enum                              │
│  - Constraint storage mechanism                         │
│  - Typedef->Union conversion logic                      │
└──────────────────────┬──────────────────────────────────┘
                       │
                       ▼ (BROKEN - No constraint info)
┌─────────────────────────────────────────────────────────┐
│                 Code Generation                         │
│  Generates bare type aliases without validation        │
│  No union-specific codegen patterns                    │
└─────────────────────────────────────────────────────────┘
```

---

## 5. IMPLEMENTATION REQUIREMENTS

### 5.1 Phase 1: HIR Support (hir_node.h)
Create union representation:
```cpp
struct HIRUnion {
    std::string name;
    std::vector<HIRType> allowed_types;          // Basic/custom/struct types
    std::vector</* LiteralConstraint */> literals;  // Literal values
    std::vector<int> array_sizes;                // For array unions
    bool is_literal_only;
    bool is_type_only;
    bool is_custom_type_only;
    bool is_array_only;
    bool is_mixed;
};
```

Add to HIRType::TypeKind enum:
```cpp
enum class TypeKind {
    // ... existing ...
    Union,  // NEW: Union type
};
```

Add union field to HIRType:
```cpp
std::unique_ptr<HIRUnion> union_def;  // If kind == Union
```

### 5.2 Phase 2: Type Conversion (hir_generator.cpp)
Implement typedef conversion:
```cpp
// Add to generate() method
case ASTNodeType::AST_TYPEDEF:
    if (node->union_definition.has_literal_values ||
        node->union_definition.has_type_values ||
        node->union_definition.has_custom_types ||
        node->union_definition.has_array_types) {
        // Convert union typedef
        auto union_def = convert_union_typedef(node.get());
        program->typedefs.push_back(union_def);
    }
    break;

// Add method
HIRTypedef convert_union_typedef(const ASTNode *node);
```

### 5.3 Phase 3: Code Generation (hir_to_cpp.cpp)
Handle union types in codegen:
1. Detect union typedefs in generate_typedefs()
2. Generate validation wrapper for literal unions
3. Generate union type definitions for struct/mixed unions
4. Generate assignment validation code

### 5.4 Validation Implementation
Options for C++:
1. **Class wrapper** - Type-safe, full validation
2. **Concept + static_assert** - Compile-time for literals
3. **Runtime checks** - Dynamic validation wrapper
4. **Variant pattern** - For struct unions (std::variant)

---

## 6. RECOMMENDATIONS

### Short Term (Critical)
1. Add `HIRUnion` structure to hir_node.h
2. Add `Union` type kind to HIRType enum
3. Implement AST_TYPEDEF handling in hir_generator.cpp
4. Store union definitions in HIRProgram

### Medium Term (Important)
1. Implement union-specific code generation patterns
2. Add literal value validation in generated C++
3. Support union variable assignments with type checking
4. Add union member access patterns

### Long Term (Enhancement)
1. Pattern matching for union types (match expression)
2. Type narrowing after validation
3. Generic union support
4. Union exhaustiveness checking

---

## 7. TEST COVERAGE SUMMARY

| Category | Test Files | Status |
|----------|-----------|--------|
| Literal unions | literal_union.cb | Needs codegen |
| Type unions | type_union.cb | Needs codegen |
| Struct unions | struct_union.cb | Needs codegen |
| Array unions | array_union.cb | Needs codegen |
| Custom unions | custom_union.cb | Needs codegen |
| Mixed unions | mixed_union.cb, comprehensive.cb | Needs codegen |
| Error handling | 6 error test files | Needs validation |
| **Total** | **16 test files** | **All need HIR + codegen** |

---

## 8. FILE LOCATIONS

### Source Files
- `/Users/shadowlink/Documents/git/Cb/src/common/ast.h` - UnionDefinition (complete)
- `/Users/shadowlink/Documents/git/Cb/src/backend/ir/hir/hir_node.h` - HIRTypedef (incomplete)
- `/Users/shadowlink/Documents/git/Cb/src/backend/ir/hir/hir_generator.cpp` - No union handling
- `/Users/shadowlink/Documents/git/Cb/src/backend/codegen/hir_to_cpp.cpp` - No union codegen

### Test Files
- `/Users/shadowlink/Documents/git/Cb/tests/cases/union/` - 16 test files

