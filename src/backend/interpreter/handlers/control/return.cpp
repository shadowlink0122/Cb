#include "return.h"
#include "../../../../common/ast.h"
#include "../../../../common/debug.h"
#include "../../../../common/debug_messages.h"
#include "../../core/interpreter.h"
#include "../../core/type_inference.h"
#include "../../evaluator/core/evaluator.h"
#include <stdexcept>

// return文の実行
void ReturnHandler::execute_return_statement(const ASTNode *node) {
    debug_msg(DebugMsgId::INTERPRETER_RETURN_STMT);

    // return実行前にdefer/デストラクタを実行
    // これにより、現在のスコープで登録されたクリーンアップ処理が実行される
    interpreter_->execute_pre_return_cleanup();

    if (!node->left) {
        // return値なし（void関数のreturn）
        // ReturnExceptionを投げて関数から抜ける
        throw ReturnException(static_cast<int64_t>(0)); // voidの場合は0を返す
    }

    debug_msg(DebugMsgId::INTERPRETER_RETURN_STMT);

    // ノードタイプに応じて処理を分岐
    switch (node->left->node_type) {
    case ASTNodeType::AST_ARRAY_LITERAL:
        handle_array_literal_return(node);
        break;

    case ASTNodeType::AST_STRING_LITERAL:
        throw ReturnException(node->left->str_value);
        break;

    case ASTNodeType::AST_IDENTIFIER:
        handle_identifier_return(node);
        break;

    case ASTNodeType::AST_VARIABLE:
        handle_variable_return(node);
        break;

    default:
        // その他の式（メンバーアクセス、関数呼び出し、算術式など）
        handle_expression_return(node);
        break;
    }
}

// 配列リテラルのreturn処理
void ReturnHandler::handle_array_literal_return(const ASTNode *node) {
    const std::vector<std::unique_ptr<ASTNode>> &elements =
        node->left->arguments;
    debug_msg(DebugMsgId::INTERPRETER_RETURN_ARRAY, elements.size());

    // 配列リテラルを処理
    std::vector<int64_t> array_values;
    std::vector<std::string> array_strings;
    bool is_string_array = false;

    // 最初の要素で型を判定
    if (!elements.empty()) {
        if (elements[0]->node_type == ASTNodeType::AST_STRING_LITERAL) {
            is_string_array = true;
        } else if (elements[0]->node_type == ASTNodeType::AST_ARRAY_LITERAL) {
            // 多次元配列リテラルの場合
            const auto &nested_elements = elements[0]->arguments;
            if (!nested_elements.empty() &&
                nested_elements[0]->node_type ==
                    ASTNodeType::AST_STRING_LITERAL) {
                is_string_array = true;
            }
        }
    }

    // 多次元配列リテラルの場合の特別処理
    if (!elements.empty() &&
        elements[0]->node_type == ASTNodeType::AST_ARRAY_LITERAL) {

        if (is_string_array) {
            // 多次元文字列配列を3D形式に変換
            std::vector<std::vector<std::vector<std::string>>> str_array_3d;
            std::vector<std::vector<std::string>> str_array_2d;

            for (const auto &row_element : elements) {
                if (row_element->node_type != ASTNodeType::AST_ARRAY_LITERAL) {
                    throw std::runtime_error("Expected nested array literal");
                }

                std::vector<std::string> row;
                for (const auto &cell_element : row_element->arguments) {
                    if (cell_element->node_type !=
                        ASTNodeType::AST_STRING_LITERAL) {
                        throw std::runtime_error(
                            "Expected string literal in multidim array");
                    }
                    row.push_back(cell_element->str_value);
                }
                str_array_2d.push_back(row);
            }
            str_array_3d.push_back(str_array_2d);

            throw ReturnException(
                str_array_3d, "string[][]",
                static_cast<TypeInfo>(TYPE_ARRAY_BASE + TYPE_STRING));
        } else {
            // 多次元整数配列を3D形式に変換
            std::vector<std::vector<std::vector<int64_t>>> int_array_3d;
            std::vector<std::vector<int64_t>> int_array_2d;

            for (const auto &row_element : elements) {
                if (row_element->node_type != ASTNodeType::AST_ARRAY_LITERAL) {
                    throw std::runtime_error("Expected nested array literal");
                }

                std::vector<int64_t> row;
                for (const auto &cell_element : row_element->arguments) {
                    int64_t value =
                        interpreter_->expression_evaluator_
                            ->evaluate_expression(cell_element.get());
                    row.push_back(value);
                }
                int_array_2d.push_back(row);
            }
            int_array_3d.push_back(int_array_2d);

            throw ReturnException(int_array_3d, "int[][]", TYPE_INT);
        }
    }

    // 全要素を評価
    for (size_t i = 0; i < elements.size(); i++) {
        const auto &element = elements[i];
        if (is_string_array) {
            if (element->node_type != ASTNodeType::AST_STRING_LITERAL) {
                throw std::runtime_error(
                    "Type mismatch in array literal return: expected string");
            }
            array_strings.push_back(element->str_value);
        } else {
            if (element->node_type == ASTNodeType::AST_STRING_LITERAL) {
                throw std::runtime_error(
                    "Type mismatch in array literal return: expected number");
            }
            int64_t value =
                interpreter_->expression_evaluator_->evaluate_expression(
                    element.get());
            array_values.push_back(value);
        }
    }

    // ReturnExceptionで配列を返す
    if (is_string_array) {
        std::vector<std::vector<std::vector<std::string>>> str_array_3d;
        std::vector<std::vector<std::string>> str_array_2d;
        str_array_2d.push_back(array_strings);
        str_array_3d.push_back(str_array_2d);
        throw ReturnException(str_array_3d, "string[]", TYPE_STRING);
    } else {
        std::vector<std::vector<std::vector<int64_t>>> int_array_3d;
        std::vector<std::vector<int64_t>> int_array_2d;
        int_array_2d.push_back(array_values);
        int_array_3d.push_back(int_array_2d);
        throw ReturnException(int_array_3d, "int[]", TYPE_INT);
    }
}

