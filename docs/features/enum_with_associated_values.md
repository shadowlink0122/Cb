# Enum with Associated Values - Usage Guide

## Overview

Cb v0.11.0 introduces support for **enums with associated values**, inspired by Rust's powerful enum system. Enums can be generic and each variant can optionally carry associated data.

## Syntax

### Enum Declaration

```cb
enum EnumName<T> {
    VariantWithValue(T),
    VariantWithoutValue
};
```

### Enum Construction

```cb
// With associated value
EnumName<int> x = EnumName<int>::VariantWithValue(42);

// Without associated value
EnumName<int> y = EnumName<int>::VariantWithoutValue;
```

### Member Access

```cb
// Access variant name
string variant_name = x.variant;  // "VariantWithValue"

// Access associated value
int value = x.value;  // 42
```

## Complete Example

```cb
enum Option<T> {
    Some(T),
    None
};

int main() {
    // Create Some variant
    Option<int> some_num = Option<int>::Some(42);
    println("Variant:", some_num.variant);  // "Some"
    println("Value:", some_num.value);      // 42
    
    // Create None variant
    Option<int> none_num = Option<int>::None;
    println("Variant:", none_num.variant);  // "None"
    
    return 0;
}
```

## See Also

- Comprehensive tests: `tests/cases/generics/enum_comprehensive.cb`
- Implementation docs: `docs/todo/enum_implementation_summary.md`
