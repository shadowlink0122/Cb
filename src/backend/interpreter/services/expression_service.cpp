#include "services/expression_service.h"
#include "../../../common/debug.h"
#include "core/interpreter.h"
#include "evaluator/expression_evaluator.h"
#include <stdexcept>

ExpressionService::ExpressionService(Interpreter *interpreter)
    : interpreter_(interpreter),
      expression_evaluator_(interpreter->get_expression_evaluator()), stats_{} {
}

int64_t ExpressionService::evaluate_safe(
    const ASTNode *node, const std::string &context,
    std::function<void(const std::string &)> error_handler) {
    if (!node) {
        std::string error_msg = "Null ASTNode in expression evaluation";
        if (!context.empty()) {
            error_msg += " (context: " + context + ")";
        }

        if (error_handler) {
            error_handler(error_msg);
        } else {
            handle_evaluation_error(error_msg, context);
        }
        increment_stats("general", false);
        return 0;
    }

    try {
        int64_t result = expression_evaluator_->evaluate_expression(node);
        increment_stats("general", true);
        return result;
    } catch (const std::exception &e) {
        std::string error_msg =
            "Expression evaluation failed: " + std::string(e.what());
        if (!context.empty()) {
            error_msg += " (context: " + context + ")";
        }

        if (error_handler) {
            error_handler(error_msg);
        } else {
            handle_evaluation_error(error_msg, context);
        }
        increment_stats("general", false);
        return 0;
    }
}

int64_t ExpressionService::evaluate_condition(const ASTNode *condition_node,
                                              const std::string &context) {
    if (!condition_node) {
        handle_evaluation_error("Null condition node", context);
        increment_stats("condition", false);
        return 0;
    }

    try {
        int64_t result =
            expression_evaluator_->evaluate_expression(condition_node);
        increment_stats("condition", true);
        return result;
    } catch (const std::exception &e) {
        std::string error_msg =
            "Condition evaluation failed: " + std::string(e.what());
        handle_evaluation_error(error_msg, context);
        increment_stats("condition", false);
        return 0;
    }
}

int64_t ExpressionService::evaluate_array_index(const ASTNode *index_node,
                                                size_t array_size,
                                                const std::string &var_name) {
    if (!index_node) {
        std::string error_msg =
            "Null array index expression for variable: " + var_name;
        handle_evaluation_error(error_msg, "array index");
        increment_stats("array_index", false);
        return 0;
    }

    try {
        int64_t index = expression_evaluator_->evaluate_expression(index_node);

        // 境界チェック
        if (index < 0 || static_cast<size_t>(index) >= array_size) {
            std::string error_msg = "Array index " + std::to_string(index) +
                                    " out of bounds for variable '" + var_name +
                                    "' (size: " + std::to_string(array_size) +
                                    ")";
            handle_evaluation_error(error_msg, "array bounds check");
            increment_stats("array_index", false);
            return 0;
        }

        increment_stats("array_index", true);
        return index;
    } catch (const std::exception &e) {
        std::string error_msg = "Array index evaluation failed for variable '" +
                                var_name + "': " + std::string(e.what());
        handle_evaluation_error(error_msg, "array index");
        increment_stats("array_index", false);
        return 0;
    }
}

void ExpressionService::handle_evaluation_error(const std::string &error_msg,
                                                const std::string &context) {
    // 統一されたエラーハンドリング
    std::string formatted_error = "[ExpressionService] " + error_msg;
    if (!context.empty()) {
        formatted_error += " (Context: " + context + ")";
    }

    // デバッグ出力（統一フォーマット）
    debug_print("Expression evaluation error: %s\n", formatted_error.c_str());

    // 実際のエラー処理は各クラスに委譲するか、統一エラーハンドリングシステムを使用
    throw std::runtime_error(formatted_error);
}

void ExpressionService::increment_stats(const std::string &evaluation_type,
                                        bool success) {
    stats_.total_evaluations++;

    if (!success) {
        stats_.failed_evaluations++;
    }

    if (evaluation_type == "condition") {
        stats_.condition_evaluations++;
    } else if (evaluation_type == "array_index") {
        stats_.array_index_evaluations++;
    }

    // デバッグモードでは統計情報をログ出力（コメントアウト：無限再帰を防ぐ）
    // if (is_debug_mode()) {
    //     std::string stats_message = "ExpressionService Stats: Total=" +
    //                                 std::to_string(stats_.total_evaluations)
    //                                 +
    //                                 ", Failed=" +
    //                                 std::to_string(stats_.failed_evaluations);
    //     debug_msg(DebugMsgId::ARRAY_DECL_DEBUG, stats_message.c_str());
    // }
}

bool ExpressionService::is_debug_mode() const {
    return interpreter_ && interpreter_->is_debug_mode();
}