// 識別子のreturn処理（変数、self等）
void ReturnHandler::handle_identifier_return(const ASTNode *node) {
    // この部分は非常に大きいので、interpreter側で実装を続ける
    // 将来的にはこちらに完全移行すべき

    // 今回は単純な委譲に留める
    if (node->left->name == "self") {
        Variable *self_var = interpreter_->find_variable("self");
        if (self_var && self_var->is_struct) {
            interpreter_->sync_struct_members_from_direct_access("self");
            if (self_var->type != TYPE_INTERFACE) {
                self_var->type = TYPE_STRUCT;
            }
            throw ReturnException(*self_var);
        }
    } else {
        Variable *var = interpreter_->find_variable(node->left->name);
        if (var) {
            // 参照型チェック
            bool return_as_reference = false;
            if (!interpreter_->current_function_name.empty()) {
                const ASTNode *func =
                    interpreter_->global_scope
                        .functions[interpreter_->current_function_name];
                if (func &&
                    func->return_type_name.find('&') != std::string::npos) {
                    return_as_reference = true;
                }
            }

            if (return_as_reference) {
                if (var->is_reference) {
                    Variable *target_var =
                        reinterpret_cast<Variable *>(var->value);
                    throw ReturnException(target_var);
                } else {
                    throw ReturnException(var);
                }
            } else if (var->is_array) {
                // 配列の場合、handle_array_variable_returnに委譲
                handle_array_variable_return(node, var);
                return;
            } else if (var->is_struct) {
                interpreter_->sync_struct_members_from_direct_access(
                    node->left->name);
                if (var->type != TYPE_INTERFACE) {
                    var->type = TYPE_STRUCT;
                }
                throw ReturnException(*var);
            } else if (!var->interface_name.empty()) {
                Variable interface_copy = *var;
                interface_copy.type = TYPE_INTERFACE;
                throw ReturnException(interface_copy);
            } else if (var->type == TYPE_STRING) {
                throw ReturnException(var->str_value);
            } else if (var->type == TYPE_POINTER) {
                // ポインタ戻り値の場合、const情報を保持する（Phase 2: v0.9.2）
                ReturnException ret(var->value);
                ret.type = TYPE_POINTER;
                ret.is_pointer = true;
                ret.is_pointee_const = var->is_pointee_const;
                ret.is_pointer_const = var->is_pointer_const;
                ret.pointer_depth = var->pointer_depth;
                ret.pointer_base_type = var->pointer_base_type;
                ret.pointer_base_type_name = var->pointer_base_type_name;
                throw ret;
            } else {
                throw ReturnException(var->value);
            }
        } else {
            // 変数が見つからない場合、式として評価
            handle_expression_return(node);
        }
    }
}

