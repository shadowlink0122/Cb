#include "expression_evaluator.h"
#include "../interpreter.h"
#include "../../frontend/debug_messages.h"
#include <stdexcept>

ExpressionEvaluator::ExpressionEvaluator(Interpreter& interpreter) 
    : interpreter_(interpreter) {}

int64_t ExpressionEvaluator::evaluate_expression(const ASTNode *node) {
    if (!node)
        return 0;

    debug_msg(DebugMsgId::EXPR_EVAL_NUMBER, static_cast<int>(node->node_type));

    switch (node->node_type) {
    case ASTNodeType::AST_NUMBER: {
        debug_msg(DebugMsgId::EXPR_EVAL_NUMBER, node->int_value);
        return node->int_value;
    }

    case ASTNodeType::AST_STRING_LITERAL: {
        debug_msg(DebugMsgId::STRING_LITERAL_DEBUG, node->str_value.c_str());
        // 文字列リテラルは値として0を返す
        return 0;
    }

    case ASTNodeType::AST_VARIABLE: {
        debug_msg(DebugMsgId::EXPR_EVAL_VAR_REF, node->name.c_str());
        
        // 修飾された変数参照（module.variable）の場合
        if (node->is_qualified_call) {
            return evaluate_qualified_variable_ref(node);
        }
        
        // 通常の変数参照
        Variable *var = interpreter_.find_variable(node->name);
        if (!var) {
            error_msg(DebugMsgId::UNDEFINED_VAR_ERROR, node->name.c_str());
            throw std::runtime_error("Undefined variable");
        }
        debug_msg(DebugMsgId::VAR_VALUE, var->value);
        return var->value;
    }

    case ASTNodeType::AST_ARRAY_REF: {
        debug_msg(DebugMsgId::EXPR_EVAL_ARRAY_REF, node->name.c_str());
        Variable *var = interpreter_.find_variable(node->name);
        if (!var) {
            error_msg(DebugMsgId::UNDEFINED_VAR_ERROR, node->name.c_str());
            throw std::runtime_error("Undefined variable");
        }

        if (!var->is_array) {
            error_msg(DebugMsgId::NON_ARRAY_REF_ERROR, node->name.c_str());
            throw std::runtime_error("Not an array");
        }

        int64_t index = evaluate_expression(node->array_index.get());
        debug_msg(DebugMsgId::ARRAY_INDEX, index);

        if (index < 0 || index >= (int64_t)var->array_values.size()) {
            error_msg(DebugMsgId::ARRAY_OUT_OF_BOUNDS_ERROR, node->name.c_str());
            throw std::runtime_error("Array index out of bounds");
        }

        debug_msg(DebugMsgId::ARRAY_ELEMENT_VALUE, var->array_values[index]);
        return var->array_values[index];
    }

    case ASTNodeType::AST_BINARY_OP: {
        debug_msg(DebugMsgId::EXPR_EVAL_BINARY_OP, node->op.c_str());
        int64_t left = evaluate_expression(node->left.get());
        int64_t right = evaluate_expression(node->right.get());
        debug_msg(DebugMsgId::BINARY_OP_VALUES, left, right);

        int64_t result = 0;
        if (node->op == "+")
            result = left + right;
        else if (node->op == "-")
            result = left - right;
        else if (node->op == "*")
            result = left * right;
        else if (node->op == "/") {
            if (right == 0) {
                error_msg(DebugMsgId::ZERO_DIVISION_ERROR);
                throw std::runtime_error("Division by zero");
            }
            result = left / right;
        } else if (node->op == "%") {
            if (right == 0) {
                error_msg(DebugMsgId::ZERO_DIVISION_ERROR);
                throw std::runtime_error("Division by zero");
            }
            result = left % right;
        } else if (node->op == "==")
            result = (left == right) ? 1 : 0;
        else if (node->op == "!=")
            result = (left != right) ? 1 : 0;
        else if (node->op == "<")
            result = (left < right) ? 1 : 0;
        else if (node->op == ">")
            result = (left > right) ? 1 : 0;
        else if (node->op == "<=")
            result = (left <= right) ? 1 : 0;
        else if (node->op == ">=")
            result = (left >= right) ? 1 : 0;
        else if (node->op == "&&")
            result = ((left != 0) && (right != 0)) ? 1 : 0;
        else if (node->op == "||")
            result = ((left != 0) || (right != 0)) ? 1 : 0;
        else {
            error_msg(DebugMsgId::UNKNOWN_BINARY_OP_ERROR, node->op.c_str());
            throw std::runtime_error("Unknown binary operator");
        }

        debug_msg(DebugMsgId::BINARY_OP_RESULT_DEBUG, result);
        return result;
    }

    case ASTNodeType::AST_UNARY_OP: {
        debug_msg(DebugMsgId::UNARY_OP_DEBUG, node->op.c_str());
        int64_t operand = evaluate_expression(node->left.get());
        debug_msg(DebugMsgId::UNARY_OP_OPERAND_DEBUG, operand);

        int64_t result = 0;
        if (node->op == "!")
            result = (operand == 0) ? 1 : 0;
        else if (node->op == "-")
            result = -operand;
        else {
            error_msg(DebugMsgId::UNKNOWN_UNARY_OP_ERROR, node->op.c_str());
            throw std::runtime_error("Unknown unary operator");
        }

        debug_msg(DebugMsgId::UNARY_OP_RESULT_DEBUG, result);
        return result;
    }

    case ASTNodeType::AST_PRE_INCDEC:
    case ASTNodeType::AST_POST_INCDEC: {
        Variable *var = interpreter_.find_variable(node->name);
        if (!var) {
            error_msg(DebugMsgId::UNDEFINED_VAR_ERROR, node->name.c_str());
            throw std::runtime_error("Undefined variable");
        }

        int64_t old_value = var->value;
        if (node->op == "++") {
            var->value += 1;
        } else if (node->op == "--") {
            var->value -= 1;
        }

        interpreter_.check_type_range(var->type, var->value, node->name);

        return (node->node_type == ASTNodeType::AST_PRE_INCDEC) ? var->value
                                                                : old_value;
    }

    case ASTNodeType::AST_FUNC_CALL: {
        // 修飾された関数呼び出し（module.function）の場合
        if (node->is_qualified_call) {
            return evaluate_qualified_function_call(node);
        }
        
        // 通常の関数呼び出し
        const ASTNode *func = interpreter_.find_function(node->name);
        if (!func) {
            error_msg(DebugMsgId::UNDEFINED_FUNC_ERROR, node->name.c_str());
            throw std::runtime_error("Undefined function");
        }

        // 引数の数チェック
        if (node->arguments.size() != func->parameters.size()) {
            error_msg(DebugMsgId::ARG_COUNT_MISMATCH_ERROR, node->name.c_str());
            throw std::runtime_error("Argument count mismatch");
        }

        // ローカルスコープ作成
        interpreter_.push_scope();

        // 引数を評価してパラメータに束縛
        for (size_t i = 0; i < func->parameters.size(); ++i) {
            int64_t arg_value = evaluate_expression(node->arguments[i].get());
            Variable param;
            param.type = func->parameters[i]->type_info;
            param.value = arg_value;
            param.is_assigned = true;
            interpreter_.current_scope().variables[func->parameters[i]->name] = param;
        }

        try {
            interpreter_.execute_statement(func->body.get());
            interpreter_.pop_scope();
            return 0; // void関数
        } catch (const ReturnException &e) {
            interpreter_.pop_scope();
            return e.value;
        }
    }

    case ASTNodeType::AST_ARRAY_DECL:
        // 配列宣言は式として評価できない
        // デバッグ用：どこから呼び出されたかを調べる
        debug_msg(DebugMsgId::ARRAY_DECL_EVAL_DEBUG, node->name.c_str());
        error_msg(DebugMsgId::ARRAY_DECL_AS_EXPR_ERROR, node->name.c_str());
        throw std::runtime_error("Array declaration as expression error");

    case ASTNodeType::AST_STMT_LIST:
        // 配列リテラル処理（パーサーがcreate_array_literalでAST_STMT_LISTを使用）
        {
            // これは配列リテラル [1,2,3,...] として扱う
            // node->childrenには各要素のノードが含まれている
            Variable result;
            result.array_values.clear();

            // 各要素を評価して配列に追加
            for (auto &child : node->children) {
                int64_t element_value = evaluate_expression(child.get());
                result.array_values.push_back(element_value);
            }

            // 配列として返す（値としては0）
            return 0;
        }

    case ASTNodeType::AST_TRY_STMT:
    case ASTNodeType::AST_CATCH_STMT:
    case ASTNodeType::AST_FINALLY_STMT:
    case ASTNodeType::AST_THROW_STMT:
        // これらは文として処理されるべきで、式として評価されるべきではない
        return 0;

    default:
        // デバッグ用: どのノード型が未対応かを表示
        std::string node_type_str =
            "unknown(" + std::to_string(static_cast<int>(node->node_type)) +
            ")";
        error_msg(DebugMsgId::UNSUPPORTED_EXPR_NODE_ERROR,
                  node_type_str.c_str());
        throw std::runtime_error("Unsupported expression node");
    }
}

