# Union Type Code Examples

## Example 1: Literal Value Union (Status Codes)

### Cb Code
```cb
typedef StatusCode = 200 | 404 | 500;

int main() {
    StatusCode ok = 200;
    StatusCode notFound = 404;
    StatusCode serverError = 500;
    
    println("Status: {ok}");
    return 0;
}
```

### AST Representation (Current - Working)
```cpp
UnionDefinition {
    name: "StatusCode",
    allowed_values: [200, 404, 500],
    has_literal_values: true,
    is_literal_union(): true
}
```

### What's Missing: HIR and C++ Generation

**Needed HIR:**
```cpp
HIRTypedef {
    name: "StatusCode",
    target_type: {kind: Union, ...},
    union_def: {
        allowed_literals: [(TYPE_INT, "200"), (TYPE_INT, "404"), (TYPE_INT, "500")],
        is_literal_union: true
    }
}
```

**Needed C++ Output:**
```cpp
class StatusCode {
    int value_;
public:
    StatusCode(int v) : value_(v) {
        if (v != 200 && v != 404 && v != 500) {
            throw std::invalid_argument("Invalid StatusCode value");
        }
    }
    operator int() const { return value_; }
};
```

---

## Example 2: Type Union (Numbers)

### Cb Code
```cb
typedef NumericType = int | long;

int main() {
    NumericType small = 42;
    NumericType large = 1000000;
    
    println("Values: {small}, {large}");
    return 0;
}
```

### AST Representation (Current - Working)
```cpp
UnionDefinition {
    name: "NumericType",
    allowed_types: [TYPE_INT, TYPE_LONG],
    has_type_values: true,
    is_type_union(): true
}
```

### What's Missing: HIR and C++ Generation

**Needed HIR:**
```cpp
HIRTypedef {
    name: "NumericType",
    target_type: {kind: Union, ...},
    union_def: {
        allowed_types: [
            {kind: Int},
            {kind: Long}
        ],
        is_type_union: true
    }
}
```

**Needed C++ Output (Option 1 - Type Alias):**
```cpp
using NumericType = int64_t;
// Type checking enforced in semantic analysis
```

**Alternative C++ Output (Option 2 - Variant):**
```cpp
using NumericType = std::variant<int, int64_t>;
```

---

## Example 3: Struct Union (Entity Types)

### Cb Code
```cb
struct Point {
    int x, y;
};

struct Person {
    string name;
    int age;
};

typedef EntityUnion = Point | Person;

int main() {
    Point origin = {x: 0, y: 0};
    Person alice = {name: "Alice", age: 25};
    
    EntityUnion entity1 = origin;
    EntityUnion entity2 = alice;
    
    println("Point: ({origin.x}, {origin.y})");
    println("Person: {alice.name}, {alice.age}");
    return 0;
}
```

### AST Representation (Current - Working)
```cpp
UnionDefinition {
    name: "EntityUnion",
    allowed_custom_types: ["Point", "Person"],
    has_custom_types: true,
    is_custom_type_union(): true
}
```

### What's Missing: HIR and C++ Generation

**Needed HIR:**
```cpp
HIRTypedef {
    name: "EntityUnion",
    target_type: {kind: Union, ...},
    union_def: {
        allowed_types: [
            {kind: Struct, name: "Point"},
            {kind: Struct, name: "Person"}
        ],
        is_custom_type_union: true
    }
}
```

**Needed C++ Output:**
```cpp
using EntityUnion = std::variant<Point, Person>;

// Or with base class pattern:
class EntityUnion {
    std::variant<Point, Person> value_;
public:
    EntityUnion(const Point& p) : value_(p) {}
    EntityUnion(const Person& p) : value_(p) {}
    
    bool is_point() const { return std::holds_alternative<Point>(value_); }
    bool is_person() const { return std::holds_alternative<Person>(value_); }
    
    const Point& as_point() const { return std::get<Point>(value_); }
    const Person& as_person() const { return std::get<Person>(value_); }
};
```

---

## Example 4: Array Union (Mixed Arrays)

### Cb Code
```cb
typedef ArrayUnion = int[3] | bool[2];

int main() {
    int[3] numbers = [1, 2, 3];
    bool[2] flags = [true, false];
    
    ArrayUnion array1 = numbers;
    ArrayUnion array2 = flags;
    
    println("Numbers: [{numbers[0]}, {numbers[1]}, {numbers[2]}]");
    println("Flags: [{flags[0]}, {flags[1]}]");
    return 0;
}
```

### AST Representation (Current - Working)
```cpp
UnionDefinition {
    name: "ArrayUnion",
    allowed_array_types: ["int[3]", "bool[2]"],
    has_array_types: true,
    is_array_type_union(): true
}
```

### What's Missing: HIR and C++ Generation

**Needed HIR:**
```cpp
HIRTypedef {
    name: "ArrayUnion",
    target_type: {kind: Union, ...},
    union_def: {
        allowed_types: [
            {kind: Array, inner: {kind: Int}, array_size: 3},
            {kind: Array, inner: {kind: Bool}, array_size: 2}
        ],
        is_array_type_union: true
    }
}
```