// 変数のreturn処理
void ReturnHandler::handle_variable_return(const ASTNode *node) {
    debug_msg(DebugMsgId::INTERPRETER_RETURN_VAR, node->left->name.c_str());
    Variable *var = interpreter_->find_variable(node->left->name);

    // 現在の関数の戻り値型が参照型かチェック
    bool return_as_reference = false;
    if (!interpreter_->current_function_name.empty()) {
        const ASTNode *func =
            interpreter_->global_scope
                .functions[interpreter_->current_function_name];
        if (func && func->return_type_name.find('&') != std::string::npos) {
            return_as_reference = true;
        }
    }

    if (return_as_reference && var) {
        if (var->is_reference) {
            Variable *target_var = reinterpret_cast<Variable *>(var->value);
            throw ReturnException(target_var);
        } else {
            throw ReturnException(var);
        }
    }

    if (var && var->is_struct) {
        interpreter_->sync_struct_members_from_direct_access(node->left->name);
        if (var->type != TYPE_INTERFACE) {
            var->type = TYPE_STRUCT;
        }

        if (var->is_array) {
            handle_array_variable_return(node, var);
            return;
        }

        debug_msg(DebugMsgId::INTERPRETER_RETURN_ARRAY_VAR);
        throw ReturnException(*var);
    } else if (var && !var->interface_name.empty()) {
        Variable interface_copy = *var;
        interface_copy.type = TYPE_INTERFACE;
        throw ReturnException(interface_copy);
    } else if (var && var->is_array) {
        handle_array_variable_return(node, var);
        return;
    }

    // 非配列変数の処理
    if (var && var->is_struct) {
        interpreter_->sync_struct_members_from_direct_access(node->left->name);
        throw ReturnException(*var);
    } else if (var && (var->type == TYPE_STRING ||
                       (var->is_assigned && !var->str_value.empty()))) {
        throw ReturnException(var->str_value);
    } else if (var && var->type == TYPE_POINTER) {
        // ポインタ戻り値の場合、const情報を保持する（Phase 2: v0.9.2）
        ReturnException ret(var->value);
        ret.type = TYPE_POINTER;
        ret.is_pointer = true;
        ret.is_pointee_const = var->is_pointee_const;
        ret.is_pointer_const = var->is_pointer_const;
        ret.pointer_depth = var->pointer_depth;
        ret.pointer_base_type = var->pointer_base_type;
        ret.pointer_base_type_name = var->pointer_base_type_name;
        throw ret;
    } else if (var) {
        // 数値変数を型推論で正しく返す
        TypedValue typed_result =
            interpreter_->expression_evaluator_->evaluate_typed_expression(
                node->left.get());
        if (typed_result.numeric_type == TYPE_FLOAT) {
            throw ReturnException(typed_result.double_value, TYPE_FLOAT);
        } else if (typed_result.numeric_type == TYPE_DOUBLE) {
            throw ReturnException(typed_result.double_value, TYPE_DOUBLE);
        } else if (typed_result.numeric_type == TYPE_QUAD) {
            throw ReturnException(typed_result.quad_value, TYPE_QUAD);
        } else {
            throw ReturnException(typed_result.value,
                                  typed_result.numeric_type);
        }
    } else {
        // 変数が見つからない場合
        debug_msg(DebugMsgId::INTERPRETER_VAR_NOT_FOUND,
                  node->left->name.c_str());
        TypedValue typed_result =
            interpreter_->expression_evaluator_->evaluate_typed_expression(
                node->left.get());
        if (typed_result.numeric_type == TYPE_FLOAT) {
            throw ReturnException(typed_result.double_value, TYPE_FLOAT);
        } else if (typed_result.numeric_type == TYPE_DOUBLE) {
            throw ReturnException(typed_result.double_value, TYPE_DOUBLE);
        } else if (typed_result.numeric_type == TYPE_QUAD) {
            throw ReturnException(typed_result.quad_value, TYPE_QUAD);
        } else {
            throw ReturnException(typed_result.value,
                                  typed_result.numeric_type);
        }
    }
}

