/**
 * @file hir_generator.cpp
 * @brief HIR Generator - Main coordinator for AST to HIR conversion
 * 
 * This file coordinates AST to HIR conversion by delegating to specialized converters.
 */

#include "hir_generator.h"
#include "hir_expr_converter.h"
#include "hir_stmt_converter.h"
#include "hir_decl_type_converter.h"
#include "../../../common/debug.h"
#include "hir_builder.h"
#include <iostream>

namespace cb {
namespace ir {

using namespace hir;

// ============================================================================
// SECTION 1: Constructor/Destructor
// ============================================================================

HIRGenerator::HIRGenerator() {
    // Create specialized converters
    expr_converter_ = std::make_unique<HIRExprConverter>(this);
    stmt_converter_ = std::make_unique<HIRStmtConverter>(this);
    decl_type_converter_ = std::make_unique<HIRDeclTypeConverter>(this);
}

HIRGenerator::~HIRGenerator() {}

// ============================================================================
// SECTION 2: Delegation Methods
// ============================================================================

HIRExpr HIRGenerator::convert_expr(const ASTNode *node) {
    return expr_converter_->convert_expr(node);
}

HIRStmt HIRGenerator::convert_stmt(const ASTNode *node) {
    return stmt_converter_->convert_stmt(node);
}

HIRFunction HIRGenerator::convert_function(const ASTNode *node) {
    return decl_type_converter_->convert_function(node);
}

HIRStruct HIRGenerator::convert_struct(const ASTNode *node) {
    return decl_type_converter_->convert_struct(node);
}

HIREnum HIRGenerator::convert_enum(const ASTNode *node) {
    return decl_type_converter_->convert_enum(node);
}

HIRUnion HIRGenerator::convert_union(const ASTNode *node) {
    return decl_type_converter_->convert_union(node);
}

HIRInterface HIRGenerator::convert_interface(const ASTNode *node) {
    return decl_type_converter_->convert_interface(node);
}

HIRImpl HIRGenerator::convert_impl(const ASTNode *node) {
    return decl_type_converter_->convert_impl(node);
}

HIRType HIRGenerator::convert_type(TypeInfo type_info, const std::string &type_name) {
    return decl_type_converter_->convert_type(type_info, type_name);
}

HIRType HIRGenerator::convert_array_type(const ArrayTypeInfo &array_info) {
    return decl_type_converter_->convert_array_type(array_info);
}

// ============================================================================
// SECTION 3: Utility Methods
// ============================================================================

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

const ASTNode* HIRGenerator::lookup_function(const std::string& name) const {
    if (!ast_nodes_) return nullptr;
    
    for (const auto& node : *ast_nodes_) {
        if (node && node->node_type == ASTNodeType::AST_FUNC_DECL &&
            node->name == name) {
            return node.get();
        }
    }
    return nullptr;
}

// ============================================================================
// SECTION 4: Main Entry Points - HIR Program Generation
// ============================================================================

/**
 * @brief Generate HIR from AST nodes
 * @param ast_nodes Vector of AST nodes to convert
 * @return Unique pointer to generated HIR program
 */
std::unique_ptr<HIRProgram>
HIRGenerator::generate(const std::vector<std::unique_ptr<ASTNode>> &ast_nodes) {
    DEBUG_PRINT(DebugMsgId::HIR_GENERATION_START);

    // Store AST nodes for lookup during conversion
    ast_nodes_ = &ast_nodes;

    auto program = std::make_unique<HIRProgram>();

    // v0.14.0: First pass - collect all interface names for value type resolution
    for (const auto &node : ast_nodes) {
        if (node && node->node_type == ASTNodeType::AST_INTERFACE_DECL) {
            interface_names_.insert(node->name);
        }
    }

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

        case ASTNodeType::AST_UNION_TYPEDEF_DECL: {
            auto union_def = convert_union(node.get());
            program->unions.push_back(std::move(union_def));
            break;
        }

        case ASTNodeType::AST_TYPEDEF_DECL: {
            // 単純なtypedef（typedef int MyInt; など）の処理
            HIRTypedef typedef_def;
            typedef_def.name = node->name;
            typedef_def.target_type = convert_type(node->type_info, node->type_name);
            typedef_def.location = convert_location(node->location);
            program->typedefs.push_back(std::move(typedef_def));
            break;
        }

        case ASTNodeType::AST_FUNCTION_POINTER_TYPEDEF: {
            // 関数ポインタのtypedef処理
            HIRTypedef typedef_def;
            typedef_def.name = node->name;
            
            // 関数ポインタ型を構築
            HIRType func_ptr_type;
            func_ptr_type.kind = HIRType::TypeKind::Function;
            
            auto &fp = node->function_pointer_type;
            
            // 戻り値型
            func_ptr_type.return_type = std::make_unique<HIRType>(
                convert_type(fp.return_type, fp.return_type_name));
            
            // パラメータ型
            for (size_t i = 0; i < fp.param_types.size(); ++i) {
                HIRType param_type = convert_type(
                    fp.param_types[i],
                    i < fp.param_type_names.size() ? fp.param_type_names[i] : "");
                func_ptr_type.param_types.push_back(param_type);
            }
            
            typedef_def.target_type = std::move(func_ptr_type);
            typedef_def.location = convert_location(node->location);
            program->typedefs.push_back(std::move(typedef_def));
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
            // トップレベルの変数宣言は全てグローバル変数として扱う
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
            
            if (debug_mode) {
                std::cerr << "[HIR_GLOBAL] Global variable: " << node->name << std::endl;
            }
            break;
        }

        // グローバル配列宣言
        case ASTNodeType::AST_ARRAY_DECL: {
            if (debug_mode) {
                std::cerr << "[HIR_GLOBAL] Processing AST_ARRAY_DECL: " << node->name
                          << ", type_info=" << node->type_info 
                          << ", type_name=" << node->type_name << std::endl;
            }
            
            HIRGlobalVar global_var;
            global_var.name = node->name;
            global_var.type = convert_type(node->type_info, node->type_name);
            global_var.is_const = node->is_const;
            global_var.is_exported = node->is_exported;
            // 配列のサイズ情報は型に含まれる
            global_var.location = convert_location(node->location);
            
            if (debug_mode) {
                std::cerr << "[HIR_GLOBAL] Global array: " << node->name 
                          << ", array_dimensions.size=" << global_var.type.array_dimensions.size()
                          << ", array_size=" << global_var.type.array_size << std::endl;
            }
            
            program->global_vars.push_back(std::move(global_var));
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

        // v0.14.0: Track interface names for value type resolution
        interface_names_.insert(def.name);

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

} // namespace ir
} // namespace cb
