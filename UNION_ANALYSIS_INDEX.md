# Cb Union Type Analysis - Complete Index

## Overview
This directory contains a comprehensive analysis of the Cb language's union type system, including current implementation status, detailed technical requirements, and practical code examples.

## Analysis Documents

### 1. UNION_ANALYSIS_REPORT.md (14 KB)
**Purpose:** Complete technical analysis of the union type system

**Contents:**
- Executive Summary
- Union syntax overview (6 categories)
- Test case overview (16 test files)
- Current implementation status (AST/HIR/Codegen)
- Detailed analysis of gaps and missing components
- Implementation requirements by phase
- Architecture gap analysis
- File locations and references

**Best for:** Understanding the complete technical picture and design decisions

**Key sections:**
- Section 1: Union syntax and features (comprehensive breakdown)
- Section 2: Test cases overview (10 positive + 6 error tests)
- Section 3: Implementation status by component
- Section 4: Detailed gap analysis
- Section 5: Implementation requirements
- Section 6: Recommendations
- Section 7: Test coverage summary
- Section 8: File locations

---

### 2. UNION_IMPLEMENTATION_QUICK_REF.md (6.2 KB)
**Purpose:** Quick reference guide for developers implementing union support

**Contents:**
- Status at a glance (table format)
- Key files to modify (3 files with specific changes)
- Union category code patterns (5 patterns)
- 6-phase testing strategy
- Helper functions available from AST
- Critical implementation points
- Testing commands
- References

**Best for:** Quick lookup while implementing features

**Key sections:**
- Status table (AST/HIR/Codegen)
- Code patterns for each union category
- Phase-by-phase testing strategy
- Helper functions you can leverage
- Critical implementation points

---

### 3. UNION_CODE_EXAMPLES.md (9.1 KB)
**Purpose:** Practical code examples showing before/after for each union category

**Contents:**
- 5 detailed examples:
  1. Literal value union (StatusCode = 200 | 404 | 500)
  2. Type union (NumericType = int | long)
  3. Struct union (EntityUnion = Point | Person)
  4. Array union (ArrayUnion = int[3] | bool[2])
  5. Mixed union (complex example)
- For each example: Cb code, AST representation, needed HIR, needed C++ output
- Current issue: What gets generated now vs. what should be generated
- Implementation checklist

**Best for:** Understanding the exact code transformations needed

**Key examples:**
- Example 1: Shows literal union with validation wrapper pattern
- Example 2: Shows type union with variant pattern
- Example 3: Shows struct union with variant pattern
- Example 4: Shows array union with variant pattern
- Example 5: Shows mixed union with complex variant pattern

---

## Quick Navigation

### By Task

**I need to understand what needs to be done:**
→ Read: UNION_ANALYSIS_REPORT.md (Section 3-5)

**I'm implementing the feature, need quick reference:**
→ Read: UNION_IMPLEMENTATION_QUICK_REF.md

**I need code examples to understand transformations:**
→ Read: UNION_CODE_EXAMPLES.md

**I need to know what to test:**
→ Read: UNION_ANALYSIS_REPORT.md (Section 2 and 7)

### By Implementation Phase

**Phase 1 - Add HIR structures:**
→ UNION_IMPLEMENTATION_QUICK_REF.md (Key Files section 1)
→ UNION_CODE_EXAMPLES.md (Implementation checklist)

**Phase 2 - Implement type conversion:**
→ UNION_IMPLEMENTATION_QUICK_REF.md (Key Files section 2)
→ UNION_ANALYSIS_REPORT.md (Section 5.2)

**Phase 3 - Implement code generation:**
→ UNION_IMPLEMENTATION_QUICK_REF.md (Key Files section 3)
→ UNION_CODE_EXAMPLES.md (Code patterns section)

**Phase 4 - Testing:**
→ UNION_IMPLEMENTATION_QUICK_REF.md (Testing Strategy)
→ UNION_ANALYSIS_REPORT.md (Section 7 - Test Coverage)

---

## Key Findings Summary

### Current Status
| Component | Status | Details |
|-----------|--------|---------|
| AST Parsing | ✓ Complete | UnionDefinition fully defined |
| HIR Support | ✗ Missing | No HIRUnion struct, no Union type kind |
| C++ Codegen | ✗ Broken | Only generates type aliases, no validation |
| Tests | ⚠ Ready | 16 test files ready, all currently fail |

### Critical Missing Components
1. `HIRUnion` structure in hir_node.h
2. `Union` type kind in `HIRType::TypeKind` enum
3. AST_TYPEDEF handling in hir_generator.cpp
4. Union-specific code generation in hir_to_cpp.cpp

### Implementation Path
1. Add HIR structures (1-2 hours)
2. Implement type conversion (2-3 hours)
3. Implement code generation (2-3 hours)
4. Testing and validation (1-2 hours)

**Total estimated effort: 6-8 hours**

---

## Union Categories (6 Total)

1. **Literal Value Unions**
   - Example: `StatusCode = 200 | 404 | 500`
   - Code pattern: Class wrapper with validation

2. **Type Unions**
   - Example: `NumericType = int | long`
   - Code pattern: Type alias or std::variant

3. **Custom Type Unions**
   - Example: `CustomUnion = MyInt | MyString`
   - Code pattern: Type alias or wrapper

