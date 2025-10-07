#include "expression_helpers.h"
#include "../core/interpreter.h"
#include "../../../common/debug.h"
#include <stdexcept>

// ============================================================================
// 式評価のヘルパー関数群の実装
// ============================================================================
// ExpressionEvaluatorから分離されたヘルパーメソッド群
// Tier 2リファクタリングで抽出されたメソッドをここに集約
// ============================================================================

namespace ExpressionHelpers {

// ============================================================================
// 二項演算のヘルパー実装
// ============================================================================

// 算術演算（+, -, *, /, %）の評価
int64_t evaluate_arithmetic_binary(const std::string& op, int64_t left, int64_t right) {
    if (op == "+") {
        return left + right;
    } else if (op == "-") {
        return left - right;
    } else if (op == "*") {
        return left * right;
    } else if (op == "/") {
        if (right == 0) {
            error_msg(DebugMsgId::ZERO_DIVISION_ERROR);
            throw std::runtime_error("Division by zero");
        }
        return left / right;
    } else if (op == "%") {
        if (right == 0) {
            error_msg(DebugMsgId::ZERO_DIVISION_ERROR);
            throw std::runtime_error("Modulo by zero");
        }
        return left % right;
    }
    throw std::runtime_error("Unknown arithmetic operator: " + op);
}

// 比較演算（<, >, <=, >=, ==, !=）の評価
int64_t evaluate_comparison_binary(const std::string& op, int64_t left, int64_t right) {
    if (op == "==") {
        return (left == right) ? 1 : 0;
    } else if (op == "!=") {
        return (left != right) ? 1 : 0;
    } else if (op == "<") {
        return (left < right) ? 1 : 0;
    } else if (op == ">") {
        return (left > right) ? 1 : 0;
    } else if (op == "<=") {
        return (left <= right) ? 1 : 0;
    } else if (op == ">=") {
        return (left >= right) ? 1 : 0;
    }
    throw std::runtime_error("Unknown comparison operator: " + op);
}

// 論理演算（&&, ||）の評価
int64_t evaluate_logical_binary(const std::string& op, int64_t left, int64_t right) {
    if (op == "&&") {
        return (left && right) ? 1 : 0;
    } else if (op == "||") {
        return (left || right) ? 1 : 0;
    }
    throw std::runtime_error("Unknown logical operator: " + op);
}

// ビット演算（&, |, ^, <<, >>）の評価
int64_t evaluate_bitwise_binary(const std::string& op, int64_t left, int64_t right) {
    if (op == "&") {
        return left & right;
    } else if (op == "|") {
        return left | right;
    } else if (op == "^") {
        return left ^ right;
    } else if (op == "<<") {
        return left << right;
    } else if (op == ">>") {
        return left >> right;
    }
    throw std::runtime_error("Unknown bitwise operator: " + op);
}

// ============================================================================
// リテラル評価のヘルパー実装
// ============================================================================

// 数値リテラル（整数・浮動小数点）の評価
int64_t evaluate_number_literal(const ASTNode* node) {
    debug_msg(DebugMsgId::EXPR_EVAL_NUMBER, node->int_value);
    
    // 浮動小数点リテラルの場合
    if (node->is_float_literal) {
        TypeInfo literal_type = node->literal_type != TYPE_UNKNOWN ? node->literal_type : TYPE_DOUBLE;
        if (literal_type == TYPE_QUAD) {
            return static_cast<int64_t>(node->quad_value);
        }
        return static_cast<int64_t>(node->double_value);
    }
    
    // 整数リテラル
    return node->int_value;
}

// 特殊なリテラル（nullptr, 文字列リテラル）の評価
int64_t evaluate_special_literal(const ASTNode* node) {
    if (node->node_type == ASTNodeType::AST_NULLPTR) {
        // nullptr は 0 として評価
        return 0;
    } else if (node->node_type == ASTNodeType::AST_STRING_LITERAL) {
        debug_msg(DebugMsgId::EXPR_EVAL_STRING_LITERAL, node->str_value.c_str());
        // 文字列リテラルは現在の評価コンテキストでは数値として扱えないため、
        // 特別な値を返すか、エラーを投げる必要がある
        // とりあえず0を返す（文字列処理は別途output_managerで処理）
        return 0;
    }
    throw std::runtime_error("Unknown special literal type");
}

// ============================================================================
// インクリメント/デクリメントのヘルパー実装
// ============================================================================

// 前置インクリメント/デクリメント（++x, --x）
int64_t evaluate_prefix_incdec(const ASTNode* node, Interpreter& interpreter) {
    if (!node->left || node->left->node_type != ASTNodeType::AST_VARIABLE) {
        error_msg(DebugMsgId::DIRECT_ARRAY_ASSIGN_ERROR);
        throw std::runtime_error("Invalid prefix operation");
    }
    
    Variable *var = interpreter.find_variable(node->left->name);
    if (!var) {
        error_msg(DebugMsgId::UNDEFINED_VAR_ERROR, node->left->name.c_str());
        throw std::runtime_error("Undefined variable");
    }

    if (node->op == "++") {
        var->value += 1;
    } else if (node->op == "--") {
        var->value -= 1;
    }

    return var->value; // プリフィックスは新しい値を返す
}

// 後置インクリメント/デクリメント（x++, x--）
int64_t evaluate_postfix_incdec(const ASTNode* node, Interpreter& interpreter) {
    if (!node->left || node->left->node_type != ASTNodeType::AST_VARIABLE) {
        error_msg(DebugMsgId::DIRECT_ARRAY_ASSIGN_ERROR);
        throw std::runtime_error("Invalid postfix operation");
    }
    
    Variable *var = interpreter.find_variable(node->left->name);
    if (!var) {
        error_msg(DebugMsgId::UNDEFINED_VAR_ERROR, node->left->name.c_str());
        throw std::runtime_error("Undefined variable");
    }

    int64_t old_value = var->value;
    if (node->op == "++_post") {
        var->value += 1;
    } else if (node->op == "--_post") {
        var->value -= 1;
    }

    return old_value; // ポストフィックスは古い値を返す
}

// ============================================================================
// 単項演算のヘルパー実装
// ============================================================================

// 単純な単項演算（+, -, !, ~）
int64_t evaluate_simple_unary(const std::string& op, int64_t operand) {
    if (op == "+") {
        return operand;
    } else if (op == "-") {
        return -operand;
    } else if (op == "!") {
        return operand ? 0 : 1;
    } else if (op == "~") {
        return ~operand;
    }
    error_msg(DebugMsgId::UNKNOWN_UNARY_OP_ERROR, op.c_str());
    throw std::runtime_error("Unknown unary operator: " + op);
}

// ============================================================================
// 型情報のヘルパー実装
// ============================================================================

// TypeInfoを文字列に変換
std::string type_info_to_string(TypeInfo type) {
    const char* name = ::type_info_to_string(type);
    if (name && *name) {
        return std::string(name);
    }
    return "unknown";
}

} // namespace ExpressionHelpers
