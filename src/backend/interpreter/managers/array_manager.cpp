#include "managers/array_manager.h"
#include "core/interpreter.h" // Variable の定義のため
#include "managers/type_manager.h" // TypeManager の定義のため
#include "../../../common/debug_messages.h"
#include "../../../common/type_alias.h"
#include "../services/debug_service.h"
#include "core/error_handler.h"
#include "evaluator/expression_evaluator.h"
#include "services/expression_service.h" // DRY効率化: 統一式評価サービス
#include "managers/variable_manager.h"
#include <stdexcept>

namespace {

TypeInfo resolve_base_type(const Variable &var) {
    if (var.array_type_info.base_type != TYPE_UNKNOWN) {
        return var.array_type_info.base_type;
    }
    if (var.type >= TYPE_ARRAY_BASE) {
        return static_cast<TypeInfo>(var.type - TYPE_ARRAY_BASE);
    }
    return var.type;
}

bool is_floating_type(TypeInfo type) {
    return type == TYPE_FLOAT || type == TYPE_DOUBLE || type == TYPE_QUAD;
}

void ensure_numeric_storage(Variable &var, size_t total_size,
                            bool is_multidim, TypeInfo base_type) {
    if (is_multidim) {
        if (base_type == TYPE_FLOAT) {
            var.multidim_array_float_values.assign(total_size, 0.0f);
            var.multidim_array_double_values.clear();
            var.multidim_array_quad_values.clear();
            var.multidim_array_values.clear();
        } else if (base_type == TYPE_DOUBLE) {
            var.multidim_array_double_values.assign(total_size, 0.0);
            var.multidim_array_float_values.clear();
            var.multidim_array_quad_values.clear();
            var.multidim_array_values.clear();
        } else if (base_type == TYPE_QUAD) {
            var.multidim_array_quad_values.assign(total_size, 0.0L);
            var.multidim_array_float_values.clear();
            var.multidim_array_double_values.clear();
            var.multidim_array_values.clear();
        } else {
            var.multidim_array_values.assign(total_size, 0);
            var.multidim_array_float_values.clear();
            var.multidim_array_double_values.clear();
            var.multidim_array_quad_values.clear();
        }
    } else {
        if (base_type == TYPE_FLOAT) {
            var.array_float_values.assign(total_size, 0.0f);
            var.array_double_values.clear();
            var.array_quad_values.clear();
            var.array_values.clear();
        } else if (base_type == TYPE_DOUBLE) {
            var.array_double_values.assign(total_size, 0.0);
            var.array_float_values.clear();
            var.array_quad_values.clear();
            var.array_values.clear();
        } else if (base_type == TYPE_QUAD) {
            var.array_quad_values.assign(total_size, 0.0L);
            var.array_float_values.clear();
            var.array_double_values.clear();
            var.array_values.clear();
        } else {
            var.array_values.assign(total_size, 0);
            var.array_float_values.clear();
            var.array_double_values.clear();
            var.array_quad_values.clear();
        }
    }
}

long double get_numeric_storage_value(const Variable &var, size_t index,
                                      bool is_multidim, TypeInfo base_type) {
    if (is_multidim) {
        if (base_type == TYPE_FLOAT) {
            if (index < var.multidim_array_float_values.size()) {
                return var.multidim_array_float_values[index];
            }
        } else if (base_type == TYPE_DOUBLE) {
            if (index < var.multidim_array_double_values.size()) {
                return var.multidim_array_double_values[index];
            }
        } else if (base_type == TYPE_QUAD) {
            if (index < var.multidim_array_quad_values.size()) {
                return var.multidim_array_quad_values[index];
            }
        } else {
            if (index < var.multidim_array_values.size()) {
                return static_cast<long double>(var.multidim_array_values[index]);
            }
        }
    } else {
        if (base_type == TYPE_FLOAT) {
            if (index < var.array_float_values.size()) {
                return var.array_float_values[index];
            }
        } else if (base_type == TYPE_DOUBLE) {
            if (index < var.array_double_values.size()) {
                return var.array_double_values[index];
            }
        } else if (base_type == TYPE_QUAD) {
            if (index < var.array_quad_values.size()) {
                return var.array_quad_values[index];
            }
        } else {
            if (index < var.array_values.size()) {
                return static_cast<long double>(var.array_values[index]);
            }
        }
    }
    return 0.0L;
}

void set_numeric_storage_value(Variable &var, size_t index, long double value,
                               bool is_multidim, TypeInfo base_type) {
    if (is_multidim) {
        if (base_type == TYPE_FLOAT) {
            if (index >= var.multidim_array_float_values.size()) {
                var.multidim_array_float_values.resize(index + 1, 0.0f);
            }
            var.multidim_array_float_values[index] = static_cast<float>(value);
        } else if (base_type == TYPE_DOUBLE) {
            if (index >= var.multidim_array_double_values.size()) {
                var.multidim_array_double_values.resize(index + 1, 0.0);
            }
            var.multidim_array_double_values[index] =
                static_cast<double>(value);
        } else if (base_type == TYPE_QUAD) {
            if (index >= var.multidim_array_quad_values.size()) {
                var.multidim_array_quad_values.resize(index + 1, 0.0L);
            }
            var.multidim_array_quad_values[index] = value;
        } else {
            if (index >= var.multidim_array_values.size()) {
                var.multidim_array_values.resize(index + 1, 0);
            }
            var.multidim_array_values[index] = static_cast<int64_t>(value);
        }
    } else {
        if (base_type == TYPE_FLOAT) {
            if (index >= var.array_float_values.size()) {
                var.array_float_values.resize(index + 1, 0.0f);
            }
            var.array_float_values[index] = static_cast<float>(value);
        } else if (base_type == TYPE_DOUBLE) {
            if (index >= var.array_double_values.size()) {
                var.array_double_values.resize(index + 1, 0.0);
            }
            var.array_double_values[index] = static_cast<double>(value);
        } else if (base_type == TYPE_QUAD) {
            if (index >= var.array_quad_values.size()) {
                var.array_quad_values.resize(index + 1, 0.0L);
            }
            var.array_quad_values[index] = value;
        } else {
            if (index >= var.array_values.size()) {
                var.array_values.resize(index + 1, 0);
            }
            var.array_values[index] = static_cast<int64_t>(value);
        }
    }
}

TypedValue make_numeric_typed_value(long double value, TypeInfo type) {
    std::string type_name = type_info_to_string(type);
    InferredType inferred(type, type_name);
    if (type == TYPE_FLOAT) {
        return TypedValue(static_cast<double>(value), inferred);
    } else if (type == TYPE_DOUBLE) {
        return TypedValue(static_cast<double>(value), inferred);
    } else if (type == TYPE_QUAD) {
        return TypedValue(value, inferred);
    }
    return TypedValue(static_cast<int64_t>(value), inferred);
}

size_t get_numeric_storage_size(const Variable &var, bool is_multidim,
                                TypeInfo base_type) {
    if (is_multidim) {
        if (base_type == TYPE_FLOAT) {
            return var.multidim_array_float_values.size();
        }
        if (base_type == TYPE_DOUBLE) {
            return var.multidim_array_double_values.size();
        }
        if (base_type == TYPE_QUAD) {
            return var.multidim_array_quad_values.size();
        }
        return var.multidim_array_values.size();
    }

    if (base_type == TYPE_FLOAT) {
        return var.array_float_values.size();
    }
    if (base_type == TYPE_DOUBLE) {
        return var.array_double_values.size();
    }
    if (base_type == TYPE_QUAD) {
        return var.array_quad_values.size();
    }
    return var.array_values.size();
}

} // namespace

