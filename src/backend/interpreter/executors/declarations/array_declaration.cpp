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

    // struct配列の場合、要素変数を初期化
    if (debug_mode) {
        std::cerr << "[DEBUG_EXEC_ARRAY] After registration: var.is_struct="
                  << var.is_struct << ", var.is_array=" << var.is_array
                  << ", var.array_size=" << var.array_size
                  << ", struct_type_name=" << var.struct_type_name << std::endl;
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
            // 配列を返す関数呼び出し: double[2][3] arr = make_array();
            if (debug_mode) {
                std::cerr << "[DEBUG_ARRAY_DECL] Function call initialization "
                             "for array: "
                          << node->name << std::endl;
            }
            try {
                int64_t value = interpreter.evaluate(node->init_expr.get());
                if (debug_mode) {
                    std::cerr << "[DEBUG_ARRAY_DECL] evaluate() returned "
                                 "normally, value="
                              << value << std::endl;
                }
                // void関数の場合
                interpreter.current_scope().variables[node->name].value = value;
                interpreter.current_scope().variables[node->name].is_assigned =
                    true;
            } catch (const ReturnException &ret) {
                if (debug_mode) {
                    std::cerr << "[DEBUG_ARRAY_DECL] Caught ReturnException: "
                                 "is_array="
                              << ret.is_array << ", is_struct=" << ret.is_struct
                              << std::endl;
                }
                if (ret.is_array) {
                    // 配列戻り値の場合
                    Variable &target_var =
                        interpreter.current_scope().variables[node->name];

                    if (TypeHelpers::isString(ret.type)) {
                        // 文字列配列
                        if (!ret.str_array_3d.empty()) {
                            bool is_multidim =
                                (ret.array_type_name.find("[][]") !=
                                 std::string::npos);
                            if (is_multidim) {
                                target_var.array_strings.clear();
                                for (const auto &plane : ret.str_array_3d) {
                                    for (const auto &row : plane) {
                                        for (const auto &element : row) {
                                            target_var.array_strings.push_back(
                                                element);
                                        }
                                    }
                                }
                                target_var.array_size =
                                    target_var.array_strings.size();
                            } else if (!ret.str_array_3d[0].empty() &&
                                       !ret.str_array_3d[0][0].empty()) {
                                target_var.array_strings =
                                    ret.str_array_3d[0][0];
                                target_var.array_size =
                                    target_var.array_strings.size();
                            }
                            target_var.type = static_cast<TypeInfo>(
                                TYPE_ARRAY_BASE + TYPE_STRING);
                        }
                    } else if (ret.type == TYPE_FLOAT ||
                               ret.type == TYPE_DOUBLE ||
                               ret.type == TYPE_QUAD) {
                        // float/double/quad配列
                        if (!ret.double_array_3d.empty()) {
                            bool is_multidim =
                                (ret.array_type_name.find("[][]") !=
                                     std::string::npos ||
                                 ret.double_array_3d.size() > 1 ||
                                 (ret.double_array_3d.size() == 1 &&
                                  ret.double_array_3d[0].size() > 1));
                            if (is_multidim) {
                                // 多次元float/double配列の場合
                                if (ret.type == TYPE_FLOAT) {
                                    target_var.multidim_array_float_values
                                        .clear();
                                    for (const auto &plane :
                                         ret.double_array_3d) {
                                        for (const auto &row : plane) {
                                            for (const auto &element : row) {
                                                target_var
                                                    .multidim_array_float_values
                                                    .push_back(
                                                        static_cast<float>(
                                                            element));
                                            }
                                        }
                                    }
                                    target_var.array_size =
                                        target_var.multidim_array_float_values
                                            .size();
                                } else if (ret.type == TYPE_DOUBLE) {
                                    target_var.multidim_array_double_values
                                        .clear();
                                    for (const auto &plane :
                                         ret.double_array_3d) {
                                        for (const auto &row : plane) {
                                            for (const auto &element : row) {
                                                target_var
                                                    .multidim_array_double_values
                                                    .push_back(element);
                                            }
                                        }
                                    }
                                    target_var.array_size =
                                        target_var.multidim_array_double_values
                                            .size();
                                } else { // TYPE_QUAD
                                    target_var.multidim_array_quad_values
                                        .clear();
                                    for (const auto &plane :
                                         ret.double_array_3d) {
                                        for (const auto &row : plane) {
                                            for (const auto &element : row) {
                                                target_var
                                                    .multidim_array_quad_values
                                                    .push_back(static_cast<
                                                               long double>(
                                                        element));
                                            }
                                        }
                                    }
                                    target_var.array_size =
                                        target_var.multidim_array_quad_values
                                            .size();
                                }
                                target_var.is_multidimensional = true;
                                target_var.array_values.clear();

                                // 2次元配列の次元情報を設定
                                if (!ret.double_array_3d.empty() &&
                                    !ret.double_array_3d[0].empty()) {
                                    target_var.array_dimensions.clear();
                                    target_var.array_dimensions.push_back(
                                        ret.double_array_3d[0].size());
                                    target_var.array_dimensions.push_back(
                                        ret.double_array_3d[0][0].size());
                                }
                            } else if (!ret.double_array_3d[0].empty() &&
                                       !ret.double_array_3d[0][0].empty()) {
                                // 1次元float/double配列の場合
                                if (ret.type == TYPE_FLOAT) {
                                    target_var.array_float_values.clear();
                                    for (const auto &element :
                                         ret.double_array_3d[0][0]) {
                                        target_var.array_float_values.push_back(
                                            static_cast<float>(element));
                                    }
                                    target_var.array_size =
                                        target_var.array_float_values.size();
                                } else if (ret.type == TYPE_DOUBLE) {
                                    target_var.array_double_values.clear();
                                    for (const auto &element :
                                         ret.double_array_3d[0][0]) {
                                        target_var.array_double_values
                                            .push_back(element);
                                    }
                                    target_var.array_size =
                                        target_var.array_double_values.size();
                                } else { // TYPE_QUAD
                                    target_var.array_quad_values.clear();
                                    for (const auto &element :
                                         ret.double_array_3d[0][0]) {
                                        target_var.array_quad_values.push_back(
                                            static_cast<long double>(element));
                                    }
                                    target_var.array_size =
                                        target_var.array_quad_values.size();
                                }
                            }
                            target_var.type = static_cast<TypeInfo>(
                                TYPE_ARRAY_BASE + ret.type);
                        }
                    } else if (ret.is_struct) {
                        // 構造体配列の場合
                        if (debug_mode) {
                            std::cerr << "[DEBUG_ARRAY_DECL] Struct array "
                                         "return caught, calling "
                                         "assign_array_from_return for "
                                      << node->name << std::endl;
                        }
                        interpreter.assign_array_from_return(node->name, ret);
                    } else {
                        // 整数型配列
                        if (!ret.int_array_3d.empty()) {
                            bool is_multidim =
                                (ret.array_type_name.find("[][]") !=
                                     std::string::npos ||
                                 ret.int_array_3d.size() > 1 ||
                                 (ret.int_array_3d.size() == 1 &&
                                  ret.int_array_3d[0].size() > 1));
                            if (is_multidim) {
                                target_var.multidim_array_values.clear();
                                for (const auto &plane : ret.int_array_3d) {
                                    for (const auto &row : plane) {
                                        for (const auto &element : row) {
                                            target_var.multidim_array_values
                                                .push_back(element);
                                        }
                                    }
                                }
                                target_var.array_size =
                                    target_var.multidim_array_values.size();
                                target_var.is_multidimensional = true;
                                target_var.array_values.clear();

                                if (!ret.int_array_3d.empty() &&
                                    !ret.int_array_3d[0].empty()) {
                                    target_var.array_dimensions.clear();
                                    target_var.array_dimensions.push_back(
                                        ret.int_array_3d[0].size());
                                    target_var.array_dimensions.push_back(
                                        ret.int_array_3d[0][0].size());
                                }
                            } else if (!ret.int_array_3d[0].empty() &&
                                       !ret.int_array_3d[0][0].empty()) {
                                target_var.array_values =
                                    ret.int_array_3d[0][0];
                                target_var.array_size =
                                    target_var.array_values.size();
                            }
                            target_var.type = static_cast<TypeInfo>(
                                TYPE_ARRAY_BASE + ret.type);
                        }
                    }
                    target_var.is_assigned = true;
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
