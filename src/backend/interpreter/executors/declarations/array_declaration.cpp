#include "array_declaration.h"
#include "../../../../common/debug.h"
#include "../../../../common/type_helpers.h"
#include "../../core/interpreter.h"
#include "../../managers/arrays/manager.h"
#include "../statement_executor.h"

namespace DeclarationHandlers {

void execute_array_decl(StatementExecutor *executor, Interpreter &interpreter,
                        const ASTNode *node) {
    // 配列宣言をArrayManagerに委譲
    if (debug_mode) {
        std::cerr << "[DEBUG_EXEC_ARRAY] execute_array_decl for: " << node->name
                  << std::endl;
        std::cerr << "[DEBUG_EXEC_ARRAY] array_dimensions.size(): "
                  << node->array_dimensions.size() << std::endl;
        std::cerr << "[DEBUG_EXEC_ARRAY] array_type_info.dimensions.size(): "
                  << node->array_type_info.dimensions.size() << std::endl;
        if (!node->array_dimensions.empty() && node->array_dimensions[0]) {
            std::cerr << "[DEBUG_EXEC_ARRAY] First dimension exists"
                      << std::endl;
        }
    }
    Variable var;
    try {
        interpreter.get_array_manager()->processArrayDeclaration(var, node);

        if (debug_mode) {
            std::cerr
                << "[DEBUG_EXEC_ARRAY] processArrayDeclaration completed: "
                << "is_assigned=" << var.is_assigned
                << ", is_array=" << var.is_array
                << ", array_size=" << var.array_size << std::endl;
        }
    } catch (const ReturnException &ret) {
        // processArrayDeclaration内で関数呼び出しが発生し、構造体配列が返された場合
        if (ret.is_struct && ret.is_array) {
            if (debug_mode) {
                std::cerr << "[DEBUG_EXEC_ARRAY] Caught struct array "
                             "ReturnException from processArrayDeclaration"
                          << std::endl;
            }
            // まず変数を登録
            interpreter.current_scope().variables[node->name] = var;
            // 構造体配列を代入
            interpreter.assign_array_from_return(node->name, ret);
            return;
        }
        // その他のReturnExceptionは再スロー
        throw;
    }

    // 変数を現在のスコープに登録
    interpreter.current_scope().variables[node->name] = var;

    if (debug_mode) {
        std::cerr << "[DEBUG_EXEC_ARRAY] Variable registered in scope: "
                  << node->name << std::endl;
    }

    // struct配列の場合、要素変数を初期化
    if (debug_mode) {
        Variable *reg_var = interpreter.find_variable(node->name);
        std::cerr << "[DEBUG_EXEC_ARRAY] After registration: var.is_struct="
                  << var.is_struct << ", var.is_array=" << var.is_array
                  << ", var.array_size=" << var.array_size
                  << ", struct_type_name=" << var.struct_type_name
                  << ", var_ptr=" << (void *)reg_var << std::endl;
    }
    if (var.is_struct && var.is_array && var.array_size > 0 &&
        !var.struct_type_name.empty()) {
        if (debug_mode) {
            std::cerr << "[DEBUG_EXEC_ARRAY] Initializing struct array "
                         "elements, size="
                      << var.array_size << std::endl;
        }
        // 各配列要素のstruct変数を作成
        for (int i = 0; i < var.array_size; ++i) {
            std::string element_name =
                node->name + "[" + std::to_string(i) + "]";
            interpreter.create_struct_variable(element_name,
                                               var.struct_type_name);
        }
    }

    // 初期化式がある場合の処理
    if (node->init_expr) {
        if (debug_mode) {
            std::cerr << "[DEBUG_ARRAY_DECL] init_expr exists, node_type="
                      << static_cast<int>(node->init_expr->node_type)
                      << " (AST_FUNC_CALL="
                      << static_cast<int>(ASTNodeType::AST_FUNC_CALL) << ")"
                      << std::endl;
        }
        if (node->type_info == TYPE_STRUCT &&
            node->init_expr->node_type == ASTNodeType::AST_ARRAY_LITERAL) {
            // struct配列リテラル初期化: Person[3] people = [{25, "Alice"}, {30,
            // "Bob"}];
            if (debug_mode) {
                std::cerr
                    << "[DEBUG_ARRAY_DECL] Struct array literal initialization"
                    << std::endl;
            }
            execute_struct_array_literal_init(interpreter, node->name,
                                              node->init_expr.get(),
                                              node->type_name);
        } else if (node->init_expr->node_type == ASTNodeType::AST_FUNC_CALL) {
            // 配列を返す関数呼び出し
            if (debug_mode) {
                std::cerr << "[DEBUG_ARRAY_DECL] Function call initialization "
                             "for array: "
                          << node->name << std::endl;
                std::cerr
                    << "[DEBUG_ARRAY_DECL] var.is_assigned before evaluate: "
                    << var.is_assigned << std::endl;
            }

            // processArrayDeclarationが既に配列を初期化している場合はスキップ
            // （動的配列の場合、processArrayDeclaration内で関数が既に呼ばれている）
            if (!var.is_assigned) {
                if (debug_mode) {
                    std::cerr
                        << "[DEBUG_ARRAY_DECL] Variable not yet initialized, "
                           "calling evaluate"
                        << std::endl;
                }
                // NOTE: This path should not be reached for dynamic arrays,
                // as processArrayDeclaration already handles function calls.
                // However, we keep this as a safety fallback.
                try {
                    interpreter.evaluate(node->init_expr.get());
                } catch (const ReturnException &) {
                    // Rethrow to be handled by outer scope if needed
                    throw;
                }
            } else {
                if (debug_mode) {
                    std::cerr
                        << "[DEBUG_ARRAY_DECL] Variable already initialized by "
                           "processArrayDeclaration, skipping evaluate"
                        << std::endl;
                }
            }
        }
        // 他の配列初期化は既存の処理で対応
    }
}

void execute_struct_array_literal_init(Interpreter &interpreter,
                                       const std::string &array_name,
                                       const ASTNode *array_literal,
                                       const std::string &struct_type) {
    if (!array_literal ||
        array_literal->node_type != ASTNodeType::AST_ARRAY_LITERAL) {
        throw std::runtime_error(
            "Invalid array literal for struct array initialization");
    }

    // 各配列要素（struct literal）を処理
    for (size_t i = 0; i < array_literal->arguments.size(); i++) {
        const ASTNode *struct_literal = array_literal->arguments[i].get();
        if (struct_literal->node_type != ASTNodeType::AST_STRUCT_LITERAL) {
            throw std::runtime_error(
                "Expected struct literal in struct array initialization");
        }

        std::string element_name = array_name + "[" + std::to_string(i) + "]";
        interpreter.assign_struct_literal(element_name, struct_literal);
    }
}

} // namespace DeclarationHandlers
