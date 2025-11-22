// v0.14.0: HIR Node Implementation
// HIRTypeのコピーコンストラクタなどの実装

#include "hir_node.h"

namespace cb {
namespace ir {
namespace hir {

// HIRTypeのコピーコンストラクタ
HIRType::HIRType(const HIRType &other)
    : kind(other.kind), name(other.name),
      array_dimensions(other.array_dimensions), array_size(other.array_size),
      is_const(other.is_const), is_static(other.is_static),
      is_volatile(other.is_volatile), is_pointer_const(other.is_pointer_const),
      is_pointee_const(other.is_pointee_const), is_unsigned(other.is_unsigned) {

    if (other.inner_type) {
        inner_type = std::make_unique<HIRType>(*other.inner_type);
    }

    if (other.return_type) {
        return_type = std::make_unique<HIRType>(*other.return_type);
    }

    param_types = other.param_types;
    generic_args = other.generic_args;
}

// HIRTypeの代入演算子
HIRType &HIRType::operator=(const HIRType &other) {
    if (this != &other) {
        kind = other.kind;
        name = other.name;
        array_dimensions = other.array_dimensions;
        array_size = other.array_size;
        is_const = other.is_const;
        is_static = other.is_static;
        is_volatile = other.is_volatile;
        is_pointer_const = other.is_pointer_const;
        is_pointee_const = other.is_pointee_const;
        is_unsigned = other.is_unsigned;

        if (other.inner_type) {
            inner_type = std::make_unique<HIRType>(*other.inner_type);
        } else {
            inner_type.reset();
        }

        if (other.return_type) {
            return_type = std::make_unique<HIRType>(*other.return_type);
        } else {
            return_type.reset();
        }

        param_types = other.param_types;
        generic_args = other.generic_args;
    }
    return *this;
}

} // namespace hir
} // namespace ir
} // namespace cb