// 配列変数のreturn処理
void ReturnHandler::handle_array_variable_return(const ASTNode *node,
                                                 Variable *var) {
    debug_msg(DebugMsgId::INTERPRETER_RETURN_ARRAY_VAR,
              node->left->name.c_str());

    // 多次元配列の処理
    if (var->is_multidimensional) {
        debug_msg(DebugMsgId::INTERPRETER_MULTIDIM_PROCESSING);
        TypeInfo type_info = var->type;
        TypeInfo base_type =
            (type_info >= TYPE_ARRAY_BASE)
                ? static_cast<TypeInfo>(type_info - TYPE_ARRAY_BASE)
                : type_info;

        // 文字列配列
        if (base_type == TYPE_STRING || base_type == TYPE_CHAR) {
            std::vector<std::vector<std::vector<std::string>>> str_array_3d;
            if (var->array_dimensions.size() == 2) {
                int rows = var->array_dimensions[0];
                int cols = var->array_dimensions[1];
                std::vector<std::vector<std::string>> str_array_2d;
                for (int i = 0; i < rows; i++) {
                    std::vector<std::string> row;
                    for (int j = 0; j < cols; j++) {
                        int flat_index = i * cols + j;
                        if (flat_index < var->multidim_array_strings.size()) {
                            row.push_back(
                                var->multidim_array_strings[flat_index]);
                        } else {
                            row.push_back("");
                        }
                    }
                    str_array_2d.push_back(row);
                }
                str_array_3d.push_back(str_array_2d);
            } else {
                std::vector<std::vector<std::string>> str_array_2d;
                std::vector<std::string> str_array_1d;
                for (size_t i = 0; i < var->multidim_array_strings.size();
                     ++i) {
                    str_array_1d.push_back(var->multidim_array_strings[i]);
                }
                str_array_2d.push_back(str_array_1d);
                str_array_3d.push_back(str_array_2d);
            }
            throw ReturnException(str_array_3d, node->left->name, var->type);
        }

        // float/double/quad配列
        if (base_type == TYPE_FLOAT || base_type == TYPE_DOUBLE ||
            base_type == TYPE_QUAD) {
            std::vector<std::vector<std::vector<double>>> double_array_3d;
            if (var->array_dimensions.size() == 2) {
                int rows = var->array_dimensions[0];
                int cols = var->array_dimensions[1];
                std::vector<std::vector<double>> double_array_2d;
                for (int i = 0; i < rows; i++) {
                    std::vector<double> row;
                    for (int j = 0; j < cols; j++) {
                        int flat_index = i * cols + j;
                        if (base_type == TYPE_FLOAT &&
                            flat_index <
                                static_cast<int>(
                                    var->multidim_array_float_values.size())) {
                            row.push_back(static_cast<double>(
                                var->multidim_array_float_values[flat_index]));
                        } else if (base_type == TYPE_DOUBLE &&
                                   flat_index <
                                       static_cast<int>(
                                           var->multidim_array_double_values
                                               .size())) {
                            row.push_back(
                                var->multidim_array_double_values[flat_index]);
                        } else if (base_type == TYPE_QUAD &&
                                   flat_index <
                                       static_cast<int>(
                                           var->multidim_array_quad_values
                                               .size())) {
                            row.push_back(static_cast<double>(
                                var->multidim_array_quad_values[flat_index]));
                        } else {
                            row.push_back(0.0);
                        }
                    }
                    double_array_2d.push_back(row);
                }
                double_array_3d.push_back(double_array_2d);
            }
            throw ReturnException(double_array_3d, node->left->name, base_type);
        }

        // 整数型配列
        std::vector<std::vector<std::vector<int64_t>>> int_array_3d;
        if (var->array_dimensions.size() == 2) {
            int rows = var->array_dimensions[0];
            int cols = var->array_dimensions[1];
            std::vector<std::vector<int64_t>> int_array_2d;
            for (int i = 0; i < rows; i++) {
                std::vector<int64_t> row;
                for (int j = 0; j < cols; j++) {
                    int flat_index = i * cols + j;
                    if (flat_index < var->multidim_array_values.size()) {
                        row.push_back(var->multidim_array_values[flat_index]);
                    } else {
                        row.push_back(0);
                    }
                }
                int_array_2d.push_back(row);
            }
            int_array_3d.push_back(int_array_2d);
        } else {
            std::vector<std::vector<int64_t>> int_array_2d;
            std::vector<int64_t> int_array_1d;
            for (size_t i = 0; i < var->multidim_array_values.size(); ++i) {
                int_array_1d.push_back(var->multidim_array_values[i]);
            }
            int_array_2d.push_back(int_array_1d);
            int_array_3d.push_back(int_array_2d);
        }
        throw ReturnException(int_array_3d, node->left->name, var->type);
    }

    // 1次元配列の処理
    TypeInfo type_info = var->type;
    TypeInfo base_type =
        (type_info >= TYPE_ARRAY_BASE)
            ? static_cast<TypeInfo>(type_info - TYPE_ARRAY_BASE)
            : type_info;

    // 構造体配列
    bool is_struct_array = var->is_struct || base_type == TYPE_STRUCT;

    if (is_struct_array) {
        if (debug_mode) {
            std::cerr << "[DEBUG_RETURN] Throwing struct array ReturnException"
                      << std::endl;
            std::cerr << "[DEBUG_RETURN] var->array_size=" << var->array_size
                      << std::endl;
        }
        std::vector<std::vector<std::vector<Variable>>> struct_array_3d;
        std::vector<std::vector<Variable>> struct_array_2d;
        std::vector<Variable> struct_array_1d;

        // array_sizeが0の場合、実際の要素数を数える（パーサーのバグ対策）
        int actual_size = var->array_size;
        if (actual_size == 0) {
            // 実際に存在する配列要素を探す
            for (int i = 0; i < 100; ++i) { // 最大100要素まで探す
                std::string element_name =
                    node->left->name + "[" + std::to_string(i) + "]";
                Variable *element_var =
                    interpreter_->find_variable(element_name);
                if (element_var && element_var->is_struct) {
                    actual_size = i + 1;
                } else {
                    break;
                }
            }
            if (debug_mode) {
                std::cerr << "[DEBUG_RETURN] Actual array size found: "
                          << actual_size << std::endl;
            }
        }

        for (int i = 0; i < actual_size; ++i) {
            std::string element_name =
                node->left->name + "[" + std::to_string(i) + "]";
            Variable *element_var = interpreter_->find_variable(element_name);

            if (element_var && element_var->is_struct) {
                // 配列要素に対する個別メンバー変数と構造体本体を同期
                interpreter_->sync_struct_members_from_direct_access(
                    element_name);

                Variable struct_element = *element_var;
                struct_element.is_struct = true;
                struct_element.type = TYPE_STRUCT;
                if (struct_element.struct_type_name.empty()) {
                    struct_element.struct_type_name =
                        element_var->struct_type_name;
                }
                struct_array_1d.push_back(struct_element);
            } else {
                Variable empty_struct;
                empty_struct.type = TYPE_STRUCT;
                empty_struct.is_struct = true;
                empty_struct.struct_type_name = element_name;
                struct_array_1d.push_back(empty_struct);
            }
        }

        struct_array_2d.push_back(struct_array_1d);
        struct_array_3d.push_back(struct_array_2d);

        std::string struct_type_name =
            var->type_name.empty() ? node->left->name : var->type_name;
        throw ReturnException(struct_array_3d, struct_type_name);
    }

    // float/double/quad配列
    if (base_type == TYPE_FLOAT || base_type == TYPE_DOUBLE ||
        base_type == TYPE_QUAD) {
        std::vector<std::vector<std::vector<double>>> double_array_3d;
        std::vector<std::vector<double>> double_array_2d;
        std::vector<double> double_array_1d;

        if (base_type == TYPE_FLOAT) {
            for (size_t i = 0; i < var->array_float_values.size(); ++i) {
                double_array_1d.push_back(
                    static_cast<double>(var->array_float_values[i]));
            }
        } else if (base_type == TYPE_DOUBLE) {
            for (size_t i = 0; i < var->array_double_values.size(); ++i) {
                double_array_1d.push_back(var->array_double_values[i]);
            }
        } else { // TYPE_QUAD
            for (size_t i = 0; i < var->array_quad_values.size(); ++i) {
                double_array_1d.push_back(
                    static_cast<double>(var->array_quad_values[i]));
            }
        }
        double_array_2d.push_back(double_array_1d);
        double_array_3d.push_back(double_array_2d);

        throw ReturnException(double_array_3d, node->left->name, base_type);
    }

    // 整数型配列
    if (type_info == static_cast<TypeInfo>(TYPE_ARRAY_BASE + TYPE_INT) ||
        type_info == static_cast<TypeInfo>(TYPE_ARRAY_BASE + TYPE_LONG) ||
        type_info == static_cast<TypeInfo>(TYPE_ARRAY_BASE + TYPE_SHORT) ||
        type_info == static_cast<TypeInfo>(TYPE_ARRAY_BASE + TYPE_TINY) ||
        type_info == static_cast<TypeInfo>(TYPE_ARRAY_BASE + TYPE_BOOL)) {
        std::vector<std::vector<std::vector<int64_t>>> int_array_3d;
        std::vector<std::vector<int64_t>> int_array_2d;
        std::vector<int64_t> int_array_1d;

        for (size_t i = 0; i < var->array_values.size(); ++i) {
            int_array_1d.push_back(var->array_values[i]);
        }
        int_array_2d.push_back(int_array_1d);
        int_array_3d.push_back(int_array_2d);

        throw ReturnException(int_array_3d, node->left->name, type_info);
    }

    // 文字列配列
    if (type_info == static_cast<TypeInfo>(TYPE_ARRAY_BASE + TYPE_STRING) ||
        type_info == static_cast<TypeInfo>(TYPE_ARRAY_BASE + TYPE_CHAR)) {
        std::vector<std::vector<std::vector<std::string>>> str_array_3d;
        std::vector<std::vector<std::string>> str_array_2d;
        std::vector<std::string> str_array_1d;

        for (size_t i = 0; i < var->array_strings.size(); ++i) {
            str_array_1d.push_back(var->array_strings[i]);
        }
        str_array_2d.push_back(str_array_1d);
        str_array_3d.push_back(str_array_2d);

        throw ReturnException(str_array_3d, node->left->name, type_info);
    }
}

