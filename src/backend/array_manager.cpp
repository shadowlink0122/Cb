#include "array_manager.h"
#include "../backend/interpreter.h" // Variable の定義のため
#include "../common/debug_messages.h"
#include "error_handler.h"
#include "evaluator/expression_evaluator.h"
#include "variable_manager.h"
#include <stdexcept>

void ArrayManager::processArrayDeclaration(Variable &var, const ASTNode *node) {
    if (!node) {
        throw std::runtime_error(
            "ArrayManager::processArrayDeclaration: node is null");
    }
    if (!expression_evaluator_) {
        throw std::runtime_error("ArrayManager::processArrayDeclaration: "
                                 "expression_evaluator_ is null");
    }

    debug_msg(DebugMsgId::ARRAY_DECL_DEBUG, node->name.c_str());
    debug_msg(DebugMsgId::ARRAY_DIMENSIONS_COUNT,
              node->array_dimensions.size());

    var.type = static_cast<TypeInfo>(TYPE_ARRAY_BASE + node->type_info);
    var.is_const = node->is_const;
    var.is_array = true;
    var.is_assigned = false;

    // 多次元配列かどうかチェック
    if (node->array_dimensions.size() > 1) {
        debug_msg(DebugMsgId::MULTIDIM_ARRAY_PROCESSING);
        // 多次元配列の場合
        std::vector<ArrayDimension> dimensions;
        for (const auto &dim_expr : node->array_dimensions) {
            if (dim_expr.get() == nullptr) {
                error_msg(DebugMsgId::DYNAMIC_ARRAY_NOT_SUPPORTED,
                          node->name.c_str());
                throw std::runtime_error(
                    "Dynamic arrays are not supported yet");
            }
            if (!expression_evaluator_) {
                throw std::runtime_error(
                    "ExpressionEvaluator is null during dimension evaluation");
            }
            int dim_size = static_cast<int>(
                expression_evaluator_->evaluate_expression(dim_expr.get()));
            var.array_dimensions.push_back(dim_size);
            dimensions.push_back(ArrayDimension(dim_size, false));
        }

        // ArrayTypeInfoを作成
        var.array_type_info = ArrayTypeInfo(node->type_info, dimensions);
        var.is_multidimensional = true;

        // 総要素数を計算
        int total_size = 1;
        for (int dim : var.array_dimensions) {
            total_size *= dim;
        }
        var.array_size = total_size;

        // 要素の型
        TypeInfo elem_type = node->type_info;
        if (elem_type == TYPE_STRING) {
            var.multidim_array_strings.resize(total_size, "");
        } else {
            var.multidim_array_values.resize(total_size, 0);
        }
    } else {
        // 1次元配列
        if (node->array_dimensions.size() == 1) {
            if (node->array_dimensions[0].get() == nullptr) {
                // 動的配列（サイズ指定なし）は初期化がない場合のみ禁止
                if (!node->init_expr) {
                    error_msg(DebugMsgId::DYNAMIC_ARRAY_NOT_SUPPORTED,
                              node->name.c_str());
                    throw std::runtime_error(
                        "Dynamic arrays are not supported yet");
                }
                // 関数呼び出しまたは配列リテラルで初期化される場合は許可
                var.array_size = 0; // 動的配列のサイズは初期化時に決定
            } else {
                // サイズ計算はExpressionEvaluatorを使用
                int size =
                    static_cast<int>(expression_evaluator_->evaluate_expression(
                        node->array_dimensions[0].get()));
                var.array_size = size;

                // 1次元配列でもarray_dimensionsを設定
                var.array_dimensions.push_back(size);

                // 配列要素を初期化
                if (node->type_info == TYPE_STRING) {
                    var.array_strings.resize(size, "");
                } else {
                    var.array_values.resize(size, 0);
                }
            }
        } else if (node->array_size_expr) {
            // array_size_expr が設定されている場合（create_array_init_with_size
            // から）
            int size =
                static_cast<int>(expression_evaluator_->evaluate_expression(
                    node->array_size_expr.get()));
            var.array_size = size;

            // 1次元配列でもarray_dimensionsを設定
            var.array_dimensions.push_back(size);

            // 配列要素を初期化
            if (node->type_info == TYPE_STRING) {
                var.array_strings.resize(size, "");
            } else {
                var.array_values.resize(size, 0);
            }
        }
    }

    // 配列初期化がある場合
    if (node->init_expr) {
        if (node->init_expr->node_type == ASTNodeType::AST_ARRAY_LITERAL) {
            // 配列リテラルで初期化
            const ASTNode *array_literal = node->init_expr.get();
            debug_msg(DebugMsgId::ARRAY_DECL_DEBUG,
                      ("Array literal found with " +
                       std::to_string(array_literal->children.size()) +
                       " children, " +
                       std::to_string(array_literal->arguments.size()) +
                       " arguments")
                          .c_str());

            if (var.is_multidimensional) {
                // 多次元配列リテラル初期化は既存のメソッドを使用
                processMultidimensionalArrayLiteral(var, array_literal,
                                                    node->type_info);
            } else {
                // 1次元配列リテラル初期化
                if (node->type_info == TYPE_STRING) {
                    // 配列リテラルのサイズに合わせて配列を調整
                    var.array_strings.resize(array_literal->arguments.size());
                    for (size_t i = 0; i < array_literal->arguments.size();
                         i++) {
                        // 要素の型チェック -
                        // 文字列配列に数値リテラルが混入していないかチェック
                        if (array_literal->arguments[i]->node_type !=
                            ASTNodeType::AST_STRING_LITERAL) {
                            error_msg(
                                DebugMsgId::TYPE_MISMATCH_ERROR,
                                ("Type mismatch in array literal: expected "
                                 "string but found non-string at index " +
                                 std::to_string(i))
                                    .c_str());
                            throw std::runtime_error(
                                "Type mismatch in array literal");
                        }
                        var.array_strings[i] =
                            array_literal->arguments[i]->str_value;
                    }
                } else {
                    // 配列リテラルのサイズに合わせて配列を調整
                    var.array_values.resize(array_literal->arguments.size());
                    for (size_t i = 0; i < array_literal->arguments.size();
                         i++) {
                        // 要素の型チェック -
                        // 数値配列に文字列リテラルが混入していないかチェック
                        if (array_literal->arguments[i]->node_type ==
                            ASTNodeType::AST_STRING_LITERAL) {
                            error_msg(
                                DebugMsgId::TYPE_MISMATCH_ERROR,
                                ("Type mismatch in array literal: expected "
                                 "number but found string at index " +
                                 std::to_string(i))
                                    .c_str());
                            throw std::runtime_error(
                                "Type mismatch in array literal");
                        }
                        // 式評価はExpressionEvaluatorを使用
                        var.array_values[i] = static_cast<int64_t>(
                            expression_evaluator_->evaluate_expression(
                                array_literal->arguments[i].get()));
                    }
                }
                // 配列サイズを更新
                var.array_size = array_literal->arguments.size();
                debug_msg(DebugMsgId::ARRAY_DECL_DEBUG,
                          ("Array initialized with size: " +
                           std::to_string(var.array_size))
                              .c_str());

                // 1次元配列の場合、array_dimensionsも設定（動的配列の場合）
                if (!var.is_multidimensional && var.array_dimensions.empty()) {
                    var.array_dimensions.push_back(var.array_size);
                }

                // 静的配列サイズとリテラルサイズの不一致チェック
                if (!var.is_multidimensional) {
                    int declared_size = 0;
                    bool has_declared_size = false;

                    if (node->array_dimensions.size() == 1 &&
                        node->array_dimensions[0]) {
                        // array_dimensions を使用した場合
                        declared_size = static_cast<int>(
                            expression_evaluator_->evaluate_expression(
                                node->array_dimensions[0].get()));
                        has_declared_size = true;
                    } else if (node->array_size_expr) {
                        // array_size_expr
                        // を使用した場合（create_array_init_with_size から）
                        declared_size = static_cast<int>(
                            expression_evaluator_->evaluate_expression(
                                node->array_size_expr.get()));
                        has_declared_size = true;
                    }

                    // 静的配列の場合のみサイズ不一致をチェック
                    if (has_declared_size &&
                        declared_size != (int)var.array_size) {
                        error_msg(DebugMsgId::DYNAMIC_ARRAY_NOT_SUPPORTED,
                                  ("Array size mismatch: declared " +
                                   std::to_string(declared_size) +
                                   " but initialized with " +
                                   std::to_string(var.array_size) + " elements")
                                      .c_str());
                        throw std::runtime_error("Array size mismatch");
                    }
                }
            }
            var.is_assigned = true;
        } else if (node->init_expr->node_type == ASTNodeType::AST_FUNC_CALL) {
            // 関数呼び出しで初期化 - 静的配列と動的配列の両方を許可
            debug_msg(DebugMsgId::ARRAY_DECL_DEBUG,
                      "Array initialization from function call detected");

            // サイズが指定されている場合はサイズチェックを実行
            bool has_size_spec = (node->array_dimensions.size() == 1 &&
                                  node->array_dimensions[0]) ||
                                 node->array_size_expr;

            if (has_size_spec) {
                debug_msg(
                    DebugMsgId::ARRAY_DECL_DEBUG,
                    "Static array with function call - performing size check");

                try {
                    // 関数を実行して配列を取得
                    expression_evaluator_->evaluate_expression(
                        node->init_expr.get());
                } catch (const ReturnException &ret) {
                    if (ret.is_array) {
                        debug_msg(DebugMsgId::ARRAY_DECL_DEBUG,
                                  "Function returned array, checking size "
                                  "compatibility");

                        // 戻り値の配列サイズを取得
                        int actual_return_size = 0;
                        if (!ret.int_array_3d.empty()) {
                            for (const auto &plane : ret.int_array_3d) {
                                for (const auto &row : plane) {
                                    actual_return_size += row.size();
                                }
                            }
                        } else if (!ret.str_array_3d.empty()) {
                            for (const auto &plane : ret.str_array_3d) {
                                for (const auto &row : plane) {
                                    actual_return_size += row.size();
                                }
                            }
                        }

                        // 宣言されたサイズを取得
                        int declared_size = 0;
                        if (node->array_dimensions.size() == 1 &&
                            node->array_dimensions[0]) {
                            declared_size = static_cast<int>(
                                expression_evaluator_->evaluate_expression(
                                    node->array_dimensions[0].get()));
                        } else if (node->array_size_expr) {
                            declared_size = static_cast<int>(
                                expression_evaluator_->evaluate_expression(
                                    node->array_size_expr.get()));
                        }

                        // サイズチェック
                        if (declared_size > 0 &&
                            declared_size != actual_return_size) {
                            error_msg(DebugMsgId::DYNAMIC_ARRAY_NOT_SUPPORTED,
                                      ("Array size mismatch: declared " +
                                       std::to_string(declared_size) +
                                       " but function returned " +
                                       std::to_string(actual_return_size) +
                                       " elements")
                                          .c_str());
                            throw std::runtime_error(
                                "Array size mismatch in function return "
                                "assignment");
                        }

                        // 配列データを設定
                        if (!ret.int_array_3d.empty()) {
                            var.array_values.clear();
                            for (const auto &plane : ret.int_array_3d) {
                                for (const auto &row : plane) {
                                    for (const auto &element : row) {
                                        var.array_values.push_back(element);
                                    }
                                }
                            }
                            var.type = static_cast<TypeInfo>(TYPE_ARRAY_BASE +
                                                             TYPE_INT);
                        } else if (!ret.str_array_3d.empty()) {
                            var.array_strings.clear();
                            for (const auto &plane : ret.str_array_3d) {
                                for (const auto &row : plane) {
                                    for (const auto &element : row) {
                                        var.array_strings.push_back(element);
                                    }
                                }
                            }
                            var.type = static_cast<TypeInfo>(TYPE_ARRAY_BASE +
                                                             TYPE_STRING);
                        }

                        var.array_size = actual_return_size;
                        var.is_assigned = true;
                        debug_msg(DebugMsgId::ARRAY_DECL_DEBUG,
                                  ("Static array initialized from function "
                                   "with size: " +
                                   std::to_string(actual_return_size))
                                      .c_str());
                    } else {
                        throw std::runtime_error(
                            "Function does not return an array");
                    }
                }
            } else {
                // 動的配列の場合 -
                // サイズをチェックせずに関数の戻り値をそのまま受け入れる
                debug_msg(DebugMsgId::ARRAY_DECL_DEBUG,
                          "Dynamic array with function call - accepting "
                          "returned array as-is");

                try {
                    // 関数を実行して配列を取得
                    expression_evaluator_->evaluate_expression(
                        node->init_expr.get());
                } catch (const ReturnException &ret) {
                    if (ret.is_array) {
                        debug_msg(DebugMsgId::ARRAY_DECL_DEBUG,
                                  "Function returned array, setting up dynamic "
                                  "array");

                        // 戻り値の配列サイズを取得
                        int actual_return_size = 0;
                        if (!ret.int_array_3d.empty()) {
                            for (const auto &plane : ret.int_array_3d) {
                                for (const auto &row : plane) {
                                    actual_return_size += row.size();
                                }
                            }
                        } else if (!ret.str_array_3d.empty()) {
                            for (const auto &plane : ret.str_array_3d) {
                                for (const auto &row : plane) {
                                    actual_return_size += row.size();
                                }
                            }
                        }

                        // 配列データを設定
                        if (!ret.int_array_3d.empty()) {
                            var.array_values.clear();
                            for (const auto &plane : ret.int_array_3d) {
                                for (const auto &row : plane) {
                                    for (const auto &element : row) {
                                        var.array_values.push_back(element);
                                    }
                                }
                            }
                            var.type = static_cast<TypeInfo>(TYPE_ARRAY_BASE +
                                                             TYPE_INT);
                        } else if (!ret.str_array_3d.empty()) {
                            var.array_strings.clear();
                            for (const auto &plane : ret.str_array_3d) {
                                for (const auto &row : plane) {
                                    for (const auto &element : row) {
                                        var.array_strings.push_back(element);
                                    }
                                }
                            }
                            var.type = static_cast<TypeInfo>(TYPE_ARRAY_BASE +
                                                             TYPE_STRING);
                        }

                        var.array_size = actual_return_size;

                        // 1次元動的配列の場合、array_dimensionsも設定
                        if (var.array_dimensions.empty()) {
                            var.array_dimensions.push_back(actual_return_size);
                        }

                        var.is_assigned = true;
                        debug_msg(DebugMsgId::ARRAY_DECL_DEBUG,
                                  ("Dynamic array initialized from function "
                                   "with size: " +
                                   std::to_string(actual_return_size))
                                      .c_str());
                    } else {
                        throw std::runtime_error(
                            "Function does not return an array");
                    }
                }
            }
        }
    }

    // デバッグ: 最終的な配列状態を確認
    std::string debug_info =
        "Final array '" + node->name +
        "': size=" + std::to_string(var.array_size) +
        ", is_assigned=" + (var.is_assigned ? "true" : "false");
    debug_msg(DebugMsgId::ARRAY_DECL_DEBUG, debug_info.c_str());
}

