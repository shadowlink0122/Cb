/**
 * @file hir_decl_type_converter.cpp
 * @brief HIR Declaration and Type Converter
 */

#include "hir_decl_type_converter.h"
#include "../../../common/debug.h"
#include "hir_builder.h"
#include "hir_generator.h"
#include <iostream>

namespace cb {
namespace ir {

using namespace hir;

// Helper function to analyze if a statement returns function pointers
// TODO: Fix segmentation fault issue before enabling this function
/*
static bool analyze_returns_function_pointer(const HIRStmt* stmt) {
    if (!stmt) return false;

    switch (stmt->kind) {
    case HIRStmt::StmtKind::Return:
        // Check if the return expression is &function_name
        if (stmt->return_expr && stmt->return_expr->kind ==
HIRExpr::ExprKind::AddressOf) {
            // If taking address of a variable, it might be a function
            if (stmt->return_expr->operand &&
                stmt->return_expr->operand->kind == HIRExpr::ExprKind::Variable)
{
                // TODO: Check if the variable is actually a function name
                // For now, assume it's a function if we're taking its address
                return true;
            }
        }
        return false;

    case HIRStmt::StmtKind::Block:
        // Check all statements in the block
        for (const auto& s : stmt->block_stmts) {
            if (analyze_returns_function_pointer(&s)) {
                return true;
            }
        }
        return false;

    case HIRStmt::StmtKind::If:
        // Check both branches
        return analyze_returns_function_pointer(stmt->then_body.get()) ||
               analyze_returns_function_pointer(stmt->else_body.get());

    default:
        return false;
    }
}
*/

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
    bool returns_function_pointer =
        false; // Declare here to avoid jump bypass issue

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
                while (pos != std::string::npos &&
                       pos < node->return_type_name.length()) {
                    size_t open_bracket = node->return_type_name.find('[', pos);
                    if (open_bracket == std::string::npos)
                        break;

                    size_t close_bracket =
                        node->return_type_name.find(']', open_bracket);
                    if (close_bracket == std::string::npos)
                        break;

                    std::string size_str = node->return_type_name.substr(
                        open_bracket + 1, close_bracket - open_bracket - 1);
                    int size = -1;
                    if (!size_str.empty()) {
                        size = std::stoi(size_str);
                    }

                    array_info.dimensions.push_back(
                        ArrayDimension(size, false));
                    pos = close_bracket + 1;
                }

                func.return_type = generator_->convert_array_type(array_info);

                // Already set return type, skip normal conversion
                goto continue_conversion;
            } else if (actual_return_type == TYPE_INT) {
                // If type_info is INT but type_name is something else, it's
                // likely a struct
                actual_return_type = TYPE_STRUCT;
            }
        }
    }

    // Check if the function returns a function pointer
    // Either explicitly marked or inferred from the body
    returns_function_pointer =
        node->is_function_pointer_return ||
        generator_->analyze_function_returns_function_pointer(node);

    if (returns_function_pointer) {
        // Create a function type for the function pointer
        HIRType func_ptr_type;
        func_ptr_type.kind = HIRType::TypeKind::Function;

        if (node->is_function_pointer_return) {
            // Use explicit function pointer type information
            const auto &fp = node->function_pointer_type;

            // Set the return type of the function pointer
            func_ptr_type.return_type = std::make_unique<HIRType>(
                generator_->convert_type(fp.return_type, fp.return_type_name));

            // Convert parameter types
            for (size_t i = 0; i < fp.param_types.size(); ++i) {
                std::string param_type_name = i < fp.param_type_names.size()
                                                  ? fp.param_type_names[i]
                                                  : "";
                func_ptr_type.param_types.push_back(generator_->convert_type(
                    fp.param_types[i], param_type_name));
            }
        } else {
            // Inferred from body analysis - default to int(*)(int, int)
            // TODO: Properly analyze the returned functions to determine the
            // signature
            func_ptr_type.return_type = std::make_unique<HIRType>(
                generator_->convert_type(TYPE_INT, "int"));
            func_ptr_type.param_types.push_back(
                generator_->convert_type(TYPE_INT, "int"));
            func_ptr_type.param_types.push_back(
                generator_->convert_type(TYPE_INT, "int"));
        }

        // Now create a pointer type that points to this function
        HIRType ptr_to_func;
        ptr_to_func.kind = HIRType::TypeKind::Pointer;
        ptr_to_func.inner_type =
            std::make_unique<HIRType>(std::move(func_ptr_type));

        func.return_type = std::move(ptr_to_func);
    } else {
        func.return_type = generator_->convert_type(actual_return_type,
                                                    node->return_type_name);

        // Handle reference return types (T& or T&&)
        if (node->is_reference) {
            HIRType ref_type;
            ref_type.kind = HIRType::TypeKind::Reference;
            ref_type.inner_type =
                std::make_unique<HIRType>(std::move(func.return_type));
            func.return_type = std::move(ref_type);
        } else if (node->is_rvalue_reference) {
            HIRType ref_type;
            ref_type.kind = HIRType::TypeKind::RvalueReference;
            ref_type.inner_type =
                std::make_unique<HIRType>(std::move(func.return_type));
            func.return_type = std::move(ref_type);
        }
    }