void ArrayManager::processArrayDeclaration(Variable &var, const ASTNode *node) {
    std::string debug_message = "Processing array declaration for variable: " + node->name;
    debug_msg(DebugMsgId::ARRAY_DECL_DEBUG, debug_message.c_str());
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

    // struct配列の特別処理
    if (node->type_info == TYPE_STRUCT) {
        debug_msg(DebugMsgId::ARRAY_DECL_DEBUG, "Processing struct array");
        debug_msg(DebugMsgId::ARRAY_DECL_DEBUG,
                  ("Struct type: " + node->type_name).c_str());

        var.type = TYPE_STRUCT;
        var.is_struct = false; // 配列自体はstructではない
        var.struct_type_name = node->type_name;
    } else {
        var.type = static_cast<TypeInfo>(TYPE_ARRAY_BASE + node->type_info);
    }

    var.is_const = node->is_const;
    var.is_array = true;
    var.is_assigned = false;
    var.is_unsigned = node->is_unsigned;

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
                evaluate_expression_safe(dim_expr.get(), "dimension_size"));
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
            ensure_numeric_storage(var, static_cast<size_t>(total_size), true,
                                   elem_type);
        }
    } else {
        // 1次元配列
        if (node->array_dimensions.size() == 1) {
            debug_print("ARRAY_DEBUG: first dimension ptr=%p has_value=%d\n",
                        static_cast<const void*>(node->array_dimensions[0].get()),
                        node->array_dimensions[0] ? 1 : 0);
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
                debug_msg(DebugMsgId::ARRAY_DECL_DEBUG,
                          "Evaluating array size expression");
                int size = static_cast<int>(evaluate_expression_safe(
                    node->array_dimensions[0].get(), "array_size"));
                debug_msg(DebugMsgId::ARRAY_DECL_DEBUG,
                          ("Array size evaluated: " +
                           std::to_string(size))
                              .c_str());
                var.array_size = size;

                // 1次元配列でもarray_dimensionsを設定
                var.array_dimensions.push_back(size);

                // 配列要素を初期化
                if (node->type_info == TYPE_STRING) {
                    var.array_strings.resize(size, "");
                } else {
                    debug_msg(DebugMsgId::ARRAY_DECL_DEBUG,
                              "Ensuring numeric storage for 1D array");
                    ensure_numeric_storage(var, static_cast<size_t>(size),
                                           false, node->type_info);
                    debug_msg(DebugMsgId::ARRAY_DECL_DEBUG,
                              "Numeric storage prepared");
                }
            }
        } else if (node->array_size_expr) {
            // array_size_expr が設定されている場合（create_array_init_with_size
            // から）
            int size = static_cast<int>(evaluate_expression_safe(
                node->array_size_expr.get(), "array_size_expr"));
            var.array_size = size;

            // 1次元配列でもarray_dimensionsを設定
            var.array_dimensions.push_back(size);

            // 配列要素を初期化
            if (node->type_info == TYPE_STRING) {
                var.array_strings.resize(size, "");
            } else {
                ensure_numeric_storage(var, static_cast<size_t>(size), false,
                                       node->type_info);
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
                debug_msg(DebugMsgId::ARRAY_DECL_DEBUG,
                          ("Processing multidimensional array literal for: " + node->name).c_str());
                processMultidimensionalArrayLiteral(var, array_literal,
                                                    node->type_info);
            } else {
                // 1次元配列リテラル初期化
                const std::string resolved_name =
                    node->name.empty() ? std::string("<anonymous array>")
                                       : node->name;
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
                    TypeInfo base_type = node->type_info;
                    bool expects_float = is_floating_type(base_type);
                    ensure_numeric_storage(
                        var, array_literal->arguments.size(), false,
                        base_type);

                    for (size_t i = 0; i < array_literal->arguments.size();
                         i++) {
                        // 要素の型チェック -
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

                        if (debug_mode) {
                            std::cerr << "[ARRAY_INIT_DEBUG] Element[" << i << "] node_type: "
                                     << static_cast<int>(array_literal->arguments[i]->node_type) << std::endl;
                        }
                        
                        TypedValue element_value =
                            evaluate_expression_typed_safe(
                                array_literal->arguments[i].get(),
                                "array_literal_element");

                        if (debug_mode) {
                            std::cerr << "[ARRAY_INIT_DEBUG] Element[" << i << "] evaluated: "
                                     << "is_numeric=" << element_value.is_numeric()
                                     << ", is_floating=" << element_value.is_floating() << std::endl;
                            if (element_value.is_numeric()) {
                                std::cerr << "[ARRAY_INIT_DEBUG] Numeric value: " << element_value.as_numeric() << std::endl;
                            }
                        }

                        if (!element_value.is_numeric()) {
                            throw std::runtime_error(
                                "Array literal element is not numeric");
                        }

                        long double numeric_value = element_value.is_floating()
                                                       ? element_value.as_quad()
                                                       : static_cast<long double>(element_value.as_numeric());

                        if (!expects_float) {
                            int64_t coerced_value =
                                static_cast<int64_t>(numeric_value);
                            if (var.is_unsigned && coerced_value < 0) {
                                DEBUG_WARN(
                                    VARIABLE,
                                    "Unsigned array %s literal element [%zu] negative (%lld); clamping to 0",
                                    resolved_name.c_str(), i,
                                    static_cast<long long>(coerced_value));
                                coerced_value = 0;
                            }
                            numeric_value =
                                static_cast<long double>(coerced_value);
                        }

                        set_numeric_storage_value(var, i, numeric_value, false,
                                                   base_type);
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
                        declared_size =
                            static_cast<int>(evaluate_expression_safe(
                                node->array_dimensions[0].get(),
                                "declared_size"));
                        has_declared_size = true;
                    } else if (node->array_size_expr) {
                        // array_size_expr
                        // を使用した場合（create_array_init_with_size から）
                        declared_size =
                            static_cast<int>(evaluate_expression_safe(
                                node->array_size_expr.get(),
                                "declared_size_expr"));
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
                    evaluate_expression_safe(node->init_expr.get(),
                                             "function_return");
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
                            declared_size =
                                static_cast<int>(evaluate_expression_safe(
                                    node->array_dimensions[0].get(),
                                    "multidim_size"));
                        } else if (node->array_size_expr) {
                            declared_size =
                                static_cast<int>(evaluate_expression_safe(
                                    node->array_size_expr.get(),
                                    "multidim_size_expr"));
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

                            // 多次元配列かどうかを判定（元のArrayTypeInfoがあれば使用）
                            if (var.array_type_info.dimensions.size() > 1) {
                                var.is_multidimensional = true;
                                var.multidim_array_values.clear();
                                for (const auto &plane : ret.int_array_3d) {
                                    for (const auto &row : plane) {
                                        for (const auto &element : row) {
                                            var.multidim_array_values.push_back(
                                                element);
                                        }
                                    }
                                }
                            } else {
                                var.array_values.clear();
                                for (const auto &plane : ret.int_array_3d) {
                                    for (const auto &row : plane) {
                                        for (const auto &element : row) {
                                            var.array_values.push_back(element);
                                        }
                                    }
                                }
                            }
                            var.type = static_cast<TypeInfo>(TYPE_ARRAY_BASE +
                                                             TYPE_INT);
                        } else if (!ret.str_array_3d.empty()) {
                            if (var.is_multidimensional) {
                                // 多次元文字列配列の場合
                                var.multidim_array_strings.clear();
                                for (const auto &plane : ret.str_array_3d) {
                                    for (const auto &row : plane) {
                                        for (const auto &element : row) {
                                            var.multidim_array_strings.push_back(element);
                                        }
                                    }
                                }
                                var.array_strings.clear();
                            } else {
                                // 1次元文字列配列の場合
                                var.array_strings.clear();
                                for (const auto &plane : ret.str_array_3d) {
                                    for (const auto &row : plane) {
                                        for (const auto &element : row) {
                                            var.array_strings.push_back(element);
                                        }
                                    }
                                }
                                var.multidim_array_strings.clear();
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
                    evaluate_expression_safe(node->init_expr.get(),
                                             "dynamic_array_return");
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
                            if (var.is_multidimensional) {
                                // 多次元配列の場合、multidim_array_valuesに設定
                                var.multidim_array_values.clear();
                                for (const auto &plane : ret.int_array_3d) {
                                    for (const auto &row : plane) {
                                        for (const auto &element : row) {
                                            var.multidim_array_values.push_back(element);
                                        }
                                    }
                                }
                                var.array_values.clear();
                            } else {
                                // 1次元配列の場合、array_valuesに設定
                                var.array_values.clear();
                                for (const auto &plane : ret.int_array_3d) {
                                    for (const auto &row : plane) {
                                        for (const auto &element : row) {
                                            var.array_values.push_back(element);
                                        }
                                    }
                                }
                                var.multidim_array_values.clear();
                            }
                            var.type = static_cast<TypeInfo>(TYPE_ARRAY_BASE +
                                                             TYPE_INT);
                        } else if (!ret.str_array_3d.empty()) {
                            if (var.is_multidimensional) {
                                // 多次元文字列配列の場合、multidim_array_stringsに設定
                                var.multidim_array_strings.clear();
                                for (const auto &plane : ret.str_array_3d) {
                                    for (const auto &row : plane) {
                                        for (const auto &element : row) {
                                            var.multidim_array_strings.push_back(element);
                                        }
                                    }
                                }
                                var.array_strings.clear();
                            } else {
                                // 1次元文字列配列の場合、array_stringsに設定
                                var.array_strings.clear();
                                for (const auto &plane : ret.str_array_3d) {
                                    for (const auto &row : plane) {
                                        for (const auto &element : row) {
                                            var.array_strings.push_back(element);
                                        }
                                    }
                                }
                                var.multidim_array_strings.clear();
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

    // struct配列の要素初期化
    if (node->type_info == TYPE_STRUCT && var.array_size > 0) {
        debug_msg(DebugMsgId::ARRAY_DECL_DEBUG,
                  "Initializing struct array elements");
        debug_msg(DebugMsgId::ARRAY_DECL_DEBUG,
                  ("Struct array: " + node->name + ", size: " + std::to_string(var.array_size)).c_str());

        const StructDefinition *struct_def =
            variable_manager_->getInterpreter()->find_struct_definition(
                variable_manager_->getInterpreter()->type_manager_->resolve_typedef(node->type_name));
        if (!struct_def) {
            debug_msg(DebugMsgId::INTERPRETER_VAR_NOT_FOUND,
                      ("Struct definition not found: " + node->type_name).c_str());
            throw std::runtime_error("Struct definition not found: " +
                                     node->type_name);
        }

        debug_msg(DebugMsgId::ARRAY_DECL_DEBUG,
                  ("Found struct definition: " + node->type_name).c_str());

        for (int i = 0; i < var.array_size; i++) {
            std::string element_name =
                node->name + "[" + std::to_string(i) + "]";
            debug_msg(DebugMsgId::ARRAY_DECL_DEBUG,
                      ("Creating struct element: " + element_name).c_str());

            // struct要素変数を作成 - 完全な初期化
            Variable struct_element;
            // 基本フィールドの明示的初期化
            struct_element.type = TYPE_STRUCT;
            struct_element.is_struct = true;
            struct_element.struct_type_name = node->type_name;
            struct_element.is_assigned = false;
            struct_element.is_array = false;
            struct_element.is_multidimensional = false;
            struct_element.is_const = false;
            struct_element.array_size = 0;
            struct_element.value = 0;
            struct_element.str_value = "";
            
            // コンテナの明示的初期化
            struct_element.struct_members.clear();
            struct_element.array_values.clear();
            struct_element.array_strings.clear();
            struct_element.array_dimensions.clear();
            struct_element.multidim_array_values.clear();
            struct_element.multidim_array_strings.clear();
            
            debug_msg(DebugMsgId::INTERPRETER_STRUCT_REGISTERED,
                      element_name.c_str(), node->type_name.c_str());

            // メンバーを初期化
            for (const auto &member : struct_def->members) {
                Variable member_var;
                member_var.type = member.type;
                member_var.is_assigned = false;
                member_var.is_private_member = member.is_private;

                if (member.type == TYPE_STRING) {
                    member_var.str_value = "";
                } else {
                    member_var.value = 0;
                }

                struct_element.struct_members[member.name] = member_var;
                std::string member_debug = "Added member: " + member.name;
                debug_msg(DebugMsgId::ARRAY_DECL_DEBUG,
                          member_debug.c_str());

                // メンバー変数の直接アクセス用変数も作成
                std::string member_path = element_name + "." + member.name;
                Variable member_direct_var = member_var;
                member_direct_var.is_private_member = member.is_private;
                variable_manager_->getInterpreter()
                    ->current_scope()
                    .variables[member_path] = member_direct_var;
            }

            struct_element.is_assigned = true;

            // 現在のスコープに登録（StatementExecutorが使用するスコープ）
            variable_manager_->getInterpreter()
                ->current_scope()
                .variables[element_name] = struct_element;
            std::string element_debug = "Registered struct element: " + element_name;
            debug_msg(DebugMsgId::ARRAY_DECL_DEBUG,
                      element_debug.c_str());
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
        std::string dimension_error = "Dimension mismatch: literal=" +
                                      std::to_string(dimensions.size()) +
                                      ", declared=" +
                                      std::to_string(
                                          var.array_type_info.dimensions.size());
        debug_msg(DebugMsgId::TYPE_MISMATCH_ERROR,
                  dimension_error.c_str());
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
        ensure_numeric_storage(var, static_cast<size_t>(total_size), true,
                               elem_type);
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
        ensure_numeric_storage(var, static_cast<size_t>(total_size), true,
                               base_type);
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
            debug_msg(DebugMsgId::ARRAY_DECL_DEBUG,
                      ("Set multidim string element[" + std::to_string(flat_index) + "] = '" + node->str_value + "'").c_str());
        } else {
            // 数値の場合、型付き評価を使用
            TypedValue element_value = evaluate_expression_typed_safe(
                node, "array_element");

            if (!element_value.is_numeric()) {
                throw std::runtime_error(
                    "Array literal element is not numeric");
            }

            long double numeric_value = element_value.is_floating()
                                             ? element_value.as_quad()
                                             : static_cast<long double>(
                                                   element_value.as_numeric());

            if (!is_floating_type(base_type)) {
                int64_t coerced_value =
                    static_cast<int64_t>(numeric_value);
                if (var.is_unsigned && coerced_value < 0) {
                    std::string resolved_name = "<anonymous array>";
                    if (interpreter_) {
                        std::string candidate =
                            interpreter_->find_variable_name(&var);
                        if (!candidate.empty()) {
                            resolved_name = candidate;
                        }
                    }
                    DEBUG_WARN(
                        VARIABLE,
                        "Unsigned array %s literal element negative (%lld); clamping to 0",
                        resolved_name.c_str(),
                        static_cast<long long>(coerced_value));
                    coerced_value = 0;
                }
                numeric_value =
                    static_cast<long double>(coerced_value);
            }

            set_numeric_storage_value(
                var, static_cast<size_t>(flat_index), numeric_value, true,
                base_type);
        }
    }

    debug_msg(DebugMsgId::ARRAY_DECL_DEBUG,
              "Array declaration completed");
}

int64_t ArrayManager::getMultidimensionalArrayElement(
    const Variable &var, const std::vector<int64_t> &indices) {
    TypedValue typed =
        getMultidimensionalArrayElementTyped(var, indices);
    if (!typed.is_numeric()) {
        throw std::runtime_error(
            "Cannot get string array element as integer");
    }
    if (typed.is_floating()) {
        return static_cast<int64_t>(typed.as_quad());
    }
    return typed.as_numeric();
}

TypedValue ArrayManager::getMultidimensionalArrayElementTyped(
    const Variable &var, const std::vector<int64_t> &indices) {
    if (!var.is_multidimensional) {
        throw std::runtime_error("Variable is not a multidimensional array");
    }

    std::vector<int> int_indices;
    int_indices.reserve(indices.size());
    for (int64_t idx : indices) {
        int_indices.push_back(static_cast<int>(idx));
    }

    size_t flat_index = 0;
    if (!var.array_dimensions.empty()) {
        if (indices.size() != var.array_dimensions.size()) {
            throw std::runtime_error(
                "Dimension mismatch in struct member array access");
        }

        flat_index = 0;
        size_t multiplier = 1;
        for (int i = static_cast<int>(indices.size()) - 1; i >= 0; --i) {
            if (int_indices[i] < 0 ||
                int_indices[i] >= var.array_dimensions[i]) {
                throw std::runtime_error(
                    "Array index out of bounds in struct member access");
            }
            flat_index += static_cast<size_t>(int_indices[i]) * multiplier;
            multiplier *= static_cast<size_t>(var.array_dimensions[i]);
        }
    } else {
        flat_index = static_cast<size_t>(var.calculate_flat_index(int_indices));
    }

    TypeInfo base_type = resolve_base_type(var);

    if (base_type == TYPE_STRING) {
        if (flat_index >= var.multidim_array_strings.size()) {
            throw std::runtime_error("Array index out of bounds");
        }
        return TypedValue(
            var.multidim_array_strings[flat_index],
            InferredType(TYPE_STRING, type_info_to_string(TYPE_STRING)));
    }

    size_t storage_size =
        get_numeric_storage_size(var, true, base_type);
    if (flat_index >= storage_size) {
        throw std::runtime_error("Array index out of bounds");
    }

    long double numeric_value =
        get_numeric_storage_value(var, flat_index, true, base_type);
    return make_numeric_typed_value(numeric_value, base_type);
}

TypedValue ArrayManager::getArrayElementTyped(
    const Variable &var, const std::vector<int64_t> &indices) {
    if (!var.is_array) {
        throw std::runtime_error("Variable is not an array");
    }

    if (var.is_multidimensional) {
        return getMultidimensionalArrayElementTyped(var, indices);
    }

    if (indices.size() != 1) {
        throw std::runtime_error("Invalid index count for array element");
    }

    int index = static_cast<int>(indices[0]);
    if (index < 0) {
        throw std::runtime_error("Negative array index");
    }

    TypeInfo base_type = resolve_base_type(var);

    if (base_type == TYPE_STRING) {
        if (static_cast<size_t>(index) >= var.array_strings.size()) {
            throw std::runtime_error("Array index out of bounds");
        }
        return TypedValue(
            var.array_strings[static_cast<size_t>(index)],
            InferredType(TYPE_STRING, type_info_to_string(TYPE_STRING)));
    }

    size_t storage_size =
        get_numeric_storage_size(var, false, base_type);
    if (static_cast<size_t>(index) >= storage_size) {
        throw std::runtime_error("Array index out of bounds");
    }

    long double numeric_value = get_numeric_storage_value(
        var, static_cast<size_t>(index), false, base_type);
    return make_numeric_typed_value(numeric_value, base_type);
}

void ArrayManager::setMultidimensionalArrayElement(
    Variable &var, const std::vector<int64_t> &indices, int64_t value) {
    if (!var.is_multidimensional) {
        throw std::runtime_error("Variable is not a multidimensional array");
    }

    // const配列への書き込みチェック
    if (var.is_const && var.is_assigned) {
        throw std::runtime_error(
            "Cannot assign to const multidimensional array");
    }

    int64_t adjusted_value = value;
    if (var.is_unsigned && adjusted_value < 0) {
        std::string resolved_name = "<anonymous array>";
        if (interpreter_) {
            std::string candidate = interpreter_->find_variable_name(&var);
            if (!candidate.empty()) {
                resolved_name = candidate;
            }
        }
        DEBUG_WARN(
            VARIABLE,
            "Unsigned array %s element assignment with negative value (%lld); clamping to 0",
            resolved_name.c_str(), static_cast<long long>(adjusted_value));
        adjusted_value = 0;
    }

    std::vector<int> int_indices;
    for (int64_t idx : indices) {
        int_indices.push_back(static_cast<int>(idx));
    }

    // 構造体メンバーの場合は array_dimensions を使用してフラットインデックスを計算
    int flat_index;
    if (!var.array_dimensions.empty()) {
        // 構造体メンバーの場合
        if (indices.size() != var.array_dimensions.size()) {
            throw std::runtime_error("Dimension mismatch in struct member array access");
        }

        flat_index = 0;
        int multiplier = 1;

        // 最後の次元から計算（row-major order）
        for (int i = static_cast<int>(indices.size()) - 1; i >= 0; --i) {
            if (int_indices[i] < 0 || int_indices[i] >= static_cast<int>(var.array_dimensions[i])) {
                throw std::runtime_error("Array index out of bounds in struct member access");
            }
            flat_index += int_indices[i] * multiplier;
            multiplier *= static_cast<int>(var.array_dimensions[i]);
        }
        
        debug_msg(DebugMsgId::FLAT_INDEX_CALCULATED, flat_index);
    } else {
        // 通常の配列の場合
        flat_index = var.calculate_flat_index(int_indices);
    }

    TypeInfo base_type = resolve_base_type(var);
    if (base_type == TYPE_STRING) {
        throw std::runtime_error(
            "Cannot set string array element with integer value");
    }

    size_t storage_size = get_numeric_storage_size(var, true, base_type);
    if (static_cast<size_t>(flat_index) >= storage_size) {
        throw std::runtime_error("Array index out of bounds");
    }

    long double numeric_value = static_cast<long double>(adjusted_value);
    set_numeric_storage_value(var, static_cast<size_t>(flat_index),
                               numeric_value, true, base_type);
}

// float/double値での多次元配列要素設定（オーバーロード）
void ArrayManager::setMultidimensionalArrayElement(
    Variable &var, const std::vector<int64_t> &indices, double value) {
    if (!var.is_multidimensional) {
        throw std::runtime_error("Variable is not a multidimensional array");
    }

    // const配列への書き込みチェック
    if (var.is_const && var.is_assigned) {
        throw std::runtime_error(
            "Cannot assign to const multidimensional array");
    }

    std::vector<int> int_indices;
    for (int64_t idx : indices) {
        int_indices.push_back(static_cast<int>(idx));
    }

    // 構造体メンバーの場合は array_dimensions を使用してフラットインデックスを計算
    int flat_index;
    if (!var.array_dimensions.empty()) {
        // 構造体メンバーの場合
        if (indices.size() != var.array_dimensions.size()) {
            throw std::runtime_error("Dimension mismatch in struct member array access");
        }

        flat_index = 0;
        int multiplier = 1;

        // 最後の次元から計算（row-major order）
        for (int i = static_cast<int>(indices.size()) - 1; i >= 0; --i) {
            if (int_indices[i] < 0 || int_indices[i] >= static_cast<int>(var.array_dimensions[i])) {
                throw std::runtime_error("Array index out of bounds in struct member access");
            }
            flat_index += int_indices[i] * multiplier;
            multiplier *= static_cast<int>(var.array_dimensions[i]);
        }
        
        debug_msg(DebugMsgId::FLAT_INDEX_CALCULATED, flat_index);
    } else {
        // 通常の配列の場合
        flat_index = var.calculate_flat_index(int_indices);
    }

    TypeInfo base_type = resolve_base_type(var);
    if (base_type == TYPE_STRING) {
        throw std::runtime_error(
            "Cannot set string array element with numeric value");
    }

    size_t storage_size = get_numeric_storage_size(var, true, base_type);
    if (static_cast<size_t>(flat_index) >= storage_size) {
        throw std::runtime_error("Array index out of bounds");
    }

    long double numeric_value = static_cast<long double>(value);
    set_numeric_storage_value(var, static_cast<size_t>(flat_index),
                               numeric_value, true, base_type);
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

    // 構造体メンバーの場合は array_dimensions を使用してフラットインデックスを計算
    int flat_index;
    if (!var.array_dimensions.empty()) {
        // 構造体メンバーの場合
        if (indices.size() != var.array_dimensions.size()) {
            throw std::runtime_error("Dimension mismatch in struct member array access");
        }

        flat_index = 0;
        int multiplier = 1;

        // 最後の次元から計算（row-major order）
        for (int i = static_cast<int>(indices.size()) - 1; i >= 0; --i) {
            if (int_indices[i] < 0 || int_indices[i] >= static_cast<int>(var.array_dimensions[i])) {
                throw std::runtime_error("Array index out of bounds in struct member access");
            }
            flat_index += int_indices[i] * multiplier;
            multiplier *= static_cast<int>(var.array_dimensions[i]);
        }
    } else {
        // 通常の配列の場合
        flat_index = var.calculate_flat_index(int_indices);
    }

    if (var.array_type_info.base_type != TYPE_STRING) {
        throw std::runtime_error(
            "Cannot get non-string array element as string");
    }

    debug_msg(DebugMsgId::ARRAY_DECL_DEBUG,
              ("Getting multidim string element at flat_index=" + std::to_string(flat_index) + 
               ", multidim_array_strings.size()=" + std::to_string(var.multidim_array_strings.size())).c_str());
    
    if (flat_index < static_cast<int>(var.multidim_array_strings.size())) {
        debug_msg(DebugMsgId::ARRAY_DECL_DEBUG,
                  ("Returning string: '" + var.multidim_array_strings[flat_index] + "'").c_str());
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
    if (var.is_const && var.is_assigned) {
        throw std::runtime_error(
            "Cannot assign to const multidimensional string array");
    }

    std::vector<int> int_indices;
    for (int64_t idx : indices) {
        int_indices.push_back(static_cast<int>(idx));
    }

    // 構造体メンバーの場合は array_dimensions を使用してフラットインデックスを計算
    int flat_index;
    if (!var.array_dimensions.empty()) {
        // 構造体メンバーの場合
        if (indices.size() != var.array_dimensions.size()) {
            throw std::runtime_error("Dimension mismatch in struct member array access");
        }

        flat_index = 0;
        int multiplier = 1;

        // 最後の次元から計算（row-major order）
        for (int i = static_cast<int>(indices.size()) - 1; i >= 0; --i) {
            if (int_indices[i] < 0 || int_indices[i] >= static_cast<int>(var.array_dimensions[i])) {
                throw std::runtime_error("Array index out of bounds in struct member access");
            }
            flat_index += int_indices[i] * multiplier;
            multiplier *= static_cast<int>(var.array_dimensions[i]);
        }
    } else {
        // 通常の配列の場合
        flat_index = var.calculate_flat_index(int_indices);
    }

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
        ensure_numeric_storage(var, static_cast<size_t>(total_size),
                               var.is_multidimensional, base_type);
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
        ensure_numeric_storage(var, static_cast<size_t>(total_size), true,
                               array_info.base_type);
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
    // NOTE: このメソッドはprocessArrayDeclarationと一部機能が重複しています。
    // 将来的にはArrayProcessingServiceによる統一化が推奨されます。
    // 現在はstruct配列処理等の特殊ケースのために維持されています。
    Variable var;

    debug_msg(DebugMsgId::ARRAY_DECL_START, node->name.c_str());
    debug_msg(DebugMsgId::ARRAY_DIMENSIONS_COUNT,
              static_cast<int>(node->array_type_info.dimensions.size()));

    // struct配列のデバッグ情報
    if (node->type_info == TYPE_STRUCT) {
        debug_msg(DebugMsgId::ARRAY_DECL_DEBUG, "This is a struct array");
        debug_msg(DebugMsgId::ARRAY_DECL_DEBUG,
                  ("Struct type: " + node->type_name).c_str());
        if (node->array_size_expr) {
            debug_msg(DebugMsgId::ARRAY_DECL_DEBUG, "Has array_size_expr");
        }
    }

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
            ensure_numeric_storage(var, static_cast<size_t>(total_size), true,
                                   node->array_type_info.base_type);
        }

        // グローバルスコープに保存（AST_ARRAY_DECLはグローバル配列宣言のみ）
        variable_manager_->getInterpreter()
            ->global_scope.variables[node->name] = var;
        debug_msg(DebugMsgId::MULTIDIM_ARRAY_DECL_SUCCESS, node->name.c_str());

        // 初期化式がある場合は配列リテラル初期化を実行
        if (node->init_expr &&
            node->init_expr->node_type == ASTNodeType::AST_ARRAY_LITERAL) {
            debug_msg(DebugMsgId::ARRAY_DECL_DEBUG,
                      "Processing multidim array literal initialization");
            variable_manager_->getInterpreter()->assign_array_literal(
                node->name, node->init_expr.get());
        }
    } else if (node->array_type_info.dimensions.size() == 1 ||
               (node->type_info == TYPE_STRUCT && node->array_size_expr)) {
    // 単一次元配列の場合またはstruct配列の場合
    debug_msg(DebugMsgId::ARRAY_DECL_DEBUG,
          "Processing single-dimension or struct array");

        var.is_array = true;
        var.is_multidimensional = false;

        debug_msg(
            DebugMsgId::ARRAY_DECL_DEBUG,
            ("Array type_info: " + std::to_string(node->type_info)).c_str());
        debug_msg(DebugMsgId::ARRAY_DECL_DEBUG,
                  ("TYPE_STRUCT: " + std::to_string(TYPE_STRUCT)).c_str());

        // struct配列の場合の特別処理
        if (node->type_info == TYPE_STRUCT) {
            var.type = TYPE_STRUCT;
            var.is_struct = false; // 配列自体はstructではないが、要素がstruct
            var.struct_type_name = node->type_name; // struct型名を保存
        } else {
            var.type = static_cast<TypeInfo>(TYPE_ARRAY_BASE +
                                             node->array_type_info.base_type);
        }

        var.is_const = node->is_const;
        var.is_assigned = false;

        // サイズを取得（struct配列の場合はarray_size_exprから評価）
        int size;
        if (node->type_info == TYPE_STRUCT && node->array_size_expr) {
            size = evaluate_expression_safe(node->array_size_expr.get(),
                                            "struct_array_size");
        } else if (node->array_type_info.dimensions[0].size < 0) {
            // サイズが設定されていない場合は式から評価
            if (node->array_size_expr) {
                size = evaluate_expression_safe(node->array_size_expr.get(),
                                                "array_size");
            } else if (!node->array_dimensions.empty() && node->array_dimensions[0]) {
                size = evaluate_expression_safe(node->array_dimensions[0].get(),
                                                "array_dimension_0");
            } else {
                throw std::runtime_error("Array size could not be determined for " + node->name);
            }
        } else {
            size = node->array_type_info.dimensions[0].size;
        }

        var.array_size = size; // array_sizeを設定
        debug_msg(DebugMsgId::ARRAY_TOTAL_SIZE, size);

        // 単一次元配列の場合もarray_dimensionsを設定
        var.array_dimensions.clear();
        var.array_dimensions.push_back(size);
        
        if (debug_mode) {
            std::cerr << "[ARRAY_DEBUG] About to initialize storage for type: " << static_cast<int>(node->type_info) << std::endl;
        }

        // 配列用のストレージを初期化
        if (node->type_info == TYPE_STRUCT) {
            // struct配列の場合、各要素はstruct変数として管理
            // 各要素をstruct変数として初期化
            debug_msg(DebugMsgId::ARRAY_DECL_DEBUG,
                      "Initializing struct array elements");

            const StructDefinition *struct_def =
                variable_manager_->getInterpreter()->find_struct_definition(
                    variable_manager_->getInterpreter()->type_manager_->resolve_typedef(node->type_name));
            if (!struct_def) {
                throw std::runtime_error("Struct definition not found: " +
                                         node->type_name);
            }

            debug_msg(DebugMsgId::ARRAY_DECL_DEBUG,
                      ("Found struct definition: " + node->type_name).c_str());

            for (int i = 0; i < size; i++) {
                std::string element_name =
                    node->name + "[" + std::to_string(i) + "]";
                debug_msg(DebugMsgId::ARRAY_DECL_DEBUG,
                          ("Creating struct element: " + element_name).c_str());

                // struct要素変数を作成
                Variable struct_element(node->type_name);
                struct_element.is_struct = true;
                struct_element.struct_type_name = node->type_name;

                // メンバーを初期化
                for (const auto &member : struct_def->members) {
                    Variable member_var;
                    member_var.type = member.type;
                    member_var.is_assigned = false;
                    member_var.is_private_member = member.is_private;

                    if (member.type == TYPE_STRING) {
                        member_var.str_value = "";
                    } else {
                        member_var.value = 0;
                    }

                    struct_element.struct_members[member.name] = member_var;
                    debug_msg(DebugMsgId::ARRAY_DECL_DEBUG,
                              ("Added member: " + member.name).c_str());
                }

                struct_element.is_assigned = true;

                // グローバルスコープに登録
                variable_manager_->getInterpreter()
                    ->global_scope.variables[element_name] = struct_element;
                debug_msg(
                    DebugMsgId::ARRAY_DECL_DEBUG,
                    ("Registered struct element: " + element_name).c_str());
            }
            var.array_values.resize(size, 0); // プレースホルダー
        } else if (node->array_type_info.base_type == TYPE_STRING) {
            var.array_strings.resize(size, "");
        } else {
            if (debug_mode) {
                std::cerr << "[ARRAY_DEBUG] Calling ensure_numeric_storage, size=" << size << std::endl;
            }
            ensure_numeric_storage(var, static_cast<size_t>(size), false,
                                   node->array_type_info.base_type);
            if (debug_mode) {
                std::cerr << "[ARRAY_DEBUG] ensure_numeric_storage completed" << std::endl;
            }
        }

        if (debug_mode) {
            std::cerr << "[ARRAY_DEBUG] About to save to global_scope.variables" << std::endl;
        }
        
        // グローバルスコープに保存（AST_ARRAY_DECLはグローバル配列宣言のみ）
        variable_manager_->getInterpreter()
            ->global_scope.variables[node->name] = var;
            
        if (debug_mode) {
            std::cerr << "[ARRAY_DEBUG] Saved to global_scope.variables" << std::endl;
        }
        debug_msg(DebugMsgId::ARRAY_DECL_SUCCESS, node->name.c_str());

        // 初期化式がある場合は配列リテラル初期化を実行
        if (node->init_expr &&
            node->init_expr->node_type == ASTNodeType::AST_ARRAY_LITERAL) {
            debug_msg(DebugMsgId::ARRAY_DECL_DEBUG,
                      "Processing array literal initialization");
            variable_manager_->getInterpreter()->assign_array_literal(
                node->name, node->init_expr.get());
        }
    } else {
        // 配列情報が不正または未サポートの場合
        debug_msg(DebugMsgId::ARRAY_DECL_DEBUG,
                  "Unsupported array type or missing dimensions");
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
        dest.multidim_array_strings = src.multidim_array_strings;
        dest.multidim_array_values = src.multidim_array_values;
        dest.multidim_array_float_values = src.multidim_array_float_values;
        dest.multidim_array_double_values = src.multidim_array_double_values;
        dest.multidim_array_quad_values = src.multidim_array_quad_values;
    } else {
        dest.array_strings = src.array_strings;
        dest.array_values = src.array_values;
        dest.array_float_values = src.array_float_values;
        dest.array_double_values = src.array_double_values;
        dest.array_quad_values = src.array_quad_values;
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
        dest.is_unsigned = src.is_unsigned;

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
            ensure_numeric_storage(dest, slice_dimensions[0], false,
                                   src.array_type_info.base_type);
            for (int i = 0; i < slice_dimensions[0]; i++) {
                std::vector<int64_t> full_indices = slice_indices;
                full_indices.push_back(i);
                TypedValue typed_value =
                    getMultidimensionalArrayElementTyped(src, full_indices);
                if (!typed_value.is_numeric()) {
                    throw std::runtime_error(
                        "Expected numeric value in array slice");
                }
                long double numeric_value = typed_value.is_floating()
                                                ? typed_value.as_quad()
                                                : static_cast<long double>(
                                                      typed_value
                                                          .as_numeric());
                set_numeric_storage_value(dest, i, numeric_value, false,
                                           src.array_type_info.base_type);
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

    // 基本型が一致する場合は許可
    if (dest_base == src_base) {
        return true;
    }

    // typedef配列の場合を考慮した緩和的なチェック
    // 両方が配列で、サイズが一致していれば typedef の可能性として許可
    if (dest.is_array && src.is_array && dest.array_size == src.array_size &&
        dest.array_dimensions == src.array_dimensions) {
        return true;
    }

    return false;
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

// DRY効率化: 統一式評価メソッド
int64_t ArrayManager::evaluate_expression_safe(const ASTNode *node,
                                               const std::string &context) {
    if (!node) {
        throw std::runtime_error(
            "Null expression node" +
            (context.empty() ? "" : " in array " + context));
    }

    try {
        // ExpressionServiceが利用可能な場合は統一サービス使用
        if (interpreter_ && interpreter_->get_expression_service()) {
            return interpreter_->get_expression_service()->evaluate_safe(
                node, "array_" + context, [&](const std::string &) {
                    return expression_evaluator_->evaluate_expression(node);
                });
        }

        // フォールバック: 従来の評価器使用
        return expression_evaluator_->evaluate_expression(node);
    } catch (const std::exception &e) {
        throw std::runtime_error("Array expression evaluation failed" +
                                 (context.empty() ? "" : " in " + context) +
                                 ": " + e.what());
    }
}

TypedValue
ArrayManager::evaluate_expression_typed_safe(const ASTNode *node,
                                             const std::string &context) {
    if (!node) {
        throw std::runtime_error(
            "Null expression node" +
            (context.empty() ? "" : " in array " + context));
    }

    if (!expression_evaluator_) {
        throw std::runtime_error(
            "Expression evaluator not available for typed evaluation" +
            (context.empty() ? "" : " in " + context));
    }

    try {
        return expression_evaluator_->evaluate_typed_expression(node);
    } catch (const ReturnException &) {
        throw;
    } catch (const std::exception &e) {
        throw std::runtime_error("Typed array expression evaluation failed" +
                                 (context.empty() ? "" : " in " + context) +
                                 ": " + e.what());
    }
}
