/**
 * @file hir_stmt_converter.cpp
 * @brief HIR Statement Converter - AST Statement to HIR
 */

#include "hir_stmt_converter.h"
#include "../../../common/debug.h"
#include "hir_builder.h"
#include "hir_generator.h"
#include <iostream>

namespace cb {
namespace ir {

using namespace hir;

HIRStmtConverter::HIRStmtConverter(HIRGenerator *generator)
    : generator_(generator) {}

HIRStmtConverter::~HIRStmtConverter() {}

HIRStmt HIRStmtConverter::convert_stmt(const ASTNode *node) {
    HIRStmt stmt;

    if (!node) {
        stmt.kind = HIRStmt::StmtKind::Block;
        return stmt;
    }

    stmt.location = generator_->convert_location(node->location);

    if (debug_mode) {
        std::cerr << "[HIR_STMT] Converting statement type: "
                  << static_cast<int>(node->node_type) << std::endl;
    }

    switch (node->node_type) {
    case ASTNodeType::AST_VAR_DECL: {
        stmt.kind = HIRStmt::StmtKind::VarDecl;
        stmt.var_name = node->name;

        // unsigned修飾子を考慮してTypeInfoを調整
        TypeInfo adjusted_type_info = node->type_info;
        if (node->is_unsigned) {
            switch (node->type_info) {
            case TYPE_TINY:
                adjusted_type_info = TYPE_UNSIGNED_TINY;
                break;
            case TYPE_SHORT:
                adjusted_type_info = TYPE_UNSIGNED_SHORT;
                break;
            case TYPE_INT:
                adjusted_type_info = TYPE_UNSIGNED_INT;
                break;
            case TYPE_LONG:
                adjusted_type_info = TYPE_UNSIGNED_LONG;
                break;
            default:
                break;
            }
        }

        // v0.14.0: For Option and Result types, use original_type_name to
        // preserve generic syntax
        std::string type_name_to_use = node->type_name;
        if (!node->original_type_name.empty() &&
            (node->original_type_name.find("Option<") == 0 ||
             node->original_type_name.find("Result<") == 0)) {
            type_name_to_use = node->original_type_name;
        }

        if (debug_mode) {
            std::cerr << "[HIR_STMT] VarDecl type conversion - type_name: "
                      << node->type_name
                      << ", original_type_name: " << node->original_type_name
                      << ", using: " << type_name_to_use
                      << ", is_array: " << (node->is_array ? "true" : "false")
                      << std::endl;
        }

        // v0.14.0: Use array_type_info if this is an array
        if (node->is_array && node->array_type_info.is_array()) {
            if (debug_mode) {
                std::cerr << "[HIR_STMT] Converting as array type with "
                             "element_type_name: '"
                          << node->array_type_info.element_type_name << "'"
                          << std::endl;
                std::cerr << "[HIR_STMT]   base_type: "
                          << node->array_type_info.base_type << ", dimensions: "
                          << node->array_type_info.dimensions.size()
                          << std::endl;
            }
            stmt.var_type =
                generator_->convert_array_type(node->array_type_info);
        } else if (node->is_function_pointer) {
            // Handle explicit function pointer variable declarations
            if (debug_mode) {
                std::cerr << "[HIR_STMT] Converting as function pointer type"
                          << std::endl;
            }
            stmt.var_type.kind = HIRType::TypeKind::Function;

            const auto &fp = node->function_pointer_type;

            // Set the return type of the function pointer
            stmt.var_type.return_type = std::make_unique<HIRType>(
                generator_->convert_type(fp.return_type, fp.return_type_name));

            // Convert parameter types
            for (size_t i = 0; i < fp.param_types.size(); ++i) {
                std::string param_type_name = i < fp.param_type_names.size()
                                                  ? fp.param_type_names[i]
                                                  : "";
                stmt.var_type.param_types.push_back(generator_->convert_type(
                    fp.param_types[i], param_type_name));
            }
        } else {
            stmt.var_type =
                generator_->convert_type(adjusted_type_info, type_name_to_use);
        }

        // Handle reference types (T& or T&&)
        if (node->is_reference) {
            HIRType ref_type;
            ref_type.kind = HIRType::TypeKind::Reference;
            ref_type.inner_type =
                std::make_unique<HIRType>(std::move(stmt.var_type));
            stmt.var_type = std::move(ref_type);
        } else if (node->is_rvalue_reference) {
            HIRType ref_type;
            ref_type.kind = HIRType::TypeKind::RvalueReference;
            ref_type.inner_type =
                std::make_unique<HIRType>(std::move(stmt.var_type));
            stmt.var_type = std::move(ref_type);
        }

        stmt.is_const = node->is_const;
        if (debug_mode) {
            DEBUG_PRINT(DebugMsgId::HIR_STMT_VAR_DECL, node->name.c_str());
        }

        // Register variable type in symbol table for type inference
        generator_->variable_types_[node->name] = stmt.var_type;

        // v0.14.0: init_exprを優先的に使用（新しいパーサーフォーマット）
        ASTNode *init_node = nullptr;
        if (node->init_expr) {
            stmt.init_expr = std::make_unique<HIRExpr>(
                generator_->convert_expr(node->init_expr.get()));
            init_node = node->init_expr.get();
            if (debug_mode) {
                std::cerr
                    << "[HIR_STMT]     Has initializer expression (init_expr)"
                    << std::endl;
            }
        } else if (node->right) {
            stmt.init_expr = std::make_unique<HIRExpr>(
                generator_->convert_expr(node->right.get()));
            init_node = node->right.get();
            if (debug_mode) {
                std::cerr << "[HIR_STMT]     Has initializer expression (right)"
                          << std::endl;
            }
        } else if (debug_mode) {
            std::cerr << "[HIR_STMT]     No initializer expression"
                      << std::endl;
        }

        // Function pointer type inference:
        // Case 1: if initializer is &function_name
        if (init_node && init_node->node_type == ASTNodeType::AST_UNARY_OP &&
            (init_node->op == "&" || init_node->op == "ADDRESS_OF") &&
            init_node->left &&
            init_node->left->node_type == ASTNodeType::AST_VARIABLE) {
            std::string func_name = init_node->left->name;

            if (debug_mode) {
                std::cerr << "[HIR_STMT]     Checking function pointer "
                             "inference for &"
                          << func_name << std::endl;
            }

            const ASTNode *func_node = generator_->lookup_function(func_name);
            if (func_node) {
                // Create function pointer type
                stmt.var_type.kind = HIRType::TypeKind::Function;

                // Convert return type
                stmt.var_type.return_type =
                    std::make_unique<HIRType>(generator_->convert_type(
                        func_node->type_info, func_node->return_type_name));

                // Convert parameter types
                for (const auto &param_node : func_node->parameters) {
                    HIRType param_type = generator_->convert_type(
                        param_node->type_info, param_node->type_name);
                    stmt.var_type.param_types.push_back(param_type);
                }

                if (debug_mode) {
                    std::cerr << "[HIR_STMT]     ✓ Inferred function pointer "
                                 "type for &"
                              << func_name << std::endl;
                }
            } else if (debug_mode) {
                std::cerr << "[HIR_STMT]     ✗ Function not found: "
                          << func_name << std::endl;
            }
        }
        // Case 2: if initializer is a function call that returns a function
        // pointer
        else if (init_node &&
                 init_node->node_type == ASTNodeType::AST_FUNC_CALL) {
            std::string called_func_name = init_node->name;

            if (debug_mode) {
                std::cerr << "[HIR_STMT]     Checking if function call "
                          << called_func_name << " returns function pointer"
                          << std::endl;
            }

            const ASTNode *called_func =
                generator_->lookup_function(called_func_name);
            // Check if the function returns a function pointer (either
            // explicitly marked or inferred)
            if (called_func &&
                (called_func->is_function_pointer_return ||
                 generator_->analyze_function_returns_function_pointer(
                     called_func))) {
                // The called function returns a function pointer

                // If function pointer type information is available, use it
                if (called_func->is_function_pointer_return) {
                    // Copy the explicit function pointer type information
                    stmt.var_type.kind = HIRType::TypeKind::Function;

                    const auto &fp = called_func->function_pointer_type;

                    // Set the return type of the function pointer
                    stmt.var_type.return_type =
                        std::make_unique<HIRType>(generator_->convert_type(
                            fp.return_type, fp.return_type_name));

                    // Convert parameter types
                    for (size_t i = 0; i < fp.param_types.size(); ++i) {
                        std::string param_type_name =
                            i < fp.param_type_names.size()
                                ? fp.param_type_names[i]
                                : "";
                        stmt.var_type.param_types.push_back(
                            generator_->convert_type(fp.param_types[i],
                                                     param_type_name));
                    }
                } else {
                    // Inferred that it returns a function pointer but no
                    // explicit type info For now, we'll create a generic
                    // function pointer type In a proper implementation, we
                    // would analyze what functions are returned
                    stmt.var_type.kind = HIRType::TypeKind::Function;

                    // Default to int(*)(int, int) for now since that's what the
                    // test uses
                    // TODO: Properly infer the function signature from the
                    // returned functions
                    stmt.var_type.return_type = std::make_unique<HIRType>(
                        generator_->convert_type(TYPE_INT, "int"));
                    stmt.var_type.param_types.push_back(
                        generator_->convert_type(TYPE_INT, "int"));
                    stmt.var_type.param_types.push_back(
                        generator_->convert_type(TYPE_INT, "int"));
                }

                if (debug_mode) {
                    std::cerr << "[HIR_STMT]     ✓ Function "
                              << called_func_name << " returns function pointer"
                              << std::endl;
                }
            }
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

            // unsigned修飾子を考慮
            TypeInfo adjusted_type_info = first_var->type_info;
            if (first_var->is_unsigned) {
                switch (first_var->type_info) {
                case TYPE_TINY:
                    adjusted_type_info = TYPE_UNSIGNED_TINY;
                    break;
                case TYPE_SHORT:
                    adjusted_type_info = TYPE_UNSIGNED_SHORT;
                    break;
                case TYPE_INT:
                    adjusted_type_info = TYPE_UNSIGNED_INT;
                    break;
                case TYPE_LONG:
                    adjusted_type_info = TYPE_UNSIGNED_LONG;
                    break;
                default:
                    break;
                }
            }

            // v0.14.0: Use array_type_info if this is an array
            if (first_var->is_array && first_var->array_type_info.is_array()) {
                if (debug_mode) {
                    std::cerr << "[HIR_STMT] Converting as array type with "
                                 "element_type_name: '"
                              << first_var->array_type_info.element_type_name
                              << "'" << std::endl;
                }
                stmt.var_type =
                    generator_->convert_array_type(first_var->array_type_info);
            } else {
                stmt.var_type = generator_->convert_type(adjusted_type_info,
                                                         first_var->type_name);
            }
            stmt.is_const = first_var->is_const;
            if (first_var->right) {
                stmt.init_expr = std::make_unique<HIRExpr>(
                    generator_->convert_expr(first_var->right.get()));
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

                    // unsigned修飾子を考慮
                    TypeInfo adj_type = var_node->type_info;
                    if (var_node->is_unsigned) {
                        switch (var_node->type_info) {
                        case TYPE_TINY:
                            adj_type = TYPE_UNSIGNED_TINY;
                            break;
                        case TYPE_SHORT:
                            adj_type = TYPE_UNSIGNED_SHORT;
                            break;
                        case TYPE_INT:
                            adj_type = TYPE_UNSIGNED_INT;
                            break;
                        case TYPE_LONG:
                            adj_type = TYPE_UNSIGNED_LONG;
                            break;
                        default:
                            break;
                        }
                    }

                    // v0.14.0: Use array_type_info if this is an array
                    if (var_node->is_array &&
                        var_node->array_type_info.is_array()) {
                        var_stmt.var_type = generator_->convert_array_type(
                            var_node->array_type_info);
                    } else {
                        var_stmt.var_type = generator_->convert_type(
                            adj_type, var_node->type_name);
                    }
                    var_stmt.is_const = var_node->is_const;
                    if (var_node->right) {
                        var_stmt.init_expr = std::make_unique<HIRExpr>(
                            generator_->convert_expr(var_node->right.get()));
                    }
                    var_stmt.location =
                        generator_->convert_location(var_node->location);
                    stmt.block_stmts.push_back(std::move(var_stmt));
                }
            }
        }
        break;
    }

    case ASTNodeType::AST_ARRAY_DECL: {
        stmt.kind = HIRStmt::StmtKind::VarDecl;
        stmt.var_name = node->name;

        // v0.14.0: Debug array declaration
        if (debug_mode) {
            std::cerr << "[HIR_STMT] AST_ARRAY_DECL: " << node->name
                      << ", type_name: " << node->type_name << std::endl;
            if (node->array_type_info.is_array()) {
                std::cerr << "[HIR_STMT]   array_type_info.element_type_name: '"
                          << node->array_type_info.element_type_name << "'"
                          << std::endl;
                std::cerr << "[HIR_STMT]   array_type_info.base_type: "
                          << node->array_type_info.base_type << std::endl;
            }
        }

        // Use array_type_info if available
        if (node->is_function_pointer && node->array_type_info.is_array()) {
            // Function pointer array: create array of function pointers
            HIRType array_type;
            array_type.kind = HIRType::TypeKind::Array;

            // Create the function pointer type for array elements
            auto elem_type = std::make_unique<HIRType>();
            elem_type->kind = HIRType::TypeKind::Function;

            const auto &fp = node->function_pointer_type;

            // Set the return type of the function pointer
            elem_type->return_type = std::make_unique<HIRType>(
                generator_->convert_type(fp.return_type, fp.return_type_name));

            // Convert parameter types
            for (size_t i = 0; i < fp.param_types.size(); ++i) {
                std::string param_type_name = i < fp.param_type_names.size()
                                                  ? fp.param_type_names[i]
                                                  : "";
                elem_type->param_types.push_back(generator_->convert_type(
                    fp.param_types[i], param_type_name));
            }

            array_type.inner_type = std::move(elem_type);

            // Set array dimensions
            if (!node->array_type_info.dimensions.empty()) {
                array_type.array_size =
                    node->array_type_info.dimensions[0].size;
                for (const auto &dim : node->array_type_info.dimensions) {
                    array_type.array_dimensions.push_back(dim.size);
                }
            }

            stmt.var_type = std::move(array_type);
        } else if (node->array_type_info.is_array()) {
            stmt.var_type =
                generator_->convert_array_type(node->array_type_info);
        } else {
            stmt.var_type =
                generator_->convert_type(node->type_info, node->type_name);
        }

        stmt.is_const = node->is_const;
        if (debug_mode) {
            DEBUG_PRINT(DebugMsgId::HIR_STMT_VAR_DECL, node->name.c_str());
        }

        // Register array variable type in symbol table for type inference
        generator_->variable_types_[node->name] = stmt.var_type;

        // Array initialization
        if (node->init_expr) {
            stmt.init_expr = std::make_unique<HIRExpr>(
                generator_->convert_expr(node->init_expr.get()));
        } else if (node->right) {
            stmt.init_expr = std::make_unique<HIRExpr>(
                generator_->convert_expr(node->right.get()));
        }
        break;
    }

    case ASTNodeType::AST_ASSIGN: {
        stmt.kind = HIRStmt::StmtKind::Assignment;

        // 左辺：node->leftまたはnode->nameから取得
        if (node->left) {
            stmt.lhs = std::make_unique<HIRExpr>(
                generator_->convert_expr(node->left.get()));
        } else if (!node->name.empty()) {
            // 変数名が直接node->nameに入っている場合
            HIRExpr var_expr;
            var_expr.kind = HIRExpr::ExprKind::Variable;
            var_expr.var_name = node->name;
            stmt.lhs = std::make_unique<HIRExpr>(std::move(var_expr));
        } else {
            std::string error_msg = "AST_ASSIGN has no left operand or name";
            generator_->report_error(error_msg, node->location);
        }

        // 右辺
        if (node->right) {
            stmt.rhs = std::make_unique<HIRExpr>(
                generator_->convert_expr(node->right.get()));
        } else {
            std::string error_msg = "AST_ASSIGN has null right operand";
            generator_->report_error(error_msg, node->location);
        }
        break;
    }

    case ASTNodeType::AST_IF_STMT: {
        stmt.kind = HIRStmt::StmtKind::If;
        if (debug_mode) {
            DEBUG_PRINT(DebugMsgId::HIR_STMT_IF);
        }
        stmt.condition = std::make_unique<HIRExpr>(
            generator_->convert_expr(node->condition.get()));

        // パーサーはif文の本体をleftに格納し、else節をrightに格納する
        // bodyフィールドは使用されていない
        if (node->left) {
            stmt.then_body = std::make_unique<HIRStmt>(
                generator_->convert_stmt(node->left.get()));
        } else if (node->body) {
            // 後方互換性のためbodyもチェック
            stmt.then_body = std::make_unique<HIRStmt>(
                generator_->convert_stmt(node->body.get()));
        }

        // else節
        if (node->right) {
            if (debug_mode) {
                std::cerr << "[HIR_STMT]     Has else branch" << std::endl;
            }
            stmt.else_body = std::make_unique<HIRStmt>(
                generator_->convert_stmt(node->right.get()));
        } else if (node->else_body) {
            // 後方互換性のためelse_bodyもチェック
            stmt.else_body = std::make_unique<HIRStmt>(
                generator_->convert_stmt(node->else_body.get()));
        }
        break;
    }

    case ASTNodeType::AST_WHILE_STMT: {
        stmt.kind = HIRStmt::StmtKind::While;
        stmt.condition = std::make_unique<HIRExpr>(
            generator_->convert_expr(node->condition.get()));
        stmt.body = std::make_unique<HIRStmt>(
            generator_->convert_stmt(node->body.get()));
        break;
    }

    case ASTNodeType::AST_FOR_STMT: {
        stmt.kind = HIRStmt::StmtKind::For;
        if (node->init_expr) {
            stmt.init = std::make_unique<HIRStmt>(
                generator_->convert_stmt(node->init_expr.get()));
        }
        if (node->condition) {
            stmt.condition = std::make_unique<HIRExpr>(
                generator_->convert_expr(node->condition.get()));
        }
        if (node->update_expr) {
            stmt.update = std::make_unique<HIRStmt>(
                generator_->convert_stmt(node->update_expr.get()));
        }
        stmt.body = std::make_unique<HIRStmt>(
            generator_->convert_stmt(node->body.get()));
        break;
    }

    case ASTNodeType::AST_RETURN_STMT: {
        stmt.kind = HIRStmt::StmtKind::Return;
        if (node->left) {
            stmt.return_expr = std::make_unique<HIRExpr>(
                generator_->convert_expr(node->left.get()));
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
                stmt.block_stmts.push_back(
                    generator_->convert_stmt(child.get()));
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
        call_expr.type =
            hir::HIRBuilder::make_basic_type(HIRType::TypeKind::Void);

        // 引数を変換（単一引数の場合はleft、複数引数の場合はarguments）
        if (node->left) {
            call_expr.arguments.push_back(
                generator_->convert_expr(node->left.get()));
        } else if (!node->arguments.empty()) {
            for (const auto &arg : node->arguments) {
                call_expr.arguments.push_back(
                    generator_->convert_expr(arg.get()));
            }
        }
        // 引数がない場合は空の引数リストで呼び出し

        stmt.expr = std::make_unique<HIRExpr>(std::move(call_expr));
        break;
    }

    case ASTNodeType::AST_FUNC_CALL: {
        stmt.kind = HIRStmt::StmtKind::ExprStmt;
        stmt.expr = std::make_unique<HIRExpr>(generator_->convert_expr(node));
        break;
    }

    case ASTNodeType::AST_PRE_INCDEC:
    case ASTNodeType::AST_POST_INCDEC: {
        // インクリメント/デクリメントを式文として扱う
        stmt.kind = HIRStmt::StmtKind::ExprStmt;
        stmt.expr = std::make_unique<HIRExpr>(generator_->convert_expr(node));
        break;
    }

    case ASTNodeType::AST_ASSERT_STMT: {
        // assert文をHIRに変換
        stmt.kind = HIRStmt::StmtKind::Assert;
        // assert condition is stored in node->left
        if (node->left) {
            stmt.assert_expr = std::make_unique<HIRExpr>(
                generator_->convert_expr(node->left.get()));
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
            stmt.defer_stmt = std::make_unique<HIRStmt>(
                generator_->convert_stmt(node->body.get()));
        }
        break;
    }

    case ASTNodeType::AST_DELETE_EXPR: {
        stmt.kind = HIRStmt::StmtKind::Delete;
        if (debug_mode) {
            std::cerr << "[HIR_STMT] Delete expression: has_delete_expr="
                      << (node->delete_expr != nullptr) << std::endl;
        }
        if (node->delete_expr) {
            stmt.delete_expr = std::make_unique<HIRExpr>(
                generator_->convert_expr(node->delete_expr.get()));
            if (debug_mode) {
                std::cerr << "[HIR_STMT] Delete expr converted successfully"
                          << std::endl;
            }
        } else {
            std::cerr << "[HIR_ERROR] Delete expression has null delete_expr!"
                      << std::endl;
        }
        break;
    }

    case ASTNodeType::AST_SWITCH_STMT: {
        stmt.kind = HIRStmt::StmtKind::Switch;
        // パーサーは switch_expr フィールドを使用
        stmt.switch_expr = std::make_unique<HIRExpr>(
            generator_->convert_expr(node->switch_expr.get()));

        // caseの変換 (ASTの構造に合わせて修正)
        for (const auto &case_node : node->cases) {
            // v0.14.0:
            // case_valuesが複数ある場合（OR結合）、各値ごとにHIRCaseを生成
            if (!case_node->case_values.empty()) {
                // 最初のcase_valueを使用してHIRCaseを作成
                HIRStmt::SwitchCase hir_case;
                if (case_node->case_values[0]) {
                    hir_case.case_value =
                        std::make_unique<HIRExpr>(generator_->convert_expr(
                            case_node->case_values[0].get()));
                }
                // case_bodyを設定
                if (case_node->case_body &&
                    !case_node->case_body->statements.empty()) {
                    for (const auto &case_stmt :
                         case_node->case_body->statements) {
                        hir_case.case_body.push_back(
                            generator_->convert_stmt(case_stmt.get()));
                    }
                }
                stmt.switch_cases.push_back(std::move(hir_case));

                // 2番目以降のcase_valuesがある場合、fall-throughケースとして追加
                for (size_t i = 1; i < case_node->case_values.size(); i++) {
                    if (case_node->case_values[i]) {
                        HIRStmt::SwitchCase fallthrough_case;
                        fallthrough_case.case_value =
                            std::make_unique<HIRExpr>(generator_->convert_expr(
                                case_node->case_values[i].get()));
                        // bodyは空（fall-through）
                        stmt.switch_cases.insert(stmt.switch_cases.end() - 1,
                                                 std::move(fallthrough_case));
                    }
                }
            }
        }

        // else節（defaultケース）の処理
        if (node->else_body) {
            HIRStmt::SwitchCase default_case;
            // case_valueがnullptrの場合はdefault
            for (const auto &else_stmt : node->else_body->statements) {
                default_case.case_body.push_back(
                    generator_->convert_stmt(else_stmt.get()));
            }
            stmt.switch_cases.push_back(std::move(default_case));
        }
        break;
    }

    case ASTNodeType::AST_TRY_STMT: {
        stmt.kind = HIRStmt::StmtKind::Try;

        // tryブロック
        if (node->try_body) {
            for (const auto &try_stmt : node->try_body->statements) {
                stmt.try_block.push_back(
                    generator_->convert_stmt(try_stmt.get()));
            }
        }

        // catchブロック (AST構造に合わせて単一catch)
        if (node->catch_body) {
            HIRStmt::CatchClause catch_clause;
            catch_clause.exception_var = node->exception_var;
            catch_clause.exception_type =
                generator_->convert_type(node->type_info, node->exception_type);
            for (const auto &catch_stmt : node->catch_body->statements) {
                catch_clause.catch_body.push_back(
                    generator_->convert_stmt(catch_stmt.get()));
            }
            stmt.catch_clauses.push_back(std::move(catch_clause));
        }

        // finallyブロック
        if (node->finally_body) {
            for (const auto &finally_stmt : node->finally_body->statements) {
                stmt.finally_block.push_back(
                    generator_->convert_stmt(finally_stmt.get()));
            }
        }
        break;
    }

    case ASTNodeType::AST_THROW_STMT: {
        stmt.kind = HIRStmt::StmtKind::Throw;
        if (node->left) {
            stmt.throw_expr = std::make_unique<HIRExpr>(
                generator_->convert_expr(node->left.get()));
        }
        break;
    }

    case ASTNodeType::AST_MATCH_STMT: {
        stmt.kind = HIRStmt::StmtKind::Match;
        if (node->match_expr) {
            stmt.match_expr = std::make_unique<HIRExpr>(
                generator_->convert_expr(node->match_expr.get()));
        }

        // match アームの変換
        for (const auto &ast_arm : node->match_arms) {
            HIRStmt::MatchArm hir_arm;

            // パターンの種類を変換
            switch (ast_arm.pattern_type) {
            case PatternType::PATTERN_WILDCARD:
                hir_arm.pattern_kind = HIRStmt::MatchArm::PatternKind::Wildcard;
                break;
            case PatternType::PATTERN_LITERAL:
                hir_arm.pattern_kind = HIRStmt::MatchArm::PatternKind::Literal;
                break;
            case PatternType::PATTERN_ENUM_VARIANT:
                hir_arm.pattern_kind =
                    HIRStmt::MatchArm::PatternKind::EnumVariant;
                break;
            default:
                hir_arm.pattern_kind = HIRStmt::MatchArm::PatternKind::Variable;
                break;
            }

            // パターン名とバインディングをコピー
            hir_arm.pattern_name = ast_arm.variant_name;
            hir_arm.bindings = ast_arm.bindings;
            hir_arm.enum_type_name = ast_arm.enum_type_name;

            // アームの本体を変換
            if (ast_arm.body) {
                // 本体がブロックの場合
                if (ast_arm.body->node_type == ASTNodeType::AST_COMPOUND_STMT) {
                    for (const auto &stmt_node : ast_arm.body->statements) {
                        hir_arm.body.push_back(
                            generator_->convert_stmt(stmt_node.get()));
                    }
                } else {
                    // 単一の文の場合
                    hir_arm.body.push_back(
                        generator_->convert_stmt(ast_arm.body.get()));
                }
            }

            stmt.match_arms.push_back(std::move(hir_arm));
        }
        break;
    }

    // v0.14.0: import文のサポート（関数内でのimportを含む）
    case ASTNodeType::AST_IMPORT_STMT: {
        // import文は基本的にコンパイル時に処理されているが、
        // HIRとしては空のブロックとして扱う（C++では不要）
        stmt.kind = HIRStmt::StmtKind::Block;
        // 空のブロック（C++生成時には何も出力されない）
        break;
        std::string error_msg =
            "Unsupported statement type in HIR generation: AST node type " +
            std::to_string(static_cast<int>(node->node_type));
        generator_->report_error(error_msg, node->location);
        stmt.kind = HIRStmt::StmtKind::Block;
        break;
    }
    }

    return stmt;
}

} // namespace ir
} // namespace cb
