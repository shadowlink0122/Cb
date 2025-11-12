# Week 2 Day 1: Vector Operations Implementation

**Date**: 2025/10/27  
**Branch**: feature/trait-allocator  
**Status**: ✅ Complete

## Overview

Day 1 of Week 2 focuses on implementing basic Vector operations: push, pop, and resize. These operations demonstrate the foundation for dynamic array functionality with pluggable allocators.

## Completed Features

### 1. Vector Structure Extension

Added `data` field to store the actual data pointer:

```cb
struct Vector<T, A: Allocator> {
    int capacity;
    int length;
    void* data;        // データ領域（将来的に実際のメモリを指す）
};
```

### 2. Push Operation

**Signature**: `void vector_push_int_system(Vector<int, SystemAllocator>& vec, int value)`

**Behavior**:
- Checks if capacity is full
- Increments length
- Logs the operation (placeholder for actual data storage)

```cb
void vector_push_int_system(Vector<int, SystemAllocator>& vec, int value) {
    if (vec.length >= vec.capacity) {
        println("[Vector] Capacity full! Need resize");
        return;
    }
    
    println("[Vector] Push value=%d at index=%d", value, vec.length);
    vec.length = vec.length + 1;
}
```

**Test Results**:
```
[Vector] Push value=10 at index=0
[Vector] Push value=20 at index=1
[Vector] Push value=30 at index=2
[Vector] length=3, capacity=5
```

### 3. Pop Operation

**Signature**: `int vector_pop_int_system(Vector<int, SystemAllocator>& vec)`

**Behavior**:
- Checks if vector is empty
- Decrements length
- Returns placeholder value (future: actual data)

```cb
int vector_pop_int_system(Vector<int, SystemAllocator>& vec) {
    if (vec.length <= 0) {
        println("[Vector] Empty! Cannot pop");
        return 0;
    }
    
    vec.length = vec.length - 1;
    println("[Vector] Pop from index=%d", vec.length);
    return 0;  // プレースホルダー
}
```

**Test Results**:
```
[Vector] Pop from index=2
[Vector] Pop from index=1
[Vector] length=1, capacity=5
```

### 4. Resize Operation

**Signature**: `void vector_resize_int_system(Vector<int, SystemAllocator>& vec, int new_capacity)`

**Behavior**:
- Checks if resize is needed
- Updates capacity
- Logs the operation (placeholder for actual memory reallocation)

```cb
void vector_resize_int_system(Vector<int, SystemAllocator>& vec, int new_capacity) {
    if (new_capacity <= vec.capacity) {
        println("[Vector] No resize needed");
        return;
    }
    
    println("[Vector] Resizing from capacity=%d to %d", vec.capacity, new_capacity);
    vec.capacity = new_capacity;
    println("[Vector] Resize complete (new capacity=%d)", new_capacity);
}
```

**Test Results**:
```
Before resize:
[Vector] length=3, capacity=3

[Vector] Resizing from capacity=3 to 10
[Vector] Resize complete (new capacity=10)

After resize:
[Vector] length=3, capacity=10
```

## Implemented for Both Allocators

All operations are implemented for both allocator types:

1. **SystemAllocator**: `vector_push_int_system()`, `vector_pop_int_system()`, `vector_resize_int_system()`
2. **BumpAllocator**: `vector_push_int_bump()`, `vector_pop_int_bump()`, `vector_resize_int_bump()`

This demonstrates **zero-cost abstraction** - same interface, different implementations.

## Test Coverage

### Test Suite 1: Basic Operations
- ✅ Vector initialization
- ✅ Push 3 elements
- ✅ Pop 2 elements
- ✅ Check length/capacity

### Test Suite 2: Edge Cases
- ✅ Push when capacity full (error handling)
- ✅ Pop from empty vector (error handling)
- ✅ Resize operation
- ✅ Push after resize

### Test Suite 3: Multiple Allocators
- ✅ SystemAllocator vector operations
- ✅ BumpAllocator vector operations
- ✅ Both work with same interface

**Total**: 11 test scenarios, all passing ✅

## Current Limitations (Known Issues)

### 1. Placeholder Implementation

Current operations are **logical placeholders**:
- `push`: Increments length, but doesn't store actual data
- `pop`: Decrements length, but returns placeholder 0
- `resize`: Updates capacity, but doesn't reallocate memory

**Future Implementation**:
```cb
void vector_push_int_system(Vector<int, SystemAllocator>& vec, int value) {
    if (vec.length >= vec.capacity) {
        vector_resize_int_system(vec, vec.capacity * 2);  // Auto-grow
    }
    
    // Actual data storage:
    // ((int*)vec.data)[vec.length] = value;
    vec.length = vec.length + 1;
}
```

### 2. No Actual Memory Allocation

Resize doesn't call allocator yet:

