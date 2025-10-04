#include "managers/common_operations.h"
#include "../../../common/debug_messages.h"
#include "../services/debug_service.h"
#include "evaluator/expression_evaluator.h"
#include "services/expression_service.h" // DRY効率化: 統一式評価サービス
#include "core/interpreter.h"
#include "managers/type_manager.h"
#include <iostream>
#include <stdexcept>

CommonOperations::CommonOperations(Interpreter *interpreter)
    : interpreter_(interpreter),
      expression_evaluator_(interpreter->get_expression_evaluator()) {}

CommonOperations::ArrayLiteralResult
CommonOperations::parse_array_literal(const ASTNode *literal_node) {
    ArrayLiteralResult result = {};

    if (!literal_node ||
        literal_node->node_type != ASTNodeType::AST_ARRAY_LITERAL) {
        throw std::runtime_error("Invalid array literal");
    }

    // 配列リテラルが空の場合
    if (literal_node->arguments.empty()) {
        result.size = 0;
        result.element_type = TYPE_UNKNOWN;
        result.is_string_array = false;
        return result;
    }

    // ネストした配列リテラルを再帰的にフラット化
    std::vector<int64_t> flattened_values;
    std::vector<std::string> flattened_strings;
    std::vector<double> flattened_floats;
    bool is_string_array = false;
    bool is_float_array = false;
    
    // 最初の要素から型を判定
    if (!literal_node->arguments.empty()) {
        result.element_type = infer_array_element_type(literal_node);
        is_string_array = (result.element_type == TYPE_STRING);
        is_float_array = (result.element_type == TYPE_FLOAT || 
                         result.element_type == TYPE_DOUBLE || 
                         result.element_type == TYPE_QUAD);
    }
    
    // 再帰的にフラット化
    flatten_array_literal(literal_node, flattened_values, flattened_strings, 
                         flattened_floats, is_string_array, is_float_array);
    
    result.is_string_array = is_string_array;
    result.is_float_array = is_float_array;
    if (is_string_array) {
        result.string_values = flattened_strings;
        result.size = flattened_strings.size();
    } else if (is_float_array) {
        result.float_values = flattened_floats;
        result.size = flattened_floats.size();
    } else {
        result.int_values = flattened_values;
        result.size = flattened_values.size();
    }

    return result;
}

