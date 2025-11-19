// v0.14.0: HIR to C++ Transpiler - Statement Generation
// 文生成モジュール

#include "hir_to_cpp.h"
#include "../../common/debug.h"

namespace cb {
namespace codegen {

using namespace ir::hir;

void HIRToCpp::generate_stmt(const HIRStmt &stmt) {
    switch (stmt.kind) {
    case HIRStmt::StmtKind::VarDecl:
        generate_var_decl(stmt);
        break;
    case HIRStmt::StmtKind::Assignment:
        generate_assignment(stmt);
        break;
    case HIRStmt::StmtKind::ExprStmt:
        emit_indent();
        emit(generate_expr(*stmt.expr));
        emit(";\n");
        break;
    case HIRStmt::StmtKind::If:
        generate_if(stmt);
        break;
    case HIRStmt::StmtKind::While:
        generate_while(stmt);
        break;
    case HIRStmt::StmtKind::For:
        generate_for(stmt);
        break;
    case HIRStmt::StmtKind::Return:
        generate_return(stmt);
        break;
    case HIRStmt::StmtKind::Break:
        emit_line("break;");
        break;
    case HIRStmt::StmtKind::Continue:
        emit_line("continue;");
        break;
    case HIRStmt::StmtKind::Block:
        generate_block(stmt);
        break;
    case HIRStmt::StmtKind::Switch:
        generate_switch(stmt);
        break;
    case HIRStmt::StmtKind::Defer:
        generate_defer(stmt);
        break;
    case HIRStmt::StmtKind::Delete:
        generate_delete(stmt);
        break;
    case HIRStmt::StmtKind::Try:
        generate_try_catch(stmt);
        break;
    case HIRStmt::StmtKind::Throw:
        emit_indent();
        emit("throw ");
        if (stmt.throw_expr) {
            emit(generate_expr(*stmt.throw_expr));
        }
        emit(";\n");
        break;
    case HIRStmt::StmtKind::Assert:
        generate_assert(stmt);
        break;
    default:
        emit_line("// TODO: Unsupported statement");
        break;
    }
}

void HIRToCpp::generate_var_decl(const HIRStmt &stmt) {
    debug_msg(DebugMsgId::CODEGEN_CPP_STMT_VAR_DECL, 
              generate_type(stmt.var_type).c_str(), 
              stmt.var_name.c_str());
    
    emit_indent();
    if (stmt.is_const) {
        emit("const ");
    }

    // Special handling for arrays (including multidimensional)
    if (stmt.var_type.kind == HIRType::TypeKind::Array) {
        // Get base type and all dimensions
        const HIRType *base_type = nullptr;
        std::vector<int> dimensions;
        get_array_base_type_and_dimensions(stmt.var_type, &base_type,
                                           dimensions);

        // Check if we're in a function that returns an array
        // OR if initializer is a function call that returns array
        // v0.14.0: Always use std::array for fixed-size arrays to support union assignment
        bool use_std_array = true;

        if (!dimensions.empty() && dimensions[0] > 0 && use_std_array) {
            // Use std::array when in a function returning array or initialized
            // by function call
            emit(generate_type(stmt.var_type));
            emit(" " + add_hir_prefix(stmt.var_name));
        } else if (stmt.var_type.array_size == -1 &&
                   !stmt.var_type.name.empty()) {
            // VLA: int[size] -> int array_name[size_expr]
            if (stmt.var_type.inner_type) {
                emit(generate_type(*stmt.var_type.inner_type));
            } else {
                emit("int"); // fallback
            }
            emit(" " + add_hir_prefix(stmt.var_name));
            emit("[" + add_hir_prefix(stmt.var_type.name) + "]");
        } else {
            // Dynamic array without size - use std::vector
            emit(generate_type(stmt.var_type));
            emit(" " + add_hir_prefix(stmt.var_name));
        }
    } else {
        emit(generate_type(stmt.var_type));
        emit(" " + add_hir_prefix(stmt.var_name));
    }

    if (stmt.init_expr) {
        emit(" = ");

        // Type-based cast insertion
        std::string var_type_str = generate_type(stmt.var_type);
        std::string init_expr_str = generate_expr(*stmt.init_expr);

        // void**への代入の場合、キャストを追加
        if (var_type_str == "void**") {
            emit("(void**)");
        }
        // int64_tなど整数型への代入で、式がvoid*キャストを含む場合
        else if ((var_type_str == "int64_t" || var_type_str == "long long" ||
                  var_type_str == "int" || var_type_str == "long") &&
                 init_expr_str.find("(void*)") != std::string::npos) {
            // void*からの変換が必要な場合、明示的にキャスト
            emit("(" + var_type_str + ")");
        }
        // v0.14.0: enum型への代入で、初期化式が整数リテラルの場合はキャスト
        else if (stmt.var_type.kind == HIRType::TypeKind::Enum &&
                 stmt.init_expr->kind == HIRExpr::ExprKind::Literal) {
            emit("static_cast<" + var_type_str + ">(");
            emit(init_expr_str);
            emit(")");
            goto skip_normal_emit;
        }

        emit(init_expr_str);
    skip_normal_emit:;
    } else if (stmt.var_type.kind != HIRType::TypeKind::Array) {
        // No initializer - use default initialization {} (only for non-arrays)
        // Arrays with VLA or fixed size don't need initialization
        emit("{}");
    }

    emit(";\n");
}

void HIRToCpp::generate_assignment(const HIRStmt &stmt) {
    if (debug_mode) {
        std::cerr << "[CODEGEN_ASSIGN] Assignment: has_lhs=" << (stmt.lhs != nullptr)
                  << ", has_rhs=" << (stmt.rhs != nullptr) << std::endl;
    }
    
    emit_indent();
    
    if (!stmt.lhs) {
        std::cerr << "[ERROR] Assignment has null lhs!" << std::endl;
        emit("/* null lhs */ = ");
    } else {
        emit(generate_expr(*stmt.lhs));
        if (debug_mode) {
            std::cerr << "[CODEGEN_ASSIGN] LHS generated" << std::endl;
        }
        emit(" = ");
    }
    
    if (!stmt.rhs) {
        std::cerr << "[ERROR] Assignment has null rhs!" << std::endl;
        emit("/* null rhs */");
    } else {
        emit(generate_expr(*stmt.rhs));
        if (debug_mode) {
            std::cerr << "[CODEGEN_ASSIGN] RHS generated" << std::endl;
        }
    }
    
    emit(";\n");
}

void HIRToCpp::generate_if(const HIRStmt &stmt) {
    emit_indent();
    emit("if (");
    emit(remove_outer_parens(generate_expr(*stmt.condition)));
    emit(") {\n");

    increase_indent();
    if (stmt.then_body) {
        generate_stmt(*stmt.then_body);
    }
    decrease_indent();

    if (stmt.else_body) {
        emit_line("} else {");
        increase_indent();
        generate_stmt(*stmt.else_body);
        decrease_indent();
    }

    emit_line("}");
}

void HIRToCpp::generate_while(const HIRStmt &stmt) {
    emit_indent();
    emit("while (");
    emit(remove_outer_parens(generate_expr(*stmt.condition)));
    emit(") {\n");

    increase_indent();
    if (stmt.body) {
        generate_stmt(*stmt.body);
    }
    decrease_indent();

    emit_line("}");
}

void HIRToCpp::generate_for(const HIRStmt &stmt) {
    emit_indent();
    emit("for (");

    // init
    if (stmt.init) {
        // セミコロンなしで生成
        if (stmt.init->kind == HIRStmt::StmtKind::VarDecl) {
            if (stmt.init->is_const)
                emit("const ");
            emit(generate_type(stmt.init->var_type));
            emit(" " + add_hir_prefix(stmt.init->var_name));
            if (stmt.init->init_expr) {
                emit(" = ");
                emit(generate_expr(*stmt.init->init_expr));
            }
        } else if (stmt.init->kind == HIRStmt::StmtKind::Assignment) {
            if (stmt.init->lhs && stmt.init->rhs) {
                emit(generate_expr(*stmt.init->lhs));
                emit(" = ");
                emit(generate_expr(*stmt.init->rhs));
            }
        } else if (stmt.init->kind == HIRStmt::StmtKind::ExprStmt) {
            if (stmt.init->expr) {
                emit(generate_expr(*stmt.init->expr));
            }
        }
    }
    emit("; ");

    // condition
    if (stmt.condition) {
        emit(remove_outer_parens(generate_expr(*stmt.condition)));
    }
    emit("; ");

    // update
    if (stmt.update) {
        if (stmt.update->kind == HIRStmt::StmtKind::Assignment) {
            if (stmt.update->lhs && stmt.update->rhs) {
                emit(generate_expr(*stmt.update->lhs));
                emit(" = ");
                emit(generate_expr(*stmt.update->rhs));
            }
        } else if (stmt.update->kind == HIRStmt::StmtKind::ExprStmt) {
            if (stmt.update->expr) {
                emit(generate_expr(*stmt.update->expr));
            }
        }
    }

    emit(") {\n");

    increase_indent();
    if (stmt.body) {
        generate_stmt(*stmt.body);
    }
    decrease_indent();

    emit_line("}");
}

void HIRToCpp::generate_return(const HIRStmt &stmt) {
    if (current_function_is_async && stmt.return_expr) {
        // async関数の場合、Future<T>を構築して返す
        // Extract inner type from Future<T>
        std::string inner_type = "int"; // default
        if (current_function_return_type.kind == HIRType::TypeKind::Struct &&
            current_function_return_type.name.find("Future<") == 0) {
            size_t start = current_function_return_type.name.find('<');
            size_t end = current_function_return_type.name.rfind('>');
            if (start != std::string::npos && end != std::string::npos &&
                end > start) {
                inner_type = current_function_return_type.name.substr(
                    start + 1, end - start - 1);
            }
        }

        emit_indent();
        emit("{\n");
        increase_indent();
        emit_indent();
        emit(generate_type(current_function_return_type));
        emit(" __future;\n");
        emit_indent();
        emit("__future.value = ");
        emit(generate_expr(*stmt.return_expr));
        emit(";\n");
        emit_indent();
        emit("__future.is_ready = true;\n");
        emit_indent();
        emit("return __future;\n");
        decrease_indent();
        emit_indent();
        emit("}\n");
    } else {
        // 通常の関数
        emit_indent();
        emit("return");
        if (stmt.return_expr) {
            std::string return_expr_str = generate_expr(*stmt.return_expr);

            emit(" ");
            // void*式を含む場合、reinterpret_castを使用
            // これはvoid*をint等に変換する際の型エラーを防ぐ
            if (return_expr_str.find("(void*)") != std::string::npos) {
                // void*を含む式は、通常ポインタ演算の結果
                // intptr_t経由で安全にキャスト
                emit("(intptr_t)(");
                emit(return_expr_str);
                emit(")");
            } else {
                emit(return_expr_str);
            }
        }
        emit(";\n");
    }
}

void HIRToCpp::generate_block(const HIRStmt &stmt) {
    // デバッグ: ブロック内の文の種類を確認
    if (stmt.block_stmts.empty()) {
        // 空のブロックの場合は何も出力しない（ただし、デバッグのためコメントを残す）
        emit_line("// Empty block");
        return;
    }

    // 変数宣言のみのブロックの場合、中括弧を省略して直接展開
    bool all_var_decls = !stmt.block_stmts.empty();
    for (const auto &s : stmt.block_stmts) {
        if (s.kind != HIRStmt::StmtKind::VarDecl) {
            all_var_decls = false;
            break;
        }
    }

    if (all_var_decls) {
        // 変数宣言のみなので、中括弧なしで直接出力
        for (const auto &s : stmt.block_stmts) {
            generate_stmt(s);
        }
    } else {
        // 通常のブロック
        emit_line("{");
        increase_indent();

        for (const auto &s : stmt.block_stmts) {
            generate_stmt(s);
        }

        decrease_indent();
        emit_line("}");
    }
}

void HIRToCpp::generate_switch(const HIRStmt &stmt) {
    emit_indent();
    emit("switch (");
    emit(generate_expr(*stmt.switch_expr));
    emit(") {\n");

    increase_indent();

    for (const auto &case_item : stmt.switch_cases) {
        if (case_item.case_value) {
            // v0.14.0: 範囲式の場合は複数のcaseラベルを生成
            if (case_item.case_value->kind == HIRExpr::ExprKind::Range) {
                // 範囲の開始と終了を取得（整数リテラルと仮定）
                if (case_item.case_value->range_start &&
                    case_item.case_value->range_end) {
                    int start = std::stoi(
                        case_item.case_value->range_start->literal_value);
                    int end = std::stoi(
                        case_item.case_value->range_end->literal_value);
                    // 範囲の各値に対してcaseラベルを生成
                    for (int i = start; i <= end; i++) {
                        emit_indent();
                        emit("case " + std::to_string(i) + ":\n");
                    }
                }
            }
            // v0.14.0: OR条件の場合は複数のcaseラベルを生成（ネスト対応）
            else if (case_item.case_value->kind ==
                         HIRExpr::ExprKind::BinaryOp &&
                     case_item.case_value->op == "||") {
                // OR式からすべての値を収集
                std::vector<const HIRExpr *> or_values;
                collect_or_values(case_item.case_value.get(), or_values);
                // 各値に対してcaseラベルを生成
                for (const auto *val : or_values) {
                    emit_indent();
                    emit("case ");
                    emit(generate_expr(*val));
                    emit(":\n");
                }
            } else {
                emit_indent();
                emit("case ");
                emit(generate_expr(*case_item.case_value));
                emit(":\n");
            }
        } else {
            emit_line("default:");
        }

        increase_indent();
        for (const auto &case_stmt : case_item.case_body) {
            generate_stmt(case_stmt);
        }
        // v0.14.0: bodyが空の場合はfall-throughなのでbreakを生成しない
        if (!case_item.case_body.empty()) {
            emit_line("break;");
        }
        decrease_indent();
    }

    decrease_indent();
    emit_line("}");
}

void HIRToCpp::collect_or_values(const HIRExpr *expr,
                                 std::vector<const HIRExpr *> &values) {
    if (!expr)
        return;

    // OR演算子の場合は再帰的に左右を収集
    if (expr->kind == HIRExpr::ExprKind::BinaryOp && expr->op == "||") {
        collect_or_values(expr->left.get(), values);
        collect_or_values(expr->right.get(), values);
    } else {
        // OR演算子以外は値として追加
        values.push_back(expr);
    }
}

void HIRToCpp::generate_defer(const HIRStmt &stmt) {
    // C++でdeferを実装するにはRAIIラッパーが必要
    emit_line("// TODO: defer statement");
    if (stmt.defer_stmt) {
        generate_stmt(*stmt.defer_stmt);
    }
}

void HIRToCpp::generate_delete(const HIRStmt &stmt) {
    if (debug_mode) {
        std::cerr << "[CODEGEN_DELETE] Delete statement: has_expr=" 
                  << (stmt.delete_expr != nullptr) << std::endl;
    }
    
    if (!stmt.delete_expr) {
        std::cerr << "[ERROR] Delete statement has null delete_expr!" << std::endl;
        emit_indent();
        emit("delete /* null expr */;\n");
        return;
    }
    
    std::string expr_str = generate_expr(*stmt.delete_expr);
    debug_msg(DebugMsgId::CODEGEN_CPP_STMT_DELETE, expr_str.c_str());
    
    if (debug_mode) {
        std::cerr << "[CODEGEN_DELETE] Generated: delete " << expr_str << std::endl;
    }
    
    emit_indent();
    // 配列の場合は delete[] を使用
    // TODO: HIRで配列かどうかの情報を持たせる
    emit("delete ");
    emit(expr_str);
    emit(";\n");
}

void HIRToCpp::generate_try_catch(const HIRStmt &stmt) {
    emit_line("try {");
    increase_indent();

    for (const auto &s : stmt.try_block) {
        generate_stmt(s);
    }

    decrease_indent();
    emit_line("}");

    for (const auto &catch_clause : stmt.catch_clauses) {
        emit_indent();
        emit("catch (");
        emit(generate_type(catch_clause.exception_type));
        emit(" " + catch_clause.exception_var + ") {\n");

        increase_indent();
        for (const auto &s : catch_clause.catch_body) {
            generate_stmt(s);
        }
        decrease_indent();

        emit_line("}");
    }

    if (!stmt.finally_block.empty()) {
        emit_line("// finally block (executed via RAII)");
        for (const auto &s : stmt.finally_block) {
            generate_stmt(s);
        }
    }
}

void HIRToCpp::generate_assert(const HIRStmt &stmt) {
    emit_indent();
    emit("assert(");
    if (stmt.assert_expr) {
        emit(generate_expr(*stmt.assert_expr));
    } else {
        // If no expression, use false to trigger assertion failure
        emit("false && \"assertion failed\"");
    }
    emit(")");
    if (!stmt.assert_message.empty()) {
        emit(" /* " + stmt.assert_message + " */");
    }
    emit(";\n");
}

} // namespace codegen
} // namespace cb