void ArrayManager::processMultidimensionalArrayLiteral(
    Variable &var, const ASTNode *literal_node, TypeInfo elem_type) {
    if (!literal_node ||
        literal_node->node_type != ASTNodeType::AST_ARRAY_LITERAL) {
        throw std::runtime_error("Invalid array literal node");
    }

    // 空の配列リテラルのチェック
    if (literal_node->arguments.empty()) {
        throw std::runtime_error(
            "Empty array literal for multidimensional array");
    }

    // 多次元配列の次元を検証
    std::vector<int> dimensions = extractArrayDimensions(literal_node);

    // 宣言された次元と一致するかチェック
    if (dimensions.size() != var.array_type_info.dimensions.size()) {
        debug_msg(DebugMsgId::TYPE_MISMATCH_ERROR,
                  ("Dimension mismatch: literal=" +
                   std::to_string(dimensions.size()) + ", declared=" +
                   std::to_string(var.array_type_info.dimensions.size()))
                      .c_str());
        throw std::runtime_error(
            "Array literal dimensions don't match declaration");
    }

    for (size_t i = 0; i < dimensions.size(); ++i) {
        if (dimensions[i] != var.array_type_info.dimensions[i].size) {
            debug_msg(DebugMsgId::TYPE_MISMATCH_ERROR,
                      ("Size mismatch at dimension " + std::to_string(i) +
                       ": literal=" + std::to_string(dimensions[i]) +
                       ", declared=" +
                       std::to_string(var.array_type_info.dimensions[i].size))
                          .c_str());
            throw std::runtime_error(
                "Array literal size doesn't match declaration");
        }
    }

    // 総要素数を計算
    std::vector<int> dim_sizes;
    for (const auto &dim : var.array_type_info.dimensions) {
        dim_sizes.push_back(dim.size);
    }
    int total_size = calculateTotalSize(dim_sizes);

    // データ配列を初期化
    if (elem_type == TYPE_STRING) {
        var.multidim_array_strings.resize(total_size);
    } else {
        var.multidim_array_values.resize(total_size);
    }

    // 配列リテラルの値を設定
    std::vector<int> current_indices;
    processArrayLiteralRecursive(var, literal_node, elem_type, 0,
                                 current_indices);
}

