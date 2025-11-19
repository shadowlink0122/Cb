# Union Type Implementation - Quick Reference

## Current Status at a Glance

| Component | Status | Details |
|-----------|--------|---------|
| AST Parsing | ✓ Complete | UnionDefinition fully defined with constraint helpers |
| AST Representation | ✓ Complete | UnionValue and UnionDefinition structures present |
| HIR Structure | ✗ Missing | No HIRUnion struct, no Union type kind in enum |
| HIR Conversion | ✗ Missing | No AST_TYPEDEF handling for unions |
| C++ Codegen | ✗ Broken | Generates type aliases only, no validation |
| Test Coverage | 16 tests | All tests require HIR + codegen fixes |

---

## Key Files to Modify

### 1. hir_node.h (Add Union Support)
**Location:** `/Users/shadowlink/Documents/git/Cb/src/backend/ir/hir/hir_node.h`

**Changes needed:**
1. Add `Union` to `HIRType::TypeKind` enum (after `Result`)
2. Create `HIRUnion` struct with:
   - name
   - allowed_types (vector<HIRType>)
   - allowed_literals (vector<pair<HIRType, string>>)
   - is_literal_union, is_type_union, is_custom_type_union, is_array_union, is_mixed_union
3. Add `union_def` field to `HIRTypedef`
4. Add `union_def` field to `HIRType` (optional<unique_ptr<HIRUnion>>)

### 2. hir_generator.cpp (Implement Conversion)
**Location:** `/Users/shadowlink/Documents/git/Cb/src/backend/ir/hir/hir_generator.cpp`

**Changes needed:**
1. Add case for `AST_TYPEDEF` in `generate()` method
2. Implement `convert_union_typedef()` method:
   ```cpp
   HIRTypedef convert_union_typedef(const ASTNode *node);
   ```
3. Extract union constraints from AST UnionDefinition
4. Build HIRUnion with appropriate type classifications
5. Add union typedefs to `program->typedefs`

### 3. hir_to_cpp.cpp (Implement Codegen)
**Location:** `/Users/shadowlink/Documents/git/Cb/src/backend/codegen/hir_to_cpp.cpp`

**Changes needed:**
1. Modify `generate_typedefs()` to detect union types:
   ```cpp
   if (typedef_def.union_def) {
       // Generate union-specific code
       generate_union_typedef(typedef_def);
   } else {
       // Generate regular type alias
   }
   ```
2. Implement `generate_union_typedef()` with support for:
   - Literal unions: Class wrapper with value validation
   - Type unions: Type alias with constraint metadata
   - Struct unions: Variant or polymorphic wrapper
   - Array unions: Polymorphic array handler
   - Mixed unions: Appropriate composite pattern

---

## Union Category Code Patterns

### Pattern 1: Literal Union (int)
Input:
```cb
typedef StatusCode = 200 | 404 | 500;
StatusCode status = 200;
```

Target C++ Output:
```cpp
class StatusCode {
    int value_;
    static constexpr int ALLOWED[] = {200, 404, 500};
public:
    StatusCode(int v) : value_(v) {
        if (!is_valid(v)) throw std::invalid_argument("Invalid StatusCode");
    }
    static bool is_valid(int v) {
        for (int allowed : ALLOWED) if (allowed == v) return true;
        return false;
    }
    operator int() const { return value_; }
};
```

### Pattern 2: Type Union
Input:
```cb
typedef NumericType = int | long;
NumericType num = 1000000;
```

Target C++ Output:
```cpp
// Store type info with union type alias
using NumericType = int64_t;  // or std::variant<int, int64_t>
// Type checking enforced in semantic analysis
```

### Pattern 3: Struct Union
Input:
```cb
typedef EntityUnion = Point | Person;
EntityUnion entity = origin;
```

Target C++ Output:
```cpp
using EntityUnion = std::variant<Point, Person>;
// Or polymorphic base class pattern
```

### Pattern 4: Array Union
Input:
```cb
typedef ArrayUnion = int[3] | bool[2];
ArrayUnion array = numbers;
```

Target C++ Output:
```cpp
class ArrayUnion {
    std::variant<std::array<int, 3>, std::array<bool, 2>> value_;
public:
    // Constructor and accessors
};
```

---

## Testing Strategy

### Phase 1: Basic Literals
1. Ensure `literal_union.cb` works
2. Validate assignment constraints
3. Test string interpolation

### Phase 2: Type Unions
1. Ensure `type_union.cb` works
2. Test type narrowing
3. Validate type inference

### Phase 3: Struct Unions
1. Ensure `struct_union.cb` works
2. Test field access through union
3. Validate struct assignment

### Phase 4: Array Unions
1. Ensure `array_union.cb` works
2. Test element access
3. Validate array assignment

### Phase 5: Mixed Unions
1. Ensure `mixed_union.cb` works
2. Ensure `comprehensive.cb` passes all 38 tests
3. Run all error tests for validation

### Phase 6: Error Handling
1. Invalid literal assignments → Error
2. Type mismatches → Error
3. Undefined types → Error
4. Invalid struct/custom types → Error

---

## Key Helper Functions from AST

Available in `UnionDefinition` (ast.h, lines 194-336):

```cpp
// Classification
bool is_literal_union()        // Only literal values
bool is_type_union()           // Only basic types
bool is_custom_type_union()    // Only custom types
bool is_array_type_union()     // Only array types
bool is_mixed_union()          // Multiple categories

// Validation
bool is_literal_value_allowed(const UnionValue &value)
bool is_type_allowed(TypeInfo type)
```

**Note:** These can be called during HIR generation to determine union category.

---

## Critical Implementation Points

1. **Order of conversion:** Structs and typedefs before unions
2. **Type resolution:** Need to resolve typedef names to actual types
3. **Validation:** Must distinguish between compile-time and runtime checks
4. **Inference:** Type inference from assignment needed for variables
5. **Access patterns:** Union variables need to access struct/array members

---

## Testing Commands

```bash
# Test individual union type categories
./build/Cb tests/cases/union/literal_union.cb
./build/Cb tests/cases/union/type_union.cb
./build/Cb tests/cases/union/struct_union.cb
./build/Cb tests/cases/union/array_union.cb

# Test complex unions
./build/Cb tests/cases/union/mixed_union.cb
./build/Cb tests/cases/union/comprehensive.cb

# Test error handling
./build/Cb tests/cases/union/error_invalid_literal.cb
./build/Cb tests/cases/union/error_type_mismatch.cb
```

---

## References

- Full analysis: `/Users/shadowlink/Documents/git/Cb/UNION_ANALYSIS_REPORT.md`
- AST definitions: `/Users/shadowlink/Documents/git/Cb/src/common/ast.h` lines 28, 137-336
- Test files: `/Users/shadowlink/Documents/git/Cb/tests/cases/union/` (16 files)