4. **Struct Type Unions**
   - Example: `EntityUnion = Point | Person`
   - Code pattern: std::variant

5. **Array Type Unions**
   - Example: `ArrayUnion = int[3] | bool[2]`
   - Code pattern: std::variant with std::array

6. **Mixed Unions**
   - Example: `MegaUnion = 999 | "special" | int | Point | int[2]`
   - Code pattern: Composite variant with type tag

---

## Test Files (16 Total)

### Positive Tests (10)
- `literal_union.cb` - Literal value unions
- `type_union.cb` - Type unions
- `struct_union.cb` - Struct type unions
- `array_union.cb` - Array type unions
- `custom_union.cb` - Custom typedef unions
- `mixed_union.cb` - Complex mixed unions
- `string_processing.cb` - String manipulation
- `comprehensive.cb` - All categories (38 test items)
- `struct_union_compound_assignment.cb` - Compound operations

### Error Tests (6)
- `error_invalid_literal.cb` - Invalid literal values
- `error_type_mismatch.cb` - Type mismatch validation
- `error_undefined_type.cb` - Undefined types
- `error_struct_type.cb` - Invalid struct types
- `error_custom_type.cb` - Invalid custom types
- `error_multiple.cb` - Multiple errors

**Location:** `/Users/shadowlink/Documents/git/Cb/tests/cases/union/`

---

## Implementation Checklists

### Phase 1: HIR Support (hir_node.h)
- [ ] Add `Union` to `HIRType::TypeKind` enum
- [ ] Create `HIRUnion` struct with:
  - [ ] name
  - [ ] allowed_types (vector<HIRType>)
  - [ ] allowed_literals
  - [ ] classification flags
- [ ] Add `union_def` to `HIRTypedef`
- [ ] Add optional `union_def` to `HIRType`

### Phase 2: Type Conversion (hir_generator.cpp)
- [ ] Add case for `AST_TYPEDEF` in `generate()`
- [ ] Implement `convert_union_typedef()` method
- [ ] Extract union constraints from `UnionDefinition`
- [ ] Build `HIRUnion` with classifications
- [ ] Add union typedefs to program

### Phase 3: Code Generation (hir_to_cpp.cpp)
- [ ] Modify `generate_typedefs()` to detect unions
- [ ] Implement `generate_union_typedef()` method
- [ ] Support literal union code generation
- [ ] Support type union code generation
- [ ] Support struct union code generation
- [ ] Support array union code generation
- [ ] Support mixed union code generation

### Phase 4: Testing
- [ ] Test literal_union.cb
- [ ] Test type_union.cb
- [ ] Test struct_union.cb
- [ ] Test array_union.cb
- [ ] Test custom_union.cb
- [ ] Test mixed_union.cb
- [ ] Test comprehensive.cb (38 items)
- [ ] Verify error cases

---

## Helper Functions Available

From `UnionDefinition` in `ast.h` (already implemented):

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

**Usage:** Call these in `convert_union_typedef()` to determine union category and generate appropriate C++ code.

---

## File Locations

### Source Files
```
src/common/ast.h
  - Lines 28: TYPE_UNION enum value
  - Lines 137-336: UnionValue and UnionDefinition structures

src/backend/ir/hir/hir_node.h
  - Lines 16-68: HIRType structure (needs Union addition)
  - Lines 346-351: HIRTypedef structure (needs union_def addition)

src/backend/ir/hir/hir_generator.cpp
  - Lines 15-120: generate() method (needs AST_TYPEDEF case)
  - Lines 1852+: convert_type() method

src/backend/codegen/hir_to_cpp.cpp
  - Lines 225-237: generate_typedefs() method (needs union detection)
  - Lines 2013+: generate_type() method
```

### Test Files
```
tests/cases/union/
  - 10 positive test files
  - 6 error/validation test files
  - Total: 16 test files
```

---

## Recommended Reading Order

1. **First reading (overview):**
   - This file (UNION_ANALYSIS_INDEX.md)

2. **Second reading (detailed analysis):**
   - UNION_ANALYSIS_REPORT.md (Sections 1-4)

3. **Third reading (implementation details):**
   - UNION_IMPLEMENTATION_QUICK_REF.md

4. **Fourth reading (code examples):**
   - UNION_CODE_EXAMPLES.md

5. **Fifth reading (practical reference):**
   - Return to UNION_IMPLEMENTATION_QUICK_REF.md while coding

---

## Document Statistics

| Document | Size | Sections | Complexity |
|----------|------|----------|-----------|
| UNION_ANALYSIS_REPORT.md | 14 KB | 8 | High |
| UNION_IMPLEMENTATION_QUICK_REF.md | 6.2 KB | 7 | Medium |
| UNION_CODE_EXAMPLES.md | 9.1 KB | 9 | Medium |
| UNION_ANALYSIS_INDEX.md | This file | 10+ | Low |

**Total analysis content:** ~38 KB, ~5000+ lines of detailed documentation

---

## Next Steps

1. Choose your implementation path based on the checklists
2. Review the appropriate sections from the detailed reports
3. Start with Phase 1 (HIR support)
4. Test incrementally as you implement each phase
5. Use UNION_IMPLEMENTATION_QUICK_REF.md as a reference while coding
6. Cross-reference code examples from UNION_CODE_EXAMPLES.md

---

*Last updated: 2025-11-19*
*Analysis created for Cb v0.14.0*
