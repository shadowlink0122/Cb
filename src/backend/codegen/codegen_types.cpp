// v0.14.0: HIR to C++ Transpiler - Type Generation
// 型生成モジュール

#include "hir_to_cpp.h"
#include "../../common/debug.h"

namespace cb {
namespace codegen {

using namespace ir::hir;

std::string HIRToCpp::generate_type(const HIRType &type) {
    if (debug_mode) {
        std::cerr << "[CODEGEN_TYPE] Generating type: kind=" << static_cast<int>(type.kind) 
                  << ", name=" << type.name << std::endl;
    }
    
    std::string result;
    
    // static修飾子
    if (type.is_static) {
        result += "static ";
    }
    
    // const修飾子（値型の場合）
    if (type.is_const && type.kind != HIRType::TypeKind::Pointer) {
        result += "const ";
    }
    
    // 基本型
    switch (type.kind) {
    case HIRType::TypeKind::Void:
        result += "void";
        break;
    case HIRType::TypeKind::Tiny:
        result += "int8_t";
        break;
    case HIRType::TypeKind::Short:
        result += "int16_t";
        break;
    case HIRType::TypeKind::Int:
        result += "int";
        break;
    case HIRType::TypeKind::Long:
        result += "int64_t";
        break;
    case HIRType::TypeKind::UnsignedTiny:
        result += "uint8_t";
        break;
    case HIRType::TypeKind::UnsignedShort:
        result += "uint16_t";
        break;
    case HIRType::TypeKind::UnsignedInt:
        result += "unsigned";
        break;
    case HIRType::TypeKind::UnsignedLong:
        result += "uint64_t";
        break;
    case HIRType::TypeKind::Char:
        result += "char";
        break;
    case HIRType::TypeKind::String:
        result += "std::string";
        break;
    case HIRType::TypeKind::Bool:
        result += "bool";
        break;
    case HIRType::TypeKind::Float:
        result += "float";
        break;
    case HIRType::TypeKind::Double:
        result += "double";
        break;
    case HIRType::TypeKind::Struct:
    case HIRType::TypeKind::Enum:
    case HIRType::TypeKind::Interface:
        result += type.name;
        break;
    case HIRType::TypeKind::Pointer:
        if (debug_mode) {
            std::cerr << "[CODEGEN_TYPE] Delegating to generate_pointer_type" << std::endl;
        }
        return generate_pointer_type(type);
    case HIRType::TypeKind::Reference:
        return generate_reference_type(type);
    case HIRType::TypeKind::RvalueReference:
        return generate_rvalue_reference_type(type);
    case HIRType::TypeKind::Array:
        return generate_array_type(type);
    case HIRType::TypeKind::Function:
        return generate_function_type(type);
    case HIRType::TypeKind::Generic:
        result += type.name;
        break;
    case HIRType::TypeKind::Nullptr:
        result += "std::nullptr_t";
        break;
    case HIRType::TypeKind::Unknown:
        // ジェネリック型パラメータがある場合は最初のものを使用
        if (!current_generic_params.empty()) {
            result += current_generic_params[0];
        } else {
            result += "/* unknown type */";
        }
        break;
    default:
        result += "/* unknown type */";
        break;
    }
    
    return result;
}

std::string HIRToCpp::generate_basic_type(const HIRType &type) {
    return generate_type(type);
}

std::string HIRToCpp::generate_pointer_type(const HIRType &type) {
    std::string inner_type_name = type.inner_type ? generate_type(*type.inner_type) : type.name;
    debug_msg(DebugMsgId::CODEGEN_CPP_POINTER_TYPE_START, inner_type_name.c_str());
    
    if (debug_mode) {
        std::cerr << "[CODEGEN_PTR] Pointer type: has_inner=" << (type.inner_type != nullptr)
                  << ", name=" << type.name << std::endl;
    }
    
    std::string result;
    
    // const T* (pointer to const)
    if (type.is_pointee_const && type.inner_type) {
        if (debug_mode) {
            std::cerr << "[CODEGEN_PTR] Generating const T* (recursive call)" << std::endl;
        }
        result = "const " + generate_type(*type.inner_type) + "*";
        debug_msg(DebugMsgId::CODEGEN_CPP_POINTER_TO_CONST, 
                  generate_type(*type.inner_type).c_str());
    } else if (type.inner_type) {
        if (debug_mode) {
            std::cerr << "[CODEGEN_PTR] Generating T* (recursive call)" << std::endl;
        }
        result = generate_type(*type.inner_type) + "*";
        debug_msg(DebugMsgId::CODEGEN_CPP_POINTER_TYPE, 
                  generate_type(*type.inner_type).c_str());
    } else if (!type.name.empty()) {
        if (debug_mode) {
            std::cerr << "[CODEGEN_PTR] Using type.name=" << type.name << std::endl;
        }
        // If name contains "*", it's already a pointer type with the * in the name
        if (type.name.back() == '*') {
            result = type.name;
        } else {
            result = type.name + "*";
        }
        debug_msg(DebugMsgId::CODEGEN_CPP_POINTER_TYPE, type.name.c_str());
    } else {
        if (debug_mode) {
            std::cerr << "[CODEGEN_PTR] Fallback to void*" << std::endl;
        }
        result = "void*";
        debug_msg(DebugMsgId::CODEGEN_CPP_POINTER_TYPE, "void");
    }
    
    // T* const (const pointer)
    if (type.is_pointer_const) {
        result += " const";
        debug_msg(DebugMsgId::CODEGEN_CPP_POINTER_CONST, inner_type_name.c_str());
    }
    
    return result;
}

std::string HIRToCpp::generate_reference_type(const HIRType &type) {
    if (type.inner_type) {
        return generate_type(*type.inner_type) + "&";
    }
    return type.name + "&";
}

std::string HIRToCpp::generate_rvalue_reference_type(const HIRType &type) {
    if (type.inner_type) {
        return generate_type(*type.inner_type) + "&&";
    }
    return type.name + "&&";
}

std::string HIRToCpp::generate_array_type(const HIRType &type) {
    if (!type.inner_type) {
        return "std::vector<int>"; // fallback
    }
    
    // 多次元配列のサポート
    if (!type.array_dimensions.empty()) {
        std::string result = generate_type(*type.inner_type);
        
        // 各次元に対してstd::arrayまたはstd::vectorでラップ
        for (auto it = type.array_dimensions.rbegin(); it != type.array_dimensions.rend(); ++it) {
            int size = *it;
            if (size > 0) {
                // 固定長配列
                result = "std::array<" + result + ", " + std::to_string(size) + ">";
            } else {
                // 動的配列
                result = "std::vector<" + result + ">";
            }
        }
        
        return result;
    }
    
    // 1次元配列（後方互換性）
    if (type.array_size > 0) {
        // 固定長配列
        return "std::array<" + generate_type(*type.inner_type) + ", " +
               std::to_string(type.array_size) + ">";
    } else {
        // 動的配列
        return "std::vector<" + generate_type(*type.inner_type) + ">";
    }
}

std::string HIRToCpp::generate_function_type(const HIRType &type) {
    // Generate function pointer type: RetType (*)(Param1, Param2, ...)
    // Note: The actual pointer syntax with name is handled in var_decl
    
    std::string result;
    
    if (type.return_type) {
        result += generate_type(*type.return_type);
    } else {
        result += "void";
    }
    
    result += " (*)(";
    for (size_t i = 0; i < type.param_types.size(); i++) {
        if (i > 0)
            result += ", ";
        result += generate_type(type.param_types[i]);
    }
    result += ")";
    
    return result;
}

} // namespace codegen
} // namespace cb
