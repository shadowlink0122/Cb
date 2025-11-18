#include "hir_generator.h"
#include "../../../common/debug.h"
#include "hir_builder.h"
#include <iostream>

namespace cb {
namespace ir {

using namespace hir;

HIRGenerator::HIRGenerator() {}

HIRGenerator::~HIRGenerator() {}

std::unique_ptr<HIRProgram>
HIRGenerator::generate(const std::vector<std::unique_ptr<ASTNode>> &ast_nodes) {
    DEBUG_PRINT(DebugMsgId::HIR_GENERATION_START);

    auto program = std::make_unique<HIRProgram>();

    for (const auto &node : ast_nodes) {
        if (!node)
            continue;

        switch (node->node_type) {
        case ASTNodeType::AST_FUNC_DECL: {
            if (debug_mode && !node->name.empty()) {
                DEBUG_PRINT(DebugMsgId::HIR_FUNCTION_PROCESSING,
                            node->name.c_str());
            }
            auto func = convert_function(node.get());
            program->functions.push_back(std::move(func));
            break;
        }

        case ASTNodeType::AST_STRUCT_DECL:
        case ASTNodeType::AST_STRUCT_TYPEDEF_DECL: {
            auto struct_def = convert_struct(node.get());
            program->structs.push_back(std::move(struct_def));
            break;
        }

        case ASTNodeType::AST_ENUM_DECL:
        case ASTNodeType::AST_ENUM_TYPEDEF_DECL: {
            auto enum_def = convert_enum(node.get());
            program->enums.push_back(std::move(enum_def));
            break;
        }

        case ASTNodeType::AST_INTERFACE_DECL: {
            auto interface_def = convert_interface(node.get());
            program->interfaces.push_back(std::move(interface_def));
            break;
        }

        case ASTNodeType::AST_IMPL_DECL: {
            auto impl_def = convert_impl(node.get());
            program->impls.push_back(std::move(impl_def));
            break;
        }

        // v0.14.0: FFI support
        case ASTNodeType::AST_FOREIGN_MODULE_DECL: {
            if (node->foreign_module_decl) {
                auto &module = *node->foreign_module_decl;
                for (const auto &ffi_func : module.functions) {
                    HIRForeignFunction hir_ffi;
                    hir_ffi.module_name = module.module_name;
                    hir_ffi.function_name = ffi_func.function_name;
                    hir_ffi.return_type = convert_type(
                        ffi_func.return_type, ffi_func.return_type_name);

                    // パラメータ変換
                    for (const auto &param : ffi_func.parameters) {
                        HIRFunction::Parameter hir_param;
                        hir_param.name = param.name;
                        hir_param.type =
                            convert_type(param.type, param.type_name);
                        hir_ffi.parameters.push_back(hir_param);
                    }

                    hir_ffi.location = convert_location(node->location);
                    program->foreign_functions.push_back(std::move(hir_ffi));
                }
            }
            break;
        }

        // グローバル変数（トップレベルの変数宣言）
        case ASTNodeType::AST_VAR_DECL: {
            // トップレベルかどうかは、関数内にいないかで判定
            // ここではis_staticやis_exportedで判断
            if (node->is_static || node->is_exported) {
                HIRGlobalVar global_var;
                global_var.name = node->name;
                global_var.type =
                    convert_type(node->type_info, node->type_name);
                global_var.is_const = node->is_const;
                global_var.is_exported = node->is_exported;
                if (node->right) {
                    global_var.init_expr = std::make_unique<HIRExpr>(
                        convert_expr(node->right.get()));
                }
                global_var.location = convert_location(node->location);
                program->global_vars.push_back(std::move(global_var));
            }
            break;
        }

        default:
            // その他のトップレベル要素は現在サポートしていない
            break;
        }
    }

    if (debug_mode) {
        DEBUG_PRINT(DebugMsgId::HIR_GENERATION_COMPLETE);
        fprintf(stderr, "HIR generation successful!\n");
        fprintf(stderr, "  Functions: %zu\n", program->functions.size());
        fprintf(stderr, "  Structs: %zu\n", program->structs.size());
        fprintf(stderr, "  Enums: %zu\n", program->enums.size());
        fprintf(stderr, "  Interfaces: %zu\n", program->interfaces.size());
        fprintf(stderr, "  Impls: %zu\n", program->impls.size());
        fprintf(stderr, "  Foreign Functions: %zu\n",
                program->foreign_functions.size());
        fprintf(stderr, "  Global Vars: %zu\n", program->global_vars.size());
    }

    return program;
}

// Generate HIR including imported definitions from parser
std::unique_ptr<HIRProgram> HIRGenerator::generate_with_parser_definitions(
    const std::vector<std::unique_ptr<ASTNode>> &ast_nodes,
    const std::unordered_map<std::string, StructDefinition> &struct_defs,
    const std::unordered_map<std::string, InterfaceDefinition> &interface_defs,
    const std::vector<ImplDefinition> &impl_defs) {

    // First generate HIR from AST nodes
    auto program = generate(ast_nodes);

    if (!program) {
        return nullptr;
    }

    // Now add structs from parser definitions (these include imported modules)
    // Also補完 missing fields for structs from AST
    for (const auto &pair : struct_defs) {
        const StructDefinition &def = pair.second;

        // Skip instantiated generic types (like "Vector<int>", "Map<int, int>")
        // Only include the generic template itself (like "Vector", "Map")
        // Instantiated types have '<' in their name
        if (def.name.find('<') != std::string::npos) {
            continue;
        }

        // Check if this struct is already in the program (from AST)
        bool already_exists = false;
        HIRStruct *existing_struct_ptr = nullptr;
        for (auto &existing_struct : program->structs) {
            if (existing_struct.name == def.name) {
                already_exists = true;
                existing_struct_ptr = &existing_struct;
                break;
            }
        }

        if (already_exists && existing_struct_ptr) {
            // Struct exists in AST but may be missing fields
            // Add fields from parser definition if not present
            if (existing_struct_ptr->fields.empty() && !def.members.empty()) {
                for (const auto &field : def.members) {
                    HIRStruct::Field hir_field;
                    hir_field.name = field.name;

                    // Check if this field type is a generic parameter
                    bool is_generic_param = false;
                    if (!field.type_alias.empty() && def.is_generic) {
                        for (const auto &param : def.type_parameters) {
                            if (field.type_alias == param) {
                                is_generic_param = true;
                                break;
                            }
                        }
                    }

                    if (is_generic_param) {
                        // This is a generic type parameter
                        hir_field.type =
                            convert_type(TYPE_GENERIC, field.type_alias);
                    } else {
                        hir_field.type =
                            convert_type(field.type, field.type_alias);
                    }

                    hir_field.is_private = field.is_private;
                    existing_struct_ptr->fields.push_back(hir_field);
                }
            }
        } else if (!already_exists) {
            // Add new struct from parser definitions (imported module)
            HIRStruct hir_struct;
            hir_struct.name = def.name;

            // Add generic parameters
            if (def.is_generic) {
                hir_struct.generic_params = def.type_parameters;
            }

            // Add fields
            for (const auto &field : def.members) {
                HIRStruct::Field hir_field;
                hir_field.name = field.name;

                // Check if this field type is a generic parameter
                bool is_generic_param = false;
                if (!field.type_alias.empty() && def.is_generic) {
                    for (const auto &param : def.type_parameters) {
                        if (field.type_alias == param) {
                            is_generic_param = true;
                            break;
                        }
                    }
                }

                if (is_generic_param) {
                    // This is a generic type parameter
                    hir_field.type =
                        convert_type(TYPE_GENERIC, field.type_alias);
                } else {
                    hir_field.type = convert_type(field.type, field.type_alias);
                }

                hir_field.is_private = field.is_private;
                hir_struct.fields.push_back(hir_field);
            }

            program->structs.push_back(std::move(hir_struct));
        }
    }

    // Add interfaces from parser definitions
    for (const auto &pair : interface_defs) {
        const InterfaceDefinition &def = pair.second;

        // Check if this interface is already in the program
        bool already_exists = false;
        HIRInterface *existing_interface_ptr = nullptr;
        for (auto &existing_interface : program->interfaces) {
            if (existing_interface.name == def.name) {
                already_exists = true;
                existing_interface_ptr = &existing_interface;
                break;
            }
        }

        if (already_exists && existing_interface_ptr) {
            // Interface exists in AST but may be missing methods
            // Add methods from parser definition if not present
            if (existing_interface_ptr->methods.empty() &&
                !def.methods.empty()) {
                if (def.is_generic) {
                    existing_interface_ptr->generic_params =
                        def.type_parameters;
                }

                for (const auto &method : def.methods) {
                    HIRInterface::MethodSignature hir_method;
                    hir_method.name = method.name;

                    // Check if return type is a generic parameter
                    bool return_is_generic = false;
                    if (!method.return_type_name.empty() && def.is_generic) {
                        for (const auto &param : def.type_parameters) {
                            if (method.return_type_name == param) {
                                return_is_generic = true;
                                break;
                            }
                        }
                    }

                    if (return_is_generic) {
                        hir_method.return_type =
                            convert_type(TYPE_GENERIC, method.return_type_name);
                    } else {
                        hir_method.return_type = convert_type(
                            method.return_type, method.return_type_name);
                    }

                    // Add parameters
                    for (size_t i = 0; i < method.parameters.size(); i++) {
                        const auto &param_pair = method.parameters[i];
                        HIRFunction::Parameter hir_param;
                        hir_param.name = param_pair.first;

                        std::string param_type_name = "";
                        if (i < method.parameter_type_names.size()) {
                            param_type_name = method.parameter_type_names[i];
                        }

                        TypeInfo param_type = param_pair.second;

                        // Fix parameter type based on type_name
                        bool param_is_generic = false;
                        if (!param_type_name.empty() && def.is_generic) {
                            for (const auto &gp : def.type_parameters) {
                                if (param_type_name == gp) {
                                    param_is_generic = true;
                                    break;
                                }
                            }
                        }

                        if (param_is_generic) {
                            hir_param.type =
                                convert_type(TYPE_GENERIC, param_type_name);
                        } else if (!param_type_name.empty() &&
                                   param_type == TYPE_INT) {
                            // Similar logic as return type: check if it's
                            // actually a different type
                            if (param_type_name == "void") {
                                param_type = TYPE_VOID;
                            } else if (param_type_name == "bool") {
                                param_type = TYPE_BOOL;
                            } else if (param_type_name == "int64_t" ||
                                       param_type_name == "long") {
                                param_type = TYPE_LONG;
                            } else if (param_type_name == "int") {
                                param_type = TYPE_INT;
                            } else if (param_type_name == "string") {
                                param_type = TYPE_STRING;
                            } else {
                                // It's likely a struct type or pointer to
                                // struct Check if it contains * (pointer)
                                if (param_type_name.find('*') !=
                                    std::string::npos) {
                                    param_type = TYPE_POINTER;
                                } else {
                                    param_type = TYPE_STRUCT;
                                }
                            }
                            hir_param.type =
                                convert_type(param_type, param_type_name);
                        } else {
                            hir_param.type = convert_type(param_pair.second,
                                                          param_type_name);
                        }

                        hir_param.is_const = false;
                        hir_method.parameters.push_back(hir_param);
                    }

                    existing_interface_ptr->methods.push_back(hir_method);
                }
            }
        } else if (!already_exists) {
            // Add new interface from parser definitions (imported module)
            HIRInterface hir_interface;
            hir_interface.name = def.name;

            // Add generic parameters
            if (def.is_generic) {
                hir_interface.generic_params = def.type_parameters;
            }

            // Add methods
            for (const auto &method : def.methods) {
                HIRInterface::MethodSignature hir_method;
                hir_method.name = method.name;

                // Check if return type is a generic parameter
                bool return_is_generic = false;
                if (!method.return_type_name.empty() && def.is_generic) {
                    for (const auto &param : def.type_parameters) {
                        if (method.return_type_name == param) {
                            return_is_generic = true;
                            break;
                        }
                    }
                }

                if (return_is_generic) {
                    hir_method.return_type =
                        convert_type(TYPE_GENERIC, method.return_type_name);
                } else {
                    hir_method.return_type = convert_type(
                        method.return_type, method.return_type_name);
                }

                // Add parameters
                for (size_t i = 0; i < method.parameters.size(); i++) {
                    const auto &param_pair = method.parameters[i];
                    HIRFunction::Parameter hir_param;
                    hir_param.name = param_pair.first; // name

                    // Get type name from parameter_type_names if available
                    std::string param_type_name = "";
                    if (i < method.parameter_type_names.size()) {
                        param_type_name = method.parameter_type_names[i];
                    }

                    // Check if parameter type is a generic parameter
                    bool param_is_generic = false;
                    if (!param_type_name.empty() && def.is_generic) {
                        for (const auto &gp : def.type_parameters) {
                            if (param_type_name == gp) {
                                param_is_generic = true;
                                break;
                            }
                        }
                    }

                    if (param_is_generic) {
                        hir_param.type =
                            convert_type(TYPE_GENERIC, param_type_name);
                    } else {
                        hir_param.type = convert_type(param_pair.second,
                                                      param_type_name); // type
                    }

                    hir_param.is_const =
                        false; // InterfaceMember doesn't store this
                    hir_method.parameters.push_back(hir_param);
                }

                hir_interface.methods.push_back(hir_method);
            }

            program->interfaces.push_back(std::move(hir_interface));
        }
    }

    // Add impls from parser definitions
    for (const auto &def : impl_defs) {
        // Check if this impl is already in the program
        bool already_exists = false;
        HIRImpl *existing_impl_ptr = nullptr;
        for (auto &existing_impl : program->impls) {
            if (existing_impl.struct_name == def.struct_name &&
                existing_impl.interface_name == def.interface_name) {
                already_exists = true;
                existing_impl_ptr = &existing_impl;
                break;
            }
        }

        if (already_exists && existing_impl_ptr) {
            // Impl exists but may be missing methods
            // Add methods from parser definition if not present
            if (existing_impl_ptr->methods.empty()) {
                // Find the struct to get its generic parameters if not set
                if (existing_impl_ptr->generic_params.empty()) {
                    for (const auto &str : program->structs) {
                        if (str.name == def.struct_name) {
                            existing_impl_ptr->generic_params =
                                str.generic_params;
                            break;
                        }
                    }
                }

                // Convert methods from impl definition
                if (def.impl_node && def.impl_node->body) {
                    if (debug_mode) {
                        fprintf(stderr,
                                "补完ing impl for %s using impl_node (methods: "
                                "%zu)\n",
                                def.struct_name.c_str(),
                                def.impl_node->body->statements.size());
                    }
                    for (const auto &child : def.impl_node->body->statements) {
                        if (child &&
                            child->node_type == ASTNodeType::AST_FUNC_DECL) {
                            auto hir_method = convert_function(child.get());
                            existing_impl_ptr->methods.push_back(
                                std::move(hir_method));
                            if (debug_mode) {
                                fprintf(stderr, "  Converted method: %s\n",
                                        child->name.c_str());
                            }
                        }
                    }
                } else if (!def.methods.empty()) {
                    if (debug_mode) {
                        fprintf(stderr,
                                "补完ing impl for %s using methods vector "
                                "(methods: %zu)\n",
                                def.struct_name.c_str(), def.methods.size());
                    }
                    // Convert methods from the methods vector
                    for (const auto *method_node : def.methods) {
                        if (method_node && method_node->node_type ==
                                               ASTNodeType::AST_FUNC_DECL) {
                            auto hir_method = convert_function(method_node);

                            // Fix parameter types based on struct's generic
                            // parameters
                            std::string base_struct_name = def.struct_name;
                            size_t lt_pos = base_struct_name.find('<');
                            if (lt_pos != std::string::npos) {
                                base_struct_name =
                                    base_struct_name.substr(0, lt_pos);
                            }

                            std::vector<std::string> struct_generic_params;
                            for (const auto &str : program->structs) {
                                if (str.name == base_struct_name) {
                                    struct_generic_params = str.generic_params;
                                    break;
                                }
                            }

                            // Fix each parameter's type if it matches a generic
                            // parameter name
                            for (size_t i = 0;
                                 i < hir_method.parameters.size() &&
                                 i < method_node->parameters.size();
                                 i++) {
                                const auto &param_node =
                                    method_node->parameters[i];
                                auto &hir_param = hir_method.parameters[i];

                                // Check if type_name matches a generic
                                // parameter
                                for (const auto &gp : struct_generic_params) {
                                    if (param_node->type_name == gp) {
                                        hir_param.type =
                                            convert_type(TYPE_GENERIC,
                                                         param_node->type_name);
                                        break;
                                    }
                                }
                            }

                            existing_impl_ptr->methods.push_back(
                                std::move(hir_method));
                            if (debug_mode) {
                                fprintf(stderr, "  Converted method: %s\n",
                                        method_node->name.c_str());
                            }
                        }
                    }
                }
            }
        } else if (!already_exists) {
            // Add new impl from parser definitions (imported module)
            HIRImpl hir_impl;
            hir_impl.struct_name = def.struct_name;
            hir_impl.interface_name = def.interface_name;

            // Find the struct to get its generic parameters
            for (const auto &str : program->structs) {
                if (str.name == def.struct_name) {
                    hir_impl.generic_params = str.generic_params;
                    break;
                }
            }

            // Convert methods from impl definition
            // Use impl_node if available, otherwise use methods vector
            if (def.impl_node && def.impl_node->body) {
                if (debug_mode) {
                    fprintf(stderr,
                            "Converting impl for %s using impl_node (methods: "
                            "%zu)\n",
                            def.struct_name.c_str(),
                            def.impl_node->body->statements.size());
                }
                // Process impl_node's body which contains method declarations
                for (const auto &child : def.impl_node->body->statements) {
                    if (child &&
                        child->node_type == ASTNodeType::AST_FUNC_DECL) {
                        auto hir_method = convert_function(child.get());
                        hir_impl.methods.push_back(std::move(hir_method));
                        if (debug_mode) {
                            fprintf(stderr, "  Converted method: %s\n",
                                    child->name.c_str());
                        }
                    }
                }
            } else if (!def.methods.empty()) {
                if (debug_mode) {
                    fprintf(stderr,
                            "Converting impl for %s using methods vector "
                            "(methods: %zu)\n",
                            def.struct_name.c_str(), def.methods.size());
                }
                // Convert methods from the methods vector (for imported
                // modules)
                for (const auto *method_node : def.methods) {
                    if (method_node &&
                        method_node->node_type == ASTNodeType::AST_FUNC_DECL) {
                        auto hir_method = convert_function(method_node);

                        // Fix parameter types based on struct's generic
                        // parameters This is necessary because convert_function
                        // doesn't have struct context
                        std::string base_struct_name = def.struct_name;
                        size_t lt_pos = base_struct_name.find('<');
                        if (lt_pos != std::string::npos) {
                            base_struct_name =
                                base_struct_name.substr(0, lt_pos);
                        }

                        std::vector<std::string> struct_generic_params;
                        for (const auto &str : program->structs) {
                            if (str.name == base_struct_name) {
                                struct_generic_params = str.generic_params;
                                break;
                            }
                        }

                        // Fix each parameter's type if it matches a generic
                        // parameter name
                        for (size_t i = 0; i < hir_method.parameters.size() &&
                                           i < method_node->parameters.size();
                             i++) {
                            const auto &param_node = method_node->parameters[i];
                            auto &hir_param = hir_method.parameters[i];

                            // Check if type_name matches a generic parameter
                            for (const auto &gp : struct_generic_params) {
                                if (param_node->type_name == gp) {
                                    // It's a generic parameter, make sure HIR
                                    // reflects this
                                    hir_param.type = convert_type(
                                        TYPE_GENERIC, param_node->type_name);
                                    break;
                                }
                            }
                        }

                        hir_impl.methods.push_back(std::move(hir_method));
                        if (debug_mode) {
                            fprintf(stderr, "  Converted method: %s\n",
                                    method_node->name.c_str());
                        }
                    }
                }
            }

            program->impls.push_back(std::move(hir_impl));
        }
    }

    if (debug_mode) {
        fprintf(stderr, "HIR generation with parser definitions complete!\n");
        fprintf(stderr, "  Total Structs: %zu\n", program->structs.size());
        fprintf(stderr, "  Total Interfaces: %zu\n",
                program->interfaces.size());
        fprintf(stderr, "  Total Impls: %zu\n", program->impls.size());
    }

    return program;
}

HIRExpr HIRGenerator::convert_expr(const ASTNode *node) {
    HIRExpr expr;

    if (!node) {
        expr.kind = HIRExpr::ExprKind::Literal;
        return expr;
    }

    expr.location = convert_location(node->location);
    expr.type = convert_type(node->type_info, node->type_name);

    switch (node->node_type) {
    case ASTNodeType::AST_NUMBER: {
        expr.kind = HIRExpr::ExprKind::Literal;
        expr.literal_value = std::to_string(node->int_value);
        expr.literal_type = convert_type(node->type_info);
        break;
    }

    case ASTNodeType::AST_STRING_LITERAL: {
        expr.kind = HIRExpr::ExprKind::Literal;
        expr.literal_value = node->str_value;
        expr.literal_type = convert_type(TYPE_STRING);
        break;
    }

    // v0.14.0: String interpolation support
    case ASTNodeType::AST_INTERPOLATED_STRING: {
        // 補間文字列は複数のセグメントを連結したBinaryOp(+)として変換
        if (node->children.empty()) {
            expr.kind = HIRExpr::ExprKind::Literal;
            expr.literal_value = "";
            expr.literal_type = convert_type(TYPE_STRING);
            break;
        }

        if (node->children.size() == 1) {
            // セグメントが1つだけの場合、そのまま返す
            return convert_expr(node->children[0].get());
        }

        // 複数セグメントの場合、順次連結
        auto result = convert_expr(node->children[0].get());
        for (size_t i = 1; i < node->children.size(); i++) {
            HIRExpr concat;
            concat.kind = HIRExpr::ExprKind::BinaryOp;
            concat.op = "+";
            concat.left = std::make_unique<HIRExpr>(std::move(result));
            concat.right = std::make_unique<HIRExpr>(
                convert_expr(node->children[i].get()));
            result = std::move(concat);
        }
        return result;
    }

    case ASTNodeType::AST_STRING_INTERPOLATION_SEGMENT: {
        // セグメントは文字列リテラルまたは式
        if (node->left) {
            // 式セグメント - std::to_string()でラップ
            expr.kind = HIRExpr::ExprKind::FunctionCall;
            expr.func_name = "std::to_string";
            expr.arguments.push_back(convert_expr(node->left.get()));
        } else {
            // 文字列リテラルセグメント
            expr.kind = HIRExpr::ExprKind::Literal;
            expr.literal_value = node->str_value;
            expr.literal_type = convert_type(TYPE_STRING);
        }
        break;
    }

    case ASTNodeType::AST_VARIABLE:
    case ASTNodeType::AST_IDENTIFIER: {
        expr.kind = HIRExpr::ExprKind::Variable;
        expr.var_name = node->name;
        break;
    }

    case ASTNodeType::AST_BINARY_OP: {
        expr.kind = HIRExpr::ExprKind::BinaryOp;
        expr.op = node->op;
        expr.left = std::make_unique<HIRExpr>(convert_expr(node->left.get()));
        expr.right = std::make_unique<HIRExpr>(convert_expr(node->right.get()));
        break;
    }

    case ASTNodeType::AST_UNARY_OP: {
        // Special case: await is treated as Await expr kind
        if (node->op == "await") {
            expr.kind = HIRExpr::ExprKind::Await;
            expr.operand =
                std::make_unique<HIRExpr>(convert_expr(node->left.get()));
        } else {
            expr.kind = HIRExpr::ExprKind::UnaryOp;
            expr.op = node->op;
            expr.operand =
                std::make_unique<HIRExpr>(convert_expr(node->left.get()));
        }
        break;
    }

    case ASTNodeType::AST_FUNC_CALL: {
        // v0.14.0: メソッド呼び出しのサポート (obj.method(), ptr->method())
        if (node->left) {
            // レシーバーオブジェクトがある場合はメソッド呼び出し
            expr.kind = HIRExpr::ExprKind::MethodCall;
            expr.receiver =
                std::make_unique<HIRExpr>(convert_expr(node->left.get()));
            expr.method_name = node->name;
            expr.is_arrow = node->is_arrow_call;

            for (const auto &arg : node->arguments) {
                expr.arguments.push_back(convert_expr(arg.get()));
            }
        } else {
            // 通常の関数呼び出し
            expr.kind = HIRExpr::ExprKind::FunctionCall;

            // v0.14.0: 修飾名のサポート (m.sqrt, c.abs)
            if (node->is_qualified_call && !node->qualified_name.empty()) {
                expr.func_name = node->qualified_name;
            } else {
                expr.func_name = node->name;
            }

            for (const auto &arg : node->arguments) {
                expr.arguments.push_back(convert_expr(arg.get()));
            }
        }
        break;
    }

    case ASTNodeType::AST_MEMBER_ACCESS: {
        expr.kind = HIRExpr::ExprKind::MemberAccess;
        expr.object = std::make_unique<HIRExpr>(convert_expr(node->left.get()));
        expr.member_name = node->name;
        break;
    }

    case ASTNodeType::AST_ARROW_ACCESS: {
        expr.kind = HIRExpr::ExprKind::MemberAccess;
        expr.object = std::make_unique<HIRExpr>(convert_expr(node->left.get()));
        expr.member_name = node->name;
        expr.is_arrow = true;
        break;
    }

    case ASTNodeType::AST_ARRAY_REF: {
        expr.kind = HIRExpr::ExprKind::ArrayAccess;
        expr.array = std::make_unique<HIRExpr>(convert_expr(node->left.get()));
        expr.index =
            std::make_unique<HIRExpr>(convert_expr(node->array_index.get()));
        break;
    }

    case ASTNodeType::AST_CAST_EXPR: {
        expr.kind = HIRExpr::ExprKind::Cast;
        // Cast target is in cast_expr field, not left
        if (node->cast_expr) {
            expr.cast_expr =
                std::make_unique<HIRExpr>(convert_expr(node->cast_expr.get()));
        } else if (debug_mode) {
            fprintf(stderr, "[HIR_CAST] Warning: Cast expression has no "
                            "cast_expr (target)\n");
        }
        // Use cast_type_info and cast_target_type if available
        if (node->cast_type_info != TYPE_UNKNOWN) {
            expr.cast_type =
                convert_type(node->cast_type_info, node->cast_target_type);
        } else {
            expr.cast_type = convert_type(node->type_info, node->type_name);
        }
        break;
    }

    case ASTNodeType::AST_TERNARY_OP: {
        expr.kind = HIRExpr::ExprKind::Ternary;
        expr.condition =
            std::make_unique<HIRExpr>(convert_expr(node->condition.get()));
        expr.then_expr =
            std::make_unique<HIRExpr>(convert_expr(node->left.get()));
        expr.else_expr =
            std::make_unique<HIRExpr>(convert_expr(node->right.get()));
        break;
    }

    case ASTNodeType::AST_STRUCT_LITERAL: {
        expr.kind = HIRExpr::ExprKind::StructLiteral;
        expr.struct_type_name = node->type_name;
        for (const auto &child : node->children) {
            if (child->node_type == ASTNodeType::AST_ASSIGN) {
                expr.field_names.push_back(child->name);
                expr.field_values.push_back(convert_expr(child->right.get()));
            }
        }
        break;
    }

    case ASTNodeType::AST_ARRAY_LITERAL: {
        expr.kind = HIRExpr::ExprKind::ArrayLiteral;
        for (const auto &element : node->children) {
            expr.array_elements.push_back(convert_expr(element.get()));
        }
        break;
    }

    case ASTNodeType::AST_NULLPTR: {
        expr.kind = HIRExpr::ExprKind::Literal;
        expr.literal_value = "nullptr";
        expr.literal_type = convert_type(TYPE_NULLPTR);
        break;
    }

        // v0.14.0: 追加のHIR式サポート
        // TODO:
        // これらのASTノードタイプは将来実装予定、または既存のAST_UNARY_OPで処理
        // case ASTNodeType::AST_ADDRESS_OF: {
        //     expr.kind = HIRExpr::ExprKind::AddressOf;
        //     expr.operand =
        //     std::make_unique<HIRExpr>(convert_expr(node->left.get())); break;
        // }

        // case ASTNodeType::AST_DEREFERENCE: {
        //     expr.kind = HIRExpr::ExprKind::Dereference;
        //     expr.operand =
        //     std::make_unique<HIRExpr>(convert_expr(node->left.get())); break;
        // }

    case ASTNodeType::AST_SIZEOF_EXPR: {
        expr.kind = HIRExpr::ExprKind::SizeOf;
        if (node->left) {
            expr.sizeof_expr =
                std::make_unique<HIRExpr>(convert_expr(node->left.get()));
        } else {
            expr.sizeof_type = convert_type(node->type_info, node->type_name);
        }
        break;
    }

    case ASTNodeType::AST_PRE_INCDEC: {
        expr.kind = HIRExpr::ExprKind::PreIncDec;
        expr.op = node->op;
        expr.operand =
            std::make_unique<HIRExpr>(convert_expr(node->left.get()));
        break;
    }

    case ASTNodeType::AST_POST_INCDEC: {
        expr.kind = HIRExpr::ExprKind::PostIncDec;
        expr.op = node->op;
        expr.operand =
            std::make_unique<HIRExpr>(convert_expr(node->left.get()));
        break;
    }

    case ASTNodeType::AST_NEW_EXPR: {
        expr.kind = HIRExpr::ExprKind::New;
        expr.new_type = convert_type(node->type_info, node->type_name);
        for (const auto &arg : node->arguments) {
            expr.new_args.push_back(convert_expr(arg.get()));
        }
        break;
    }

    case ASTNodeType::AST_LAMBDA_EXPR: {
        expr.kind = HIRExpr::ExprKind::Lambda;
        // ラムダパラメータの変換
        for (const auto &param : node->parameters) {
            HIRExpr::LambdaParameter hir_param;
            hir_param.name = param->name;
            hir_param.type = convert_type(param->type_info, param->type_name);
            hir_param.is_const = param->is_const;
            expr.lambda_params.push_back(hir_param);
        }
        expr.lambda_return_type =
            convert_type(node->type_info, node->return_type_name);
        if (node->body) {
            expr.lambda_body =
                std::make_unique<HIRStmt>(convert_stmt(node->body.get()));
        }
        break;
    }

        // TODO: メソッド呼び出しは通常の関数呼び出しとして処理されるか、
        // または別のノードタイプで実装される可能性がある
        // case ASTNodeType::AST_METHOD_CALL: {
        //     expr.kind = HIRExpr::ExprKind::MethodCall;
        //     expr.receiver =
        //     std::make_unique<HIRExpr>(convert_expr(node->left.get()));
        //     expr.method_name = node->name;
        //     for (const auto &arg : node->arguments) {
        //         expr.arguments.push_back(convert_expr(arg.get()));
        //     }
        //     break;
        // }

    default: {
        std::string error_msg =
            "Unsupported expression type in HIR generation: AST node type " +
            std::to_string(static_cast<int>(node->node_type));
        report_error(error_msg, node->location);
        expr.kind = HIRExpr::ExprKind::Literal;
        break;
    }
    }

    return expr;
}

HIRStmt HIRGenerator::convert_stmt(const ASTNode *node) {
    HIRStmt stmt;

    if (!node) {
        stmt.kind = HIRStmt::StmtKind::Block;
        return stmt;
    }

    stmt.location = convert_location(node->location);

    if (debug_mode) {
        std::cerr << "[HIR_STMT] Converting statement type: "
                  << static_cast<int>(node->node_type) << std::endl;
    }

    switch (node->node_type) {
    case ASTNodeType::AST_VAR_DECL: {
        stmt.kind = HIRStmt::StmtKind::VarDecl;
        stmt.var_name = node->name;
        stmt.var_type = convert_type(node->type_info, node->type_name);
        stmt.is_const = node->is_const;
        if (debug_mode) {
            DEBUG_PRINT(DebugMsgId::HIR_STMT_VAR_DECL, node->name.c_str());
        }
        // v0.14.0: init_exprを優先的に使用（新しいパーサーフォーマット）
        if (node->init_expr) {
            stmt.init_expr =
                std::make_unique<HIRExpr>(convert_expr(node->init_expr.get()));
            if (debug_mode) {
                std::cerr
                    << "[HIR_STMT]     Has initializer expression (init_expr)"
                    << std::endl;
            }
        } else if (node->right) {
            stmt.init_expr =
                std::make_unique<HIRExpr>(convert_expr(node->right.get()));
            if (debug_mode) {
                std::cerr << "[HIR_STMT]     Has initializer expression (right)"
                          << std::endl;
            }
        } else if (debug_mode) {
            std::cerr << "[HIR_STMT]     No initializer expression"
                      << std::endl;
        }
        break;
    }

    case ASTNodeType::AST_MULTIPLE_VAR_DECL: {
        // 複数変数宣言を個別のVarDeclに展開
        // ブロックではなく、各変数宣言を順番に処理
        stmt.kind = HIRStmt::StmtKind::VarDecl;

        // 最初の変数を現在の文として処理
        if (!node->children.empty()) {
            const auto &first_var = node->children[0];
            stmt.var_name = first_var->name;
            stmt.var_type =
                convert_type(first_var->type_info, first_var->type_name);
            stmt.is_const = first_var->is_const;
            if (first_var->right) {
                stmt.init_expr = std::make_unique<HIRExpr>(
                    convert_expr(first_var->right.get()));
            }

            // 残りの変数は...実際には1つの文に複数の宣言を含めることはできない
            // HIRの設計上、ブロックとして扱う必要がある
            // しかし、スコープ問題を避けるため、親に展開されるべき
            // 一旦、Blockとして扱い、generate側で特別処理する
            if (node->children.size() > 1) {
                stmt.kind = HIRStmt::StmtKind::Block;
                stmt.block_stmts.clear();

                for (const auto &var_node : node->children) {
                    HIRStmt var_stmt;
                    var_stmt.kind = HIRStmt::StmtKind::VarDecl;
                    var_stmt.var_name = var_node->name;
                    var_stmt.var_type =
                        convert_type(var_node->type_info, var_node->type_name);
                    var_stmt.is_const = var_node->is_const;
                    if (var_node->right) {
                        var_stmt.init_expr = std::make_unique<HIRExpr>(
                            convert_expr(var_node->right.get()));
                    }
                    var_stmt.location = convert_location(var_node->location);
                    stmt.block_stmts.push_back(std::move(var_stmt));
                }
            }
        }
        break;
    }

    case ASTNodeType::AST_ASSIGN: {
        stmt.kind = HIRStmt::StmtKind::Assignment;

        // 左辺：node->leftまたはnode->nameから取得
        if (node->left) {
            stmt.lhs =
                std::make_unique<HIRExpr>(convert_expr(node->left.get()));
        } else if (!node->name.empty()) {
            // 変数名が直接node->nameに入っている場合
            HIRExpr var_expr;
            var_expr.kind = HIRExpr::ExprKind::Variable;
            var_expr.var_name = node->name;
            stmt.lhs = std::make_unique<HIRExpr>(std::move(var_expr));
        } else {
            std::string error_msg = "AST_ASSIGN has no left operand or name";
            report_error(error_msg, node->location);
        }

        // 右辺
        if (node->right) {
            stmt.rhs =
                std::make_unique<HIRExpr>(convert_expr(node->right.get()));
        } else {
            std::string error_msg = "AST_ASSIGN has null right operand";
            report_error(error_msg, node->location);
        }
        break;
    }

    case ASTNodeType::AST_IF_STMT: {
        stmt.kind = HIRStmt::StmtKind::If;
        if (debug_mode) {
            DEBUG_PRINT(DebugMsgId::HIR_STMT_IF);
        }
        stmt.condition =
            std::make_unique<HIRExpr>(convert_expr(node->condition.get()));

        // パーサーはif文の本体をleftに格納し、else節をrightに格納する
        // bodyフィールドは使用されていない
        if (node->left) {
            stmt.then_body =
                std::make_unique<HIRStmt>(convert_stmt(node->left.get()));
        } else if (node->body) {
            // 後方互換性のためbodyもチェック
            stmt.then_body =
                std::make_unique<HIRStmt>(convert_stmt(node->body.get()));
        }

        // else節
        if (node->right) {
            if (debug_mode) {
                std::cerr << "[HIR_STMT]     Has else branch" << std::endl;
            }
            stmt.else_body =
                std::make_unique<HIRStmt>(convert_stmt(node->right.get()));
        } else if (node->else_body) {
            // 後方互換性のためelse_bodyもチェック
            stmt.else_body =
                std::make_unique<HIRStmt>(convert_stmt(node->else_body.get()));
        }
        break;
    }

    case ASTNodeType::AST_WHILE_STMT: {
        stmt.kind = HIRStmt::StmtKind::While;
        stmt.condition =
            std::make_unique<HIRExpr>(convert_expr(node->condition.get()));
        stmt.body = std::make_unique<HIRStmt>(convert_stmt(node->body.get()));
        break;
    }

    case ASTNodeType::AST_FOR_STMT: {
        stmt.kind = HIRStmt::StmtKind::For;
        if (node->init_expr) {
            stmt.init =
                std::make_unique<HIRStmt>(convert_stmt(node->init_expr.get()));
        }
        if (node->condition) {
            stmt.condition =
                std::make_unique<HIRExpr>(convert_expr(node->condition.get()));
        }
        if (node->update_expr) {
            stmt.update = std::make_unique<HIRStmt>(
                convert_stmt(node->update_expr.get()));
        }
        stmt.body = std::make_unique<HIRStmt>(convert_stmt(node->body.get()));
        break;
    }

    case ASTNodeType::AST_RETURN_STMT: {
        stmt.kind = HIRStmt::StmtKind::Return;
        if (node->left) {
            stmt.return_expr =
                std::make_unique<HIRExpr>(convert_expr(node->left.get()));
        }
        break;
    }

    case ASTNodeType::AST_BREAK_STMT: {
        stmt.kind = HIRStmt::StmtKind::Break;
        break;
    }

    case ASTNodeType::AST_CONTINUE_STMT: {
        stmt.kind = HIRStmt::StmtKind::Continue;
        break;
    }

    case ASTNodeType::AST_COMPOUND_STMT:
    case ASTNodeType::AST_STMT_LIST: {
        stmt.kind = HIRStmt::StmtKind::Block;
        if (debug_mode) {
            DEBUG_PRINT(DebugMsgId::HIR_STMT_BLOCK,
                        static_cast<int>(node->statements.size()));
        }
        for (const auto &child : node->statements) {
            if (child) {
                stmt.block_stmts.push_back(convert_stmt(child.get()));
            }
        }
        if (debug_mode && stmt.block_stmts.empty()) {
            std::cerr << "Warning: Empty block generated from "
                         "COMPOUND_STMT/STMT_LIST at "
                      << node->location.filename << ":" << node->location.line
                      << std::endl;
        }
        break;
    }

    // v0.14.0: 組み込み関数（println, print等）のサポート
    case ASTNodeType::AST_PRINTLN_STMT:
    case ASTNodeType::AST_PRINT_STMT: {
        stmt.kind = HIRStmt::StmtKind::ExprStmt;

        // println/print呼び出しをHIR式に変換
        HIRExpr call_expr;
        call_expr.kind = HIRExpr::ExprKind::FunctionCall;
        call_expr.func_name = (node->node_type == ASTNodeType::AST_PRINTLN_STMT)
                                  ? "println"
                                  : "print";
        call_expr.type = HIRBuilder::make_basic_type(HIRType::TypeKind::Void);

        // 引数を変換（単一引数の場合はleft、複数引数の場合はarguments）
        if (node->left) {
            call_expr.arguments.push_back(convert_expr(node->left.get()));
        } else if (!node->arguments.empty()) {
            for (const auto &arg : node->arguments) {
                call_expr.arguments.push_back(convert_expr(arg.get()));
            }
        }
        // 引数がない場合は空の引数リストで呼び出し

        stmt.expr = std::make_unique<HIRExpr>(std::move(call_expr));
        break;
    }

    case ASTNodeType::AST_FUNC_CALL: {
        stmt.kind = HIRStmt::StmtKind::ExprStmt;
        stmt.expr = std::make_unique<HIRExpr>(convert_expr(node));
        break;
    }

    case ASTNodeType::AST_PRE_INCDEC:
    case ASTNodeType::AST_POST_INCDEC: {
        // インクリメント/デクリメントを式文として扱う
        stmt.kind = HIRStmt::StmtKind::ExprStmt;
        stmt.expr = std::make_unique<HIRExpr>(convert_expr(node));
        break;
    }

    case ASTNodeType::AST_ASSERT_STMT: {
        // assert文をHIRに変換
        stmt.kind = HIRStmt::StmtKind::Assert;
        if (node->condition) {
            stmt.assert_expr =
                std::make_unique<HIRExpr>(convert_expr(node->condition.get()));
        }
        if (!node->name.empty()) {
            stmt.assert_message = node->name;
        }
        break;
    }

    // v0.14.0: 追加のHIR文サポート
    case ASTNodeType::AST_DEFER_STMT: {
        stmt.kind = HIRStmt::StmtKind::Defer;
        if (node->body) {
            stmt.defer_stmt =
                std::make_unique<HIRStmt>(convert_stmt(node->body.get()));
        }
        break;
    }

    case ASTNodeType::AST_DELETE_EXPR: {
        stmt.kind = HIRStmt::StmtKind::Delete;
        if (node->left) {
            stmt.delete_expr =
                std::make_unique<HIRExpr>(convert_expr(node->left.get()));
        }
        break;
    }

    case ASTNodeType::AST_SWITCH_STMT: {
        stmt.kind = HIRStmt::StmtKind::Switch;
        stmt.switch_expr =
            std::make_unique<HIRExpr>(convert_expr(node->condition.get()));

        // caseの変換 (ASTの構造に合わせて修正)
        for (const auto &case_node : node->cases) {
            HIRStmt::SwitchCase hir_case;
            // case_valuesの最初の値を使用（複数値は将来対応）
            if (!case_node->case_values.empty() && case_node->case_values[0]) {
                hir_case.case_value = std::make_unique<HIRExpr>(
                    convert_expr(case_node->case_values[0].get()));
            }
            for (const auto &case_stmt : case_node->statements) {
                hir_case.case_body.push_back(convert_stmt(case_stmt.get()));
            }
            stmt.switch_cases.push_back(std::move(hir_case));
        }
        break;
    }

    case ASTNodeType::AST_TRY_STMT: {
        stmt.kind = HIRStmt::StmtKind::Try;

        // tryブロック
        if (node->try_body) {
            for (const auto &try_stmt : node->try_body->statements) {
                stmt.try_block.push_back(convert_stmt(try_stmt.get()));
            }
        }

        // catchブロック (AST構造に合わせて単一catch)
        if (node->catch_body) {
            HIRStmt::CatchClause catch_clause;
            catch_clause.exception_var = node->exception_var;
            catch_clause.exception_type =
                convert_type(node->type_info, node->exception_type);
            for (const auto &catch_stmt : node->catch_body->statements) {
                catch_clause.catch_body.push_back(
                    convert_stmt(catch_stmt.get()));
            }
            stmt.catch_clauses.push_back(std::move(catch_clause));
        }

        // finallyブロック
        if (node->finally_body) {
            for (const auto &finally_stmt : node->finally_body->statements) {
                stmt.finally_block.push_back(convert_stmt(finally_stmt.get()));
            }
        }
        break;
    }

    case ASTNodeType::AST_THROW_STMT: {
        stmt.kind = HIRStmt::StmtKind::Throw;
        if (node->left) {
            stmt.throw_expr =
                std::make_unique<HIRExpr>(convert_expr(node->left.get()));
        }
        break;
    }

    case ASTNodeType::AST_MATCH_STMT: {
        stmt.kind = HIRStmt::StmtKind::Match;
        if (node->condition) {
            stmt.match_expr =
                std::make_unique<HIRExpr>(convert_expr(node->condition.get()));
        }
        // TODO: matchアームの変換を実装
        break;
    }

    // v0.14.0: import文のサポート（関数内でのimportを含む）
    case ASTNodeType::AST_IMPORT_STMT: {
        // import文は基本的にコンパイル時に処理されているが、
        // HIRとしては空のブロックとして扱う（C++では不要）
        stmt.kind = HIRStmt::StmtKind::Block;
        // 空のブロック（C++生成時には何も出力されない）
        break;
    }

    default: {
        std::string error_msg =
            "Unsupported statement type in HIR generation: AST node type " +
            std::to_string(static_cast<int>(node->node_type));
        report_error(error_msg, node->location);
        stmt.kind = HIRStmt::StmtKind::Block;
        break;
    }
    }

    return stmt;
}

HIRFunction HIRGenerator::convert_function(const ASTNode *node) {
    HIRFunction func;

    if (!node)
        return func;

    func.name = node->name;
    func.location = convert_location(node->location);

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
        } else if (actual_return_type == TYPE_INT) {
            // If type_info is INT but type_name is something else, it's likely
            // a struct
            actual_return_type = TYPE_STRUCT;
        }
    }