void ArrayManager::processNDimensionalArrayLiteral(Variable &var,
                                                   const ASTNode *literal_node,
                                                   TypeInfo base_type) {
    if (!literal_node ||
        literal_node->node_type != ASTNodeType::AST_ARRAY_LITERAL) {
        throw std::runtime_error(
            "Invalid array literal for N-dimensional array");
    }

    // 空配列の場合
    if (literal_node->arguments.empty()) {
        throw std::runtime_error(
            "Empty array literal not allowed for N-dimensional arrays");
    }

    // リテラルから次元情報を推定
    std::vector<int> inferred_dimensions;
    const ASTNode *current = literal_node;

    while (current && current->node_type == ASTNodeType::AST_ARRAY_LITERAL) {
        inferred_dimensions.push_back(current->arguments.size());
        if (!current->arguments.empty() && current->arguments[0]->node_type ==
                                               ASTNodeType::AST_ARRAY_LITERAL) {
            current = current->arguments[0].get();
        } else {
            break;
        }
    }

    // 宣言された次元との照合
    validateArrayDimensions(var.array_dimensions, inferred_dimensions);

    // 総要素数計算と初期化
    int total_size = calculateTotalSize(inferred_dimensions);
    var.array_dimensions = inferred_dimensions;

    if (base_type == TYPE_STRING) {
        var.multidim_array_strings.resize(total_size);
    } else {
        var.multidim_array_values.resize(total_size);
    }

    // データを再帰的に処理
    std::vector<int> current_indices;
    processArrayLiteralRecursive(var, literal_node, base_type, 0,
                                 current_indices);
}

