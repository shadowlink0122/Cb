#include "managers/common_operations.h"
#include "../../../common/debug_messages.h"
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

    // 最初の要素から型を推測
    result.element_type = infer_array_element_type(literal_node);
    result.is_string_array = (result.element_type == TYPE_STRING);
    result.size = literal_node->arguments.size();

    // 一貫性チェック
    validate_array_literal_consistency(literal_node);

    // 要素を解析
    if (result.is_string_array) {
        result.string_values.reserve(result.size);
        for (const auto &element : literal_node->arguments) {
            if (element->node_type == ASTNodeType::AST_STRING_LITERAL) {
                result.string_values.push_back(element->str_value);
            } else {
                throw std::runtime_error(
                    "Type mismatch in string array literal");
            }
        }
    } else {
        result.int_values.reserve(result.size);
        for (const auto &element : literal_node->arguments) {
            if (element->node_type == ASTNodeType::AST_STRING_LITERAL) {
                throw std::runtime_error(
                    "Type mismatch in numeric array literal");
            }
            int64_t value = evaluate_expression_safe(element.get(),
                                                     "array literal element");
            result.int_values.push_back(value);
        }
    }

    return result;
}

void CommonOperations::assign_array_literal_to_variable(
    Variable *var, const ArrayLiteralResult &result) {
    if (!var) {
        throw std::runtime_error("Null variable pointer");
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
        var->array_strings = result.string_values;
        // 残りの要素を空文字で埋める
        var->array_strings.resize(var->array_size, "");
        var->array_values.clear();

        // 型を適切に設定
        if (var->type >= TYPE_ARRAY_BASE) {
            var->type = static_cast<TypeInfo>(TYPE_ARRAY_BASE + TYPE_STRING);
        } else {
            var->type = TYPE_STRING;
        }
    } else {
        var->array_values = result.int_values;
        // 残りの要素を0で埋める
        var->array_values.resize(var->array_size, 0);
        var->array_strings.clear();

        // 型を適切に設定
        if (var->type < TYPE_ARRAY_BASE) {
            var->type = static_cast<TypeInfo>(TYPE_ARRAY_BASE + TYPE_INT);
        }
    }

    // 次元情報を設定
    var->array_dimensions.clear();
    var->array_dimensions.push_back(var->array_size);

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

    // 型チェック（配列の要素型に対して）
    TypeInfo elem_type =
        (var->type >= TYPE_ARRAY_BASE)
            ? static_cast<TypeInfo>(var->type - TYPE_ARRAY_BASE)
            : var->type;
    interpreter_->get_type_manager()->check_type_range(elem_type, value,
                                                       var_name);

    var->array_values[index] = value;
    debug_array_operation("assign_element", var_name, index, value);
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
        throw std::runtime_error("Array index out of bounds for '" + var_name +
                                 "': " + std::to_string(index) +
                                 " (valid range: 0-" +
                                 std::to_string(var->array_size - 1) + ")");
    }
}

void CommonOperations::check_const_assignment(const Variable *var,
                                              const std::string &var_name) {
    if (var->is_const) {
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
            debug_msg(DebugMsgId::ARRAY_DECL_DEBUG,
                      (operation + " for " + var_name).c_str());
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