    func.return_type = convert_type(actual_return_type, node->return_type_name);
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
        hir_param.type = convert_type(param->type_info, param->type_name);
        hir_param.is_const = param->is_const;

        // TODO: デフォルト引数は将来実装
        // if (param->default_value) {
        //     hir_param.default_value =
        //     std::make_unique<HIRExpr>(convert_expr(param->default_value.get()));
        // }

        func.parameters.push_back(hir_param);
    }

    // 関数本体の変換
    if (node->body) {
        func.body = std::make_unique<HIRStmt>(convert_stmt(node->body.get()));
    }

    return func;
}

HIRStruct HIRGenerator::convert_struct(const ASTNode *node) {
    HIRStruct struct_def;

    if (!node)
        return struct_def;

    struct_def.name = node->name;
    struct_def.location = convert_location(node->location);

    // v0.14.0: ジェネリックパラメータ
    if (node->is_generic) {
        struct_def.generic_params = node->type_parameters;
    }

    // フィールドの変換 (childrenを使用)
    for (const auto &child : node->children) {
        if (child->node_type == ASTNodeType::AST_VAR_DECL) {
            HIRStruct::Field hir_field;
            hir_field.name = child->name;
            hir_field.type = convert_type(child->type_info, child->type_name);
            hir_field.is_private = child->is_private_member;

            // TODO: デフォルト値は将来実装
            // if (child->right) {
            //     hir_field.default_value =
            //     std::make_unique<HIRExpr>(convert_expr(child->right.get()));
            // }

            struct_def.fields.push_back(hir_field);
        }
    }

    return struct_def;
}

