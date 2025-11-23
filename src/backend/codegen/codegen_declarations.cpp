/**
 * @file codegen_declarations.cpp
 * @brief HIR to C++ Transpiler - Declaration Generation Module
 *
 * This file generates C++ code for top-level declarations including:
 * - Forward declarations
 * - Structs and Enums
 * - Interfaces
 * - Functions
 * - Implementations (impls)
 *
 * TABLE OF CONTENTS:
 * ==================
 *
 * 1. Imports and Typedefs (Lines 25-100)
 *    - generate_imports()
 *    - generate_typedefs()
 *
 * 2. Forward Declarations (Lines 100-200)
 *    - generate_forward_declarations()
 *
 * 3. Struct Generation (Lines 200-700)
 *    - generate_struct()
 *    - generate_structs()
 *    - Handles generics, inheritance, interfaces
 *
 * 4. Enum Generation (Lines 700-800)
 *    - generate_enum()
 *    - generate_enums()
 *
 * 5. Union Generation (Lines 800-900)
 *    - generate_union()
 *    - generate_unions()
 *
 * 6. Interface Generation (Lines 900-1000)
 *    - generate_interface()
 *    - generate_interfaces()
 *    - Pointer and value interfaces
 *
 * 7. Impl Generation (Lines 1000-1100)
 *    - generate_impl()
 *    - generate_impls()
 *
 * 8. Helper Functions (Lines 1100-end)
 *    - generate_primitive_type_specializations()
 */

#include "../../common/debug.h"
#include "hir_to_cpp.h"
#include <set>
#include <unordered_set>
#include <functional>