void ArrayManager::processArrayLiteralRecursive(
    Variable &var, const ASTNode *node, TypeInfo base_type, int current_dim,
    std::vector<int> &current_indices) {
    if (!node)
        return;

    if (node->node_type == ASTNodeType::AST_ARRAY_LITERAL) {
        // 現在の次元のインデックスをセット
        for (int i = 0; i < static_cast<int>(node->arguments.size()); ++i) {
            current_indices.resize(current_dim + 1);
            current_indices[current_dim] = i;

            // 再帰的に処理
            processArrayLiteralRecursive(var, node->arguments[i].get(),
                                         base_type, current_dim + 1,
                                         current_indices);
        }
    } else {
        // 葉ノード（実際の値）
        int flat_index = var.calculate_flat_index(current_indices);

        if (base_type == TYPE_STRING) {
            var.multidim_array_strings[flat_index] = node->str_value;
        } else {
            var.multidim_array_values[flat_index] = node->int_value;
        }
    }
}

int64_t ArrayManager::getMultidimensionalArrayElement(
    const Variable &var, const std::vector<int64_t> &indices) {
    if (!var.is_multidimensional) {
        throw std::runtime_error("Variable is not a multidimensional array");
    }

    std::vector<int> int_indices;
    for (int64_t idx : indices) {
        int_indices.push_back(static_cast<int>(idx));
    }

    int flat_index = var.calculate_flat_index(int_indices);

    if (var.array_type_info.base_type == TYPE_STRING) {
        // 文字列配列の場合、文字列を数値として扱うのは適切でない
        throw std::runtime_error("Cannot get string array element as integer");
    }

    return var.multidim_array_values[flat_index];
}