HIREnum HIRGenerator::convert_enum(const ASTNode *node) {
    HIREnum enum_def;

    if (!node)
        return enum_def;

    enum_def.name = node->enum_definition.name;
    enum_def.location = convert_location(node->location);

    // メンバーの変換
    for (const auto &member : node->enum_definition.members) {
        HIREnum::Variant variant;
        variant.name = member.name;
        variant.value = member.value;
        variant.has_associated_value = member.has_associated_value;
        if (member.has_associated_value) {
            variant.associated_type = convert_type(member.associated_type,
                                                   member.associated_type_name);
        }
        enum_def.variants.push_back(variant);
    }

    return enum_def;
}

HIRInterface HIRGenerator::convert_interface(const ASTNode *node) {
    HIRInterface interface_def;

    if (!node)
        return interface_def;

    interface_def.name = node->name;
    interface_def.location = convert_location(node->location);

    // メソッドシグネチャの変換
    for (const auto &child : node->children) {
        if (child->node_type == ASTNodeType::AST_FUNC_DECL) {
            HIRInterface::MethodSignature method;
            method.name = child->name;
            method.return_type =
                convert_type(child->type_info, child->return_type_name);

            for (const auto &param : child->parameters) {
                HIRFunction::Parameter hir_param;
                hir_param.name = param->name;
                hir_param.type =
                    convert_type(param->type_info, param->type_name);
                method.parameters.push_back(hir_param);
            }

            interface_def.methods.push_back(method);
        }
    }

    return interface_def;
}

