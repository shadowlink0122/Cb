#include "evaluator/expression_evaluator.h"
#include "core/interpreter.h"
#include "managers/enum_manager.h"    // EnumManager定義が必要
#include "managers/type_manager.h"    // TypeManager定義が必要
#include "../../../common/debug_messages.h"
#include "../../../common/utf8_utils.h"
#include "core/error_handler.h"
#include "managers/array_manager.h"
#include "services/array_processing_service.h"
#include <stdexcept>
#include <iostream>

ExpressionEvaluator::ExpressionEvaluator(Interpreter& interpreter) 
    : interpreter_(interpreter) {}

int64_t ExpressionEvaluator::evaluate_expression(const ASTNode* node) {
    if (!node) {
        debug_msg(DebugMsgId::EXPR_EVAL_START, "Null node in expression evaluation");
        if (debug_mode) {
            std::cerr << "[ERROR] Null node in expression evaluation" << std::endl;
            // スタックトレース的な情報を出力
            std::cerr << "[ERROR] This usually means a parser error occurred" << std::endl;
        }
        throw std::runtime_error("Null node in expression evaluation");
    }

    debug_msg(DebugMsgId::EXPR_EVAL_START, std::to_string((int)node->node_type).c_str());

    // 多次元配列アクセスの場合のみ詳細ログ
    if (node->node_type == ASTNodeType::AST_ARRAY_REF && node->name.empty()) {
        debug_msg(DebugMsgId::EXPR_EVAL_ARRAY_REF_START);
    }

    switch (node->node_type) {
    case ASTNodeType::AST_NUMBER: {
        debug_msg(DebugMsgId::EXPR_EVAL_NUMBER, node->int_value);
        return node->int_value;
    }

    case ASTNodeType::AST_STRING_LITERAL: {
        debug_msg(DebugMsgId::EXPR_EVAL_STRING_LITERAL, node->str_value.c_str());
        // 文字列リテラルは現在の評価コンテキストでは数値として扱えないため、
        // 特別な値を返すか、エラーを投げる必要がある
        // とりあえず0を返す（文字列処理は別途output_managerで処理）
        return 0;
    }

    case ASTNodeType::AST_VARIABLE: {
        debug_msg(DebugMsgId::EXPR_EVAL_VAR_REF, node->name.c_str());
        
        Variable *var = interpreter_.find_variable(node->name);
        if (!var) {
            debug_msg(DebugMsgId::EXPR_EVAL_VAR_NOT_FOUND, node->name.c_str());
            // エラー時にソースコード位置を表示
            std::string error_message = (debug_language == DebugLanguage::JAPANESE) ? 
                "未定義の変数です: " + node->name : "Undefined variable: " + node->name;
            interpreter_.throw_runtime_error_with_location(error_message, node);
        }

        // ユニオン型変数の場合、current_typeに応じて適切な値を返す
        if (var->type == TYPE_UNION) {
            if (var->current_type == TYPE_STRING) {
                // 文字列の場合は、数値評価コンテキストでは0を返す
                // 実際の文字列値はstr_valueに格納されている
                debug_msg(DebugMsgId::EXPR_EVAL_VAR_VALUE, node->name.c_str(), 0);
                return 0;
            } else {
                debug_msg(DebugMsgId::EXPR_EVAL_VAR_VALUE, node->name.c_str(), var->value);
                return var->value;
            }
        }

        debug_msg(DebugMsgId::EXPR_EVAL_VAR_VALUE, node->name.c_str(), var->value);
        return var->value;
    }

    case ASTNodeType::AST_ARRAY_REF: {
        debug_msg(DebugMsgId::EXPR_EVAL_ARRAY_REF, node->name.c_str());
        
        if (debug_mode) {
            debug_print("AST_ARRAY_REF: Processing array access\n");
            debug_print("  node->left exists: %s\n", node->left ? "true" : "false");
            if (node->left) {
                debug_print("  node->left->node_type: %d\n", static_cast<int>(node->left->node_type));
                debug_print("  node->left has name: %s\n", !node->left->name.empty() ? node->left->name.c_str() : "empty");
                if (node->left->left) {
                    debug_print("  node->left->left->node_type: %d\n", static_cast<int>(node->left->left->node_type));
                    debug_print("  node->left->left has name: %s\n", !node->left->left->name.empty() ? node->left->left->name.c_str() : "empty");
                }
            }
        }
        
        // 多次元メンバ配列アクセスの処理: obj.member[i][j]
        if (node->left && node->left->node_type == ASTNodeType::AST_ARRAY_REF &&
            node->left->left && node->left->left->node_type == ASTNodeType::AST_MEMBER_ACCESS) {
            
            debug_msg(DebugMsgId::EXPR_EVAL_MULTIDIM_MEMBER_ARRAY_ACCESS, "");
            // obj.member[i][j] の場合
            std::string obj_name = node->left->left->left->name;
            std::string member_name = node->left->left->name;
            debug_msg(DebugMsgId::EXPR_EVAL_MEMBER_ACCESS_DETAILS, obj_name.c_str(), member_name.c_str());
            
            // 多次元インデックスを収集（一般化されたN次元対応）
            std::vector<int64_t> indices;
            
            // ネストした AST_ARRAY_REF から全てのインデックスを再帰的に収集
            const ASTNode* current_node = node;
            while (current_node && current_node->node_type == ASTNodeType::AST_ARRAY_REF) {
                int64_t index = evaluate_expression(current_node->array_index.get());
                indices.insert(indices.begin(), index); // 先頭に挿入（逆順になるため）
                debug_msg(DebugMsgId::EXPR_EVAL_ARRAY_INDEX, index);
                current_node = current_node->left.get();
            }
            
            if (debug_mode) {
                debug_print("Collected %zu indices for multidimensional access\n", indices.size());
                for (size_t i = 0; i < indices.size(); i++) {
                    debug_print("  index[%zu] = %lld\n", i, indices[i]);
                }
            }
            
            // 構造体メンバー変数を取得
            Variable *member_var = interpreter_.get_struct_member(obj_name, member_name);
            if (!member_var) {
                throw std::runtime_error("Struct member not found: " + member_name);
            }
            
            if (debug_mode) {
                debug_print("Member variable found: %s.%s\n", obj_name.c_str(), member_name.c_str());
                debug_print("  is_multidimensional: %s\n", member_var->is_multidimensional ? "true" : "false");
                debug_print("  array_dimensions.size(): %zu\n", member_var->array_dimensions.size());
                debug_print("  indices.size(): %zu\n", indices.size());
            }
            
            debug_msg(DebugMsgId::EXPR_EVAL_STRUCT_MEMBER, member_name.c_str());
            debug_msg(DebugMsgId::EXPR_EVAL_MULTIDIM_ACCESS,
                      member_var->is_multidimensional ? 1 : 0,
                      member_var->array_dimensions.size(),
                      indices.size());
            
            // N次元配列の場合
            if (member_var->is_multidimensional && indices.size() >= 1) {
                if (debug_mode) {
                    debug_print("Calling get_struct_member_multidim_array_element\n");
                }
                return interpreter_.get_struct_member_multidim_array_element(obj_name, member_name, indices);
            }
            
            if (debug_mode) {
                debug_print("Condition failed, throwing error\n");
            }
            
            throw std::runtime_error("Invalid multidimensional member array access");
        }
        
        // メンバ配列アクセスの特別処理: obj.member[index]
        if (node->left && node->left->node_type == ASTNodeType::AST_MEMBER_ACCESS) {
            std::string obj_name = node->left->left->name;
            std::string member_name = node->left->name;
            int64_t index = evaluate_expression(node->array_index.get());
            
            // 構造体メンバー配列の場合は直接取得
            try {
                return interpreter_.get_struct_member_array_element(obj_name, member_name, static_cast<int>(index));
            } catch (const std::exception& e) {
                // 失敗した場合は従来の方式を試す
                std::string member_array_element_name = obj_name + "." + member_name + "[" + std::to_string(index) + "]";
                
                Variable *var = interpreter_.find_variable(member_array_element_name);
                if (!var) {
                    throw std::runtime_error("Member array element not found: " + member_array_element_name);
                }
                
                return var->value;
            }
        }
        
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
        
        // 1次元文字列配列のアクセス
        if (var->is_array && !var->array_strings.empty() && indices.size() == 1) {
            int64_t array_index = indices[0];
            
            if (array_index < 0 || array_index >= static_cast<int64_t>(var->array_strings.size())) {
                throw std::runtime_error("Array index out of bounds");
            }
            
            // 文字列配列の要素は数値として評価できないため、特別な処理が必要
            // printf処理では別途文字列として取得される
            return 0; // 文字列の場合は0を返すが、実際の文字列は別途取得される
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
        // ビット演算子
        else if (node->op == "&")
            result = left & right;
        else if (node->op == "|")
            result = left | right;
        else if (node->op == "^")
            result = left ^ right;
        else if (node->op == "<<")
            result = left << right;
        else if (node->op == ">>")
            result = left >> right;
        else {
            error_msg(DebugMsgId::UNKNOWN_BINARY_OP_ERROR, node->op.c_str());
            throw std::runtime_error("Unknown binary operator: " + node->op);
        }

        debug_msg(DebugMsgId::BINARY_OP_RESULT_DEBUG, result);
        return result;
    }

    case ASTNodeType::AST_TERNARY_OP: {
        // 三項演算子: condition ? true_expr : false_expr
        int64_t condition = evaluate_expression(node->left.get());
        
        if (condition) {
            return evaluate_expression(node->right.get());
        } else {
            return evaluate_expression(node->third.get());
        }
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
        } else if (node->op == "~") {
            return ~operand;
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
        
        // 現在の関数名を設定
        std::string prev_function_name = interpreter_.current_function_name;
        interpreter_.current_function_name = node->name;
        
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
                        // struct型パラメータかチェック
                        if (param->type_info == TYPE_STRUCT) {
                            Variable* source_var = nullptr;
                            std::string source_var_name;
                            
                            if (arg->node_type == ASTNodeType::AST_VARIABLE) {
                                // struct変数を引数として渡す場合
                                source_var_name = arg->name;
                                source_var = interpreter_.find_variable(arg->name);
                            } else if (arg->node_type == ASTNodeType::AST_ARRAY_REF) {
                                // 構造体配列要素を引数として渡す場合 (struct_array[0])
                                std::string array_name = arg->left->name;
                                int64_t index = evaluate_expression(arg->array_index.get());
                                source_var_name = array_name + "[" + std::to_string(index) + "]";
                                
                                // 配列要素の最新状態を同期
                                interpreter_.sync_struct_members_from_direct_access(source_var_name);
                                // 同期後に再度取得
                                source_var = interpreter_.find_variable(source_var_name);
                            }
                            
                            if (source_var && source_var->is_struct) {
                                
                                // typedef名を実際のstruct名に解決
                                std::string resolved_struct_type = interpreter_.resolve_typedef(param->type_name);
                                std::string source_resolved_type = interpreter_.resolve_typedef(source_var->struct_type_name);
                                
                                // struct型の互換性チェック
                                // "struct Point"と"Point"は同じ型として扱う
                                std::string normalized_resolved = resolved_struct_type;
                                std::string normalized_source = source_resolved_type;
                                
                                // "struct StructName"を"StructName"に正規化
                                if (normalized_resolved.substr(0, 7) == "struct " && normalized_resolved.length() > 7) {
                                    normalized_resolved = normalized_resolved.substr(7);
                                }
                                if (normalized_source.substr(0, 7) == "struct " && normalized_source.length() > 7) {
                                    normalized_source = normalized_source.substr(7);
                                }
                                
                                if (normalized_resolved != normalized_source) {
                                    throw std::runtime_error("Type mismatch: cannot pass struct type '" + source_var->struct_type_name + 
                                                            "' to parameter '" + param->name + "' of type '" + param->type_name + "'");
                                }
                                
                                // ソース構造体の最新状態を同期
                                Variable* sync_source_var = nullptr;
                                if (!source_var_name.empty()) {
                                    interpreter_.sync_struct_members_from_direct_access(source_var_name);
                                    sync_source_var = interpreter_.find_variable(source_var_name);
                                } else {
                                    debug_print("WARNING: Empty source_var_name, skipping sync\n");
                                }
                                
                                if (!sync_source_var) {
                                    throw std::runtime_error("Source struct variable not found: " + source_var_name);
                                }
                                
                                // 文字列配列メンバの場合、追加で確実にarray_stringsを同期
                                for (auto& source_member_pair : sync_source_var->struct_members) {
                                    if (source_member_pair.second.is_array && source_member_pair.second.type == TYPE_STRING) {
                                        // 個別要素変数から文字列配列を再構築
                                        std::string base_name = source_var_name.empty() ? "unknown" : source_var_name;
                                        std::string source_member_name = base_name + "." + source_member_pair.first;
                                        for (int i = 0; i < source_member_pair.second.array_size; i++) {
                                            std::string element_name = source_member_name + "[" + std::to_string(i) + "]";
                                            Variable* element_var = interpreter_.find_variable(element_name);
                                            if (element_var && element_var->type == TYPE_STRING) {
                                                if (source_member_pair.second.array_strings.size() <= static_cast<size_t>(i)) {
                                                    source_member_pair.second.array_strings.resize(i + 1);
                                                }
                                                source_member_pair.second.array_strings[i] = element_var->str_value;
                                            }
                                        }
                                    }
                                }
                                
                                // struct変数をコピーしてパラメータに設定
                                Variable param_var = *sync_source_var;
                                param_var.is_const = false;
                                param_var.is_struct = true; // 明示的にstructフラグを設定
                                param_var.type = TYPE_STRUCT; // 型情報も設定
                                // 解決されたstruct型名を設定
                                param_var.struct_type_name = resolved_struct_type;
                                
                                // struct_membersの配列要素も確実にコピー
                                for (auto& member_pair : param_var.struct_members) {
                                    if (member_pair.second.is_array && member_pair.second.type == TYPE_STRING) {
                                        // 文字列配列の場合、array_stringsを確実にコピー
                                        const auto& source_member = sync_source_var->struct_members.find(member_pair.first);
                                        if (source_member != sync_source_var->struct_members.end()) {
                                            debug_print("DEBUG: Copying string array %s: size=%d\n", 
                                                       member_pair.first.c_str(), 
                                                       static_cast<int>(source_member->second.array_strings.size()));
                                            member_pair.second.array_strings = source_member->second.array_strings;
                                            if (!source_member->second.array_strings.empty()) {
                                                debug_print("DEBUG: First element: '%s'\n", 
                                                           source_member->second.array_strings[0].c_str());
                                            }
                                        }
                                    }
                                }
                                
                                interpreter_.current_scope().variables[param->name] = param_var;
                                
                                // 個別メンバー変数も作成（値を正しく設定）
                                for (const auto& member_pair : sync_source_var->struct_members) {
                                    std::string full_member_name = param->name + "." + member_pair.first;
                                    Variable member_var = member_pair.second;
                                    // 値を確実に設定
                                    member_var.is_assigned = true;
                                    
                                    debug_print("DEBUG: Creating param member %s: is_array=%d, array_size=%d\n", 
                                               full_member_name.c_str(), member_var.is_array, member_var.array_size);
                                    if (member_var.is_array && member_var.type == TYPE_STRING) {
                                        debug_print("DEBUG: String array size=%zu\n", member_var.array_strings.size());
                                        for (size_t i = 0; i < member_var.array_strings.size(); i++) {
                                            debug_print("DEBUG: array_strings[%zu]='%s'\n", i, member_var.array_strings[i].c_str());
                                        }
                                    }
                                    
                                    interpreter_.current_scope().variables[full_member_name] = member_var;
                                    
                                    // 配列メンバの場合、個別要素変数も作成
                                    if (member_var.is_array) {
                                        // ソース側の配列要素変数をコピー
                                        std::string source_member_name = source_var_name + "." + member_pair.first;
                                        for (int i = 0; i < member_var.array_size; i++) {
                                            std::string source_element_name = source_member_name + "[" + std::to_string(i) + "]";
                                            std::string param_element_name = full_member_name + "[" + std::to_string(i) + "]";
                                            
                                            Variable* source_element = interpreter_.find_variable(source_element_name);
                                            if (source_element) {
                                                Variable element_var = *source_element;
                                                element_var.is_assigned = true;
                                                interpreter_.current_scope().variables[param_element_name] = element_var;
                                            } else {
                                                // 個別要素変数が存在しない場合、struct_membersの配列から作成
                                                Variable element_var;
                                                if (member_var.type == TYPE_STRING && i < static_cast<int>(sync_source_var->struct_members[member_pair.first].array_strings.size())) {
                                                    element_var.type = TYPE_STRING;
                                                    element_var.str_value = sync_source_var->struct_members[member_pair.first].array_strings[i];
                                                } else if (member_var.type != TYPE_STRING && i < static_cast<int>(sync_source_var->struct_members[member_pair.first].array_values.size())) {
                                                    element_var.type = member_var.type;
                                                    element_var.value = sync_source_var->struct_members[member_pair.first].array_values[i];
                                                } else {
                                                    // デフォルト値を設定
                                                    element_var.type = member_var.type;
                                                    if (member_var.type == TYPE_STRING) {
                                                        element_var.str_value = "";
                                                    } else {
                                                        element_var.value = 0;
                                                    }
                                                }
                                                element_var.is_assigned = true;
                                                interpreter_.current_scope().variables[param_element_name] = element_var;
                                            }
                                        }
                                    }
                                }
                            } else {
                                throw std::runtime_error("Type mismatch: cannot pass non-struct expression to struct parameter '" + param->name + "'");
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
            }
            
            // 関数本体を実行
            try {
                if (func->body) {
                    interpreter_.execute_statement(func->body.get());
                }
                // void関数は0を返す
                interpreter_.pop_scope();
                interpreter_.current_function_name = prev_function_name;
                return 0;
            } catch (const ReturnException &ret) {
                // return文で戻り値がある場合
                interpreter_.pop_scope();
                interpreter_.current_function_name = prev_function_name;
                
                if (ret.is_struct) {
                    // struct戻り値の場合、構造体を一時的に処理して戻り値として使用
                    debug_msg(DebugMsgId::INTERPRETER_GET_STRUCT_MEMBER, "Processing struct return value");
                    // 構造体戻り値は0を返す（実際の構造体はReturnExceptionで管理）
                    throw ret; // 上位レベルでstruct処理が必要な場合は例外を伝播
                } else if (ret.is_array) {
                    // 配列戻り値の場合は例外を再度投げる
                    throw ret;
                }
                // 文字列戻り値の場合は例外を再度投げる
                if (ret.type == TYPE_STRING) {
                    throw ret;
                }
                // 通常の戻り値の場合
                return ret.value;
                interpreter_.current_function_name = prev_function_name;
                return ret.value;
            }
        } catch (const ReturnException &ret) {
            // 再投げされたReturnExceptionを処理
            interpreter_.current_function_name = prev_function_name;
            throw ret;
        } catch (...) {
            interpreter_.pop_scope();
            interpreter_.current_function_name = prev_function_name;
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
            std::string var_name;
            if (!node->name.empty()) {
                var_name = node->name;
            } else if (node->left && node->left->node_type == ASTNodeType::AST_VARIABLE) {
                var_name = node->left->name;
            } else {
                throw std::runtime_error("Invalid assignment target in evaluator");
            }
            interpreter_.assign_variable(var_name, right_value, node->type_info);
        }
        
        return right_value;
    }
    
    case ASTNodeType::AST_MEMBER_ACCESS: {
        // メンバアクセス: obj.member または array[index].member
        std::string var_name;
        std::string member_name = node->name;
        
        if (node->left->node_type == ASTNodeType::AST_VARIABLE) {
            // 通常のstruct変数: obj.member
            var_name = node->left->name;
        } else if (node->left->node_type == ASTNodeType::AST_ARRAY_REF) {
            // struct配列要素: array[index].member
            std::string array_name = node->left->left->name;
            int64_t index = evaluate_expression(node->left->array_index.get());
            var_name = array_name + "[" + std::to_string(index) + "]";
        } else {
            throw std::runtime_error("Invalid member access");
        }
        
        // 個別変数として直接アクセスを試す（構造体配列の場合）
        std::string full_member_path = var_name + "." + member_name;
        Variable* member_var = interpreter_.find_variable(full_member_path);
        
        if (!member_var) {
            // struct_membersから探す（通常の構造体の場合）
            member_var = interpreter_.get_struct_member(var_name, member_name);
        }
        
        if (member_var->type == TYPE_STRING) {
            // 文字列メンバは別途処理が必要（呼び出し元で処理される）
            return 0; // 文字列の場合は0を返すが、実際の文字列は別途取得される
        }
        return member_var->value;
    }
    
    case ASTNodeType::AST_MEMBER_ARRAY_ACCESS: {
        // メンバの配列アクセス: obj.member[index] または obj.member[i][j]
        std::string obj_name;
        if (node->left->node_type == ASTNodeType::AST_VARIABLE) {
            obj_name = node->left->name;
        } else {
            throw std::runtime_error("Invalid object reference in member array access");
        }
        
        std::string member_name = node->name;
        
        std::cerr << "DEBUG_MEMBER_ARRAY: obj=" << obj_name << ", member=" << member_name << std::endl;
        
        // インデックスを評価（多次元対応）
        std::vector<int64_t> indices;
        if (node->right) {
            // 1次元の場合（従来通り）
            int64_t index = evaluate_expression(node->right.get());
            indices.push_back(index);
        } else if (!node->arguments.empty()) {
            // 多次元の場合
            for (const auto& arg : node->arguments) {
                int64_t index = evaluate_expression(arg.get());
                indices.push_back(index);
            }
        } else {
            throw std::runtime_error("No indices found for array access");
        }
        
        // 構造体メンバー変数を取得
        Variable *member_var = interpreter_.get_struct_member(obj_name, member_name);
        if (!member_var) {
            throw std::runtime_error("Struct member not found: " + member_name);
        }

        // 多次元配列の場合
        if (member_var->is_multidimensional && indices.size() > 1) {
            std::cerr << "DEBUG: Using getMultidimensionalArrayElement - indices.size()=" << indices.size() << std::endl;
            return interpreter_.getMultidimensionalArrayElement(*member_var, indices);
        }
        
        std::cerr << "DEBUG: Using 1D access - is_multidimensional=" << member_var->is_multidimensional 
                  << ", indices.size()=" << indices.size() << std::endl;
        
        // 1次元配列の場合（従来処理）
        int64_t index = indices[0];
        return interpreter_.get_struct_member_array_element(obj_name, member_name, static_cast<int>(index));
    }
    
    case ASTNodeType::AST_STRUCT_LITERAL: {
        // 構造体リテラルは代入時にのみ処理されるべき
        // ここでは0を返す
        return 0;
    }

    case ASTNodeType::AST_ENUM_ACCESS: {
        // enum値アクセス (EnumName::member)
        EnumManager* enum_manager = interpreter_.get_enum_manager();
        int64_t enum_value;
        
        // typedef名を実際のenum名に解決
        std::string resolved_enum_name = interpreter_.get_type_manager()->resolve_typedef(node->enum_name);
        
        if (enum_manager->get_enum_value(resolved_enum_name, node->enum_member, enum_value)) {
            debug_msg(DebugMsgId::EXPR_EVAL_NUMBER, enum_value);
            return enum_value;
        } else {
            std::string error_message = "Undefined enum value: " + 
                                       node->enum_name + "::" + node->enum_member;
            throw std::runtime_error(error_message);
        }
    }

    default:
        error_msg(DebugMsgId::UNSUPPORTED_EXPR_NODE_ERROR);
        if (debug_mode) {
            std::cerr << "[ERROR] Unsupported expression node type: " << static_cast<int>(node->node_type) << std::endl;
        }
        throw std::runtime_error("Unknown expression node type");
    }

    return 0;
}
