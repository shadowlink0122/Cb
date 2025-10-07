#include "expression_assignment.h"
#include "../../../common/debug.h"
#include "../../../common/utf8_utils.h"
#include "core/interpreter.h"
#include "core/type_inference.h"
#include <stdexcept>

namespace AssignmentHelpers {

int64_t evaluate_assignment(
    const ASTNode *node, Interpreter &interpreter,
    std::function<int64_t(const ASTNode *)> evaluate_expression_func,
    std::function<TypedValue(const ASTNode *)> evaluate_typed_expression_func) {
    // 代入式を評価し、代入された値を返す

    debug_msg(DebugMsgId::EXPR_EVAL_BINARY_OP, "Processing AST_ASSIGN");

    // 右辺が配列リテラルの場合は特別処理
    if (node->right &&
        node->right->node_type == ASTNodeType::AST_ARRAY_LITERAL) {
        debug_msg(DebugMsgId::EXPR_EVAL_BINARY_OP,
                  "Right side is array literal");
        // 配列リテラル代入処理
        if (node->left->node_type == ASTNodeType::AST_VARIABLE) {
            std::string var_name = node->left->name;
            std::string debug_text = "Array literal assignment to: " + var_name;
            debug_msg(DebugMsgId::EXPR_EVAL_BINARY_OP, debug_text.c_str());
            interpreter.assign_array_literal(var_name, node->right.get());
            return 0; // 配列代入の戻り値は0
        } else {
            throw std::runtime_error(
                "Array literal can only be assigned to variables");
        }
    }

    // 右辺を型付き評価（配列・構造体戻り値を考慮）
    TypedValue right_value(static_cast<int64_t>(0),
                           InferredType(TYPE_INT, "int"));
    bool has_typed_value = false;
    auto return_to_typed = [&](const ReturnException &ret) -> TypedValue {
        if (ret.type == TYPE_STRING) {
            return TypedValue(ret.str_value,
                              InferredType(TYPE_STRING, "string"));
        }
        if (ret.type == TYPE_FLOAT) {
            return TypedValue(ret.double_value,
                              InferredType(TYPE_FLOAT, "float"));
        }
        if (ret.type == TYPE_DOUBLE) {
            return TypedValue(ret.double_value,
                              InferredType(TYPE_DOUBLE, "double"));
        }
        if (ret.type == TYPE_QUAD) {
            return TypedValue(ret.quad_value, InferredType(TYPE_QUAD, "quad"));
        }
        TypeInfo resolved = ret.type != TYPE_UNKNOWN ? ret.type : TYPE_INT;
        return TypedValue(ret.value, InferredType(resolved, ""));
    };

    try {
        right_value = evaluate_typed_expression_func(node->right.get());
        has_typed_value = true;
    } catch (const ReturnException &ret) {
        if (ret.is_array) {
            std::string var_name;
            if (node->left->node_type == ASTNodeType::AST_VARIABLE) {
                var_name = node->left->name;
            } else {
                var_name = node->name;
            }
            interpreter.assign_array_from_return(var_name, ret);
            return 0;
        }

        if (ret.is_struct) {
            std::string var_name;
            if (!node->name.empty()) {
                var_name = node->name;
            } else if (node->left &&
                       node->left->node_type == ASTNodeType::AST_VARIABLE) {
                var_name = node->left->name;
            } else {
                throw;
            }

            interpreter.current_scope().variables[var_name] = ret.struct_value;
            Variable &assigned_var =
                interpreter.current_scope().variables[var_name];
            assigned_var.is_assigned = true;

            for (const auto &member : ret.struct_value.struct_members) {
                std::string member_path = var_name + "." + member.first;
                Variable *member_var = interpreter.find_variable(member_path);
                if (member_var) {
                    *member_var = member.second;
                    member_var->is_assigned = member.second.is_assigned;
                }
            }
            return 0;
        }

        right_value = return_to_typed(ret);
        has_typed_value = true;
    }

    auto assign_typed = [&](const std::string &target_name,
                            const TypedValue &value, TypeInfo type_hint) {
        interpreter.assign_variable(target_name, value, type_hint, false);
        if (value.is_numeric()) {
            return value.as_numeric();
        }
        if (value.is_string()) {
            return static_cast<int64_t>(0);
        }
        return static_cast<int64_t>(0);
    };

    // 代入先の処理
    if (!has_typed_value) {
        return 0;
    }

    if (node->left->node_type == ASTNodeType::AST_ARRAY_REF) {
        // 配列要素への代入
        std::string var_name;
        if (node->left->left &&
            node->left->left->node_type == ASTNodeType::AST_VARIABLE) {
            var_name = node->left->left->name;
        } else if (!node->left->name.empty()) {
            var_name = node->left->name;
        } else {
            throw std::runtime_error("Invalid array reference in assignment");
        }

        int64_t index_value =
            evaluate_expression_func(node->left->array_index.get());
        if (right_value.is_string()) {
            std::string string_value = right_value.string_value;
            std::string replacement;
            if (!string_value.empty()) {
                replacement = utf8_utils::utf8_char_at(string_value, 0);
                if (replacement.empty()) {
                    replacement = string_value.substr(0, 1);
                }
            } else {
                replacement = std::string();
            }
            interpreter.assign_string_element(
                var_name, static_cast<int>(index_value), replacement);
        } else {
            interpreter.assign_array_element(var_name,
                                             static_cast<int>(index_value),
                                             right_value.as_numeric());
        }
    } else {
        // 通常の変数への代入
        std::string var_name;
        if (!node->name.empty()) {
            var_name = node->name;
        } else if (node->left &&
                   node->left->node_type == ASTNodeType::AST_VARIABLE) {
            var_name = node->left->name;
        } else {
            throw std::runtime_error("Invalid assignment target in evaluator");
        }
        assign_typed(var_name, right_value, node->type_info);
    }

    return right_value.is_numeric() ? right_value.as_numeric() : 0;
}

} // namespace AssignmentHelpers