HIRImpl HIRGenerator::convert_impl(const ASTNode *node) {
    HIRImpl impl_def;

    if (!node)
        return impl_def;

    impl_def.struct_name = node->struct_name;
    impl_def.interface_name = node->interface_name;
    impl_def.location = convert_location(node->location);

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

HIRType HIRGenerator::convert_type(TypeInfo type_info,
                                   const std::string &type_name) {
    HIRType hir_type;

    // 基本型の変換
    switch (type_info) {
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
        hir_type.name = type_name;
        break;
    case TYPE_ENUM:
        hir_type.kind = HIRType::TypeKind::Enum;
        hir_type.name = type_name;
        break;
    case TYPE_INTERFACE:
        hir_type.kind = HIRType::TypeKind::Interface;
        hir_type.name = type_name;
        break;
    case TYPE_POINTER:
        hir_type.kind = HIRType::TypeKind::Pointer;
        hir_type.name = type_name;

        // 型名から内部型を抽出（"Type*" -> "Type"）
        if (!type_name.empty() && type_name.back() == '*') {
            std::string inner_type_name =
                type_name.substr(0, type_name.length() - 1);
            // 末尾の空白を削除
            while (!inner_type_name.empty() && inner_type_name.back() == ' ') {
                inner_type_name.pop_back();
            }

            // 内部型を推測して設定
            hir_type.inner_type = std::make_unique<HIRType>();

            // 基本型か構造体型かを判定
            if (inner_type_name == "void") {
                hir_type.inner_type->kind = HIRType::TypeKind::Void;
            } else if (inner_type_name == "int") {
                hir_type.inner_type->kind = HIRType::TypeKind::Int;
            } else if (inner_type_name == "char") {
                hir_type.inner_type->kind = HIRType::TypeKind::Char;
            } else {
                // それ以外は構造体として扱う
                hir_type.inner_type->kind = HIRType::TypeKind::Struct;
                hir_type.inner_type->name = inner_type_name;
            }
        }
        break;
    case TYPE_NULLPTR:
        hir_type.kind = HIRType::TypeKind::Nullptr;
        break;
    case TYPE_FUNCTION_POINTER:
        hir_type.kind = HIRType::TypeKind::Function;
        hir_type.name = type_name;
        break;
    case TYPE_GENERIC:
        hir_type.kind = HIRType::TypeKind::Generic;
        hir_type.name = type_name;
        break;
    default:
        if (type_info >= TYPE_ARRAY_BASE) {
            hir_type.kind = HIRType::TypeKind::Array;
            hir_type.name = type_name;
            // TODO: 配列の要素型とサイズの変換
        } else {
            hir_type.kind = HIRType::TypeKind::Unknown;
        }
        break;
    }

    return hir_type;
}

SourceLocation HIRGenerator::convert_location(const ::SourceLocation &ast_loc) {
    SourceLocation loc;
    loc.file_path = ast_loc.filename;
    loc.line = ast_loc.line;
    loc.column = ast_loc.column;
    return loc;
}

void HIRGenerator::report_error(const std::string &message,
                                const ::SourceLocation &location) {
    std::cerr << "HIR Generation Error: " << message << " at "
              << location.to_string() << std::endl;
    error_count++;
}

} // namespace ir
} // namespace cb