**Needed C++ Output:**
```cpp
class ArrayUnion {
private:
    std::variant<std::array<int, 3>, std::array<bool, 2>> value_;
public:
    ArrayUnion(const std::array<int, 3>& arr) : value_(arr) {}
    ArrayUnion(const std::array<bool, 2>& arr) : value_(arr) {}
    
    bool is_int_array() const { 
        return std::holds_alternative<std::array<int, 3>>(value_); 
    }
    bool is_bool_array() const { 
        return std::holds_alternative<std::array<bool, 2>>(value_); 
    }
};
```

---

## Example 5: Mixed Union (Complex)

### Cb Code
```cb
typedef MyInt = int;
struct Point { int x, y; };

typedef MegaUnion = 999 | "special" | true | int | MyInt | Point | int[2];

int main() {
    MegaUnion m1 = 999;           // Literal
    MegaUnion m2 = "special";     // String literal
    MegaUnion m3 = true;          // Boolean literal
    MegaUnion m4 = 888;           // Type (int)
    
    MyInt customInt = 777;
    MegaUnion m5 = customInt;     // Custom type
    
    Point origin = {10, 20};
    MegaUnion m6 = origin;        // Struct type
    
    int[2] coords = [50, 100];
    MegaUnion m7 = coords;        // Array type
    
    return 0;
}
```

### AST Representation (Current - Working)
```cpp
UnionDefinition {
    name: "MegaUnion",
    allowed_values: [999, "special", true],
    allowed_types: [TYPE_INT],
    allowed_custom_types: ["MyInt", "Point"],
    allowed_array_types: ["int[2]"],
    has_literal_values: true,
    has_type_values: true,
    has_custom_types: true,
    has_array_types: true,
    is_mixed_union(): true
}
```

### What's Missing: HIR and C++ Generation

**Needed HIR:**
```cpp
HIRTypedef {
    name: "MegaUnion",
    target_type: {kind: Union, ...},
    union_def: {
        allowed_literals: [
            (TYPE_INT, "999"),
            (TYPE_STRING, "special"),
            (TYPE_BOOL, "true")
        ],
        allowed_types: [{kind: Int}],
        allowed_custom_types: ["MyInt", "Point"],
        allowed_array_types: [{kind: Array, inner: {kind: Int}, array_size: 2}],
        is_mixed_union: true
    }
}
```

**Needed C++ Output (using variant):**
```cpp
class MegaUnion {
private:
    std::variant<
        int,                      // For literals 999 and type int
        std::string,              // For "special"
        bool,                     // For true
        MyInt,                    // Custom type
        Point,                    // Struct type
        std::array<int, 2>        // Array type
    > value_;
    
    // Type tag to track actual type
    enum Type {
        INT_LITERAL, INT_TYPE, STRING_TYPE, BOOL_TYPE, MYINT_TYPE, POINT_TYPE, ARRAY_TYPE
    } type_;
    
public:
    MegaUnion(int v) : value_(v), type_(INT_LITERAL) {
        if (v != 999) type_ = INT_TYPE;
    }
    MegaUnion(const std::string& s) : value_(s), type_(STRING_TYPE) {}
    MegaUnion(bool b) : value_(b), type_(BOOL_TYPE) {}
    MegaUnion(const MyInt& m) : value_(m), type_(MYINT_TYPE) {}
    MegaUnion(const Point& p) : value_(p), type_(POINT_TYPE) {}
    MegaUnion(const std::array<int, 2>& a) : value_(a), type_(ARRAY_TYPE) {}
};
```

---

## Current Issue: What Gets Generated Now

### Input (Cb)
```cb
typedef StatusCode = 200 | 404 | 500;
```

### What Currently Gets Generated (Broken)
```cpp
// In generate_typedefs() - reads only ONE type from HIRTypedef
using StatusCode = int;  // WRONG! No validation, no constraints
```

### What Should Be Generated
```cpp
// Proper literal union with validation
class StatusCode {
    int value_;
    static const int ALLOWED_VALUES[] = {200, 404, 500};
public:
    StatusCode(int v) : value_(v) {
        for (int allowed : ALLOWED_VALUES) {
            if (v == allowed) return;
        }
        throw std::invalid_argument("Invalid StatusCode value");
    }
    operator int() const { return value_; }
};
```

---

## Implementation Checklist

### HIR Level Changes
- [ ] Add `Union` to `HIRType::TypeKind` enum
- [ ] Create `HIRUnion` struct in hir_node.h
- [ ] Add `union_def` field to `HIRTypedef`
- [ ] Add AST_TYPEDEF case in hir_generator.cpp
- [ ] Implement `convert_union_typedef()` method
- [ ] Extract union constraints from UnionDefinition

### Codegen Level Changes
- [ ] Modify `generate_typedefs()` to detect unions
- [ ] Implement `generate_union_typedef()` method
- [ ] Support literal union code generation
- [ ] Support type union code generation
- [ ] Support struct union code generation
- [ ] Support array union code generation
- [ ] Support mixed union code generation

### Testing
- [ ] Test literal_union.cb
- [ ] Test type_union.cb
- [ ] Test struct_union.cb
- [ ] Test array_union.cb
- [ ] Test custom_union.cb
- [ ] Test mixed_union.cb
- [ ] Test comprehensive.cb (38 test items)
- [ ] Test all 6 error cases