int64_t ExpressionEvaluator::evaluate_qualified_function_call(const ASTNode *node) {
    // モジュール名と関数名を分離
    std::string module_name = node->module_name;
    std::string function_name = node->name;
    
    debug_msg(DebugMsgId::UNDEFINED_FUNC_ERROR, (module_name + "." + function_name).c_str());
    
    // モジュールが読み込まれているかチェック
    if (!interpreter_.is_module_loaded(module_name)) {
        error_msg(DebugMsgId::UNDEFINED_FUNC_ERROR, (module_name + " module not loaded").c_str());
        throw std::runtime_error("Module not loaded: " + module_name);
    }
    
    // モジュール内の関数を探す
    const ASTNode *func = interpreter_.find_module_function(module_name, function_name);
    if (!func) {
        error_msg(DebugMsgId::UNDEFINED_FUNC_ERROR, (module_name + "." + function_name).c_str());
        throw std::runtime_error("Function not found in module: " + module_name + "." + function_name);
    }
    
    // 引数の数チェック
    if (node->arguments.size() != func->parameters.size()) {
        error_msg(DebugMsgId::ARG_COUNT_MISMATCH_ERROR, function_name.c_str());
        throw std::runtime_error("Argument count mismatch in " + module_name + "." + function_name);
    }
    
    // ローカルスコープ作成
    interpreter_.push_scope();
    
    try {
        // 引数を評価してパラメータに束縛
        for (size_t i = 0; i < func->parameters.size(); ++i) {
            int64_t arg_value = evaluate_expression(node->arguments[i].get());
            Variable param;
            param.type = func->parameters[i]->type_info;
            param.value = arg_value;
            param.is_assigned = true;
            interpreter_.current_scope().variables[func->parameters[i]->name] = param;
        }
        
        // 関数本体を実行
        interpreter_.execute_statement(func->body.get());
        interpreter_.pop_scope();
        return 0; // void関数
        
    } catch (const ReturnException &e) {
        interpreter_.pop_scope();
        return e.value;
    } catch (...) {
        interpreter_.pop_scope();
        throw;
    }
}

int64_t ExpressionEvaluator::evaluate_qualified_variable_ref(const ASTNode *node) {
    // モジュール名と変数名を分離
    std::string module_name = node->module_name;
    std::string variable_name = node->name;
    
    debug_msg(DebugMsgId::UNDEFINED_VAR_ERROR, (module_name + "." + variable_name).c_str());
    
    // モジュールが読み込まれているかチェック
    if (!interpreter_.is_module_loaded(module_name)) {
        error_msg(DebugMsgId::UNDEFINED_VAR_ERROR, (module_name + " module not loaded").c_str());
        throw std::runtime_error("Module not loaded: " + module_name);
    }
    
    // モジュール内の変数を探す
    int64_t value = interpreter_.find_module_variable(module_name, variable_name);
    return value;
}