void CommonOperations::assign_array_literal_to_variable(
    Variable *var, const ArrayLiteralResult &result,
    const std::string &var_name_hint) {
    if (!var) {
        throw std::runtime_error("Null variable pointer");
    }

    if (interpreter_->is_debug_mode()) {
        debug_print("ARRAY_CLAMP_DEBUG: assigning literal to %s, is_unsigned=%d\n",
                    var_name_hint.empty() ? "<anonymous>" : var_name_hint.c_str(),
                    var->is_unsigned ? 1 : 0);
    }

    if (!var->is_array) {
        throw std::runtime_error("Variable is not declared as array");
    }

    // const配列への代入チェック
    check_const_assignment(var, "array");

    // サイズチェック
    if (result.size > static_cast<size_t>(var->array_size)) {
        throw std::runtime_error("Array literal has too many elements: " +
                                 std::to_string(result.size) + " > " +
                                 std::to_string(var->array_size));
    }

    // 配列に代入
    if (result.is_string_array) {
        // デバッグ出力を追加
        debug_msg(DebugMsgId::ARRAY_DECL_DEBUG, 
                  ("Assigning string array with " + std::to_string(result.string_values.size()) + " elements").c_str());
        for (size_t i = 0; i < result.string_values.size() && i < 10; i++) {
            debug_msg(DebugMsgId::ARRAY_DECL_DEBUG, 
                      ("String element [" + std::to_string(i) + "] = '" + result.string_values[i] + "'").c_str());
        }
        
        // 多次元配列の場合は適切なストレージを使用
        if (var->is_multidimensional && var->array_dimensions.size() > 1) {
            var->multidim_array_strings = result.string_values;
            // 残りの要素を空文字で埋める
            var->multidim_array_strings.resize(var->array_size, "");
            var->multidim_array_values.clear();
        } else {
            var->array_strings = result.string_values;
            // 残りの要素を空文字で埋める
            var->array_strings.resize(var->array_size, "");
            var->array_values.clear();
        }

        // 型を適切に設定
        if (var->type >= TYPE_ARRAY_BASE) {
            var->type = static_cast<TypeInfo>(TYPE_ARRAY_BASE + TYPE_STRING);
        } else {
            var->type = TYPE_STRING;
        }
        if (!var->is_multidimensional || var->array_dimensions.size() <= 1) {
            var->array_dimensions.clear();
            var->array_dimensions.push_back(var->array_size);
        }

        var->is_assigned = true;
        return;
    }

    // float/double配列の処理
    if (result.is_float_array) {
        TypeInfo base_type = (var->type >= TYPE_ARRAY_BASE) 
                            ? static_cast<TypeInfo>(var->type - TYPE_ARRAY_BASE)
                            : var->type;
        
        // int64_t表現も生成（後方互換性のため）
        std::vector<int64_t> int_repr;
        for (double val : result.float_values) {
            int_repr.push_back(static_cast<int64_t>(val));
        }
        
        if (var->is_multidimensional && var->array_dimensions.size() > 1) {
            // 多次元配列
            var->multidim_array_values = int_repr;
            var->multidim_array_values.resize(var->array_size, 0);
            
            if (base_type == TYPE_FLOAT) {
                var->multidim_array_float_values.clear();
                for (double val : result.float_values) {
                    var->multidim_array_float_values.push_back(static_cast<float>(val));
                }
                var->multidim_array_float_values.resize(var->array_size, 0.0f);
                var->multidim_array_double_values.clear();
                var->multidim_array_quad_values.clear();
            } else if (base_type == TYPE_DOUBLE) {
                var->multidim_array_double_values = result.float_values;
                var->multidim_array_double_values.resize(var->array_size, 0.0);
                var->multidim_array_float_values.clear();
                var->multidim_array_quad_values.clear();
            } else { // TYPE_QUAD
                var->multidim_array_quad_values.clear();
                for (double val : result.float_values) {
                    var->multidim_array_quad_values.push_back(static_cast<long double>(val));
                }
                var->multidim_array_quad_values.resize(var->array_size, 0.0L);
                var->multidim_array_float_values.clear();
                var->multidim_array_double_values.clear();
            }
            
            var->array_values = int_repr;
            var->array_values.resize(var->array_size, 0);
            var->multidim_array_strings.clear();
        } else {
            // 1次元配列
            var->array_values = int_repr;
            var->array_values.resize(var->array_size, 0);
            
            if (base_type == TYPE_FLOAT) {
                var->array_float_values.clear();
                for (double val : result.float_values) {
                    var->array_float_values.push_back(static_cast<float>(val));
                }
                var->array_float_values.resize(var->array_size, 0.0f);
                var->array_double_values.clear();
                var->array_quad_values.clear();
            } else if (base_type == TYPE_DOUBLE) {
                var->array_double_values = result.float_values;
                var->array_double_values.resize(var->array_size, 0.0);
                var->array_float_values.clear();
                var->array_quad_values.clear();
            } else { // TYPE_QUAD
                var->array_quad_values.clear();
                for (double val : result.float_values) {
                    var->array_quad_values.push_back(static_cast<long double>(val));
                }
                var->array_quad_values.resize(var->array_size, 0.0L);
                var->array_float_values.clear();
                var->array_double_values.clear();
            }
            
            var->array_strings.clear();
        }
        
        // 型を適切に設定
        if (var->type < TYPE_ARRAY_BASE) {
            var->type = static_cast<TypeInfo>(TYPE_ARRAY_BASE + base_type);
        }
        
        if (!var->is_multidimensional || var->array_dimensions.size() <= 1) {
            var->array_dimensions.clear();
            var->array_dimensions.push_back(var->array_size);
        }
        
        var->is_assigned = true;
        return;
    }

    std::vector<int64_t> adjusted_values = result.int_values;
    if (var->is_unsigned) {
        const std::string resolved_name =
            var_name_hint.empty() ? std::string("<anonymous array>")
                                  : var_name_hint;
        for (auto &value : adjusted_values) {
            if (value < 0) {
                DEBUG_WARN(
                    VARIABLE,
                    "Unsigned array %s initialized with negative element (%lld); clamping to 0",
                    resolved_name.c_str(), static_cast<long long>(value));
                value = 0;
            }
        }
    }

    if (var->is_multidimensional && var->array_dimensions.size() > 1) {
        var->multidim_array_values = adjusted_values;
        var->multidim_array_values.resize(var->array_size, 0);
        var->array_values = adjusted_values;
        var->array_values.resize(var->array_size, 0);
        var->multidim_array_strings.clear();
    } else {
        var->array_values = adjusted_values;
        var->array_values.resize(var->array_size, 0);
        var->array_strings.clear();
    }

    if (var->type < TYPE_ARRAY_BASE) {
        var->type = static_cast<TypeInfo>(TYPE_ARRAY_BASE + TYPE_INT);
    }

    if (!var->is_multidimensional || var->array_dimensions.size() <= 1) {
        var->array_dimensions.clear();
        var->array_dimensions.push_back(var->array_size);
    }

    var->is_assigned = true;
}