**Future Implementation**:
```cb
void vector_resize_int_system(Vector<int, SystemAllocator>& vec, int new_capacity) {
    SystemAllocator alloc;
    
    // 1. Allocate new memory
    void* new_data = alloc.allocate(new_capacity * sizeof(int));
    
    // 2. Copy old data
    // memcpy(new_data, vec.data, vec.length * sizeof(int));
    
    // 3. Deallocate old memory
    alloc.deallocate(vec.data);
    
    // 4. Update pointer
    vec.data = new_data;
    vec.capacity = new_capacity;
}
```

### 3. Type Casting Not Available

Cannot cast `void*` to `int*` yet:
```cb
// Future feature:
int* typed_data = (int*)vec.data;
typed_data[index] = value;
```

## Design Decisions

### 1. Manual Function Variants

Due to function type parameters not supporting interface bounds yet:
```cb
// Ideal (future):
void vector_push<T, A: Allocator>(Vector<T, A>& vec, T value);

// Current workaround:
void vector_push_int_system(Vector<int, SystemAllocator>& vec, int value);
void vector_push_int_bump(Vector<int, BumpAllocator>& vec, int value);
```

### 2. Capacity Full Handling

Currently returns early when full:
```cb
if (vec.length >= vec.capacity) {
    println("Capacity full!");
    return;
}
```

Future: Auto-resize when full (like std::vector in C++).

### 3. Error Handling

Currently uses simple checks and messages:
```cb
if (vec.length <= 0) {
    println("Empty! Cannot pop");
    return 0;
}
```

Future: Proper error types or panic mechanism.

## Performance Characteristics

### Push Operation
- **Time**: O(1) (when capacity available)
- **Space**: O(1)

### Pop Operation
- **Time**: O(1)
- **Space**: O(1)

### Resize Operation
- **Time**: O(n) (when actual copy is implemented)
- **Space**: O(n) (new allocation)

## Next Steps (Week 2 Remaining)

### Priority 1: Implement Type Casting
**Goal**: Enable `(int*)vec.data` syntax

**Requires**:
- Parser extension for cast expressions
- Type checker support
- Runtime cast operations

### Priority 2: Implement sizeof Operator
**Goal**: Calculate type sizes for allocation

```cb
void* new_data = alloc.allocate(capacity * sizeof(int));
```

**Requires**:
- Parser for `sizeof(type)` syntax
- Type size calculation
- Integration with allocators

### Priority 3: Array Access via Pointer
**Goal**: Store and retrieve actual data

```cb
((int*)vec.data)[index] = value;
int value = ((int*)vec.data)[index];
```

**Requires**:
- Type casting (Priority 1)
- Pointer arithmetic or array access
- Memory safety checks

### Priority 4: Auto-resize on Push
**Goal**: Automatic capacity growth

```cb
void vector_push(...) {
    if (vec.length >= vec.capacity) {
        vector_resize(..., vec.capacity * 2);  // Auto-grow
    }
    // ... push logic
}
```

## Week 2 Progress

```
Week 2 Timeline:
├─ Day 1: Vector Operations (✅ Complete)
│  ├─ Push operation
│  ├─ Pop operation
│  └─ Resize operation
├─ Day 2: Type Casting (⚪ Planned)
├─ Day 3: sizeof Operator (⚪ Planned)
├─ Day 4: Actual Data Storage (⚪ Planned)
└─ Day 5: Integration Tests (⚪ Planned)
```

**Current Progress**: 20% of Week 2 complete

## Code Statistics

### Lines Added
- Vector operations: ~80 lines
- Test cases: ~60 lines
- **Total**: ~140 lines

### Files Modified
- `stdlib/collections/vector.cb`

### Test Scenarios
- 11 test scenarios
- 100% passing

## Key Achievements

1. ✅ **Vector Operations Framework**: Push/pop/resize foundation
2. ✅ **Dual Allocator Support**: Both SystemAllocator and BumpAllocator
3. ✅ **Error Handling**: Capacity full and empty checks
4. ✅ **Zero-Cost Abstraction**: Same operations, different allocators
5. ✅ **Comprehensive Testing**: 11 scenarios covering edge cases

## Lessons Learned

### 1. Function Type Parameters Limitation
**Issue**: Functions can't use interface bounds yet
**Workaround**: Manual function variants per allocator
**Future**: Extend parser in Week 3

### 2. Placeholder Pattern Works
**Approach**: Implement logic first, actual memory later
**Benefit**: Can test control flow without full implementation
**Next**: Add type casting and sizeof

### 3. Test-Driven Development
**Approach**: Write tests alongside implementation
**Benefit**: Caught edge cases (empty pop, full push)
**Result**: More robust code

## Conclusion

Day 1 of Week 2 successfully implements the **logical foundation** for Vector operations. While actual memory management is still placeholder, the control flow, error handling, and interface design are complete and tested.

This foundation enables Week 2's remaining work to focus on:
1. Type system enhancements (casting, sizeof)
2. Actual memory operations
3. Integration with real allocators

**Next Session**: Implement type casting to enable actual data storage in Vector.

---

**Status**: ✅ Week 2 Day 1 Complete  
**Next**: Week 2 Day 2 - Type Casting Implementation