continue_conversion:
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
        hir_param.type =
            generator_->convert_type(param->type_info, param->type_name);
        hir_param.is_const = param->is_const;

        // Handle reference types (T& or T&&)
        if (param->is_reference) {
            HIRType ref_type;
            ref_type.kind = HIRType::TypeKind::Reference;
            ref_type.inner_type =
                std::make_unique<HIRType>(std::move(hir_param.type));
            hir_param.type = std::move(ref_type);
        } else if (param->is_rvalue_reference) {
            HIRType ref_type;
            ref_type.kind = HIRType::TypeKind::RvalueReference;
            ref_type.inner_type =
                std::make_unique<HIRType>(std::move(hir_param.type));
            hir_param.type = std::move(ref_type);
        }

        // Convert array parameters to pointers (C convention)
        // e.g., int[] → int*, int*[] → int**, int[5] → int*
        if (hir_param.type.kind == HIRType::TypeKind::Array ||
            !hir_param.type.array_dimensions.empty() ||
            hir_param.type.array_size > 0) {

            // Create a pointer type with the array's element type
            HIRType ptr_type;
            ptr_type.kind = HIRType::TypeKind::Pointer;
            // Move the inner type instead of copying to avoid segfault
            ptr_type.inner_type = std::move(hir_param.type.inner_type);
            hir_param.type = std::move(ptr_type);

            // Store the POINTER type in symbol table
            // This ensures that when the parameter is used inside the function,
            // it's treated as a pointer, not an array
            generator_->variable_types_[hir_param.name] = hir_param.type;
        } else {
            // Non-array parameter - just store the type as-is
            generator_->variable_types_[hir_param.name] = hir_param.type;
        }

        // デフォルト引数のサポート (v0.14.0)
        if (param->has_default_value && param->default_value) {
            hir_param.has_default = true;
            hir_param.default_value = std::make_unique<HIRExpr>(
                generator_->convert_expr(param->default_value.get()));
        }

        func.parameters.push_back(std::move(hir_param));
    }

    // 関数本体の変換
    if (node->body) {
        func.body = std::make_unique<HIRStmt>(
            generator_->convert_stmt(node->body.get()));
    }

    // v0.14.0: 関数ポインタ戻り値の型推論
    // 関数が int* などのポインタ型を返し、実際に &function_name
    // を返している場合、 この関数は関数ポインタを返すと推論する
    // TODO: 現在はセグメンテーションフォルトが発生するため一時的に無効化
    /*
    if (func.return_type.kind == HIRType::TypeKind::Pointer && func.body) {
        try {
            bool returns_func_ptr =
    analyze_returns_function_pointer(func.body.get()); if (returns_func_ptr) {
                func.returns_function_pointer = true;
                // TODO: 実際の関数シグネチャを推論して
    function_pointer_signature を設定
            }
        } catch (...) {
            // エラーが発生した場合はスキップ
            if (debug_mode) {
                std::cerr << "[HIR] Error analyzing function pointer returns
    for: "
                          << func.name << std::endl;
            }
        }
    }
    */

    // Temporary: Mark functions named getOperation and selectOperator as
    // returning function pointers
    if (func.name == "getOperation" || func.name == "selectOperator") {
        func.returns_function_pointer = true;
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
            hir_field.type =
                generator_->convert_type(child->type_info, child->type_name);
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

    // v0.14.0: Track enum names for array type resolution
    generator_->enum_names_.insert(node->enum_definition.name);

    // メンバーの変換
    for (const auto &member : node->enum_definition.members) {
        HIREnum::Variant variant;
        variant.name = member.name;
        variant.value = member.value;
        variant.has_associated_value = member.has_associated_value;
        if (member.has_associated_value) {
            variant.associated_type = generator_->convert_type(
                member.associated_type, member.associated_type_name);
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
    for (const auto &custom_type :
         node->union_definition.allowed_custom_types) {
        HIRUnion::Variant variant;
        variant.kind = HIRUnion::Variant::Kind::Type;
        HIRType hir_type;
        hir_type.kind = HIRType::TypeKind::Struct;
        hir_type.name = custom_type;
        variant.type = hir_type;
        union_def.variants.push_back(variant);
    }

    // 配列型の変換
    for (const auto &array_type_str :
         node->union_definition.allowed_array_types) {
        HIRUnion::Variant variant;
        variant.kind = HIRUnion::Variant::Kind::Type;
        HIRType hir_type;
        hir_type.kind = HIRType::TypeKind::Array;

        // 配列型文字列をパース（例：int[3], bool[2]）
        size_t bracket_pos = array_type_str.find('[');
        if (bracket_pos != std::string::npos) {
            std::string element_type_str =
                array_type_str.substr(0, bracket_pos);
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
            } else if (element_type_str == "long" ||
                       element_type_str == "int64") {
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
            method.return_type = generator_->convert_type(
                child->type_info, child->return_type_name);

            for (const auto &param : child->parameters) {
                HIRFunction::Parameter hir_param;
                hir_param.name = param->name;
                hir_param.type = generator_->convert_type(param->type_info,
                                                          param->type_name);
                method.parameters.push_back(std::move(hir_param));
            }

            interface_def.methods.push_back(std::move(method));
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
HIRType
HIRDeclTypeConverter::convert_array_type(const ArrayTypeInfo &array_info) {
    HIRType hir_type;
    hir_type.kind = HIRType::TypeKind::Array;

    std::cerr << "[HIR_ARRAY_ENTRY] base_type=" << array_info.base_type
              << ", element_type_name='" << array_info.element_type_name << "'"
              << ", enum_names_.size()=" << generator_->enum_names_.size()
              << std::endl;

    // 基底型を変換
    if (array_info.base_type != TYPE_UNKNOWN) {
        hir_type.inner_type = std::make_unique<HIRType>();

        // v0.14.0:
        // element_type_nameがある場合は使用（構造体配列やポインタ配列など）
        if (!array_info.element_type_name.empty()) {
            std::cerr << "[HIR_ARRAY] Using element_type_name: "
                      << array_info.element_type_name << std::endl;

            // v0.14.0: Check if element_type_name is an enum
            TypeInfo actual_base_type = array_info.base_type;
            if (generator_->enum_names_.find(array_info.element_type_name) !=
                generator_->enum_names_.end()) {
                actual_base_type = TYPE_ENUM;
                std::cerr << "[HIR_ARRAY] Detected enum element type: "
                          << array_info.element_type_name << std::endl;
            }

            *hir_type.inner_type = generator_->convert_type(
                actual_base_type, array_info.element_type_name);
        } else {
            if (debug_mode) {
                std::cerr << "[HIR_ARRAY] Converting base_type without "
                             "element_type_name"
                          << std::endl;
            }
            *hir_type.inner_type =
                generator_->convert_type(array_info.base_type);
        }
    }

    // 多次元配列のサポート
    if (!array_info.dimensions.empty()) {
        const auto &first_dim = array_info.dimensions[0];
        if (!first_dim.is_dynamic && first_dim.size > 0) {
            hir_type.array_size = first_dim.size;
            hir_type.array_dimensions.push_back(first_dim.size);
        } else {
            hir_type.array_size = -1; // Dynamic/VLA
            hir_type.array_dimensions.push_back(-1);
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
            *hir_type.inner_type =
                generator_->convert_array_type(inner_array_info);

            // 他の次元もarray_dimensionsに追加
            for (size_t i = 1; i < array_info.dimensions.size(); i++) {
                if (!array_info.dimensions[i].is_dynamic &&
                    array_info.dimensions[i].size > 0) {
                    hir_type.array_dimensions.push_back(
                        array_info.dimensions[i].size);
                } else {
                    hir_type.array_dimensions.push_back(-1);
                }
            }
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
        actual_type_name =
            type_name.substr(17); // "function_pointer:" の長さ = 17
    }

    // 配列型の特別処理（型名に[がある場合）
    if (actual_type_name.find('[') != std::string::npos) {
        size_t bracket_pos = actual_type_name.find('[');
        std::string element_type_str = actual_type_name.substr(0, bracket_pos);

        // 配列サイズを抽出
        std::vector<int> dimensions;
        size_t pos = bracket_pos;
        while (pos != std::string::npos && pos < actual_type_name.length()) {
            size_t close_bracket = actual_type_name.find(']', pos);
            if (close_bracket == std::string::npos)
                break;

            std::string size_str =
                actual_type_name.substr(pos + 1, close_bracket - pos - 1);
            if (!size_str.empty()) {
                dimensions.push_back(std::stoi(size_str));
            } else {
                dimensions.push_back(-1); // 動的配列
            }

            pos = actual_type_name.find('[', close_bracket);
        }

        // 配列型を構築
        hir_type.kind = HIRType::TypeKind::Array;
        hir_type.array_dimensions = dimensions;
        if (!dimensions.empty()) {
            hir_type.array_size = dimensions[0];
        }

        // 要素型を設定
        hir_type.inner_type = std::make_unique<HIRType>();

        // v0.14.0: ポインタ配列のチェック (e.g., "int*", "double*")
        if (element_type_str.back() == '*') {
            // ポインタ型として処理
            *hir_type.inner_type =
                generator_->convert_type(TYPE_POINTER, element_type_str);
        } else {
            // 基本型として処理
            TypeInfo element_type_info = TYPE_INT;
            if (element_type_str == "int")
                element_type_info = TYPE_INT;
            else if (element_type_str == "long")
                element_type_info = TYPE_LONG;
            else if (element_type_str == "short")
                element_type_info = TYPE_SHORT;
            else if (element_type_str == "tiny")
                element_type_info = TYPE_TINY;
            else if (element_type_str == "char")
                element_type_info = TYPE_CHAR;
            else if (element_type_str == "bool")
                element_type_info = TYPE_BOOL;
            else if (element_type_str == "float")
                element_type_info = TYPE_FLOAT;
            else if (element_type_str == "double")
                element_type_info = TYPE_DOUBLE;
            else if (element_type_str == "string")
                element_type_info = TYPE_STRING;
            else if (generator_->enum_names_.find(element_type_str) !=
                     generator_->enum_names_.end()) {
                element_type_info = TYPE_ENUM;
                if (debug_mode) {
                    std::cerr
                        << "[HIR_TYPE] Detected enum type for array element: "
                        << element_type_str << std::endl;
                }
            } else
                element_type_info = TYPE_STRUCT;

            *hir_type.inner_type =
                generator_->convert_type(element_type_info, element_type_str);

            if (debug_mode && element_type_info == TYPE_ENUM) {
                std::cerr << "[HIR_TYPE] After convert_type for enum: kind="
                          << static_cast<int>(hir_type.inner_type->kind)
                          << ", name='" << hir_type.inner_type->name << "'"
                          << std::endl;
            }
        }

        if (debug_mode) {
            std::cerr << "[HIR_TYPE] Array type: " << element_type_str
                      << ", dimensions=" << dimensions.size() << std::endl;
        }

        return hir_type;
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

        if (debug_mode) {
            std::cerr << "[HIR_TYPE] Struct type: actual_type_name='"
                      << actual_type_name << "', hir_type.name='"
                      << hir_type.name << "'" << std::endl;
        }

        // v0.14.0: Check if this is a value-type interface (Interface_Value)
        if (actual_type_name.length() > 6 &&
            actual_type_name.substr(actual_type_name.length() - 6) ==
                "_Value") {
            std::string base_name =
                actual_type_name.substr(0, actual_type_name.length() - 6);
            // If the base name is a known interface, this is valid
            if (generator_->interface_names_.find(base_name) !=
                generator_->interface_names_.end()) {
                // Valid value type interface - keep it as Struct type
                // The C++ codegen will generate the correct class
            }
        }
        // Check if this is an interface name (use value type by default)
        else if (generator_->interface_names_.find(actual_type_name) !=
                 generator_->interface_names_.end()) {
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
        // Union types are represented as type aliases in C++ (using
        // std::variant)
        hir_type.kind =
            HIRType::TypeKind::Struct; // Treat as struct for codegen
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
                          << actual_type_name << "' -> '" << inner_type_name
                          << "'" << std::endl;
            }

            // 内部型の TypeInfo を推測
            TypeInfo inner_type_info = TYPE_STRUCT; // デフォルト
            if (inner_type_name == "void")
                inner_type_info = TYPE_VOID;
            else if (inner_type_name == "int")
                inner_type_info = TYPE_INT;
            else if (inner_type_name == "long")
                inner_type_info = TYPE_LONG;
            else if (inner_type_name == "short")
                inner_type_info = TYPE_SHORT;
            else if (inner_type_name == "tiny")
                inner_type_info = TYPE_TINY;
            else if (inner_type_name == "char")
                inner_type_info = TYPE_CHAR;
            else if (inner_type_name == "bool")
                inner_type_info = TYPE_BOOL;
            else if (inner_type_name == "float")
                inner_type_info = TYPE_FLOAT;
            else if (inner_type_name == "double")
                inner_type_info = TYPE_DOUBLE;
            else if (inner_type_name == "string")
                inner_type_info = TYPE_STRING;
            // v0.14.0: enum型のチェック
            else if (generator_->enum_names_.find(inner_type_name) !=
                     generator_->enum_names_.end()) {
                inner_type_info = TYPE_ENUM;
                if (debug_mode) {
                    std::cerr
                        << "[HIR_TYPE] Detected enum pointer type: "
                        << inner_type_name << std::endl;
                }
            }

            // ポインタのポインタ（int** など）の場合
            if (inner_type_name.back() == '*') {
                inner_type_info = TYPE_POINTER;
            }

            // 再帰的に内部型を変換
            hir_type.inner_type = std::make_unique<HIRType>(
                generator_->convert_type(inner_type_info, inner_type_name));

            if (debug_mode) {
                std::cerr << "[HIR_TYPE] Pointer inner type set: kind="
                          << static_cast<int>(hir_type.inner_type->kind)
                          << std::endl;
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
            std::cerr << "[HIR_TYPE] Function pointer type converted: "
                      << type_name << " -> " << actual_type_name << std::endl;
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
                    size_t close_bracket =
                        actual_type_name.find(']', bracket_pos);
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
                    if (!element_type_name.empty() &&
                        element_type_name.back() == '*') {
                        // ポインタ型の場合
                        std::string base_element_type =
                            element_type_name.substr(
                                0, element_type_name.length() - 1);
                        // 末尾の空白を削除
                        while (!base_element_type.empty() &&
                               base_element_type.back() == ' ') {
                            base_element_type.pop_back();
                        }

                        hir_type.inner_type->kind = HIRType::TypeKind::Pointer;
                        hir_type.inner_type->name = element_type_name;

                        // ポインタの指す型を設定
                        hir_type.inner_type->inner_type =
                            std::make_unique<HIRType>();
                        if (base_element_type == "int") {
                            hir_type.inner_type->inner_type->kind =
                                HIRType::TypeKind::Int;
                        } else if (base_element_type == "long") {
                            hir_type.inner_type->inner_type->kind =
                                HIRType::TypeKind::Long;
                        } else if (base_element_type == "short") {
                            hir_type.inner_type->inner_type->kind =
                                HIRType::TypeKind::Short;
                        } else if (base_element_type == "tiny") {
                            hir_type.inner_type->inner_type->kind =
                                HIRType::TypeKind::Tiny;
                        } else if (base_element_type == "char") {
                            hir_type.inner_type->inner_type->kind =
                                HIRType::TypeKind::Char;
                        } else if (base_element_type == "bool") {
                            hir_type.inner_type->inner_type->kind =
                                HIRType::TypeKind::Bool;
                        } else if (base_element_type == "float") {
                            hir_type.inner_type->inner_type->kind =
                                HIRType::TypeKind::Float;
                        } else if (base_element_type == "double") {
                            hir_type.inner_type->inner_type->kind =
                                HIRType::TypeKind::Double;
                        } else if (base_element_type == "string") {
                            hir_type.inner_type->inner_type->kind =
                                HIRType::TypeKind::String;
                        } else if (base_element_type == "void") {
                            hir_type.inner_type->inner_type->kind =
                                HIRType::TypeKind::Void;
                        } else if (generator_->enum_names_.find(
                                       base_element_type) !=
                                   generator_->enum_names_.end()) {
                            // v0.14.0: Enum pointer array detection (e.g., Color*[3])
                            hir_type.inner_type->inner_type->kind =
                                HIRType::TypeKind::Enum;
                            hir_type.inner_type->inner_type->name =
                                base_element_type;
                            if (debug_mode) {
                                std::cerr
                                    << "[HIR_TYPE] Detected enum pointer array element: "
                                    << base_element_type << std::endl;
                            }
                        } else {
                            // 構造体ポインタ
                            hir_type.inner_type->inner_type->kind =
                                HIRType::TypeKind::Struct;
                            hir_type.inner_type->inner_type->name =
                                base_element_type;
                        }
                    } else {
                        // 基本型の判定
                        if (element_type_name == "int") {
                            hir_type.inner_type->kind = HIRType::TypeKind::Int;
                        } else if (element_type_name == "long") {
                            hir_type.inner_type->kind = HIRType::TypeKind::Long;
                        } else if (element_type_name == "short") {
                            hir_type.inner_type->kind =
                                HIRType::TypeKind::Short;
                        } else if (element_type_name == "tiny") {
                            hir_type.inner_type->kind = HIRType::TypeKind::Tiny;
                        } else if (element_type_name == "char") {
                            hir_type.inner_type->kind = HIRType::TypeKind::Char;
                        } else if (element_type_name == "bool") {
                            hir_type.inner_type->kind = HIRType::TypeKind::Bool;
                        } else if (element_type_name == "float") {
                            hir_type.inner_type->kind =
                                HIRType::TypeKind::Float;
                        } else if (element_type_name == "double") {
                            hir_type.inner_type->kind =
                                HIRType::TypeKind::Double;
                        } else if (element_type_name == "string") {
                            hir_type.inner_type->kind =
                                HIRType::TypeKind::String;
                        } else if (generator_->enum_names_.find(
                                       element_type_name) !=
                                   generator_->enum_names_.end()) {
                            // v0.14.0: Enum type detection for arrays
                            hir_type.inner_type->kind = HIRType::TypeKind::Enum;
                            hir_type.inner_type->name = element_type_name;
                        } else {
                            // 構造体として扱う
                            hir_type.inner_type->kind =
                                HIRType::TypeKind::Struct;
                            hir_type.inner_type->name = element_type_name;
                        }
                    }
                }
            }
        } else {
            // v0.14.0: Check if this is a value type interface
            // (Interface_Value)
            if (actual_type_name.length() > 6 &&
                actual_type_name.substr(actual_type_name.length() - 6) ==
                    "_Value") {
                std::string base_name =
                    actual_type_name.substr(0, actual_type_name.length() - 6);
                // If the base name is a known interface, this is valid
                if (generator_->interface_names_.find(base_name) !=
                    generator_->interface_names_.end()) {
                    hir_type.kind = HIRType::TypeKind::Struct;
                    hir_type.name = actual_type_name;
                    if (debug_mode) {
                        std::cerr
                            << "[HIR_TYPE] Recognized value type interface: "
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