void CommonOperations::assign_array_element_safe(Variable *var, int64_t index,
                                                 int64_t value,
                                                 const std::string &var_name) {
    if (!var) {
        throw std::runtime_error("Variable not found: " + var_name);
    }

    if (!var->is_array) {
        throw std::runtime_error("Variable is not an array: " + var_name);
    }

    check_const_assignment(var, var_name);
    check_array_bounds(var, index, var_name);

    int64_t adjusted_value = value;
    if (var->is_unsigned && adjusted_value < 0) {
        const std::string resolved_name =
            var_name.empty() ? std::string("<anonymous array>") : var_name;
        DEBUG_WARN(
            VARIABLE,
            "Unsigned array %s element assignment with negative value (%lld); clamping to 0",
            resolved_name.c_str(), static_cast<long long>(adjusted_value));
        adjusted_value = 0;
    }

    // 型チェック（配列の要素型に対して）
    TypeInfo elem_type =
        (var->type >= TYPE_ARRAY_BASE)
            ? static_cast<TypeInfo>(var->type - TYPE_ARRAY_BASE)
            : var->type;
    interpreter_->get_type_manager()->check_type_range(elem_type,
                                                       adjusted_value,
                                                       var_name,
                                                       var->is_unsigned);

    var->array_values[index] = adjusted_value;
    debug_array_operation("assign_element", var_name, index, adjusted_value);
}

void CommonOperations::assign_array_element_safe(Variable *var, int64_t index,
                                                 const std::string &value,
                                                 const std::string &var_name) {
    if (!var) {
        throw std::runtime_error("Variable not found: " + var_name);
    }

    if (!var->is_array) {
        throw std::runtime_error("Variable is not an array: " + var_name);
    }

    check_const_assignment(var, var_name);
    check_array_bounds(var, index, var_name);

    // 文字列配列の場合
    if (var->array_strings.size() > static_cast<size_t>(index)) {
        var->array_strings[index] = value;
    } else {
        throw std::runtime_error("String array index out of bounds: " +
                                 var_name);
    }

    debug_array_operation("assign_string_element", var_name, index, 0);
}

