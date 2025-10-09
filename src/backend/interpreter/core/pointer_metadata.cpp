#include "pointer_metadata.h"
#include "../../../common/debug.h"
#include "../managers/variables/manager.h"
#include <sstream>
#include <stdexcept>

namespace PointerSystem {

// 変数へのポインタを作成
PointerMetadata PointerMetadata::create_variable_pointer(Variable *var) {
    PointerMetadata meta;
    meta.target_type = PointerTargetType::VARIABLE;
    meta.var_ptr = var;
    return meta;
}

// 配列要素へのポインタを作成
PointerMetadata PointerMetadata::create_array_element_pointer(
    Variable *array_var_param, size_t index, TypeInfo elem_type) {
    PointerMetadata meta;
    meta.target_type = PointerTargetType::ARRAY_ELEMENT;
    meta.array_var = array_var_param;
    meta.element_index = index;
    meta.element_type = elem_type;
    meta.pointed_type = elem_type;
    meta.type_size = get_type_size(elem_type);

    // 真のポインタシステム：配列要素の実際のメモリアドレスを取得
    // array_values[index]のアドレスを取得
    if (array_var_param && !array_var_param->array_values.empty() &&
        index < array_var_param->array_values.size()) {
        meta.address =
            reinterpret_cast<uintptr_t>(&array_var_param->array_values[index]);
    } else if (array_var_param &&
               !array_var_param->multidim_array_values.empty() &&
               index < array_var_param->multidim_array_values.size()) {
        meta.address = reinterpret_cast<uintptr_t>(
            &array_var_param->multidim_array_values[index]);
    } else {
        // 配列がまだ初期化されていない場合、仮想アドレスを使用
        uintptr_t base_addr = reinterpret_cast<uintptr_t>(array_var_param);
        meta.address = base_addr + (index * meta.type_size);
    }

    // 範囲チェック用の情報を設定
    if (array_var_param && !array_var_param->array_values.empty()) {
        meta.array_start_addr =
            reinterpret_cast<uintptr_t>(&array_var_param->array_values[0]);
        meta.array_end_addr = meta.array_start_addr +
                              (array_var_param->array_size * sizeof(int64_t));
    } else if (array_var_param &&
               !array_var_param->multidim_array_values.empty()) {
        meta.array_start_addr = reinterpret_cast<uintptr_t>(
            &array_var_param->multidim_array_values[0]);
        meta.array_end_addr = meta.array_start_addr +
                              (array_var_param->array_size * sizeof(int64_t));
    } else {
        // フォールバック：仮想アドレス
        uintptr_t base_addr = reinterpret_cast<uintptr_t>(array_var_param);
        meta.array_start_addr = base_addr;
        meta.array_end_addr =
            base_addr + (array_var_param->array_size * meta.type_size);
    }

    return meta;
}

// 構造体メンバーへのポインタを作成
PointerMetadata
PointerMetadata::create_struct_member_pointer(Variable *member_var_param,
                                              const std::string &path) {
    PointerMetadata meta;
    meta.target_type = PointerTargetType::STRUCT_MEMBER;
    meta.member_var = member_var_param;
    meta.member_path = path;
    return meta;
}

// nullptrを作成
PointerMetadata PointerMetadata::create_nullptr() {
    PointerMetadata meta;
    meta.target_type = PointerTargetType::NULLPTR_VALUE;
    meta.var_ptr = nullptr;
    return meta;
}

// 整数値を読み取り
int64_t PointerMetadata::read_int_value() const {
    if (target_type == PointerTargetType::NULLPTR_VALUE) {
        throw std::runtime_error("Cannot dereference nullptr");
    }

    // 真のポインタシステム：配列要素の場合
    if (target_type == PointerTargetType::ARRAY_ELEMENT && array_var) {
        size_t idx = element_index;
        Variable *arr = array_var;

        if (!arr || !arr->is_array) {
            throw std::runtime_error("Invalid array pointer");
        }

        if (idx >= static_cast<size_t>(arr->array_size)) {
            throw std::runtime_error(
                "Array index out of bounds in pointer dereference");
        }

        // 配列要素の型に応じて読み取り
        if (arr->is_multidimensional && !arr->multidim_array_values.empty()) {
            return arr->multidim_array_values[idx];
        } else if (!arr->array_values.empty()) {
            return arr->array_values[idx];
        }

        return 0;
    }

    // 変数へのポインタの場合
    if (target_type == PointerTargetType::VARIABLE && var_ptr) {
        return var_ptr->value;
    }

    // 構造体メンバーの場合
    if (target_type == PointerTargetType::STRUCT_MEMBER && member_var) {
        return member_var->value;
    }

    throw std::runtime_error("Invalid pointer dereference");
}

// 整数値を書き込み
void PointerMetadata::write_int_value(int64_t value) {
    if (target_type == PointerTargetType::NULLPTR_VALUE) {
        throw std::runtime_error("Cannot write through nullptr");
    }

    // 真のポインタシステム：配列要素の場合
    if (target_type == PointerTargetType::ARRAY_ELEMENT && array_var) {
        size_t idx = element_index;
        Variable *arr = array_var;

        if (!arr || !arr->is_array) {
            throw std::runtime_error("Invalid array pointer");
        }

        if (idx >= static_cast<size_t>(arr->array_size)) {
            throw std::runtime_error(
                "Array index out of bounds in pointer write");
        }

        // 配列要素の型に応じて書き込み
        if (arr->is_multidimensional && !arr->multidim_array_values.empty()) {
            arr->multidim_array_values[idx] = value;
        } else {
            if (arr->array_values.empty()) {
                arr->array_values.resize(arr->array_size, 0);
            }
            arr->array_values[idx] = value;
        }
        arr->is_assigned = true;
        return;
    }

    // 変数へのポインタの場合
    if (target_type == PointerTargetType::VARIABLE && var_ptr) {
        var_ptr->value = value;
        var_ptr->is_assigned = true;
        return;
    }

    // 構造体メンバーの場合
    if (target_type == PointerTargetType::STRUCT_MEMBER && member_var) {
        member_var->value = value;
        member_var->is_assigned = true;
        return;
    }

    throw std::runtime_error("Invalid pointer write");
}

// 浮動小数点数値を読み取り
double PointerMetadata::read_float_value() const {
    switch (target_type) {
    case PointerTargetType::VARIABLE:
        if (var_ptr) {
            return var_ptr->double_value;
        }
        throw std::runtime_error("Cannot read from null variable pointer");

    case PointerTargetType::ARRAY_ELEMENT: {
        Variable *arr = array_var;
        size_t idx = element_index;
        TypeInfo elem_type_local = element_type;

        if (!arr || !arr->is_array) {
            throw std::runtime_error("Invalid array pointer");
        }

        if (idx >= static_cast<size_t>(arr->array_size)) {
            throw std::runtime_error(
                "Array index out of bounds in pointer dereference");
        }

        // 型に応じて適切な配列から読み取り
        if (elem_type_local == TYPE_FLOAT) {
            if (arr->is_multidimensional &&
                !arr->multidim_array_float_values.empty()) {
                return static_cast<double>(
                    arr->multidim_array_float_values[idx]);
            } else if (!arr->array_float_values.empty()) {
                return static_cast<double>(arr->array_float_values[idx]);
            }
        } else if (elem_type_local == TYPE_DOUBLE) {
            if (arr->is_multidimensional &&
                !arr->multidim_array_double_values.empty()) {
                return arr->multidim_array_double_values[idx];
            } else if (!arr->array_double_values.empty()) {
                return arr->array_double_values[idx];
            }
        }

        return 0.0;
    }

    case PointerTargetType::STRUCT_MEMBER:
        if (member_var) {
            return member_var->double_value;
        }
        throw std::runtime_error("Cannot read from null struct member pointer");

    case PointerTargetType::NULLPTR_VALUE:
        throw std::runtime_error("Cannot dereference nullptr");

    default:
        throw std::runtime_error("Unknown pointer target type");
    }
}

// 浮動小数点数値を書き込み
void PointerMetadata::write_float_value(double value) {
    switch (target_type) {
    case PointerTargetType::VARIABLE:
        if (var_ptr) {
            var_ptr->double_value = value;
            var_ptr->is_assigned = true;
            return;
        }
        throw std::runtime_error("Cannot write to null variable pointer");

    case PointerTargetType::ARRAY_ELEMENT: {
        Variable *arr = array_var;
        size_t idx = element_index;
        TypeInfo elem_type_local = element_type;

        if (!arr || !arr->is_array) {
            throw std::runtime_error("Invalid array pointer");
        }

        if (idx >= static_cast<size_t>(arr->array_size)) {
            throw std::runtime_error(
                "Array index out of bounds in pointer write");
        }

        // 型に応じて適切な配列に書き込み
        if (elem_type_local == TYPE_FLOAT) {
            if (arr->is_multidimensional) {
                if (arr->multidim_array_float_values.empty()) {
                    arr->multidim_array_float_values.resize(arr->array_size,
                                                            0.0f);
                }
                arr->multidim_array_float_values[idx] =
                    static_cast<float>(value);
            } else {
                if (arr->array_float_values.empty()) {
                    arr->array_float_values.resize(arr->array_size, 0.0f);
                }
                arr->array_float_values[idx] = static_cast<float>(value);
            }
        } else if (elem_type_local == TYPE_DOUBLE) {
            if (arr->is_multidimensional) {
                if (arr->multidim_array_double_values.empty()) {
                    arr->multidim_array_double_values.resize(arr->array_size,
                                                             0.0);
                }
                arr->multidim_array_double_values[idx] = value;
            } else {
                if (arr->array_double_values.empty()) {
                    arr->array_double_values.resize(arr->array_size, 0.0);
                }
                arr->array_double_values[idx] = value;
            }
        }
        arr->is_assigned = true;
        return;
    }

    case PointerTargetType::STRUCT_MEMBER:
        if (member_var) {
            member_var->double_value = value;
            member_var->is_assigned = true;
            return;
        }
        throw std::runtime_error("Cannot write to null struct member pointer");

    case PointerTargetType::NULLPTR_VALUE:
        throw std::runtime_error("Cannot write through nullptr");

    default:
        throw std::runtime_error("Unknown pointer target type");
    }
}

// デバッグ用文字列表現
std::string PointerMetadata::to_string() const {
    std::ostringstream oss;

    switch (target_type) {
    case PointerTargetType::VARIABLE:
        oss << "Variable@" << static_cast<const void *>(var_ptr);
        break;

    case PointerTargetType::ARRAY_ELEMENT:
        oss << "Array[" << element_index << "]@"
            << static_cast<const void *>(array_var);
        break;

    case PointerTargetType::STRUCT_MEMBER:
        oss << "StructMember(" << member_path << ")@"
            << static_cast<const void *>(member_var);
        break;

    case PointerTargetType::NULLPTR_VALUE:
        oss << "nullptr";
        break;

    default:
        oss << "Unknown";
        break;
    }

    return oss.str();
}

// PointerValue::as_variable_pointer の実装
Variable *PointerValue::as_variable_pointer() const {
    if (has_metadata) {
        // メタデータの場合、変数ポインタのみ変換可能
        if (metadata && metadata->is_variable()) {
            return metadata->var_ptr;
        }
        return nullptr;
    }
    // 従来の方式
    return reinterpret_cast<Variable *>(raw_pointer);
}

} // namespace PointerSystem