void ArrayManager::setMultidimensionalArrayElement(
    Variable &var, const std::vector<int64_t> &indices, int64_t value) {
    if (!var.is_multidimensional) {
        throw std::runtime_error("Variable is not a multidimensional array");
    }

    // const配列への書き込みチェック
    if (var.is_const) {
        throw std::runtime_error(
            "Cannot assign to const multidimensional array");
    }

    std::vector<int> int_indices;
    for (int64_t idx : indices) {
        int_indices.push_back(static_cast<int>(idx));
    }

    int flat_index = var.calculate_flat_index(int_indices);

    if (var.array_type_info.base_type == TYPE_STRING) {
        throw std::runtime_error(
            "Cannot set string array element with integer value");
    }

    var.multidim_array_values[flat_index] = value;
}

std::string ArrayManager::getMultidimensionalStringArrayElement(
    const Variable &var, const std::vector<int64_t> &indices) {
    if (!var.is_multidimensional) {
        throw std::runtime_error("Variable is not a multidimensional array");
    }

    std::vector<int> int_indices;
    for (int64_t idx : indices) {
        int_indices.push_back(static_cast<int>(idx));
    }

    int flat_index = var.calculate_flat_index(int_indices);

    if (var.array_type_info.base_type != TYPE_STRING) {
        throw std::runtime_error(
            "Cannot get non-string array element as string");
    }

    return var.multidim_array_strings[flat_index];
}