int64_t CommonOperations::evaluate_expression_safe(const ASTNode *node,
                                                   const std::string &context) {
    if (!node) {
        throw std::runtime_error("Null expression node" +
                                 (context.empty() ? "" : " in " + context));
    }

    try {
        return expression_evaluator_->evaluate_expression(node);
    } catch (const std::exception &e) {
        throw std::runtime_error("Expression evaluation failed" +
                                 (context.empty() ? "" : " in " + context) +
                                 ": " + e.what());
    }
}

void CommonOperations::check_type_compatibility(TypeInfo expected,
                                                TypeInfo actual,
                                                const std::string &context) {
    if (expected != actual && expected != TYPE_UNKNOWN &&
        actual != TYPE_UNKNOWN) {
        throw std::runtime_error("Type mismatch" +
                                 (context.empty() ? "" : " in " + context) +
                                 ": expected " + type_info_to_string(expected) +
                                 ", got " + type_info_to_string(actual));
    }
}

void CommonOperations::check_array_bounds(const Variable *var, int64_t index,
                                          const std::string &var_name) {
    // 統一チェック: 従来のチェックロジックを使用
    if (index < 0 || index >= var->array_size) {
        std::cerr << "ARRAY_BOUNDS_DEBUG: var=" << var_name
                  << " index=" << index
                  << " size=" << var->array_size << std::endl;
        throw std::runtime_error("Array index out of bounds for '" + var_name +
                                 "': " + std::to_string(index) +
                                 " (valid range: 0-" +
                                 std::to_string(var->array_size - 1) + ")");
    }
}

void CommonOperations::check_const_assignment(const Variable *var,
                                              const std::string &var_name) {
    if (var->is_const && var->is_assigned) {
        throw std::runtime_error("Cannot assign to const variable: " +
                                 var_name);
    }
}

void CommonOperations::initialize_array_variable(Variable *var,
                                                 TypeInfo base_type, int size,
                                                 bool is_string_array) {
    if (!var) {
        throw std::runtime_error("Null variable pointer");
    }

    var->is_array = true;
    var->array_size = size;
    var->type = static_cast<TypeInfo>(TYPE_ARRAY_BASE + base_type);

    if (is_string_array) {
        var->array_strings.resize(size, "");
        var->array_values.clear();
    } else {
        var->array_values.resize(size, 0);
        var->array_strings.clear();
    }

    var->array_dimensions.clear();
    var->array_dimensions.push_back(size);
    var->is_assigned = false;
}

void CommonOperations::debug_array_operation(const std::string &operation,
                                             const std::string &var_name,
                                             int64_t index, int64_t value) {
    if (interpreter_->is_debug_mode()) {
        if (index >= 0) {
            debug_msg(DebugMsgId::ARRAY_ELEMENT_ASSIGN_DEBUG, var_name.c_str(),
                      index, value);
        } else {
            std::string operation_info = operation + " for " + var_name;
            debug_msg(DebugMsgId::ARRAY_DECL_DEBUG,
                      operation_info.c_str());
        }
    }
}

// 内部ヘルパー関数の実装
TypeInfo
CommonOperations::infer_array_element_type(const ASTNode *literal_node) {
    if (!literal_node || literal_node->arguments.empty()) {
        return TYPE_UNKNOWN;
    }

    const ASTNode *first_element = literal_node->arguments[0].get();

    // 多次元配列の場合、再帰的に最深レベルの要素まで辿る
    while (first_element && first_element->node_type == ASTNodeType::AST_ARRAY_LITERAL) {
        if (first_element->arguments.empty()) {
            return TYPE_UNKNOWN;
        }
        first_element = first_element->arguments[0].get();
    }

    if (!first_element) {
        return TYPE_UNKNOWN;
    }

    switch (first_element->node_type) {
    case ASTNodeType::AST_STRING_LITERAL:
        return TYPE_STRING;
    case ASTNodeType::AST_NUMBER:
        return TYPE_INT;
    default:
        // 複雑な式の場合はINTと仮定（評価後に適切な型が決まる）
        return TYPE_INT;
    }
}