// メンバーアクセスのreturn処理（スタブ実装）
void ReturnHandler::handle_member_access_return(const ASTNode *node) {
    // 式評価にフォールバック
    handle_expression_return(node);
}

// 式のreturn処理（デフォルト）
void ReturnHandler::handle_expression_return(const ASTNode *node) {
    // まず、式を評価してReturnExceptionをキャッチする（関数呼び出しの場合）
    try {
        interpreter_->expression_evaluator_->evaluate_expression(
            node->left.get());
    } catch (const ReturnException &ret_ex) {
        // 関数呼び出しの結果がReturnExceptionとして返ってきた場合、そのまま再スロー
        throw ret_ex;
    }

    // それ以外の場合、型推論を使用して正しい型で返す
    TypedValue typed_result =
        interpreter_->expression_evaluator_->evaluate_typed_expression(
            node->left.get());

    if (typed_result.is_function_pointer) {
        throw ReturnException(
            typed_result.value, typed_result.function_pointer_name,
            typed_result.function_pointer_node, typed_result.numeric_type);
    } else if (typed_result.is_struct_result) {
        // 構造体の場合、再度評価してReturnExceptionを取得
        try {
            interpreter_->expression_evaluator_->evaluate_expression(
                node->left.get());
            throw std::runtime_error(
                "Struct evaluation did not throw ReturnException");
        } catch (const ReturnException &ret_ex) {
            throw ret_ex;
        }
    } else if (typed_result.is_string()) {
        throw ReturnException(typed_result.string_value);
    } else {
        // 数値の場合、型情報を保持
        if (typed_result.numeric_type == TYPE_FLOAT) {
            throw ReturnException(typed_result.double_value, TYPE_FLOAT);
        } else if (typed_result.numeric_type == TYPE_DOUBLE) {
            throw ReturnException(typed_result.double_value, TYPE_DOUBLE);
        } else if (typed_result.numeric_type == TYPE_QUAD) {
            throw ReturnException(typed_result.quad_value, TYPE_QUAD);
        } else {
            throw ReturnException(typed_result.value,
                                  typed_result.numeric_type);
        }
    }
}