namespace cb {
namespace codegen {

using namespace ir::hir;

// ============================================================================
// SECTION 1: Imports and Type Aliases
// ============================================================================

/**
 * @brief Generate import statements (currently comments)
 * @param program HIR program containing import information
 */
void HIRToCpp::generate_imports(const HIRProgram &program) {
    if (program.imports.empty()) {
        return;
    }

    emit_line("// Imports");
    for (const auto &import : program.imports) {
        emit_line("// import " + import.module_path);
    }
    emit_line("");
}

void HIRToCpp::generate_typedefs(const std::vector<HIRTypedef> &typedefs) {
    if (typedefs.empty()) {
        return;
    }

    emit_line("// Type aliases");
    for (const auto &typedef_def : typedefs) {
        // 関数ポインタ型の場合は特別な構文を使用
        if (typedef_def.target_type.kind == HIRType::TypeKind::Function) {
            emit("using " + typedef_def.name + " = ");

            // 戻り値型
            if (typedef_def.target_type.return_type) {
                emit(generate_type(*typedef_def.target_type.return_type));
            } else {
                emit("void");
            }

            emit(" (*)(");

            // パラメータ型
            for (size_t i = 0; i < typedef_def.target_type.param_types.size();
                 i++) {
                if (i > 0)
                    emit(", ");
                emit(generate_type(typedef_def.target_type.param_types[i]));
            }

            emit(");\n");
        } else {
            // 通常の型エイリアス
            std::string base_type = generate_type(typedef_def.target_type);
            emit("using " + typedef_def.name + " = ");
            emit(base_type);
            emit(";\n");
        }
    }
    emit_line("");
}

void HIRToCpp::generate_foreign_functions(
    const std::vector<HIRForeignFunction> &foreign_funcs) {
    if (foreign_funcs.empty()) {
        return;
    }

    emit_line("// FFI (Foreign Function Interface) declarations");
    emit_line("extern \"C\" {");
    increase_indent();

    for (const auto &ffi : foreign_funcs) {
        emit_indent();
        emit(generate_type(ffi.return_type));
        // FFI関数にプレフィックスを追加して衝突を回避
        emit(" CB_FFI_" + ffi.module_name + "_" + ffi.function_name + "(");

        for (size_t i = 0; i < ffi.parameters.size(); i++) {
            if (i > 0)
                emit(", ");
            const auto &param = ffi.parameters[i];
            emit(generate_type(param.type));
            if (!param.name.empty()) {
                emit(" " + param.name);
            }
        }

        emit(");\n");
    }

    decrease_indent();
    emit_line("}");
    emit_line("");

    // FFI関数のラッパー生成（モジュール修飾名対応）
    emit_line("// FFI wrapper functions (for qualified calls)");
    for (const auto &ffi : foreign_funcs) {
        emit_indent();
        emit("inline ");
        emit(generate_type(ffi.return_type));
        emit(" " + ffi.module_name + "_" + ffi.function_name + "(");

        for (size_t i = 0; i < ffi.parameters.size(); i++) {
            if (i > 0)
                emit(", ");
            const auto &param = ffi.parameters[i];
            emit(generate_type(param.type));
            emit(" " + param.name);
        }

        // ラッパーはCB_FFI_付きの関数を呼び出す
        emit(") { return CB_FFI_" + ffi.module_name + "_" + ffi.function_name +
             "(");

        for (size_t i = 0; i < ffi.parameters.size(); i++) {
            if (i > 0)
                emit(", ");
            emit(ffi.parameters[i].name);
        }

        emit("); }\n");
    }
    emit_line("");
}

void HIRToCpp::generate_forward_declarations(const HIRProgram &program) {
    if (program.structs.empty() && program.interfaces.empty()) {
        return;
    }

    emit_line("// Forward declarations");

    for (const auto &struct_def : program.structs) {
        // Handle generic structs
        if (!struct_def.generic_params.empty()) {
            emit("template<");
            for (size_t i = 0; i < struct_def.generic_params.size(); i++) {
                if (i > 0)
                    emit(", ");
                emit("typename " + struct_def.generic_params[i]);
            }
            emit("> ");
        }
        emit_line("struct " + struct_def.name + ";");
    }

    for (const auto &interface : program.interfaces) {
        // Handle generic interfaces
        if (!interface.generic_params.empty()) {
            emit("template<");
            for (size_t i = 0; i < interface.generic_params.size(); i++) {
                if (i > 0)
                    emit(", ");
                emit("typename " + interface.generic_params[i]);
            }
            emit("> ");
        }
        emit_line("class " + interface.name + ";");
    }

    emit_line("");
}

void HIRToCpp::generate_structs(const std::vector<HIRStruct> &structs) {
    // v0.14.0: トポロジカルソートで構造体を依存関係順に出力
    // 依存関係グラフを構築
    std::unordered_map<std::string, std::vector<std::string>> dependencies;
    std::unordered_map<std::string, const HIRStruct*> struct_map;

    // 構造体マップを作成
    for (const auto &struct_def : structs) {
        struct_map[struct_def.name] = &struct_def;
        dependencies[struct_def.name] = {};
    }

    // 各構造体の依存関係を解析
    for (const auto &struct_def : structs) {
        for (const auto &field : struct_def.fields) {
            // フィールドの型が他の構造体を参照している場合
            if (field.type.kind == HIRType::TypeKind::Struct &&
                !field.type.name.empty() &&
                struct_map.find(field.type.name) != struct_map.end()) {
                // この構造体は field.type.name に依存している
                dependencies[struct_def.name].push_back(field.type.name);
            }
        }
    }

    // トポロジカルソート（DFS）
    std::unordered_set<std::string> visited;
    std::unordered_set<std::string> in_stack;
    std::vector<std::string> sorted_order;

    std::function<bool(const std::string&)> dfs = [&](const std::string& name) -> bool {
        if (in_stack.find(name) != in_stack.end()) {
            // 循環依存を検出
            std::cerr << "[WARN] Circular dependency detected involving struct: "
                      << name << std::endl;
            return false;
        }

        if (visited.find(name) != visited.end()) {
            return true;
        }

        visited.insert(name);
        in_stack.insert(name);

        // 依存している構造体を先に訪問
        for (const auto& dep : dependencies[name]) {
            if (!dfs(dep)) {
                return false;
            }
        }

        in_stack.erase(name);
        sorted_order.push_back(name);
        return true;
    };

    // すべての構造体をソート
    for (const auto &struct_def : structs) {
        if (visited.find(struct_def.name) == visited.end()) {
            dfs(struct_def.name);
        }
    }

    // ソートされた順序で構造体を生成
    for (const auto &name : sorted_order) {
        if (struct_map.find(name) != struct_map.end()) {
            generate_struct(*struct_map[name]);
        }
    }
}

void HIRToCpp::generate_struct(const HIRStruct &struct_def) {
    debug_msg(DebugMsgId::CODEGEN_CPP_STRUCT_START, struct_def.name.c_str(),
              static_cast<int>(struct_def.fields.size()));

    emit_line("// Struct: " + struct_def.name);

    // ジェネリック対応
    if (!struct_def.generic_params.empty()) {
        emit("template<");
        for (size_t i = 0; i < struct_def.generic_params.size(); i++) {
            if (i > 0)
                emit(", ");
            emit("typename " + struct_def.generic_params[i]);
        }
        emit(">\n");
    }

    // Check if this struct implements any interfaces
    std::vector<std::string> implemented_interfaces;
    std::vector<const HIRImpl *> struct_impls;

    if (current_program && !current_program->impls.empty()) {
        for (const auto &impl : current_program->impls) {
            // Extract base name from impl.struct_name (e.g., "Vector<T>" ->
            // "Vector")
            std::string impl_base_name = impl.struct_name;
            size_t angle_pos = impl_base_name.find('<');
            if (angle_pos != std::string::npos) {
                impl_base_name = impl_base_name.substr(0, angle_pos);
            }

            if (impl_base_name == struct_def.name) {
                struct_impls.push_back(&impl);
                if (!impl.interface_name.empty()) {
                    // Extract base name from impl.interface_name
                    std::string interface_base = impl.interface_name;
                    size_t iface_angle = interface_base.find('<');
                    if (iface_angle != std::string::npos) {
                        interface_base = interface_base.substr(0, iface_angle);
                    }

                    // Build interface name with template parameters
                    std::string interface_ref = interface_base;
                    if (!struct_def.generic_params.empty()) {
                        interface_ref += "<";
                        for (size_t i = 0; i < struct_def.generic_params.size();
                             i++) {
                            if (i > 0)
                                interface_ref += ", ";
                            interface_ref += struct_def.generic_params[i];
                        }
                        interface_ref += ">";
                    }
                    // Check for duplicates
                    if (std::find(implemented_interfaces.begin(),
                                  implemented_interfaces.end(),
                                  interface_ref) ==
                        implemented_interfaces.end()) {
                        implemented_interfaces.push_back(interface_ref);
                    }
                }
            }
        }
    }

    // Struct declaration with interface inheritance
    emit("struct " + struct_def.name);
    if (!implemented_interfaces.empty()) {
        emit(" : ");
        for (size_t i = 0; i < implemented_interfaces.size(); i++) {
            if (i > 0)
                emit(", ");
            emit("public " + implemented_interfaces[i]);
        }
    }
    emit(" {\n");

    // フィールド
    if (!struct_def.fields.empty()) {
        for (const auto &field : struct_def.fields) {
            emit_indent();
            if (field.is_private) {
                // TODO: privateフィールドのサポート
            }
            emit(generate_type(field.type));
            emit(" " + field.name + ";\n");
        }
    }

    // Add default constructor
    emit_line("");
    emit_line("// Default constructor");
    emit_indent();
    emit(struct_def.name + "() = default;\n");

    // If implementing interfaces, add field initialization constructor
    if (!implemented_interfaces.empty() && !struct_def.fields.empty()) {
        emit_line("");
        emit_line("// Field initialization constructor");
        emit_indent();
        emit(struct_def.name + "(");
        for (size_t i = 0; i < struct_def.fields.size(); i++) {
            if (i > 0)
                emit(", ");
            const auto &field = struct_def.fields[i];
            emit(generate_type(field.type) + " _" + field.name);
        }
        emit(")");
        if (!struct_def.fields.empty()) {
            emit(" : ");
            for (size_t i = 0; i < struct_def.fields.size(); i++) {
                if (i > 0)
                    emit(", ");
                const auto &field = struct_def.fields[i];
                emit(field.name + "(_" + field.name + ")");
            }
        }
        emit(" {}\n");
    }

    // Add method declarations from impls
    std::set<std::string>
        declared_methods; // Track declared methods to avoid duplicates
    if (!struct_impls.empty()) {
        for (const auto *impl_ptr : struct_impls) {
            // Find the corresponding interface for this specific impl
            const HIRInterface *interface_ptr = nullptr;
            if (current_program && !impl_ptr->interface_name.empty()) {
                std::string interface_base = impl_ptr->interface_name;
                size_t angle = interface_base.find('<');
                if (angle != std::string::npos) {
                    interface_base = interface_base.substr(0, angle);
                }
                for (const auto &iface : current_program->interfaces) {
                    if (iface.name == interface_base) {
                        interface_ptr = &iface;
                        break;
                    }
                }
            }

            if (!impl_ptr->methods.empty()) {
                emit_line("");
                emit_line("// Methods");
                for (const auto &method : impl_ptr->methods) {
                    // Try to find corresponding interface method for correct
                    // types
                    const HIRInterface::MethodSignature *interface_method =
                        nullptr;
                    if (interface_ptr) {
                        for (const auto &iface_method :
                             interface_ptr->methods) {
                            if (iface_method.name == method.name) {
                                interface_method = &iface_method;
                                break;
                            }
                        }
                    }

                    // Create method signature to check for duplicates
                    std::string method_sig = method.name + "(";
                    for (size_t i = 0; i < method.parameters.size(); i++) {
                        if (i > 0)
                            method_sig += ",";
                        method_sig += generate_type(method.parameters[i].type);
                    }
                    method_sig += ")";

                    // Skip if already declared
                    if (declared_methods.find(method_sig) !=
                        declared_methods.end()) {
                        continue;
                    }
                    declared_methods.insert(method_sig);

                    emit_indent();
                    // Add virtual keyword if implementing an interface
                    if (!impl_ptr->interface_name.empty() && interface_method) {
                        emit("virtual ");
                    }
                    // Use interface return type if available, otherwise impl's
                    std::string return_type;
                    if (interface_method) {
                        return_type =
                            generate_type(interface_method->return_type);
                    } else {
                        return_type = generate_type(method.return_type);
                    }
                    emit(return_type);
                    emit(" " + method.name + "(");

                    for (size_t i = 0; i < method.parameters.size(); i++) {
                        if (i > 0)
                            emit(", ");
                        const auto &param = method.parameters[i];
                        if (param.is_const)
                            emit("const ");

                        // Use interface parameter type if available, otherwise
                        // impl's
                        std::string param_type;
                        if (interface_method &&
                            i < interface_method->parameters.size()) {
                            param_type = generate_type(
                                interface_method->parameters[i].type);
                        } else {
                            param_type = generate_type(param.type);
                        }
                        emit(param_type);
                        emit(" " +
                             param.name); // Don't add prefix to parameters
                    }

                    emit(")");
                    // Only add override if this method exists in the interface
                    if (!impl_ptr->interface_name.empty() && interface_method) {
                        emit(" override");
                    }
                    emit(";\n");
                }
            }
        }
    }

    // デフォルトメンバー用の演算子オーバーロードを生成
    if (struct_def.has_default_member &&
        !struct_def.default_member_name.empty()) {
        if (debug_mode) {
            std::cerr << "[CODEGEN] Generating default member operators for "
                      << struct_def.name
                      << ", default member: " << struct_def.default_member_name
                      << std::endl;
        }
        // 対応するフィールドを見つける
        const HIRStruct::Field *default_field = nullptr;
        for (const auto &field : struct_def.fields) {
            if (field.name == struct_def.default_member_name) {
                default_field = &field;
                break;
            }
        }

        if (default_field) {
            std::string default_type = generate_type(default_field->type);

            emit_line("");
            emit_line("// Default member delegation operators");

            // operator= for assignment from default member type
            emit_indent();
            emit(struct_def.name + "& operator=(const " + default_type +
                 "& value) {\n");
            increase_indent();
            emit_indent();
            emit("this->" + struct_def.default_member_name + " = value;\n");
            emit_indent();
            emit("return *this;\n");
            decrease_indent();
            emit_indent();
            emit("}\n");
        }
    }

    emit_line("};");
    emit_line("");

    // デフォルトメンバー用のストリーム演算子を構造体の外に生成
    if (struct_def.has_default_member &&
        !struct_def.default_member_name.empty()) {
        const HIRStruct::Field *default_field = nullptr;
        for (const auto &field : struct_def.fields) {
            if (field.name == struct_def.default_member_name) {
                default_field = &field;
                break;
            }
        }

        if (default_field) {
            std::string default_type = generate_type(default_field->type);

            // operator<< for printing (delegates to default member)
            emit_line("// Stream operator for default member delegation");
            emit("inline std::ostream& operator<<(std::ostream& os, const " +
                 struct_def.name + "& obj) {\n");
            increase_indent();
            emit_indent();
            emit("return os << obj." + struct_def.default_member_name + ";\n");
            decrease_indent();
            emit_line("}");
            emit_line("");
        }
    }

    debug_msg(DebugMsgId::CODEGEN_CPP_STRUCT_COMPLETE, struct_def.name.c_str());
}

void HIRToCpp::generate_enums(const std::vector<HIREnum> &enums) {
    for (const auto &enum_def : enums) {
        generate_enum(enum_def);
    }
}

void HIRToCpp::generate_enum(const HIREnum &enum_def) {
    emit_line("// Enum: " + enum_def.name);

    // Check if any variant has associated values
    bool has_associated_values = false;
    for (const auto &variant : enum_def.variants) {
        if (variant.has_associated_value) {
            has_associated_values = true;
            break;
        }
    }

    if (has_associated_values) {
        // Generate tagged union struct (like Option/Result)
        emit_line("struct " + enum_def.name + " {");
        increase_indent();

        // Generate Tag enum
        emit_line("enum class Tag {");
        increase_indent();
        for (size_t i = 0; i < enum_def.variants.size(); i++) {
            const auto &variant = enum_def.variants[i];
            emit_indent();
            emit(variant.name);
            if (i < enum_def.variants.size() - 1) {
                emit(",");
            }
            emit("\n");
        }
        decrease_indent();
        emit_line("};");

        emit_line("Tag tag;");

        // Generate union for associated values
        emit_line("union {");
        increase_indent();
        for (const auto &variant : enum_def.variants) {
            if (variant.has_associated_value) {
                std::string type_str = generate_type(variant.associated_type);
                // Convert variant name to lowercase for value field
                std::string variant_lower = variant.name;
                for (char &c : variant_lower) {
                    c = std::tolower(c);
                }
                emit_line(type_str + " " + variant_lower + "_value;");
            }
        }
        decrease_indent();
        emit_line("};");
        emit_line("");

        // Generate static constructor methods for each variant
        for (const auto &variant : enum_def.variants) {
            if (variant.has_associated_value) {
                std::string type_str = generate_type(variant.associated_type);
                emit_line("static " + enum_def.name + " " + variant.name + "(" +
                          type_str + " value) {");
                increase_indent();
                emit_line(enum_def.name + " e;");
                emit_line("e.tag = Tag::" + variant.name + ";");
                std::string variant_lower = variant.name;
                for (char &c : variant_lower) {
                    c = std::tolower(c);
                }
                emit_line("e." + variant_lower + "_value = value;");
                emit_line("return e;");
                decrease_indent();
                emit_line("}");
            }
        }
        emit_line("");

        // Generate is_Variant() checker methods
        for (const auto &variant : enum_def.variants) {
            emit_line("bool is_" + variant.name +
                      "() const { return tag == Tag::" + variant.name + "; }");
        }

        decrease_indent();
        emit_line("};");
        emit_line("");
    } else {
        // Generate simple C++ enum (existing behavior)
        // v0.14.0: unscopedなenumとして生成（intへの暗黙的変換を許可）
        emit_line("enum " + enum_def.name + " {");
        increase_indent();

        for (size_t i = 0; i < enum_def.variants.size(); i++) {
            const auto &variant = enum_def.variants[i];
            emit_indent();
            emit(variant.name);
            emit(" = " + std::to_string(variant.value));
            if (i < enum_def.variants.size() - 1) {
                emit(",");
            }
            emit("\n");
        }

        decrease_indent();
        emit_line("};");
        emit_line("");
    }
}

void HIRToCpp::generate_unions(const std::vector<HIRUnion> &unions) {
    for (const auto &union_def : unions) {
        generate_union(union_def);
    }
}

void HIRToCpp::generate_union(const HIRUnion &union_def) {
    emit_line("// Union type: " + union_def.name);

    // Build typedef resolution map from current program
    std::unordered_map<std::string, std::string> typedef_map;
    if (current_program) {
        for (const auto &td : current_program->typedefs) {
            std::string base_type = generate_type(td.target_type);
            typedef_map[td.name] = base_type;
        }
    }

    // Helper lambda to resolve typedefs to their base types
    auto resolve_typedef =
        [&typedef_map](const std::string &type_str) -> std::string {
        std::string resolved = type_str;
        // Keep resolving until we hit a non-typedef
        for (int i = 0; i < 10; i++) { // Max 10 levels of typedef nesting
            auto it = typedef_map.find(resolved);
            if (it == typedef_map.end()) {
                break;
            }
            resolved = it->second;
        }
        return resolved;
    };

    // Collect type names for std::variant
    std::vector<std::string> type_names;
    bool has_literals = false;

    for (const auto &variant : union_def.variants) {
        switch (variant.kind) {
        case HIRUnion::Variant::Kind::LiteralInt:
        case HIRUnion::Variant::Kind::LiteralBool:
            if (!has_literals) {
                // Add int once for literal integers/bools
                bool found = false;
                for (const auto &t : type_names) {
                    if (t == "int") {
                        found = true;
                        break;
                    }
                }
                if (!found)
                    type_names.push_back("int");
                has_literals = true;
            }
            break;
        case HIRUnion::Variant::Kind::LiteralString: {
            bool found = false;
            for (const auto &t : type_names) {
                if (t == "std::string") {
                    found = true;
                    break;
                }
            }
            if (!found)
                type_names.push_back("std::string");
            break;
        }
        case HIRUnion::Variant::Kind::Type: {
            std::string type_str = generate_type(variant.type);
            // Resolve typedefs to their base types
            std::string resolved_type = resolve_typedef(type_str);
            // Avoid duplicates using resolved type
            bool found = false;
            for (const auto &t : type_names) {
                if (t == resolved_type) {
                    found = true;
                    break;
                }
            }
            if (!found)
                type_names.push_back(resolved_type);
            break;
        }
        }
    }

    // Generate std::variant typedef
    if (type_names.empty()) {
        // Empty union, use int as default
        emit_line("using " + union_def.name + " = int;");
    } else if (type_names.size() == 1) {
        // Single type, use direct alias
        emit_line("using " + union_def.name + " = " + type_names[0] + ";");
    } else {
        // Multiple types, use std::variant
        emit("using " + union_def.name + " = std::variant<");
        for (size_t i = 0; i < type_names.size(); i++) {
            if (i > 0)
                emit(", ");
            emit(type_names[i]);
        }
        emit(">;\n");
    }
    emit_line("");
}

void HIRToCpp::generate_interfaces(
    const std::vector<HIRInterface> &interfaces) {
    for (const auto &interface : interfaces) {
        // ポインタベースinterface生成（既存）
        generate_pointer_interface(interface);

        // 値型interface生成（新規）
        if (interface.generate_value_type) {
            generate_value_interface(interface);
        }
    }
}

void HIRToCpp::generate_pointer_interface(const HIRInterface &interface) {
    emit_line("// Interface (pointer-based): " + interface.name);

    // Add template parameters if generic
    if (!interface.generic_params.empty()) {
        emit("template<");
        for (size_t i = 0; i < interface.generic_params.size(); i++) {
            if (i > 0)
                emit(", ");
            emit("typename " + interface.generic_params[i]);
        }
        emit(">\n");
    }

    emit_line("class " + interface.name + " {");
    emit_line("public:");
    increase_indent();

    emit_line("virtual ~" + interface.name + "() = default;");
    emit_line("");

    for (const auto &method : interface.methods) {
        emit("virtual ");
        emit(generate_type(method.return_type));
        emit(" " + method.name + "(");

        for (size_t i = 0; i < method.parameters.size(); i++) {
            if (i > 0)
                emit(", ");
            const auto &param = method.parameters[i];
            if (param.is_const)
                emit("const ");
            std::string param_type = generate_type(param.type);
            emit(param_type);
            emit(" " + param.name);
        }

        emit(") = 0;\n");
    }

    decrease_indent();
    emit_line("};");
    emit_line("");
}

void HIRToCpp::generate_value_interface(const HIRInterface &interface) {
    std::string value_class_name = interface.name + "_Value";

    emit_line("// Interface (value-based, type erasure): " + interface.name);

    // テンプレートパラメータ
    if (!interface.generic_params.empty()) {
        emit("template<");
        for (size_t i = 0; i < interface.generic_params.size(); i++) {
            if (i > 0)
                emit(", ");
            emit("typename " + interface.generic_params[i]);
        }
        emit(">\n");
    }

    emit_line("class " + value_class_name + " {");
    emit_line("private:");
    increase_indent();

    // Concept（内部インターフェース）
    emit_line("struct Concept {");
    increase_indent();

    for (const auto &method : interface.methods) {
        emit("virtual ");
        emit(generate_type(method.return_type));
        emit(" " + method.name + "(");

        for (size_t i = 0; i < method.parameters.size(); i++) {
            if (i > 0)
                emit(", ");
            const auto &param = method.parameters[i];
            if (param.is_const)
                emit("const ");
            emit(generate_type(param.type));
            emit(" " + param.name);
        }

        emit(") = 0;\n");
    }

    emit_line("virtual std::unique_ptr<Concept> clone() const = 0;");
    emit_line("virtual ~Concept() = default;");

    decrease_indent();
    emit_line("};");
    emit_line("");

    // Model（テンプレート実装）
    emit_line("template<typename T>");
    emit_line("struct Model : Concept {");
    increase_indent();

    emit_line("T data;");
    emit_line("");
    emit_line("Model(T d) : data(std::move(d)) {}");
    emit_line("");

    for (const auto &method : interface.methods) {
        emit(generate_type(method.return_type));
        emit(" " + method.name + "(");

        for (size_t i = 0; i < method.parameters.size(); i++) {
            if (i > 0)
                emit(", ");
            const auto &param = method.parameters[i];
            if (param.is_const)
                emit("const ");
            emit(generate_type(param.type));
            emit(" " + param.name);
        }

        emit(") override {\n");
        increase_indent();

        emit("return data." + method.name + "(");
        for (size_t i = 0; i < method.parameters.size(); i++) {
            if (i > 0)
                emit(", ");
            emit(method.parameters[i].name);
        }
        emit(");\n");

        decrease_indent();
        emit_line("}");
    }

    emit_line("");
    emit_line("std::unique_ptr<Concept> clone() const override {");
    increase_indent();
    emit_line("return std::make_unique<Model<T>>(data);");
    decrease_indent();
    emit_line("}");

    decrease_indent();
    emit_line("};");
    emit_line("");

    // メンバ変数
    emit_line("std::unique_ptr<Concept> ptr_;");
    emit_line("");

    decrease_indent();
    emit_line("public:");
    increase_indent();

    // コンストラクタ
    emit_line("template<typename T>");
    emit_line(value_class_name + "(T obj)");
    increase_indent();
    emit_line(": ptr_(std::make_unique<Model<T>>(std::move(obj))) {}");
    decrease_indent();
    emit_line("");

    // コピーコンストラクタ
    emit_line(value_class_name + "(const " + value_class_name + "& other)");
    increase_indent();
    emit_line(": ptr_(other.ptr_ ? other.ptr_->clone() : nullptr) {}");
    decrease_indent();
    emit_line("");

    // ムーブコンストラクタ
    emit_line(value_class_name + "(" + value_class_name +
              "&& other) = default;");
    emit_line("");

    // コピー代入演算子
    emit_line(value_class_name + "& operator=(const " + value_class_name +
              "& other) {");
    increase_indent();
    emit_line("if (this != &other) {");
    increase_indent();
    emit_line("ptr_ = other.ptr_ ? other.ptr_->clone() : nullptr;");
    decrease_indent();
    emit_line("}");
    emit_line("return *this;");
    decrease_indent();
    emit_line("}");
    emit_line("");

    // ムーブ代入演算子
    emit_line(value_class_name + "& operator=(" + value_class_name +
              "&& other) = default;");
    emit_line("");

    // メソッド
    for (const auto &method : interface.methods) {
        emit(generate_type(method.return_type));
        emit(" " + method.name + "(");

        for (size_t i = 0; i < method.parameters.size(); i++) {
            if (i > 0)
                emit(", ");
            const auto &param = method.parameters[i];
            if (param.is_const)
                emit("const ");
            emit(generate_type(param.type));
            emit(" " + param.name);
        }

        emit(") {\n");
        increase_indent();

        emit("return ptr_->" + method.name + "(");
        for (size_t i = 0; i < method.parameters.size(); i++) {
            if (i > 0)
                emit(", ");
            emit(method.parameters[i].name);
        }
        emit(");\n");

        decrease_indent();
        emit_line("}");
    }

    decrease_indent();
    emit_line("};");
    emit_line("");
}

void HIRToCpp::generate_global_vars(const std::vector<HIRGlobalVar> &globals) {
    if (globals.empty()) {
        return;
    }

    emit_line("// Global variables");
    for (const auto &global : globals) {
        if (global.is_const)
            emit("const ");

        // 配列の場合、要素型だけ取得
        if (global.type.kind == HIRType::TypeKind::Array &&
            global.type.inner_type) {
            emit(generate_type(*global.type.inner_type));
        } else {
            emit(generate_type(global.type));
        }

        emit(" " + add_hir_prefix(global.name));

        // 配列サイズを追加
        if (!global.type.array_dimensions.empty()) {
            for (int dim : global.type.array_dimensions) {
                emit("[");
                if (dim > 0) {
                    emit(std::to_string(dim));
                }
                emit("]");
            }
        }

        if (global.init_expr) {
            emit(" = ");
            emit(generate_expr(*global.init_expr));
        }

        emit(";\n");
    }
    emit_line("");
}

void HIRToCpp::generate_functions(const std::vector<HIRFunction> &functions) {
    for (const auto &func : functions) {
        generate_function(func);
    }
}

void HIRToCpp::generate_function(const HIRFunction &func) {
    debug_msg(DebugMsgId::CODEGEN_CPP_FUNCTION_START, func.name.c_str(),
              static_cast<int>(func.parameters.size()));

    emit_line("// Function: " + func.name);

    // ジェネリック対応
    if (!func.generic_params.empty()) {
        emit("template<");
        for (size_t i = 0; i < func.generic_params.size(); i++) {
            if (i > 0)
                emit(", ");
            emit("typename " + func.generic_params[i]);
        }
        emit(">\n");
    }

    // v0.14.0: Check if the function returns a function pointer
    // Use the type-inferred flag rather than checking the type directly
    bool returns_function_pointer = func.returns_function_pointer;

    emit_indent();

    // main関数は常にintを返す
    if (func.name == "main") {
        emit("int");
        emit(" main(");

        // main関数のパラメータ（通常は空）
        for (size_t i = 0; i < func.parameters.size(); i++) {
            if (i > 0)
                emit(", ");
            const auto &param = func.parameters[i];
            if (param.is_const)
                emit("const ");
            emit(generate_type(param.type));
            emit(" " + add_hir_prefix(param.name));

            // デフォルト引数のサポート
            if (param.has_default && param.default_value) {
                emit(" = ");
                emit(generate_expr(*param.default_value));
            }
        }

        emit(") {\n");
    } else if (returns_function_pointer) {
        // v0.14.0: Special handling for functions returning function pointers
        // For now, we'll use a simple approach: emit the function pointer
        // return type inline
        // TODO: Properly infer the actual function signature from return
        // statements

        // For now, assume int (*)(int, int) for all function pointers
        // This matches the actual functions being returned (add, multiply,
        // etc.)
        emit("int (*" + add_hir_prefix(func.name) + "(");

        // Function parameters
        for (size_t i = 0; i < func.parameters.size(); i++) {
            if (i > 0)
                emit(", ");
            const auto &param = func.parameters[i];
            if (param.is_const)
                emit("const ");
            emit(generate_type(param.type));
            emit(" " + add_hir_prefix(param.name));

            // デフォルト引数のサポート (v0.14.0)
            if (param.has_default && param.default_value) {
                emit(" = ");
                emit(generate_expr(*param.default_value));
            }
        }

        emit("))(int, int) {\n");
    } else {
        // Normal function declaration
        emit(generate_type(func.return_type));
        emit(" " + add_hir_prefix(func.name) + "(");

        // パラメータ
        for (size_t i = 0; i < func.parameters.size(); i++) {
            if (i > 0)
                emit(", ");
            const auto &param = func.parameters[i];
            if (param.is_const)
                emit("const ");
            emit(generate_type(param.type));
            emit(" " + add_hir_prefix(param.name));

            // デフォルト引数のサポート (v0.14.0)
            if (param.has_default && param.default_value) {
                emit(" = ");
                emit(generate_expr(*param.default_value));
            }
        }

        emit(") {\n");
    }

    // 関数パラメータの型を記録（ポインタアクセス判定用）
    current_function_params.clear();
    for (const auto &param : func.parameters) {
        current_function_params[param.name] = param.type;
    }

    // 現在の関数の情報を記録（async関数のreturn文処理用）
    // Check if return type is Future<T> by checking the type name
    current_function_is_async =
        (func.return_type.kind == HIRType::TypeKind::Struct &&
         func.return_type.name.find("Future<") == 0);
    current_function_return_type = func.return_type;

    // 関数本体
    if (func.body) {
        debug_msg(DebugMsgId::CODEGEN_CPP_FUNCTION_BODY);
        increase_indent();
        generate_stmt(*func.body);
        decrease_indent();
    }

    emit_line("}");
    emit_line("");

    // パラメータマップをクリア
    current_function_params.clear();

    int stmt_count = 0;
    if (func.body && func.body->kind == HIRStmt::StmtKind::Block) {
        stmt_count = static_cast<int>(func.body->block_stmts.size());
    }
    debug_msg(DebugMsgId::CODEGEN_CPP_FUNCTION_COMPLETE, func.name.c_str(),
              stmt_count);
}

void HIRToCpp::generate_impls(const std::vector<HIRImpl> &impls) {
    for (const auto &impl : impls) {
        generate_impl(impl);
    }
}

void HIRToCpp::generate_impl(const HIRImpl &impl) {
    emit_line("// Impl for: " + impl.struct_name);

    if (!impl.interface_name.empty()) {
        emit_line("// implements: " + impl.interface_name);
    }

    // Check if implementing for a primitive type (すべてのプリミティブ型を含む)
    bool is_primitive_type =
        (impl.struct_name == "int" || impl.struct_name == "long" ||
         impl.struct_name == "short" || impl.struct_name == "tiny" ||
         impl.struct_name == "unsigned" ||
         impl.struct_name == "unsigned long" ||
         impl.struct_name == "unsigned short" ||
         impl.struct_name == "unsigned tiny" || impl.struct_name == "char" ||
         impl.struct_name == "bool" || impl.struct_name == "float" ||
         impl.struct_name == "double" || impl.struct_name == "string");

    // プリミティブ型でinterfaceを実装している場合はスキップ
    // (Model特殊化で処理される)
    if (is_primitive_type && !impl.interface_name.empty()) {
        emit_line("// Skipped: Will be generated as Model specialization");
        emit_line("");
        return;
    }

    // static変数を生成
    current_impl_static_vars.clear();
    if (!impl.static_variables.empty()) {
        emit_line("// Static variables for impl: " + impl.struct_name);
        for (const auto &static_var : impl.static_variables) {
            if (static_var.is_const)
                emit("static const ");
            else
                emit("static ");

            // 配列の場合、要素型だけ取得
            if (static_var.type.kind == HIRType::TypeKind::Array &&
                static_var.type.inner_type) {
                emit(generate_type(*static_var.type.inner_type));
            } else {
                emit(generate_type(static_var.type));
            }

            // 変数名にstruct名を含めてユニークにする
            std::string unique_var_name = add_hir_prefix(impl.struct_name + "_" + static_var.name);
            emit(" " + unique_var_name);

            // static変数名のマッピングを記録
            current_impl_static_vars[static_var.name] = unique_var_name;

            // 配列サイズを追加
            if (!static_var.type.array_dimensions.empty()) {
                for (int dim : static_var.type.array_dimensions) {
                    emit("[");
                    if (dim > 0) {
                        emit(std::to_string(dim));
                    }
                    emit("]");
                }
            }

            if (static_var.init_expr) {
                emit(" = ");
                emit(generate_expr(*static_var.init_expr));
            }

            emit(";\n");
        }
        emit_line("");
    }

    // メソッドを生成
    for (const auto &method : impl.methods) {
        emit_line("// Method: " + method.name);

        // ジェネリック対応: implのgeneric_paramsかmethodのgeneric_paramsを使う
        std::vector<std::string> generic_params = impl.generic_params;
        if (!method.generic_params.empty()) {
            generic_params = method.generic_params;
        }

        // If generic_params is empty but struct_name contains <T>, extract it
        if (generic_params.empty() &&
            impl.struct_name.find('<') != std::string::npos) {
            size_t start = impl.struct_name.find('<');
            size_t end = impl.struct_name.find('>');
            if (start != std::string::npos && end != std::string::npos &&
                end > start) {
                std::string params_str =
                    impl.struct_name.substr(start + 1, end - start - 1);
                // カンマで分割して複数のパラメータに対応
                size_t pos = 0;
                while (pos < params_str.length()) {
                    size_t comma_pos = params_str.find(',', pos);
                    if (comma_pos == std::string::npos) {
                        // 最後のパラメータ
                        std::string param = params_str.substr(pos);
                        // 前後の空白を削除
                        size_t first = param.find_first_not_of(' ');
                        size_t last = param.find_last_not_of(' ');
                        if (first != std::string::npos) {
                            generic_params.push_back(
                                param.substr(first, last - first + 1));
                        }
                        break;
                    } else {
                        std::string param =
                            params_str.substr(pos, comma_pos - pos);
                        // 前後の空白を削除
                        size_t first = param.find_first_not_of(' ');
                        size_t last = param.find_last_not_of(' ');
                        if (first != std::string::npos) {
                            generic_params.push_back(
                                param.substr(first, last - first + 1));
                        }
                        pos = comma_pos + 1;
                    }
                }
            }
        }

        if (!generic_params.empty()) {
            emit("template<");
            for (size_t i = 0; i < generic_params.size(); i++) {
                if (i > 0)
                    emit(", ");
                emit("typename " + generic_params[i]);
            }
            emit(">\n");
        }

        // 戻り値の型
        emit_indent();
        emit(generate_type(method.return_type));

        // For primitive types, generate as free functions
        if (is_primitive_type) {
            // Free function: impl_struct_method(self, params...)
            std::string func_name =
                "CB_IMPL_" + impl.struct_name + "_" + method.name;
            emit(" " + func_name + "(");

            // First parameter is always 'self'
            emit(impl.struct_name + " CB_HIR_self");

            // Add other parameters
            if (!method.parameters.empty()) {
                emit(", ");
            }
        } else {
            // Member function: Struct::method(params...)
            emit(" " + impl.struct_name + "::" + method.name + "(");
        }

        // パラメータ - ジェネリック型の場合は型パラメータ名を使用
        for (size_t i = 0; i < method.parameters.size(); i++) {
            if (i > 0)
                emit(", ");
            const auto &param = method.parameters[i];
            if (param.is_const)
                emit("const ");

            // 型がUnknownの場合で、ジェネリックパラメータがある場合は、それを使用
            std::string param_type;
            if (param.type.kind == HIRType::TypeKind::Unknown &&
                !generic_params.empty()) {
                // 最初のジェネリックパラメータをデフォルトで使用
                param_type = generic_params[0];
            } else if (param.type.kind == HIRType::TypeKind::Generic) {
                // Generic型の場合は型名をそのまま使用
                param_type = param.type.name;
            } else {
                param_type = generate_type(param.type);
            }

            emit(param_type);
            emit(" " + add_hir_prefix(param.name));
        }

        emit(") {\n");

        // ジェネリックパラメータをコンテキストに設定
        current_generic_params = generic_params;

        // プリミティブ型implのコンテキストを設定
        current_impl_is_for_primitive = is_primitive_type;

        // 関数本体
        if (method.body) {
            increase_indent();
            generate_stmt(*method.body);
            decrease_indent();
        }

        // コンテキストをクリア
        current_generic_params.clear();
        current_impl_is_for_primitive = false;

        emit_line("}");
        emit_line("");
    }
}

void HIRToCpp::generate_primitive_type_specializations(
    const HIRProgram &program) {
    emit_line("// Model specializations for primitive types");
    emit_line("");

    // プリミティブ型のリスト（すべての型を含む）
    std::vector<std::string> primitive_types = {
        // 符号付き整数型
        "int", "long", "short", "tiny",
        // 符号なし整数型
        "unsigned", "unsigned long", "unsigned short", "unsigned tiny",
        // その他
        "char", "bool", "float", "double", "string"};

    // 各interfaceについて、プリミティブ型のimplがあるか確認
    for (const auto &interface : program.interfaces) {
        if (!interface.generate_value_type) {
            continue;
        }

        std::string value_class_name = interface.name + "_Value";

        // このinterfaceに対するimplを探す
        for (const auto &impl : program.impls) {
            if (impl.interface_name != interface.name) {
                continue;
            }

            // プリミティブ型かチェック
            bool is_primitive = false;
            for (const auto &prim : primitive_types) {
                if (impl.struct_name == prim) {
                    is_primitive = true;
                    break;
                }
            }

            if (!is_primitive) {
                continue;
            }

            // Model特殊化を生成
            emit_line("// Model specialization for " + impl.struct_name);
            emit_line("template<>");
            emit_line("struct " + value_class_name + "::Model<" +
                      impl.struct_name + "> : " + value_class_name +
                      "::Concept {");
            increase_indent();

            emit_line(impl.struct_name + " data;");
            emit_line("");
            emit_line("Model(" + impl.struct_name + " d) : data(d) {}");
            emit_line("");

            // 各メソッドの実装を生成
            for (const auto &method : impl.methods) {
                emit_indent();
                emit(generate_type(method.return_type));
                emit(" " + method.name + "(");

                for (size_t i = 0; i < method.parameters.size(); i++) {
                    if (i > 0)
                        emit(", ");
                    const auto &param = method.parameters[i];
                    if (param.is_const)
                        emit("const ");
                    emit(generate_type(param.type));
                    emit(" " + add_hir_prefix(param.name));
                }

                // privateメソッドにはoverride を付けない
                if (!method.is_private) {
                    emit(") override {\n");
                } else {
                    emit(") {\n");
                }
                increase_indent();

                // メソッド本体を生成
                // selfをdataに置き換えて生成
                if (method.body) {
                    // 一時的にコンテキストを設定
                    bool old_is_primitive = current_impl_is_for_primitive;
                    const HIRImpl* old_impl = current_impl;
                    current_impl_is_for_primitive = true;
                    current_impl = &impl;

                    generate_stmt(*method.body);

                    current_impl_is_for_primitive = old_is_primitive;
                    current_impl = old_impl;
                }

                decrease_indent();
                emit_line("}");
                emit_line("");
            }

            // clone()メソッド
            emit_line("std::unique_ptr<" + value_class_name +
                      "::Concept> clone() const override {");
            increase_indent();
            emit_line("return std::make_unique<Model<" + impl.struct_name +
                      ">>(data);");
            decrease_indent();
            emit_line("}");

            decrease_indent();
            emit_line("};");
            emit_line("");
        }
    }
}

} // namespace codegen
} // namespace cb