void CommonOperations::validate_array_literal_consistency(
    const ASTNode *literal_node) {
    if (!literal_node || literal_node->arguments.size() <= 1) {
        return; // 空配列または単一要素は常に一貫している
    }

    ASTNodeType first_type = literal_node->arguments[0]->node_type;

    for (size_t i = 1; i < literal_node->arguments.size(); i++) {
        ASTNodeType current_type = literal_node->arguments[i]->node_type;

        // 文字列リテラルと他の型の混在をチェック
        bool first_is_string = (first_type == ASTNodeType::AST_STRING_LITERAL);
        bool current_is_string =
            (current_type == ASTNodeType::AST_STRING_LITERAL);

        if (first_is_string != current_is_string) {
            throw std::runtime_error(
                "Mixed string and non-string elements in array literal");
        }
    }
}

void CommonOperations::flatten_array_literal(const ASTNode *literal_node,
                                              std::vector<int64_t> &flattened_values,
                                              std::vector<std::string> &flattened_strings,
                                              std::vector<double> &flattened_floats,
                                              bool is_string_array,
                                              bool is_float_array) {
    if (!literal_node || literal_node->node_type != ASTNodeType::AST_ARRAY_LITERAL) {
        return;
    }
    
    debug_msg(DebugMsgId::ARRAY_LITERAL_INIT_PROCESSING, 
              ("flatten_array_literal: processing " + std::to_string(literal_node->arguments.size()) + " elements").c_str());
    
    for (size_t i = 0; i < literal_node->arguments.size(); i++) {
        const auto &element = literal_node->arguments[i];
        if (element->node_type == ASTNodeType::AST_ARRAY_LITERAL) {
            // 再帰的にネストした配列リテラルを処理
            flatten_array_literal(element.get(), flattened_values, flattened_strings, 
                                flattened_floats, is_string_array, is_float_array);
        } else if (is_string_array && element->node_type == ASTNodeType::AST_STRING_LITERAL) {
            // 文字列要素
            flattened_strings.push_back(element->str_value);
        } else if (is_float_array && element->node_type != ASTNodeType::AST_STRING_LITERAL) {
            // float/double要素 - TypedValueを使用して正しい値を取得
            TypedValue typed_val = expression_evaluator_->evaluate_typed_expression(element.get());
            double float_val = typed_val.as_double();
            flattened_floats.push_back(float_val);
        } else if (!is_string_array && !is_float_array && element->node_type != ASTNodeType::AST_STRING_LITERAL) {
            // 整数要素
            if (interpreter_->is_debug_mode()) {
                std::cerr << "[ARRAY_LITERAL_DEBUG] Element[" << i << "] node_type: " 
                         << static_cast<int>(element->node_type) << std::endl;
                if (element->node_type == ASTNodeType::AST_NUMBER) {
                    std::cerr << "[ARRAY_LITERAL_DEBUG] NUMBER node int_value: " << element->int_value << std::endl;
                }
            }
            debug_msg(DebugMsgId::ARRAY_LITERAL_INIT_PROCESSING,
                      ("Evaluating array element[" + std::to_string(i) + "], node_type: " + std::to_string(static_cast<int>(element->node_type))).c_str());
            int64_t value = evaluate_expression_safe(element.get(), "array literal element");
            if (interpreter_->is_debug_mode()) {
                std::cerr << "[ARRAY_LITERAL_DEBUG] Element[" << i << "] evaluated to: " << value << std::endl;
            }
            debug_msg(DebugMsgId::ARRAY_LITERAL_INIT_PROCESSING,
                      ("Array element[" + std::to_string(i) + "] evaluated to: " + std::to_string(value)).c_str());
            flattened_values.push_back(value);
        } else {
            throw std::runtime_error("Type mismatch in array literal");
        }
    }
}
