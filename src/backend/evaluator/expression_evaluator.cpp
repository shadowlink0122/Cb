#include "expression_evaluator.h"
#include "../interpreter.h"
#include "../../common/debug_messages.h"
#include "../../common/utf8_utils.h"
#include "../error_handler.h"
#include "../array_manager.h"
#include <stdexcept>
#include <iostream>

ExpressionEvaluator::ExpressionEvaluator(Interpreter& interpreter) 
    : interpreter_(interpreter) {}

int64_t ExpressionEvaluator::evaluate_expression(const ASTNode* node) {
    if (!node) {
        throw std::runtime_error("Null node in expression evaluation");
    }

    switch (node->node_type) {
    case ASTNodeType::AST_NUMBER: {
        debug_msg(DebugMsgId::EXPR_EVAL_NUMBER, node->int_value);
        return node->int_value;
    }

    case ASTNodeType::AST_STRING_LITERAL: {
        // 文字列リテラルは現在の評価コンテキストでは数値として扱えないため、
        // 特別な値を返すか、エラーを投げる必要がある
        // とりあえず0を返す（文字列処理は別途output_managerで処理）
        return 0;
    }

    case ASTNodeType::AST_VARIABLE: {
        debug_msg(DebugMsgId::EXPR_EVAL_VAR_REF, node->name.c_str());
        
        Variable *var = interpreter_.find_variable(node->name);
        if (!var) {
            // エラー時にソースコード位置を表示
            std::string error_message = (debug_language == DebugLanguage::JAPANESE) ? 
                "未定義の変数です: " + node->name : "Undefined variable: " + node->name;
            interpreter_.throw_runtime_error_with_location(error_message, node);
        }

        return var->value;
    }

    case ASTNodeType::AST_ARRAY_REF: {
        debug_msg(DebugMsgId::EXPR_EVAL_ARRAY_REF, node->name.c_str());
        
        std::string array_name = interpreter_.extract_array_name(node);
        if (array_name.empty()) {
            throw std::runtime_error("Cannot determine array name");
        }
        
        std::vector<int64_t> indices = interpreter_.extract_array_indices(node);
        
        Variable *var = interpreter_.find_variable(array_name);
        if (!var) {
            std::string error_message = (debug_language == DebugLanguage::JAPANESE) ? 
                "未定義の配列です: " + array_name : "Undefined array: " + array_name;
            interpreter_.throw_runtime_error_with_location(error_message, node);
        }

        // 文字列配列の文字アクセス（例: names[0][0]）
        if (var->is_array && !var->array_strings.empty() && indices.size() == 2) {
            int64_t array_index = indices[0];
            int64_t char_index = indices[1];
            
            if (array_index < 0 || array_index >= static_cast<int64_t>(var->array_strings.size())) {
                throw std::runtime_error("Array index out of bounds");
            }
            
            std::string str = var->array_strings[array_index];
            if (char_index < 0 || char_index >= static_cast<int64_t>(utf8_utils::utf8_char_count(str))) {
                throw std::runtime_error("String index out of bounds");
            }
            
            std::string character = utf8_utils::utf8_char_at(str, char_index);
            return utf8_utils::utf8_char_to_int(character);
        }

        // 文字列の配列アクセス
        if (var->type == TYPE_STRING && indices.size() == 1) {
            int64_t index = indices[0];
            std::string str = var->str_value;
            
            if (index < 0 || index >= static_cast<int64_t>(utf8_utils::utf8_char_count(str))) {
                throw std::runtime_error("String index out of bounds");
            }
            
            // UTF-8対応の文字アクセス
            std::string character = utf8_utils::utf8_char_at(str, index);
            return utf8_utils::utf8_char_to_int(character);
        }

        // 多次元配列のアクセス
        if (var->is_multidimensional) {
            // 多次元配列専用の処理
            return interpreter_.getMultidimensionalArrayElement(*var, indices);
        }
        
        if (var->array_values.empty()) {
            if (!var->is_array) {
                throw std::runtime_error("Variable is not an array");
            }
            return var->value; // スカラー値の場合
        }

        // フラットインデックスの計算
        if (var->array_dimensions.empty()) {
            throw std::runtime_error("Array dimensions not available");
        }

        if (indices.size() != var->array_dimensions.size()) {
            throw std::runtime_error("Index dimension mismatch");
        }

        size_t flat_index = 0;
        size_t multiplier = 1;
        
        for (int i = var->array_dimensions.size() - 1; i >= 0; i--) {
            if (indices[i] < 0 || indices[i] >= var->array_dimensions[i]) {
                std::string error_message = (debug_language == DebugLanguage::JAPANESE) ? 
                    "配列インデックス範囲外です: " + std::to_string(indices[i]) : 
                    "Array index out of bounds: " + std::to_string(indices[i]);
                interpreter_.throw_runtime_error_with_location(error_message, node);
            }
            flat_index += indices[i] * multiplier;
            multiplier *= var->array_dimensions[i];
        }

        if (flat_index >= var->array_values.size()) {
            throw std::runtime_error("Flat index out of bounds");
        }

        return var->array_values[flat_index];
    }

    case ASTNodeType::AST_ARRAY_LITERAL: {
        debug_msg(DebugMsgId::EXPR_EVAL_ARRAY_REF, "AST_ARRAY_LITERAL: returning placeholder value for nested array processing");
        // 配列リテラルは通常は式として評価されないが、
        // N次元配列処理では内部的に参照される場合があるため
        // プレースホルダー値として0を返す
        return 0;
    }

    case ASTNodeType::AST_BINARY_OP: {
        debug_msg(DebugMsgId::EXPR_EVAL_BINARY_OP, node->op.c_str());

        int64_t left = evaluate_expression(node->left.get());
        int64_t right = evaluate_expression(node->right.get());

        debug_msg(DebugMsgId::BINARY_OP_VALUES, left, right);

        // デバッグ: 減算操作の詳細を出力
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
                throw std::runtime_error("Modulo by zero");
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
            result = (left && right) ? 1 : 0;
        else if (node->op == "||")
            result = (left || right) ? 1 : 0;
        else {
            error_msg(DebugMsgId::UNKNOWN_BINARY_OP_ERROR, node->op.c_str());
            throw std::runtime_error("Unknown binary operator: " + node->op);
        }

        debug_msg(DebugMsgId::BINARY_OP_RESULT_DEBUG, result);
        return result;
    }

    case ASTNodeType::AST_UNARY_OP: {
        debug_msg(DebugMsgId::UNARY_OP_DEBUG, node->op.c_str());
        
        // ポストフィックス演算子の場合
        if (node->op == "++_post" || node->op == "--_post") {
            if (!node->left || node->left->node_type != ASTNodeType::AST_VARIABLE) {
                error_msg(DebugMsgId::DIRECT_ARRAY_ASSIGN_ERROR);
                throw std::runtime_error("Invalid postfix operation");
            }
            
            Variable *var = interpreter_.find_variable(node->left->name);
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
        
        // プリフィックス演算子の場合
        if (node->op == "++" || node->op == "--") {
            if (!node->left || node->left->node_type != ASTNodeType::AST_VARIABLE) {
                error_msg(DebugMsgId::DIRECT_ARRAY_ASSIGN_ERROR);
                throw std::runtime_error("Invalid prefix operation");
            }
            
            Variable *var = interpreter_.find_variable(node->left->name);
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

        int64_t operand = evaluate_expression(node->left.get());
        
        if (node->op == "+") {
            return operand;
        } else if (node->op == "-") {
            return -operand;
        } else if (node->op == "!") {
            return operand ? 0 : 1;
        } else {
            error_msg(DebugMsgId::UNKNOWN_UNARY_OP_ERROR, node->op.c_str());
            throw std::runtime_error("Unknown unary operator: " + node->op);
        }
    }

    case ASTNodeType::AST_PRE_INCDEC:
    case ASTNodeType::AST_POST_INCDEC: {
        if (!node->left || node->left->node_type != ASTNodeType::AST_VARIABLE) {
            error_msg(DebugMsgId::DIRECT_ARRAY_ASSIGN_ERROR);
            throw std::runtime_error("Invalid increment/decrement operation");
        }
        
        Variable *var = interpreter_.find_variable(node->left->name);
        if (!var) {
            error_msg(DebugMsgId::UNDEFINED_VAR_ERROR, node->left->name.c_str());
            throw std::runtime_error("Undefined variable");
        }

        int64_t old_value = var->value;
        
        if (node->op == "++") {
            var->value += 1;
        } else if (node->op == "--") {
            var->value -= 1;
        }

        if (node->node_type == ASTNodeType::AST_PRE_INCDEC) {
            return var->value; // プリインクリメント/デクリメントは新しい値を返す
        } else {
            return old_value; // ポストインクリメント/デクリメントは古い値を返す
        }
    }

    case ASTNodeType::AST_FUNC_CALL: {
        // 関数を探す
        const ASTNode *func = nullptr;
        
        // グローバルスコープから関数を探す
        auto &global_scope = interpreter_.get_global_scope();
        auto it = global_scope.functions.find(node->name);
        if (it != global_scope.functions.end()) {
            func = it->second;
        }
        
        if (!func) {
            throw std::runtime_error("Undefined function: " + node->name);
        }
        
        // 新しいスコープを作成
        interpreter_.push_scope();
        
        try {
            // パラメータの評価と設定
            if (func->parameters.size() != node->arguments.size()) {
                throw std::runtime_error("Argument count mismatch for function: " + node->name);
            }
            
            for (size_t i = 0; i < func->parameters.size(); i++) {
                const auto &param = func->parameters[i];
                const auto &arg = node->arguments[i];
                
                // 配列パラメータのサポート
                if (param->is_array) {
                    if (arg->node_type == ASTNodeType::AST_VARIABLE) {
                        // 変数として渡された場合
                        Variable* source_var = interpreter_.find_variable(arg->name);
                        if (!source_var || !source_var->is_array) {
                            throw std::runtime_error("Array argument expected for parameter: " + param->name);
                        }
                        
                        // 配列をコピーしてパラメータに設定
                        interpreter_.assign_array_parameter(param->name, *source_var, param->type_info);
                    } else if (arg->node_type == ASTNodeType::AST_ARRAY_LITERAL) {
                        // 配列リテラルとして直接渡された場合
                        debug_msg(DebugMsgId::ARRAY_LITERAL_INIT_PROCESSING,
                                ("Processing array literal argument for parameter: " + param->name).c_str());
                        
                        // 一時的な配列変数を作成
                        std::string temp_var_name = "__temp_array_" + std::to_string(i);
                        Variable temp_var;
                        temp_var.is_array = true;
                        temp_var.type = param->type_info;
                        temp_var.is_assigned = false;
                        
                        // 配列リテラルから値を取得
                        std::vector<int64_t> values;
                        std::vector<std::string> str_values;
                        
                        for (const auto &element : arg->arguments) {
                            if (element->node_type == ASTNodeType::AST_STRING_LITERAL) {
                                str_values.push_back(element->str_value);
                            } else {
                                int64_t val = evaluate_expression(element.get());
                                values.push_back(val);
                            }
                        }
                        
                        // 一時変数に値を設定
                        if (!str_values.empty()) {
                            temp_var.array_strings = str_values;
                            temp_var.array_size = str_values.size();
                            temp_var.type = static_cast<TypeInfo>(TYPE_ARRAY_BASE + TYPE_STRING);
                        } else {
                            temp_var.array_values = values;
                            temp_var.array_size = values.size();
                            temp_var.type = static_cast<TypeInfo>(TYPE_ARRAY_BASE + TYPE_INT);
                        }
                        temp_var.is_assigned = true;
                        
                        // パラメータに設定
                        interpreter_.assign_array_parameter(param->name, temp_var, param->type_info);
                    } else {
                        throw std::runtime_error("Only array variables can be passed as array parameters");
                    }
                } else {
                    // 通常の値パラメータの型チェック
                    // 引数の型を事前にチェック
                    if (arg->node_type == ASTNodeType::AST_STRING_LITERAL && param->type_info != TYPE_STRING) {
                        throw std::runtime_error("Type mismatch: cannot pass string literal to non-string parameter '" + param->name + "'");
                    }
                    
                    // 文字列パラメータの場合
                    if (param->type_info == TYPE_STRING) {
                        if (arg->node_type == ASTNodeType::AST_STRING_LITERAL) {
                            // 文字列リテラルを直接代入
                            Variable param_var;
                            param_var.type = TYPE_STRING;
                            param_var.str_value = arg->str_value;
                            param_var.is_assigned = true;
                            param_var.is_const = false;
                            interpreter_.current_scope().variables[param->name] = param_var;
                        } else if (arg->node_type == ASTNodeType::AST_VARIABLE) {
                            // 文字列変数を代入
                            Variable* source_var = interpreter_.find_variable(arg->name);
                            if (!source_var || source_var->type != TYPE_STRING) {
                                throw std::runtime_error("Type mismatch: expected string variable for parameter '" + param->name + "'");
                            }
                            Variable param_var;
                            param_var.type = TYPE_STRING;
                            param_var.str_value = source_var->str_value;
                            param_var.is_assigned = true;
                            param_var.is_const = false;
                            interpreter_.current_scope().variables[param->name] = param_var;
                        } else {
                            throw std::runtime_error("Type mismatch: cannot pass non-string expression to string parameter '" + param->name + "'");
                        }
                    } else {
                        // 数値パラメータの場合
                        if (arg->node_type == ASTNodeType::AST_STRING_LITERAL) {
                            throw std::runtime_error("Type mismatch: cannot pass string literal to numeric parameter '" + param->name + "'");
                        }
                        
                        int64_t arg_value = evaluate_expression(arg.get());
                        interpreter_.assign_function_parameter(param->name, arg_value, param->type_info);
                    }
                }
            }
            
            // 関数本体を実行
            try {
                if (func->body) {
                    interpreter_.execute_statement(func->body.get());
                }
                // void関数は0を返す
                interpreter_.pop_scope();
                return 0;
            } catch (const ReturnException &ret) {
                // return文で戻り値がある場合
                if (ret.is_array) {
                    // 配列戻り値の場合は例外を再度投げる
                    interpreter_.pop_scope();
                    throw ret;
                }
                // 文字列戻り値の場合は例外を再度投げる
                if (ret.type == TYPE_STRING) {
                    interpreter_.pop_scope();
                    throw ret;
                }
                // 通常の戻り値の場合
                interpreter_.pop_scope();
                return ret.value;
            }
        } catch (const ReturnException &ret) {
            // 再投げされたReturnExceptionを処理
            throw ret;
        } catch (...) {
            interpreter_.pop_scope();
            throw;
        }
    }

    case ASTNodeType::AST_ASSIGN: {
        // 代入式を評価し、代入された値を返す
        
        debug_msg(DebugMsgId::EXPR_EVAL_BINARY_OP, "Processing AST_ASSIGN");
        
        // 右辺が配列リテラルの場合は特別処理
        if (node->right && node->right->node_type == ASTNodeType::AST_ARRAY_LITERAL) {
            debug_msg(DebugMsgId::EXPR_EVAL_BINARY_OP, "Right side is array literal");
            // 配列リテラル代入処理
            if (node->left->node_type == ASTNodeType::AST_VARIABLE) {
                std::string var_name = node->left->name;
                debug_msg(DebugMsgId::EXPR_EVAL_BINARY_OP, ("Array literal assignment to: " + var_name).c_str());
                interpreter_.assign_array_literal(var_name, node->right.get());
                return 0; // 配列代入の戻り値は0
            } else {
                throw std::runtime_error("Array literal can only be assigned to variables");
            }
        }
        
        // 右辺が関数呼び出しで配列を返す可能性がある場合の処理
        if (node->right && node->right->node_type == ASTNodeType::AST_FUNC_CALL) {
            debug_msg(DebugMsgId::EXPR_EVAL_BINARY_OP, "Right side is function call, checking for array return");
            try {
                int64_t right_value = evaluate_expression(node->right.get());
                // 通常の値を返した場合
                if (node->left->node_type == ASTNodeType::AST_VARIABLE) {
                    interpreter_.assign_variable(node->left->name, right_value, node->left->type_info);
                } else {
                    interpreter_.assign_variable(node->name, right_value, node->type_info);
                }
                return right_value;
            } catch (const ReturnException &ret) {
                // 配列が返された場合
                if (ret.is_array) {
                    debug_msg(DebugMsgId::EXPR_EVAL_BINARY_OP, "Function returned array, assigning to variable");
                    std::string var_name;
                    if (node->left->node_type == ASTNodeType::AST_VARIABLE) {
                        var_name = node->left->name;
                    } else {
                        var_name = node->name;
                    }
                    
                    // 配列を変数に代入
                    interpreter_.assign_array_from_return(var_name, ret);
                    return 0; // 配列代入の戻り値は0
                } else {
                    // 通常の値
                    int64_t right_value = ret.value;
                    if (node->left->node_type == ASTNodeType::AST_VARIABLE) {
                        interpreter_.assign_variable(node->left->name, right_value, node->left->type_info);
                    } else {
                        interpreter_.assign_variable(node->name, right_value, node->type_info);
                    }
                    return right_value;
                }
            }
        }
        
        // 右辺を評価してから代入を実行
        int64_t right_value = 0;
        if (node->right) {
            right_value = evaluate_expression(node->right.get());
        }
        
        // 代入先の処理
        if (node->left->node_type == ASTNodeType::AST_ARRAY_REF) {
            // 配列要素への代入
            std::string var_name;
            if (node->left->left && node->left->left->node_type == ASTNodeType::AST_VARIABLE) {
                var_name = node->left->left->name;
            } else if (!node->left->name.empty()) {
                var_name = node->left->name;
            } else {
                throw std::runtime_error("Invalid array reference in assignment");
            }
            
            int64_t index_value = evaluate_expression(node->left->array_index.get());
            interpreter_.assign_array_element(var_name, static_cast<int>(index_value), right_value);
        } else {
            // 通常の変数への代入
            interpreter_.assign_variable(node->name, right_value, node->type_info);
        }
        
        return right_value;
    }

    default:
        error_msg(DebugMsgId::UNSUPPORTED_EXPR_NODE_ERROR);
        std::cerr << "[ERROR] Unsupported expression node type: " << static_cast<int>(node->node_type) << std::endl;
        throw std::runtime_error("Unknown expression node type");
    }

    return 0;
}
