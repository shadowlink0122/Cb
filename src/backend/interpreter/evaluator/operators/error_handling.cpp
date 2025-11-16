#include "error_handling.h"
#include "../../../../common/debug.h"
#include "../../../../common/debug_messages.h"
#include "core/interpreter.h"
#include "core/type_inference.h"
#include "evaluator/core/evaluator.h"
#include "evaluator/core/helpers.h"
#include <algorithm>
#include <cctype>
#include <sstream>
#include <stdexcept>
#include <string>

namespace {

struct RuntimeErrorDescriptor {
    std::string variant;
    std::string message;
};

std::string to_lower_copy(const std::string &input) {
    std::string lowered = input;
    std::transform(
        lowered.begin(), lowered.end(), lowered.begin(),
        [](unsigned char ch) { return static_cast<char>(std::tolower(ch)); });
    return lowered;
}

RuntimeErrorDescriptor classify_runtime_error(const std::string &message,
                                              bool is_checked) {
    std::string lowered = to_lower_copy(message);

    if (lowered.find("division by zero") != std::string::npos ||
        (lowered.find("divide") != std::string::npos &&
         lowered.find("zero") != std::string::npos)) {
        return {"DivisionByZeroError", message};
    }
    if (lowered.find("null pointer") != std::string::npos ||
        lowered.find("nullptr") != std::string::npos) {
        return {"NullPointerError", message};
    }
    if (lowered.find("out of bounds") != std::string::npos ||
        lowered.find("bounds") != std::string::npos) {
        return {"IndexOutOfBoundsError", message};
    }
    if (lowered.find("overflow") != std::string::npos) {
        return {"ArithmeticOverflowError", message};
    }
    if (lowered.find("type") != std::string::npos &&
        (lowered.find("cast") != std::string::npos ||
         lowered.find("mismatch") != std::string::npos)) {
        return {"TypeCastError", message};
    }

    if (is_checked) {
        return {"CheckedError", message};
    }
    return {"Custom", message};
}

std::string format_payload_type_name(const InferredType &type) {
    if (!type.type_name.empty()) {
        return type.type_name;
    }
    if (type.type_info != TYPE_UNKNOWN) {
        const char *name = ::type_info_to_string(type.type_info);
        if (name && *name) {
            return name;
        }
    }
    return "auto";
}

std::string build_result_type_name(const InferredType &payload_type) {
    std::ostringstream oss;
    oss << "Result<" << format_payload_type_name(payload_type)
        << ", RuntimeError>";
    return oss.str();
}

Variable build_result_ok(const TypedValue &value,
                         const InferredType &payload_type) {
    if (value.is_struct_result) {
        throw std::runtime_error("try/checked expression does not currently "
                                 "support struct payloads");
    }

    Variable result;
    result.is_enum = true;
    result.is_struct = true;
    result.type = TYPE_ENUM;
    result.enum_variant = "Ok";
    result.enum_type_name = build_result_type_name(payload_type);
    result.struct_type_name = result.enum_type_name;
    result.has_associated_value = true;

    if (value.is_string()) {
        result.associated_str_value = value.string_value;
    } else {
        result.associated_int_value = value.as_numeric();
    }

    return result;
}

Variable build_result_err(const RuntimeErrorDescriptor &descriptor,
                          const InferredType &payload_type) {
    Variable result;
    result.is_enum = true;
    result.is_struct = true;
    result.type = TYPE_ENUM;
    result.enum_variant = "Err";
    result.enum_type_name = build_result_type_name(payload_type);
    result.struct_type_name = result.enum_type_name;
    result.has_associated_value = true;
    result.associated_str_value =
        descriptor.variant + ": " + descriptor.message;
    return result;
}

TypedValue evaluate_operand_typed(ExpressionEvaluator &expression_evaluator,
                                  const ASTNode *operand) {
    try {
        return expression_evaluator.evaluate_typed_expression(operand);
    } catch (const ReturnException &) {
        throw;
    }
}

int64_t evaluate_try_like_expression(const ASTNode *node,
                                     ExpressionEvaluator &expression_evaluator,
                                     bool is_checked) {
    if (!node || !node->left) {
        throw std::runtime_error("try/checked expression requires an operand");
    }

    const ASTNode *operand = node->left.get();
    InferredType payload_type =
        expression_evaluator.get_type_engine().infer_type(operand);

    try {
        TypedValue typed_value =
            evaluate_operand_typed(expression_evaluator, operand);
        Variable ok_result = build_result_ok(typed_value, payload_type);
        throw ReturnException(ok_result);
    } catch (const ReturnException &) {
        throw;
    } catch (const std::exception &ex) {
        RuntimeErrorDescriptor descriptor =
            classify_runtime_error(ex.what(), is_checked);
        Variable err_result = build_result_err(descriptor, payload_type);
        throw ReturnException(err_result);
    } catch (const char *message) {
        RuntimeErrorDescriptor descriptor =
            classify_runtime_error(message ? message : "", is_checked);
        Variable err_result = build_result_err(descriptor, payload_type);
        throw ReturnException(err_result);
    } catch (const std::string &message) {
        RuntimeErrorDescriptor descriptor =
            classify_runtime_error(message, is_checked);
        Variable err_result = build_result_err(descriptor, payload_type);
        throw ReturnException(err_result);
    }
}

} // namespace

namespace ErrorHandlingOperators {

int64_t evaluate_try_expression(const ASTNode *node,
                                ExpressionEvaluator &expression_evaluator,
                                Interpreter &) {
    return evaluate_try_like_expression(node, expression_evaluator, false);
}

int64_t evaluate_checked_expression(const ASTNode *node,
                                    ExpressionEvaluator &expression_evaluator,
                                    Interpreter &) {
    return evaluate_try_like_expression(node, expression_evaluator, true);
}

} // namespace ErrorHandlingOperators
