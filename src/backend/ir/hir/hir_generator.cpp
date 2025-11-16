#include "hir_generator.h"
#include "hir_builder.h"
#include <iostream>

namespace cb {
namespace ir {

using namespace hir;

HIRGenerator::HIRGenerator() {}

HIRGenerator::~HIRGenerator() {}

std::unique_ptr<HIRProgram>
HIRGenerator::generate(const std::vector<std::unique_ptr<ASTNode>> &ast_nodes) {
    auto program = std::make_unique<HIRProgram>();

    for (const auto &node : ast_nodes) {
        if (!node)
            continue;

        switch (node->node_type) {
        case ASTNodeType::AST_FUNC_DECL: {
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
        expr.kind = HIRExpr::ExprKind::UnaryOp;
        expr.op = node->op;
        expr.operand =
            std::make_unique<HIRExpr>(convert_expr(node->left.get()));
        break;
    }

    case ASTNodeType::AST_FUNC_CALL: {
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
        expr.cast_expr =
            std::make_unique<HIRExpr>(convert_expr(node->left.get()));
        expr.cast_type = convert_type(node->type_info, node->type_name);
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

        // case ASTNodeType::AST_AWAIT_EXPR: {
        //     expr.kind = HIRExpr::ExprKind::Await;
        //     expr.operand =
        //     std::make_unique<HIRExpr>(convert_expr(node->left.get())); break;
        // }

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

    default:
        report_error("Unsupported expression type in HIR generation",
                     node->location);
        expr.kind = HIRExpr::ExprKind::Literal;
        break;
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

    switch (node->node_type) {
    case ASTNodeType::AST_VAR_DECL: {
        stmt.kind = HIRStmt::StmtKind::VarDecl;
        stmt.var_name = node->name;
        stmt.var_type = convert_type(node->type_info, node->type_name);
        stmt.is_const = node->is_const;
        if (node->right) {
            stmt.init_expr =
                std::make_unique<HIRExpr>(convert_expr(node->right.get()));
        }
        break;
    }

    case ASTNodeType::AST_ASSIGN: {
        stmt.kind = HIRStmt::StmtKind::Assignment;
        stmt.lhs = std::make_unique<HIRExpr>(convert_expr(node->left.get()));
        stmt.rhs = std::make_unique<HIRExpr>(convert_expr(node->right.get()));
        break;
    }

    case ASTNodeType::AST_IF_STMT: {
        stmt.kind = HIRStmt::StmtKind::If;
        stmt.condition =
            std::make_unique<HIRExpr>(convert_expr(node->condition.get()));
        stmt.then_body =
            std::make_unique<HIRStmt>(convert_stmt(node->body.get()));
        if (node->else_body) {
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
        for (const auto &child : node->statements) {
            stmt.block_stmts.push_back(convert_stmt(child.get()));
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

    default:
        report_error("Unsupported statement type in HIR generation",
                     node->location);
        stmt.kind = HIRStmt::StmtKind::Block;
        break;
    }

    return stmt;
}

HIRFunction HIRGenerator::convert_function(const ASTNode *node) {
    HIRFunction func;

    if (!node)
        return func;

    func.name = node->name;
    func.location = convert_location(node->location);
    func.return_type = convert_type(node->type_info, node->return_type_name);
    func.is_async = node->is_async;
    func.is_exported = node->is_exported;

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

    // メソッドの変換
    for (const auto &child : node->children) {
        if (child->node_type == ASTNodeType::AST_FUNC_DECL) {
            impl_def.methods.push_back(convert_function(child.get()));
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
        // TODO: 内部型の変換
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
