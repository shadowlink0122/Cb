#include "ast.h"
#include <sstream>

std::string ArrayTypeInfo::to_string() const {
    if (!is_array()) {
        return type_info_to_string(base_type);
    }

    std::ostringstream oss;
    oss << type_info_to_string(base_type);

    for (const auto &dim : dimensions) {
        oss << "[";
        if (!dim.is_dynamic) {
            oss << dim.size;
        }
        oss << "]";
    }

    return oss.str();
}

TypeInfo ArrayTypeInfo::to_legacy_type_id() const {
    if (!is_array()) {
        return base_type;
    }

    // レガシー互換性のため、基底型 + TYPE_ARRAY_BASE を返す
    return static_cast<TypeInfo>(TYPE_ARRAY_BASE + base_type);
}