void ArrayManager::setMultidimensionalStringArrayElement(
    Variable &var, const std::vector<int64_t> &indices,
    const std::string &value) {
    if (!var.is_multidimensional) {
        throw std::runtime_error("Variable is not a multidimensional array");
    }

    // const配列への書き込みチェック
    if (var.is_const) {
        throw std::runtime_error(
            "Cannot assign to const multidimensional string array");
    }

    std::vector<int> int_indices;
    for (int64_t idx : indices) {
        int_indices.push_back(static_cast<int>(idx));
    }

    int flat_index = var.calculate_flat_index(int_indices);

    if (var.array_type_info.base_type != TYPE_STRING) {
        throw std::runtime_error(
            "Cannot set non-string array element with string value");
    }

    var.multidim_array_strings[flat_index] = value;
}

void ArrayManager::initializeArray(Variable &var, TypeInfo base_type,
                                   const std::vector<int> &dimensions) {
    var.is_array = true;
    var.array_dimensions = dimensions;

    if (dimensions.size() > 1) {
        var.is_multidimensional = true;
        // ArrayTypeInfoを設定
        var.array_type_info.base_type = base_type;
        var.array_type_info.dimensions.clear();
        for (int dim : dimensions) {
            ArrayDimension array_dim;
            array_dim.size = dim;
            var.array_type_info.dimensions.push_back(array_dim);
        }
    }

    int total_size = calculateTotalSize(dimensions);

    if (base_type == TYPE_STRING) {
        if (var.is_multidimensional) {
            var.multidim_array_strings.resize(total_size, "");
        } else {
            var.array_strings.resize(total_size, "");
        }
    } else {
        if (var.is_multidimensional) {
            var.multidim_array_values.resize(total_size, 0);
        } else {
            var.array_values.resize(total_size, 0);
        }
    }
}

void ArrayManager::initializeMultidimensionalArray(
    Variable &var, const ArrayTypeInfo &array_info) {
    var.is_array = true;
    var.is_multidimensional = true;
    var.array_type_info = array_info;

    // 次元サイズをコピー
    var.array_dimensions.clear();
    for (const auto &dim : array_info.dimensions) {
        var.array_dimensions.push_back(dim.size);
    }

    int total_size = calculateTotalSize(var.array_dimensions);

    if (array_info.base_type == TYPE_STRING) {
        var.multidim_array_strings.resize(total_size, "");
    } else {
        var.multidim_array_values.resize(total_size, 0);
    }
}

int ArrayManager::calculateTotalSize(const std::vector<int> &dimensions) {
    int total = 1;
    for (int dim : dimensions) {
        total *= dim;
    }
    return total;
}

