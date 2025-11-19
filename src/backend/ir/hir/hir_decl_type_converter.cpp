/**
 * @file hir_decl_type_converter.cpp
 * @brief HIR Declaration and Type Converter
 */

#include "hir_decl_type_converter.h"
#include "hir_generator.h"
#include "hir_builder.h"
#include "../../../common/debug.h"
#include <iostream>

namespace cb {
namespace ir {

using namespace hir;

HIRDeclTypeConverter::HIRDeclTypeConverter(HIRGenerator *generator)
    : generator_(generator) {}

HIRDeclTypeConverter::~HIRDeclTypeConverter() {}

HIRFunction HIRDeclTypeConverter::convert_function(const ASTNode *node) {
    HIRFunction func;

    if (!node)
        return func;

    func.name = node->name;
    func.location = generator_->convert_location(node->location);

    // Fix: return_type_nameが指定されている場合、適切なTypeInfoを設定
    TypeInfo actual_return_type = node->type_info;
    if (!node->return_type_name.empty()) {
        // Check for known types
        if (node->return_type_name == "void") {
            actual_return_type = TYPE_VOID;
        } else if (node->return_type_name == "int") {
            actual_return_type = TYPE_INT;
        } else if (node->return_type_name == "long") {
            actual_return_type = TYPE_LONG;
        } else if (node->return_type_name == "short") {
            actual_return_type = TYPE_SHORT;
        } else if (node->return_type_name == "tiny") {
            actual_return_type = TYPE_TINY;
        } else if (node->return_type_name == "char") {
            actual_return_type = TYPE_CHAR;
        } else if (node->return_type_name == "string") {
            actual_return_type = TYPE_STRING;
        } else if (node->return_type_name == "bool") {
            actual_return_type = TYPE_BOOL;
        } else if (node->return_type_name == "float") {
            actual_return_type = TYPE_FLOAT;
        } else if (node->return_type_name == "double") {
            actual_return_type = TYPE_DOUBLE;
        } else {
            // v0.14.0: 配列型のチェック（"int[3]" や "int[2][3]" のような形式）
            if (node->return_type_name.find('[') != std::string::npos) {
                // 配列型 - 直接ArrayTypeInfoを構築してconvert_array_typeを使用
                size_t bracket_pos = node->return_type_name.find('[');
                std::string element_type_name =
                    node->return_type_name.substr(0, bracket_pos);

                // ArrayTypeInfoを構築
                ArrayTypeInfo array_info;
                if (element_type_name == "int")
                    array_info.base_type = TYPE_INT;
                else if (element_type_name == "long")
                    array_info.base_type = TYPE_LONG;
                else if (element_type_name == "short")
                    array_info.base_type = TYPE_SHORT;
                else if (element_type_name == "tiny")
                    array_info.base_type = TYPE_TINY;
                else if (element_type_name == "char")
                    array_info.base_type = TYPE_CHAR;
                else if (element_type_name == "bool")
                    array_info.base_type = TYPE_BOOL;
                else if (element_type_name == "float")
                    array_info.base_type = TYPE_FLOAT;
                else if (element_type_name == "double")
                    array_info.base_type = TYPE_DOUBLE;
                else if (element_type_name == "string")
                    array_info.base_type = TYPE_STRING;
                else
                    array_info.base_type = TYPE_STRUCT;

                // すべての次元を解析（例: "int[2][3][4]"）
                size_t pos = bracket_pos;
                while (pos != std::string::npos && pos < node->return_type_name.length()) {
                    size_t open_bracket = node->return_type_name.find('[', pos);
                    if (open_bracket == std::string::npos) break;
                    
                    size_t close_bracket = node->return_type_name.find(']', open_bracket);
                    if (close_bracket == std::string::npos) break;
                    
                    std::string size_str = node->return_type_name.substr(
                        open_bracket + 1, close_bracket - open_bracket - 1);
                    int size = -1;
                    if (!size_str.empty()) {
                        size = std::stoi(size_str);
                    }
                    
                    array_info.dimensions.push_back(ArrayDimension(size, false));
                    pos = close_bracket + 1;
                }

                func.return_type = generator_->convert_array_type(array_info);
                func.is_async = node->is_async;
                func.is_exported = node->is_exported;

                // Skip normal conversion
                goto skip_normal_conversion;
            } else if (actual_return_type == TYPE_INT) {
                // If type_info is INT but type_name is something else, it's
                // likely a struct
                actual_return_type = TYPE_STRUCT;
            }
        }
    }

    func.return_type = generator_->convert_type(actual_return_type, node->return_type_name);
skip_normal_conversion:
    func.is_async = node->is_async;
    func.is_exported = node->is_exported;

    if (debug_mode) {
        if (node->is_async) {
            fprintf(stderr,
                    "  Function %s: ASYNC, return_type_name=%s, type_info=%d\n",
                    func.name.c_str(), node->return_type_name.c_str(),
                    actual_return_type);
        } else if (actual_return_type != TYPE_VOID) {
            fprintf(stderr,
                    "  Function %s: type_info=%d->%d, return_type_name=%s\n",
                    func.name.c_str(), node->type_info, actual_return_type,
                    node->return_type_name.c_str());
        }
    }

    // v0.14.0: ジェネリックパラメータ
    if (node->is_generic) {
        func.generic_params = node->type_parameters;
    }

    // パラメータの変換
    for (const auto &param : node->parameters) {
        HIRFunction::Parameter hir_param;
        hir_param.name = param->name;
        hir_param.type = generator_->convert_type(param->type_info, param->type_name);
        hir_param.is_const = param->is_const;

        // TODO: デフォルト引数は将来実装
        // if (param->default_value) {
        //     hir_param.default_value =
        //     std::make_unique<HIRExpr>(generator_->convert_expr(param->default_value.get()));
        // }

        func.parameters.push_back(hir_param);
    }

    // 関数本体の変換
    if (node->body) {
        func.body = std::make_unique<HIRStmt>(generator_->convert_stmt(node->body.get()));
    }

    return func;
}

HIRStruct HIRDeclTypeConverter::convert_struct(const ASTNode *node) {
    HIRStruct struct_def;

    if (!node)
        return struct_def;

    struct_def.name = node->name;
    struct_def.location = generator_->convert_location(node->location);

    // v0.14.0: ジェネリックパラメータ
    if (node->is_generic) {
        struct_def.generic_params = node->type_parameters;
    }

    // フィールドの変換 (childrenを使用)
    for (const auto &child : node->children) {
        if (child->node_type == ASTNodeType::AST_VAR_DECL) {
            HIRStruct::Field hir_field;
            hir_field.name = child->name;
            hir_field.type = generator_->convert_type(child->type_info, child->type_name);
            hir_field.is_private = child->is_private_member;

            // TODO: デフォルト値は将来実装
            // if (child->right) {
            //     hir_field.default_value =
            //     std::make_unique<HIRExpr>(generator_->convert_expr(child->right.get()));
            // }

            struct_def.fields.push_back(hir_field);
        }
    }

    return struct_def;
}

HIREnum HIRDeclTypeConverter::convert_enum(const ASTNode *node) {
    HIREnum enum_def;

    if (!node)
        return enum_def;

    enum_def.name = node->enum_definition.name;
    enum_def.location = generator_->convert_location(node->location);

    // メンバーの変換
    for (const auto &member : node->enum_definition.members) {
        HIREnum::Variant variant;
        variant.name = member.name;
        variant.value = member.value;
        variant.has_associated_value = member.has_associated_value;
        if (member.has_associated_value) {
            variant.associated_type = generator_->convert_type(member.associated_type,
                                                   member.associated_type_name);
        }
        enum_def.variants.push_back(variant);
    }

    return enum_def;
}

HIRUnion HIRDeclTypeConverter::convert_union(const ASTNode *node) {
    HIRUnion union_def;

    if (!node)
        return union_def;

    union_def.name = node->union_definition.name;
    union_def.location = generator_->convert_location(node->location);

    // リテラル値の変換
    for (const auto &value : node->union_definition.allowed_values) {
        HIRUnion::Variant variant;
        switch (value.value_type) {
        case TYPE_INT:
        case TYPE_LONG:
        case TYPE_SHORT:
        case TYPE_TINY:
        case TYPE_CHAR:
            variant.kind = HIRUnion::Variant::Kind::LiteralInt;
            variant.int_value = value.int_value;
            break;
        case TYPE_STRING:
            variant.kind = HIRUnion::Variant::Kind::LiteralString;
            variant.string_value = value.string_value;
            break;
        case TYPE_BOOL:
            variant.kind = HIRUnion::Variant::Kind::LiteralBool;
            variant.bool_value = value.bool_value;
            break;
        default:
            // Skip unknown types
            continue;
        }
        union_def.variants.push_back(variant);
    }

    // 許可される型の変換 (int | string など)
    for (const auto &type : node->union_definition.allowed_types) {
        HIRUnion::Variant variant;
        variant.kind = HIRUnion::Variant::Kind::Type;
        variant.type = generator_->convert_type(type, "");
        union_def.variants.push_back(variant);
    }

    // カスタム型の変換 (struct名など)
    for (const auto &custom_type : node->union_definition.allowed_custom_types) {
        HIRUnion::Variant variant;
        variant.kind = HIRUnion::Variant::Kind::Type;
        HIRType hir_type;
        hir_type.kind = HIRType::TypeKind::Struct;
        hir_type.name = custom_type;
        variant.type = hir_type;
        union_def.variants.push_back(variant);
    }

    // 配列型の変換
    for (const auto &array_type_str : node->union_definition.allowed_array_types) {
        HIRUnion::Variant variant;
        variant.kind = HIRUnion::Variant::Kind::Type;
        HIRType hir_type;
        hir_type.kind = HIRType::TypeKind::Array;

        // 配列型文字列をパース（例：int[3], bool[2]）
        size_t bracket_pos = array_type_str.find('[');
        if (bracket_pos != std::string::npos) {
            std::string element_type_str = array_type_str.substr(0, bracket_pos);
            std::string size_str = array_type_str.substr(bracket_pos + 1);
            // Remove closing bracket
            size_t close_pos = size_str.find(']');
            if (close_pos != std::string::npos) {
                size_str = size_str.substr(0, close_pos);
            }

            // 要素型を設定
            hir_type.inner_type = std::make_unique<HIRType>();
            if (element_type_str == "int") {
                hir_type.inner_type->kind = HIRType::TypeKind::Int;
            } else if (element_type_str == "bool") {
                hir_type.inner_type->kind = HIRType::TypeKind::Bool;
            } else if (element_type_str == "string") {
                hir_type.inner_type->kind = HIRType::TypeKind::String;
            } else if (element_type_str == "char") {
                hir_type.inner_type->kind = HIRType::TypeKind::Char;
            } else if (element_type_str == "float") {
                hir_type.inner_type->kind = HIRType::TypeKind::Float;
            } else if (element_type_str == "double") {
                hir_type.inner_type->kind = HIRType::TypeKind::Double;
            } else if (element_type_str == "long" || element_type_str == "int64") {
                hir_type.inner_type->kind = HIRType::TypeKind::Long;
            } else {
                // カスタム型（構造体など）
                hir_type.inner_type->kind = HIRType::TypeKind::Struct;
                hir_type.inner_type->name = element_type_str;
            }

            // サイズを設定
            if (!size_str.empty()) {
                hir_type.array_size = std::stoi(size_str);
            } else {
                hir_type.array_size = 0; // 動的配列
            }
        } else {
            // ブラケットがない場合は動的配列として扱う
            hir_type.inner_type = std::make_unique<HIRType>();
            hir_type.inner_type->kind = HIRType::TypeKind::Int;
            hir_type.array_size = 0;
        }

        variant.type = std::move(hir_type);
        union_def.variants.push_back(std::move(variant));
    }

    return union_def;
}

HIRInterface HIRDeclTypeConverter::convert_interface(const ASTNode *node) {
    HIRInterface interface_def;

    if (!node)
        return interface_def;

    interface_def.name = node->name;
    interface_def.location = generator_->convert_location(node->location);

    // v0.14.0: Track interface names for value type resolution
    generator_->interface_names_.insert(node->name);

    // メソッドシグネチャの変換
    for (const auto &child : node->children) {
        if (child->node_type == ASTNodeType::AST_FUNC_DECL) {
            HIRInterface::MethodSignature method;
            method.name = child->name;
            method.return_type =
                generator_->convert_type(child->type_info, child->return_type_name);

            for (const auto &param : child->parameters) {
                HIRFunction::Parameter hir_param;
                hir_param.name = param->name;
                hir_param.type =
                    generator_->convert_type(param->type_info, param->type_name);
                method.parameters.push_back(hir_param);
            }

            interface_def.methods.push_back(method);
        }
    }

    return interface_def;
}

HIRImpl HIRDeclTypeConverter::convert_impl(const ASTNode *node) {
    HIRImpl impl_def;

    if (!node)
        return impl_def;

    impl_def.struct_name = node->struct_name;
    impl_def.interface_name = node->interface_name;
    impl_def.location = generator_->convert_location(node->location);

    // v0.14.0: ジェネリックパラメータ
    if (node->is_generic) {
        impl_def.generic_params = node->type_parameters;
    }

    if (debug_mode) {
        fprintf(stderr,
                "Converting impl for %s (interface: %s, children: %zu)\n",
                impl_def.struct_name.c_str(), impl_def.interface_name.c_str(),
                node->children.size());
    }

    // メソッドの変換
    for (const auto &child : node->children) {
        if (child->node_type == ASTNodeType::AST_FUNC_DECL) {
            impl_def.methods.push_back(convert_function(child.get()));
            if (debug_mode) {
                fprintf(stderr, "  Converted impl method: %s\n",
                        child->name.c_str());
            }
        }
    }

    return impl_def;
}
HIRType HIRDeclTypeConverter::convert_array_type(const ArrayTypeInfo &array_info) {
    HIRType hir_type;
    hir_type.kind = HIRType::TypeKind::Array;

    // 基底型を変換
    if (array_info.base_type != TYPE_UNKNOWN) {
        hir_type.inner_type = std::make_unique<HIRType>();
        *hir_type.inner_type = generator_->convert_type(array_info.base_type);
    }

    // 多次元配列のサポート
    if (!array_info.dimensions.empty()) {
        const auto &first_dim = array_info.dimensions[0];
        if (!first_dim.is_dynamic && first_dim.size > 0) {
            hir_type.array_size = first_dim.size;
        } else {
            hir_type.array_size = -1; // Dynamic/VLA
        }
        // サイズ式を保存（変数参照の場合）
        if (first_dim.is_dynamic && !first_dim.size_expr.empty()) {
            hir_type.name = first_dim.size_expr; // Store size expression
        }

        // 2次元目以降を処理（再帰的にinner_typeを配列型にする）
        if (array_info.dimensions.size() > 1) {
            // 残りの次元で新しいArrayTypeInfoを作成
            ArrayTypeInfo inner_array_info;
            inner_array_info.base_type = array_info.base_type;
            inner_array_info.dimensions.assign(
                array_info.dimensions.begin() + 1, array_info.dimensions.end());

            // 既存のinner_typeを置き換え
            hir_type.inner_type = std::make_unique<HIRType>();
            *hir_type.inner_type = generator_->convert_array_type(inner_array_info);
        }

        if (debug_mode) {
            std::cerr << "[HIR_ARRAY_TYPE] is_dynamic=" << first_dim.is_dynamic
                      << ", size=" << first_dim.size
                      << ", size_expr=" << first_dim.size_expr
                      << ", array_size=" << hir_type.array_size
                      << ", name=" << hir_type.name
                      << ", dimensions=" << array_info.dimensions.size()
                      << std::endl;
        }
    }

    return hir_type;
}

/**
 * @brief Main type conversion function
 * @param type_info TypeInfo from AST
 * @param type_name Type name string (optional)
 * @return HIRType converted type
 * 
 * Handles all type conversions:
 *  - Primitive types: int, float, bool, etc.
 *  - Compound types: arrays, pointers, references
 *  - User types: struct, enum, interface
 *  - Special types: function pointers, generics, nullptr
 *  - Type modifiers: const, unsigned
 */
HIRType HIRDeclTypeConverter::convert_type(TypeInfo type_info,
                                   const std::string &type_name) {
    HIRType hir_type;
    
    // 型名から TypeInfo を推測（type_info が不明な場合）
    TypeInfo actual_type_info = type_info;
    if (type_info == -1 || type_info == 0) {
        // 型名から TypeInfo を推測
        if (type_name.find("function_pointer:") == 0) {
            actual_type_info = TYPE_FUNCTION_POINTER;
        }
    }
    
    // 関数ポインタ型の特別処理
    // 型名が "function_pointer:TypeName" の形式の場合、プレフィックスを除去
    std::string actual_type_name = type_name;
    if (type_name.find("function_pointer:") == 0) {
        actual_type_name = type_name.substr(17); // "function_pointer:" の長さ = 17
    }

    // 基本型の変換
    switch (actual_type_info) {
    case TYPE_VOID:
        hir_type.kind = HIRType::TypeKind::Void;
        break;
    case TYPE_TINY:
        hir_type.kind = HIRType::TypeKind::Tiny;
        break;
    case TYPE_SHORT:
        hir_type.kind = HIRType::TypeKind::Short;
        break;
    case TYPE_INT:
        hir_type.kind = HIRType::TypeKind::Int;
        break;
    case TYPE_LONG:
        hir_type.kind = HIRType::TypeKind::Long;
        break;
    case TYPE_UNSIGNED_TINY:
        hir_type.kind = HIRType::TypeKind::UnsignedTiny;
        hir_type.is_unsigned = true;
        break;
    case TYPE_UNSIGNED_SHORT:
        hir_type.kind = HIRType::TypeKind::UnsignedShort;
        hir_type.is_unsigned = true;
        break;
    case TYPE_UNSIGNED_INT:
        hir_type.kind = HIRType::TypeKind::UnsignedInt;
        hir_type.is_unsigned = true;
        break;
    case TYPE_UNSIGNED_LONG:
        hir_type.kind = HIRType::TypeKind::UnsignedLong;
        hir_type.is_unsigned = true;
        break;
    case TYPE_CHAR:
        hir_type.kind = HIRType::TypeKind::Char;
        break;
    case TYPE_STRING:
        hir_type.kind = HIRType::TypeKind::String;
        break;
    case TYPE_BOOL:
        hir_type.kind = HIRType::TypeKind::Bool;
        break;
    case TYPE_FLOAT:
        hir_type.kind = HIRType::TypeKind::Float;
        break;
    case TYPE_DOUBLE:
        hir_type.kind = HIRType::TypeKind::Double;
        break;
    case TYPE_STRUCT:
        hir_type.kind = HIRType::TypeKind::Struct;
        hir_type.name = actual_type_name;

        // v0.14.0: Check if this is a value-type interface (Interface_Value)
        if (actual_type_name.length() > 6 &&
            actual_type_name.substr(actual_type_name.length() - 6) == "_Value") {
            std::string base_name = actual_type_name.substr(0, actual_type_name.length() - 6);
            // If the base name is a known interface, this is valid
            if (generator_->interface_names_.find(base_name) != generator_->interface_names_.end()) {
                // Valid value type interface - keep it as Struct type
                // The C++ codegen will generate the correct class
            }
        }
        // Check if this is an interface name (use value type by default)
        else if (generator_->interface_names_.find(actual_type_name) != generator_->interface_names_.end()) {
            // This is an interface used as a value type
            hir_type.name = actual_type_name + "_Value";
            if (debug_mode) {
                std::cerr << "[HIR_TYPE] Interface " << actual_type_name 
                         << " converted to value type: " << hir_type.name 
                         << std::endl;
            }
        }
        break;
    case TYPE_ENUM:
        hir_type.kind = HIRType::TypeKind::Enum;
        hir_type.name = actual_type_name;
        break;
    case TYPE_INTERFACE:
        hir_type.kind = HIRType::TypeKind::Interface;
        // Only append _Value if it's not a pointer type
        if (!actual_type_name.empty() && actual_type_name.back() == '*') {
            // This is a pointer to interface - keep the name as is
            hir_type.name = actual_type_name;
        } else {
            // This is a value type interface - convert to _Value
            hir_type.name = actual_type_name + "_Value";
            if (debug_mode) {
                std::cerr << "[HIR_TYPE] Interface " << actual_type_name
                         << " converted to value type: " << hir_type.name
                         << std::endl;
            }
        }
        break;
    case TYPE_UNION:
        // Union types are represented as type aliases in C++ (using std::variant)
        hir_type.kind = HIRType::TypeKind::Struct;  // Treat as struct for codegen
        hir_type.name = actual_type_name;
        break;
    case TYPE_POINTER:
        hir_type.kind = HIRType::TypeKind::Pointer;
        hir_type.name = actual_type_name;

        // 型名から内部型を抽出（"Type*" -> "Type"）
        if (!actual_type_name.empty() && actual_type_name.back() == '*') {
            std::string inner_type_name =
                actual_type_name.substr(0, actual_type_name.length() - 1);
            // 末尾の空白を削除
            while (!inner_type_name.empty() && inner_type_name.back() == ' ') {
                inner_type_name.pop_back();
            }

            if (debug_mode) {
                std::cerr << "[HIR_TYPE] Pointer: extracting inner type from '" 
                          << actual_type_name << "' -> '" << inner_type_name << "'" << std::endl;
            }

            // 内部型の TypeInfo を推測
            TypeInfo inner_type_info = TYPE_STRUCT; // デフォルト
            if (inner_type_name == "void") inner_type_info = TYPE_VOID;
            else if (inner_type_name == "int") inner_type_info = TYPE_INT;
            else if (inner_type_name == "long") inner_type_info = TYPE_LONG;
            else if (inner_type_name == "short") inner_type_info = TYPE_SHORT;
            else if (inner_type_name == "tiny") inner_type_info = TYPE_TINY;
            else if (inner_type_name == "char") inner_type_info = TYPE_CHAR;
            else if (inner_type_name == "bool") inner_type_info = TYPE_BOOL;
            else if (inner_type_name == "float") inner_type_info = TYPE_FLOAT;
            else if (inner_type_name == "double") inner_type_info = TYPE_DOUBLE;
            else if (inner_type_name == "string") inner_type_info = TYPE_STRING;
            
            // ポインタのポインタ（int** など）の場合
            if (inner_type_name.back() == '*') {
                inner_type_info = TYPE_POINTER;
            }

            // 再帰的に内部型を変換
            hir_type.inner_type = std::make_unique<HIRType>(
                generator_->convert_type(inner_type_info, inner_type_name)
            );
            
            if (debug_mode) {
                std::cerr << "[HIR_TYPE] Pointer inner type set: kind=" 
                          << static_cast<int>(hir_type.inner_type->kind) << std::endl;
            }
        }
        break;
    case TYPE_NULLPTR:
        hir_type.kind = HIRType::TypeKind::Nullptr;
        break;
    case TYPE_FUNCTION_POINTER:
        // 関数ポインタ型はtypedefで定義されているので、名前をそのまま使用
        hir_type.kind = HIRType::TypeKind::Struct; // typedefとして扱う
        hir_type.name = actual_type_name;
        if (debug_mode) {
            std::cerr << "[HIR_TYPE] Function pointer type converted: " << type_name 
                     << " -> " << actual_type_name << std::endl;
        }
        break;
    case TYPE_GENERIC:
        hir_type.kind = HIRType::TypeKind::Generic;
        hir_type.name = actual_type_name;
        break;
    default:
        if (actual_type_info >= TYPE_ARRAY_BASE) {
            hir_type.kind = HIRType::TypeKind::Array;
            hir_type.name = actual_type_name;

            // v0.14.0: 配列の要素型とサイズの変換
            // type_nameは "int[3]" のような形式
            if (!actual_type_name.empty()) {
                size_t bracket_pos = actual_type_name.find('[');
                if (bracket_pos != std::string::npos) {
                    // 要素型の抽出
                    std::string element_type_name =
                        actual_type_name.substr(0, bracket_pos);

                    // サイズの抽出
                    size_t close_bracket = actual_type_name.find(']', bracket_pos);
                    if (close_bracket != std::string::npos) {
                        std::string size_str = actual_type_name.substr(
                            bracket_pos + 1, close_bracket - bracket_pos - 1);
                        if (!size_str.empty()) {
                            hir_type.array_size = std::stoi(size_str);
                        }
                    }

                    // 要素型をHIRTypeとして設定
                    hir_type.inner_type = std::make_unique<HIRType>();
                    
                    // 要素型がポインタかチェック（例: "int*", "Point*"）
                    if (!element_type_name.empty() && element_type_name.back() == '*') {
                        // ポインタ型の場合
                        std::string base_element_type = element_type_name.substr(0, element_type_name.length() - 1);
                        // 末尾の空白を削除
                        while (!base_element_type.empty() && base_element_type.back() == ' ') {
                            base_element_type.pop_back();
                        }
                        
                        hir_type.inner_type->kind = HIRType::TypeKind::Pointer;
                        hir_type.inner_type->name = element_type_name;
                        
                        // ポインタの指す型を設定
                        hir_type.inner_type->inner_type = std::make_unique<HIRType>();
                        if (base_element_type == "int") {
                            hir_type.inner_type->inner_type->kind = HIRType::TypeKind::Int;
                        } else if (base_element_type == "long") {
                            hir_type.inner_type->inner_type->kind = HIRType::TypeKind::Long;
                        } else if (base_element_type == "short") {
                            hir_type.inner_type->inner_type->kind = HIRType::TypeKind::Short;
                        } else if (base_element_type == "tiny") {
                            hir_type.inner_type->inner_type->kind = HIRType::TypeKind::Tiny;
                        } else if (base_element_type == "char") {
                            hir_type.inner_type->inner_type->kind = HIRType::TypeKind::Char;
                        } else if (base_element_type == "bool") {
                            hir_type.inner_type->inner_type->kind = HIRType::TypeKind::Bool;
                        } else if (base_element_type == "float") {
                            hir_type.inner_type->inner_type->kind = HIRType::TypeKind::Float;
                        } else if (base_element_type == "double") {
                            hir_type.inner_type->inner_type->kind = HIRType::TypeKind::Double;
                        } else if (base_element_type == "string") {
                            hir_type.inner_type->inner_type->kind = HIRType::TypeKind::String;
                        } else if (base_element_type == "void") {
                            hir_type.inner_type->inner_type->kind = HIRType::TypeKind::Void;
                        } else {
                            // 構造体ポインタ
                            hir_type.inner_type->inner_type->kind = HIRType::TypeKind::Struct;
                            hir_type.inner_type->inner_type->name = base_element_type;
                        }
                    } else {
                        // 基本型の判定
                        if (element_type_name == "int") {
                            hir_type.inner_type->kind = HIRType::TypeKind::Int;
                        } else if (element_type_name == "long") {
                            hir_type.inner_type->kind = HIRType::TypeKind::Long;
                        } else if (element_type_name == "short") {
                            hir_type.inner_type->kind = HIRType::TypeKind::Short;
                        } else if (element_type_name == "tiny") {
                            hir_type.inner_type->kind = HIRType::TypeKind::Tiny;
                        } else if (element_type_name == "char") {
                            hir_type.inner_type->kind = HIRType::TypeKind::Char;
                        } else if (element_type_name == "bool") {
                            hir_type.inner_type->kind = HIRType::TypeKind::Bool;
                        } else if (element_type_name == "float") {
                            hir_type.inner_type->kind = HIRType::TypeKind::Float;
                        } else if (element_type_name == "double") {
                            hir_type.inner_type->kind = HIRType::TypeKind::Double;
                        } else if (element_type_name == "string") {
                            hir_type.inner_type->kind = HIRType::TypeKind::String;
                        } else {
                            // 構造体として扱う
                            hir_type.inner_type->kind = HIRType::TypeKind::Struct;
                            hir_type.inner_type->name = element_type_name;
                        }
                    }
                }
            }
        } else {
            // v0.14.0: Check if this is a value type interface (Interface_Value)
            if (actual_type_name.length() > 6 &&
                actual_type_name.substr(actual_type_name.length() - 6) == "_Value") {
                std::string base_name = actual_type_name.substr(0, actual_type_name.length() - 6);
                // If the base name is a known interface, this is valid
                if (generator_->interface_names_.find(base_name) != generator_->interface_names_.end()) {
                    hir_type.kind = HIRType::TypeKind::Struct;
                    hir_type.name = actual_type_name;
                    if (debug_mode) {
                        std::cerr << "[HIR_TYPE] Recognized value type interface: "
                                 << actual_type_name << std::endl;
                    }
                } else {
                    hir_type.kind = HIRType::TypeKind::Unknown;
                }
            } else {
                hir_type.kind = HIRType::TypeKind::Unknown;
            }
        }
        break;
    }

    return hir_type;
}

} // namespace ir
} // namespace cb