std::vector<int> ArrayManager::extractDimensionSizes(
    const std::vector<ArrayDimension> &dimensions) {
    std::vector<int> sizes;
    for (const auto &dim : dimensions) {
        sizes.push_back(dim.size);
    }
    return sizes;
}

void ArrayManager::validateArrayDimensions(const std::vector<int> &expected,
                                           const std::vector<int> &actual) {
    if (expected.size() != actual.size()) {
        throw std::runtime_error("Array dimension count mismatch");
    }

    for (size_t i = 0; i < expected.size(); ++i) {
        if (expected[i] != actual[i]) {
            throw std::runtime_error(
                "Array dimension size mismatch at dimension " +
                std::to_string(i));
        }
    }
}

void ArrayManager::declare_array(const ASTNode *node) {
    Variable var;

    debug_msg(DebugMsgId::ARRAY_DECL_START, node->name.c_str());

    // 多次元配列かどうかを確認
    if (node->array_type_info.dimensions.size() > 1) {
        debug_msg(DebugMsgId::MULTIDIM_ARRAY_DECL_INFO,
                  static_cast<int>(node->array_type_info.dimensions.size()));

        // 多次元配列の場合
        var.is_array = true;
        var.is_multidimensional = true;
        var.array_type_info = node->array_type_info;
        var.type = static_cast<TypeInfo>(TYPE_ARRAY_BASE +
                                         node->array_type_info.base_type);
        var.is_const = node->is_const;
        var.is_assigned = false;

        // 全次元のサイズを計算して平坦化された配列を作成
        int total_size = 1;
        var.array_dimensions.clear();
        for (const ArrayDimension &dim : node->array_type_info.dimensions) {
            total_size *= dim.size;
            var.array_dimensions.push_back(dim.size);
        }

        var.array_size = total_size; // array_sizeを設定

        debug_msg(DebugMsgId::ARRAY_TOTAL_SIZE, total_size);

        // 多次元配列用のストレージを初期化
        if (node->array_type_info.base_type == TYPE_STRING) {
            var.multidim_array_strings.resize(total_size, "");
        } else {
            var.multidim_array_values.resize(total_size, 0);
        }

        // グローバルスコープに保存（AST_ARRAY_DECLはグローバル配列宣言のみ）
        variable_manager_->getInterpreter()
            ->global_scope.variables[node->name] = var;
        debug_msg(DebugMsgId::MULTIDIM_ARRAY_DECL_SUCCESS, node->name.c_str());
    } else {
        // 単一次元配列の場合
        debug_msg(DebugMsgId::ARRAY_DECL_DEBUG);

        var.is_array = true;
        var.is_multidimensional = false;
        var.type = static_cast<TypeInfo>(TYPE_ARRAY_BASE +
                                         node->array_type_info.base_type);
        var.is_const = node->is_const;
        var.is_assigned = false;

        int size = node->array_type_info.dimensions[0].size;
        var.array_size = size; // array_sizeを設定
        debug_msg(DebugMsgId::ARRAY_TOTAL_SIZE, size);

        // 単一次元配列の場合もarray_dimensionsを設定
        var.array_dimensions.clear();
        var.array_dimensions.push_back(size);

        // 単一次元配列用のストレージを初期化
        if (node->array_type_info.base_type == TYPE_STRING) {
            var.array_strings.resize(size, "");
        } else {
            var.array_values.resize(size, 0);
        }

        // グローバルスコープに保存（AST_ARRAY_DECLはグローバル配列宣言のみ）
        variable_manager_->getInterpreter()
            ->global_scope.variables[node->name] = var;
        debug_msg(DebugMsgId::ARRAY_DECL_SUCCESS, node->name.c_str());
    }
}

// 配列コピー機能の実装
void ArrayManager::copyArray(Variable &dest, const Variable &src) {
    if (!isCompatibleArrayType(dest, src)) {
        throw std::runtime_error("Incompatible array types for copy operation");
    }

    // 型情報をコピー
    dest.type = src.type;
    dest.is_array = src.is_array;
    dest.is_multidimensional = src.is_multidimensional;
    dest.array_dimensions = src.array_dimensions;
    dest.array_size = src.array_size;
    dest.array_type_info = src.array_type_info;

    // データをコピー
    if (src.is_multidimensional) {
        if (src.array_type_info.base_type == TYPE_STRING) {
            dest.multidim_array_strings = src.multidim_array_strings;
        } else {
            dest.multidim_array_values = src.multidim_array_values;
        }
    } else {
        if (static_cast<int>(src.type) == TYPE_ARRAY_BASE + TYPE_STRING) {
            dest.array_strings = src.array_strings;
        } else {
            dest.array_values = src.array_values;
        }
    }

    dest.is_assigned = true;
}

void ArrayManager::copyArraySlice(Variable &dest, const Variable &src,
                                  const std::vector<int64_t> &slice_indices) {
    if (!src.is_multidimensional ||
        slice_indices.size() >= src.array_dimensions.size()) {
        throw std::runtime_error("Invalid array slice operation");
    }

    // スライスのサイズを計算
    std::vector<int> slice_dimensions;
    for (size_t i = slice_indices.size(); i < src.array_dimensions.size();
         i++) {
        slice_dimensions.push_back(src.array_dimensions[i]);
    }

    // 目的配列が単一次元の場合
    if (slice_dimensions.size() == 1) {
        dest.is_array = true;
        dest.is_multidimensional = false;
        dest.array_size = slice_dimensions[0];
        dest.array_dimensions = slice_dimensions;
        dest.type = static_cast<TypeInfo>(TYPE_ARRAY_BASE +
                                          src.array_type_info.base_type);

        // データをコピー
        if (src.array_type_info.base_type == TYPE_STRING) {
            dest.array_strings.resize(slice_dimensions[0]);
            for (int i = 0; i < slice_dimensions[0]; i++) {
                std::vector<int64_t> full_indices = slice_indices;
                full_indices.push_back(i);
                dest.array_strings[i] =
                    getMultidimensionalStringArrayElement(src, full_indices);
            }
        } else {
            dest.array_values.resize(slice_dimensions[0]);
            for (int i = 0; i < slice_dimensions[0]; i++) {
                std::vector<int64_t> full_indices = slice_indices;
                full_indices.push_back(i);
                dest.array_values[i] =
                    getMultidimensionalArrayElement(src, full_indices);
            }
        }
    } else {
        // 多次元スライス（将来の拡張用）
        throw std::runtime_error(
            "Multi-dimensional array slicing not yet supported");
    }

    dest.is_assigned = true;
}

bool ArrayManager::isCompatibleArrayType(const Variable &dest,
                                         const Variable &src) {
    // 基本的な配列型チェック
    if (!dest.is_array || !src.is_array) {
        return false;
    }

    // 配列スライスの場合は次元数チェックを緩和
    // 例: int[3] = int[3][3][0] のような場合を許可

    // 基本型チェック
    TypeInfo dest_base =
        dest.is_multidimensional
            ? dest.array_type_info.base_type
            : static_cast<TypeInfo>(dest.type - TYPE_ARRAY_BASE);
    TypeInfo src_base = src.is_multidimensional
                            ? src.array_type_info.base_type
                            : static_cast<TypeInfo>(src.type - TYPE_ARRAY_BASE);

    return dest_base == src_base;
}

// 配列リテラルから次元を再帰的に抽出する関数
std::vector<int>
ArrayManager::extractArrayDimensions(const ASTNode *literal_node) {
    std::vector<int> dimensions;

    if (!literal_node ||
        literal_node->node_type != ASTNodeType::AST_ARRAY_LITERAL) {
        return dimensions;
    }

    // 現在のレベルの要素数を追加
    dimensions.push_back(literal_node->arguments.size());

    // 空の配列の場合はここで終了
    if (literal_node->arguments.empty()) {
        return dimensions;
    }

    // 最初の要素が配列リテラルの場合、再帰的に次元を取得
    const ASTNode *first_element = literal_node->arguments[0].get();
    if (first_element &&
        first_element->node_type == ASTNodeType::AST_ARRAY_LITERAL) {
        std::vector<int> sub_dimensions = extractArrayDimensions(first_element);
        dimensions.insert(dimensions.end(), sub_dimensions.begin(),
                          sub_dimensions.end());
    }

    return dimensions;
}
