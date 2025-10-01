#include "evaluator/expression_evaluator.h"
#include "core/interpreter.h"
#include "managers/enum_manager.h"    // EnumManager定義が必要
#include "managers/type_manager.h"    // TypeManager定義が必要
#include "../../../common/debug_messages.h"
#include "../../../common/debug.h"
#include "../../../common/utf8_utils.h"
#include "core/error_handler.h"
#include "managers/array_manager.h"
#include "services/array_processing_service.h"
#include <stdexcept>
#include <iostream>
#include <functional>

// MethodReceiverResolutionのデフォルトコンストラクタ実装
ExpressionEvaluator::MethodReceiverResolution::MethodReceiverResolution()
    : kind(Kind::None), canonical_name(), variable_ptr(nullptr), chain_value(nullptr) {}

// レシーバ解決ヘルパー（メソッド呼び出し用）
ExpressionEvaluator::MethodReceiverResolution ExpressionEvaluator::resolve_method_receiver(const ASTNode* receiver_node) {
    MethodReceiverResolution result;
    if (!receiver_node) {
        return result;
    }

    switch (receiver_node->node_type) {
    case ASTNodeType::AST_VARIABLE:
    case ASTNodeType::AST_IDENTIFIER: {
        std::string name = receiver_node->name;
        if (name.empty()) {
            return result;
        }
        Variable* var = interpreter_.find_variable(name);
        if (var) {
            result.kind = MethodReceiverResolution::Kind::Direct;
            result.canonical_name = name;
            result.variable_ptr = var;
            return result;
        }
        break;
    }
    case ASTNodeType::AST_MEMBER_ACCESS:
        // メンバアクセスは別ヘルパーで解決
        return resolve_member_receiver(receiver_node);
    case ASTNodeType::AST_ARRAY_REF:
        return resolve_array_receiver(receiver_node);
    case ASTNodeType::AST_FUNC_CALL:
        return create_chain_receiver_from_expression(receiver_node);
    default:
        break;
    }

    return create_chain_receiver_from_expression(receiver_node);
}

ExpressionEvaluator::ExpressionEvaluator(Interpreter& interpreter) 
    : interpreter_(interpreter), type_engine_(interpreter), last_typed_result_(0, InferredType()) {}

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

    case ASTNodeType::AST_IDENTIFIER: {
        debug_msg(DebugMsgId::EXPR_EVAL_VAR_REF, node->name.c_str());
        
        // selfキーワードの処理
        if (node->name == "self") {
            // メソッドコンテキスト内で self を処理
            // 現在のメソッド実行コンテキストからself変数を取得
            Variable *self_var = interpreter_.find_variable("self");
            if (!self_var) {
                std::string error_message = (debug_language == DebugLanguage::JAPANESE) ? 
                    "selfはメソッドコンテキスト外では使用できません" : "self can only be used within method context";
                interpreter_.throw_runtime_error_with_location(error_message, node);
            }
            
            // selfが構造体またはインターフェース型の場合、ReturnExceptionで構造体を返す
            if (self_var->type == TYPE_STRUCT || self_var->type == TYPE_INTERFACE) {
                interpreter_.sync_struct_members_from_direct_access("self");
                throw ReturnException(*self_var);
            } else {
                // プリミティブ型の場合は値を返す
                return self_var->value;
            }
        }
        
        // 通常の識別子として処理
        Variable *var = interpreter_.find_variable(node->name);
        if (!var) {
            debug_msg(DebugMsgId::EXPR_EVAL_VAR_NOT_FOUND, node->name.c_str());
            std::string error_message = (debug_language == DebugLanguage::JAPANESE) ? 
                "未定義の変数です: " + node->name : "Undefined variable: " + node->name;
            interpreter_.throw_runtime_error_with_location(error_message, node);
        }

        debug_msg(DebugMsgId::EXPR_EVAL_VAR_VALUE, node->name.c_str(), var->value);
        return var->value;
    }

    case ASTNodeType::AST_VARIABLE: {
        debug_msg(DebugMsgId::EXPR_EVAL_VAR_REF, node->name.c_str());
        
        // selfキーワードの特別処理（構造体戻り値用）
        if (node->name == "self") {
            Variable *self_var = interpreter_.find_variable("self");
            if (!self_var) {
                std::string error_message = (debug_language == DebugLanguage::JAPANESE) ? 
                    "selfはメソッドコンテキスト外では使用できません" : "self can only be used within method context";
                interpreter_.throw_runtime_error_with_location(error_message, node);
            }
            
            // デバッグ出力: self変数の詳細情報
            debug_print("SELF_DEBUG: self found - type=%d, is_struct=%d, TYPE_STRUCT=%d, TYPE_INTERFACE=%d\n", 
                       (int)self_var->type, self_var->is_struct, (int)TYPE_STRUCT, (int)TYPE_INTERFACE);
            
            // selfが構造体またはインターフェース型の場合、ReturnExceptionで構造体を返す
            if (self_var->type == TYPE_STRUCT || self_var->type == TYPE_INTERFACE) {
                debug_print("SELF_DEBUG: Throwing ReturnException for struct self\n");
                interpreter_.sync_struct_members_from_direct_access("self");
                throw ReturnException(*self_var);
            } else {
                debug_print("SELF_DEBUG: self is not struct, returning primitive value\n");
                // primitive型の場合は適切な値を返す
                // 文字列の場合、特別な処理が必要な場合があるが、まずは値を返す
                return self_var->value;
            }
        }
        
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

        // 構造体変数の場合、ReturnExceptionをスローして構造体データを返す
        if (var->type == TYPE_STRUCT) {
            throw ReturnException(*var);
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
        
        // メンバ配列アクセスの特別処理: obj.member[index] または func().member[index]
        if (node->left && node->left->node_type == ASTNodeType::AST_MEMBER_ACCESS) {
            std::string obj_name;
            std::string member_name = node->left->name;
            int64_t index = evaluate_expression(node->array_index.get());
            
            // 関数呼び出しの場合
            if (node->left->left && node->left->left->node_type == ASTNodeType::AST_FUNC_CALL) {
                try {
                    evaluate_expression(node->left->left.get());
                    throw std::runtime_error("Function did not return a struct for member array access");
                } catch (const ReturnException& ret_ex) {
                    Variable base_struct = ret_ex.struct_value;
                    Variable member_var_copy = get_struct_member_from_variable(base_struct, member_name);
                    
                    if (!member_var_copy.is_array) {
                        throw std::runtime_error("Member is not an array: " + member_name);
                    }
                    
                    if (index < 0 || index >= static_cast<int>(member_var_copy.array_values.size())) {
                        throw std::runtime_error("Array index out of bounds");
                    }
                    
                    return member_var_copy.array_values[index];
                }
            } else {
                obj_name = node->left->left->name;
            }
            
            // 通常の構造体変数の場合
            if (!obj_name.empty()) {
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
        }
        
        // 関数呼び出しの戻り値に対する配列アクセス: func()[index]
        if (node->left && node->left->node_type == ASTNodeType::AST_FUNC_CALL) {
            debug_print("Processing function call array access: %s\n", node->left->name.c_str());
            
            // インデックスを評価
            int64_t index = evaluate_expression(node->array_index.get());
            
            try {
                // 関数を実行して戻り値を取得（副作用のため実行）
                (void)evaluate_expression(node->left.get());
                throw std::runtime_error("Function did not return an array via exception");
            } catch (const ReturnException& ret) {
                if (ret.is_array) {
                    // 構造体配列の戻り値の場合
                    if (ret.is_struct_array && !ret.struct_array_3d.empty() && 
                        !ret.struct_array_3d[0].empty() && !ret.struct_array_3d[0][0].empty()) {
                        
                        if (index >= 0 && index < static_cast<int64_t>(ret.struct_array_3d[0][0].size())) {
                            // 構造体要素をReturnExceptionとして投げる
                            throw ReturnException(ret.struct_array_3d[0][0][index]);
                        } else {
                            throw std::runtime_error("Array index out of bounds");
                        }
                    }
                    // 数値配列の戻り値の場合
                    else if (!ret.int_array_3d.empty() && 
                        !ret.int_array_3d[0].empty() && !ret.int_array_3d[0][0].empty()) {
                        
                        if (index >= 0 && index < static_cast<int64_t>(ret.int_array_3d[0][0].size())) {
                            return ret.int_array_3d[0][0][index];
                        } else {
                            throw std::runtime_error("Array index out of bounds");
                        }
                    }
                    // 文字列配列の戻り値の場合 - 現時点では文字列配列要素を数値として返すことはできない
                    else if (!ret.str_array_3d.empty() && 
                             !ret.str_array_3d[0].empty() && !ret.str_array_3d[0][0].empty()) {
                        throw std::runtime_error("String array element access not supported in numeric context");
                    } else {
                        throw std::runtime_error("Empty array returned from function");
                    }
                } else {
                    throw std::runtime_error("Function does not return an array");
                }
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
            // 文字列多次元配列の場合は専用メソッドを使用
            if (var->array_type_info.base_type == TYPE_STRING) {
                // 文字列配列の場合は文字列として取得してから数値変換
                // 通常、これは printf等で文字列として処理されるべきだが、
                // int64_t を要求される場面では 0 を返す
                return 0;
            }
            // 数値多次元配列の場合
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

        // 配列要素が構造体参照の場合、構造体を取得
        std::string element_name = array_name + "[" + std::to_string(flat_index) + "]";
        Variable* element_var = interpreter_.find_variable(element_name);
        if (element_var && element_var->is_struct) {
            throw ReturnException(*element_var);
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
        // 三項演算子: condition ? true_expr : false_expr (型推論対応)
        TypedValue typed_result = evaluate_ternary_typed(node);
        
        if (typed_result.is_string()) {
            // 文字列の場合は、プロトコルとして0を返し、実際の文字列値は
            // 必要に応じて別途取得する（OutputManagerなど）
            return 0;
        } else {
            return typed_result.as_numeric();
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
        bool is_method_call = (node->left != nullptr); // レシーバーがある場合はメソッド呼び出し
        bool has_receiver = is_method_call;
        std::string receiver_name;
        MethodReceiverResolution receiver_resolution;
        struct MethodCallContext {
            bool uses_temp_receiver = false;
            std::string temp_variable_name;
            std::shared_ptr<ReturnException> chain_value;
            Variable concrete_receiver;
        } method_context;

        if (is_method_call) {
            debug_msg(DebugMsgId::METHOD_CALL_START, node->name.c_str());
            receiver_resolution = resolve_method_receiver(node->left.get());

            if (receiver_resolution.kind == MethodReceiverResolution::Kind::Direct && receiver_resolution.variable_ptr) {
                receiver_name = receiver_resolution.canonical_name;
            } else if (receiver_resolution.kind == MethodReceiverResolution::Kind::Chain && receiver_resolution.chain_value) {
                method_context.chain_value = receiver_resolution.chain_value;

                const ReturnException& chain_ret = *receiver_resolution.chain_value;
                if (chain_ret.is_array) {
                    throw chain_ret;
                }

                method_context.uses_temp_receiver = true;
                method_context.temp_variable_name = "__chain_receiver_" + std::to_string(rand() % 10000);

                Variable temp_receiver;
                temp_receiver.is_assigned = true;

                if (chain_ret.type == TYPE_STRUCT || chain_ret.is_struct) {
                    temp_receiver = chain_ret.struct_value;
                    temp_receiver.type = TYPE_STRUCT;
                    temp_receiver.is_struct = true;
                } else if (chain_ret.type == TYPE_STRING) {
                    temp_receiver.type = TYPE_STRING;
                    temp_receiver.str_value = chain_ret.str_value;
                } else {
                    temp_receiver.type = chain_ret.type;
                    temp_receiver.value = chain_ret.value;
                }

                method_context.concrete_receiver = temp_receiver;
                interpreter_.add_temp_variable(method_context.temp_variable_name, temp_receiver);
                receiver_name = method_context.temp_variable_name;
                receiver_resolution.kind = MethodReceiverResolution::Kind::Direct;
                receiver_resolution.variable_ptr = interpreter_.find_variable(receiver_name);
            } else {
                throw std::runtime_error("Invalid method receiver");
            }

            Variable* receiver_var = receiver_resolution.variable_ptr;
            if (!receiver_var) {
                receiver_var = interpreter_.find_variable(receiver_name);
            }
            if (!receiver_var) {
                throw std::runtime_error("Undefined receiver: " + receiver_name);
            }
            debug_msg(DebugMsgId::METHOD_CALL_RECEIVER_FOUND, receiver_name.c_str());
            debug_print("RECEIVER_DEBUG: Looking for receiver '%s'\n", receiver_name.c_str());

            std::string type_name;
            if (receiver_var->type == TYPE_STRUCT) {
                type_name = receiver_var->struct_type_name;
            } else if (!receiver_var->interface_name.empty()) {
                type_name = receiver_var->struct_type_name;
                debug_msg(DebugMsgId::METHOD_CALL_INTERFACE, node->name.c_str(), type_name.c_str());
            } else if (receiver_var->type >= TYPE_ARRAY_BASE) {
                if (!receiver_var->struct_type_name.empty()) {
                    type_name = receiver_var->struct_type_name;
                } else {
                    TypeInfo base_type = static_cast<TypeInfo>(receiver_var->type - TYPE_ARRAY_BASE);
                    type_name = type_info_to_string(base_type) + "[]";
                }
            } else {
                if (!receiver_var->struct_type_name.empty()) {
                    type_name = receiver_var->struct_type_name;
                } else {
                    type_name = type_info_to_string(receiver_var->type);
                }
            }

            std::string method_key = type_name + "::" + node->name;
            auto &global_scope = interpreter_.get_global_scope();
            auto it = global_scope.functions.find(method_key);
            if (it != global_scope.functions.end()) {
                func = it->second;
            } else {
                for (const auto& impl_def : interpreter_.get_impl_definitions()) {
                    if (impl_def.struct_name == type_name) {
                        std::string method_full_name = impl_def.interface_name + "_" + impl_def.struct_name + "_" + node->name;
                        auto it2 = global_scope.functions.find(method_full_name);
                        if (it2 != global_scope.functions.end()) {
                            func = it2->second;
                            break;
                        }
                    }
                }
            }
        } else {
            auto &global_scope = interpreter_.get_global_scope();
            auto it = global_scope.functions.find(node->name);
            if (it != global_scope.functions.end()) {
                func = it->second;
            }
        }

        if (!func) {
            throw std::runtime_error("Undefined function: " + node->name);
        }

        if (is_method_call && !receiver_name.empty()) {
            std::string private_check_name = receiver_name;

            if (private_check_name != "self") {
                Variable* receiver_var = interpreter_.find_variable(private_check_name);
                if (!receiver_var && receiver_resolution.variable_ptr) {
                    receiver_var = receiver_resolution.variable_ptr;
                }
                if (receiver_var) {
                    std::string type_name;
                    if (receiver_var->type == TYPE_STRUCT) {
                        type_name = receiver_var->struct_type_name;
                    } else if (!receiver_var->interface_name.empty()) {
                        type_name = receiver_var->struct_type_name;
                    } else {
                        type_name = type_info_to_string(receiver_var->type);
                    }
                    for (const auto& impl_def : interpreter_.get_impl_definitions()) {
                        if (impl_def.struct_name == type_name) {
                            for (const auto& method : impl_def.methods) {
                                if (method->name == node->name && method->is_private_method) {
                                    throw std::runtime_error("Cannot access private method '" + node->name + "' from outside the impl block");
                                }
                            }
                            break;
                        }
                    }
                }
            }
        }

        // 新しいスコープを作成
        auto cleanup_method_context = [&]() {
            if (method_context.uses_temp_receiver && !method_context.temp_variable_name.empty()) {
                Variable* temp_var = interpreter_.find_variable(method_context.temp_variable_name);
                if (temp_var && method_context.chain_value) {
                    if (temp_var->type == TYPE_STRUCT || temp_var->is_struct) {
                        method_context.chain_value->struct_value = *temp_var;
                        method_context.chain_value->struct_value.type = TYPE_STRUCT;
                        method_context.chain_value->struct_value.is_struct = true;
                        method_context.chain_value->is_struct = true;
                        method_context.chain_value->type = TYPE_STRUCT;
                    } else if (temp_var->type == TYPE_STRING) {
                        method_context.chain_value->str_value = temp_var->str_value;
                        method_context.chain_value->type = TYPE_STRING;
                        method_context.chain_value->is_struct = false;
                        method_context.chain_value->is_array = false;
                    } else {
                        method_context.chain_value->value = temp_var->value;
                        method_context.chain_value->type = temp_var->type;
                        method_context.chain_value->is_struct = false;
                        method_context.chain_value->is_array = false;
                    }
                }
                interpreter_.remove_temp_variable(method_context.temp_variable_name);
                method_context.uses_temp_receiver = false;
            }
        };
    interpreter_.push_scope();
    bool method_scope_active = true;
        
        // メソッド呼び出しの場合、selfコンテキストを設定
        if (is_method_call) {
            Variable* receiver_var = nullptr;
            if (!receiver_name.empty()) {
                receiver_var = interpreter_.find_variable(receiver_name);
            }
            if (!receiver_var && receiver_resolution.variable_ptr) {
                receiver_var = receiver_resolution.variable_ptr;
            }
            if (!receiver_var && node->left &&
                (node->left->node_type == ASTNodeType::AST_VARIABLE || node->left->node_type == ASTNodeType::AST_IDENTIFIER)) {
                receiver_var = interpreter_.find_variable(node->left->name);
                if (receiver_name.empty()) {
                    receiver_name = node->left->name;
                }
            }

            if (!receiver_var) {
                std::string error_name = receiver_name;
                if (error_name.empty() && node->left) {
                    error_name = node->left->name;
                }
                throw std::runtime_error("Receiver variable not found: " + error_name);
            }

            if (!receiver_name.empty() &&
                (receiver_var->type == TYPE_STRUCT || receiver_var->type == TYPE_INTERFACE || receiver_var->is_struct)) {
                interpreter_.sync_struct_members_from_direct_access(receiver_name);
                receiver_var = interpreter_.find_variable(receiver_name);
                if (!receiver_var) {
                    throw std::runtime_error("Receiver variable not found after sync: " + receiver_name);
                }
            }

            auto& current_scope = interpreter_.get_current_scope();
            current_scope.variables["self"] = *receiver_var;

            if (!receiver_name.empty()) {
                Variable receiver_info;
                receiver_info.type = TYPE_STRING;
                receiver_info.str_value = receiver_name;
                receiver_info.is_assigned = true;
                current_scope.variables["__self_receiver__"] = receiver_info;
                debug_msg(DebugMsgId::METHOD_CALL_SELF_CONTEXT_SET, receiver_name.c_str());
            }

            if (receiver_var->type == TYPE_STRUCT || receiver_var->type == TYPE_INTERFACE) {
                for (const auto& member_pair : receiver_var->struct_members) {
                    const std::string& member_name = member_pair.first;
                    std::string self_member_path = "self." + member_name;
                    Variable member_value = member_pair.second;

                    if (!receiver_name.empty()) {
                        if (Variable* direct_member_var = interpreter_.find_variable(receiver_name + "." + member_name)) {
                            member_value = *direct_member_var;
                        } else {
                            try {
                                if (Variable* struct_member = interpreter_.get_struct_member(receiver_name, member_name)) {
                                    member_value = *struct_member;
                                }
                            } catch (...) {
                                // ignore fallback failures
                            }
                        }
                    }

                    if (member_pair.second.is_multidimensional) {
                        member_value.is_multidimensional = true;
                        member_value.array_dimensions = member_pair.second.array_dimensions;
                        member_value.multidim_array_values = member_pair.second.multidim_array_values;
                        debug_print("SELF_SETUP: Preserved multidimensional info for %s (dimensions: %zu, values: %zu)\n",
                                    self_member_path.c_str(),
                                    member_pair.second.array_dimensions.size(),
                                    member_pair.second.multidim_array_values.size());
                    }

                    if (member_value.is_array) {
                        const bool is_string_array = (member_value.type == TYPE_STRING);

                        int total_elements = member_value.array_size;
                        if (total_elements <= 0) {
                            if (member_value.is_multidimensional && !member_value.multidim_array_values.empty()) {
                                total_elements = static_cast<int>(member_value.multidim_array_values.size());
                            } else if (!member_value.array_values.empty()) {
                                total_elements = static_cast<int>(member_value.array_values.size());
                            } else if (!member_value.array_dimensions.empty()) {
                                total_elements = 1;
                                for (int dim_size : member_value.array_dimensions) {
                                    if (dim_size == 0) {
                                        total_elements = 0;
                                        break;
                                    }
                                    total_elements *= dim_size;
                                }
                            }
                        }

                        if (total_elements < 0) {
                            total_elements = 0;
                        }

                        member_value.array_size = total_elements;

                        if (!is_string_array) {
                            if (member_value.is_multidimensional) {
                                if (member_value.array_values.size() < member_value.multidim_array_values.size()) {
                                    member_value.array_values = member_value.multidim_array_values;
                                } else if (member_value.array_values.empty() && !member_value.multidim_array_values.empty()) {
                                    member_value.array_values = member_value.multidim_array_values;
                                }
                            }
                            if (member_value.array_values.size() < static_cast<size_t>(total_elements)) {
                                member_value.array_values.resize(total_elements, 0);
                            }
                            if (member_value.is_multidimensional && member_value.multidim_array_values.size() < static_cast<size_t>(total_elements)) {
                                member_value.multidim_array_values.resize(total_elements, 0);
                            }
                        } else {
                            if (member_value.array_strings.size() < static_cast<size_t>(total_elements)) {
                                member_value.array_strings.resize(total_elements);
                            }
                        }

                        for (int idx = 0; idx < total_elements; ++idx) {
                            std::string element_path = self_member_path + "[" + std::to_string(idx) + "]";
                            Variable element_var;
                            bool element_assigned = false;

                            if (!receiver_name.empty()) {
                                std::string receiver_element_path = receiver_name + "." + member_name + "[" + std::to_string(idx) + "]";
                                if (Variable* receiver_element = interpreter_.find_variable(receiver_element_path)) {
                                    element_var = *receiver_element;
                                    element_assigned = true;
                                }
                            }

                            if (!element_assigned) {
                                element_var.type = is_string_array ? TYPE_STRING : member_value.type;
                                element_var.is_assigned = true;
                                if (is_string_array) {
                                    std::string value = (idx < static_cast<int>(member_value.array_strings.size()))
                                                        ? member_value.array_strings[idx]
                                                        : std::string();
                                    element_var.str_value = value;
                                } else {
                                    int64_t value = 0;
                                    if (member_value.is_multidimensional && idx < static_cast<int>(member_value.multidim_array_values.size())) {
                                        value = member_value.multidim_array_values[idx];
                                    } else if (idx < static_cast<int>(member_value.array_values.size())) {
                                        value = member_value.array_values[idx];
                                    }
                                    element_var.value = value;
                                }
                            }

                            current_scope.variables[element_path] = element_var;

                            if (is_string_array) {
                                if (idx >= static_cast<int>(member_value.array_strings.size())) {
                                    member_value.array_strings.resize(idx + 1);
                                }
                                member_value.array_strings[idx] = element_var.str_value;
                            } else {
                                if (idx >= static_cast<int>(member_value.array_values.size())) {
                                    member_value.array_values.resize(idx + 1);
                                }
                                member_value.array_values[idx] = element_var.value;
                                if (member_value.is_multidimensional) {
                                    if (idx >= static_cast<int>(member_value.multidim_array_values.size())) {
                                        member_value.multidim_array_values.resize(idx + 1);
                                    }
                                    member_value.multidim_array_values[idx] = element_var.value;
                                }
                            }
                        }
                    }

                    current_scope.variables[self_member_path] = member_value;
                    debug_print("SELF_SETUP: Created %s\n", self_member_path.c_str());
                }
                debug_msg(DebugMsgId::METHOD_CALL_SELF_MEMBER_SETUP);
            }
        }
        
        // 現在の関数名を設定
        std::string prev_function_name = interpreter_.current_function_name;
        interpreter_.current_function_name = node->name;
        
        debug_msg(DebugMsgId::METHOD_CALL_EXECUTE, node->name.c_str());
        
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
                        auto is_interface_compatible = [](const Variable* var) {
                            if (!var) {
                                return false;
                            }
                            if (var->is_struct || var->type == TYPE_INTERFACE) {
                                return true;
                            }
                            if (var->type >= TYPE_ARRAY_BASE) {
                                return true;
                            }
                            switch (var->type) {
                                case TYPE_INT:
                                case TYPE_LONG:
                                case TYPE_SHORT:
                                case TYPE_TINY:
                                case TYPE_BOOL:
                                case TYPE_STRING:
                                case TYPE_CHAR:
                                    return true;
                                default:
                                    return false;
                            }
                        };

                        auto assign_interface_argument = [&](const Variable& source, const std::string& source_name) {
                            Variable interface_placeholder(param->type_name, true);
                            interpreter_.assign_interface_view(param->name, interface_placeholder, source, source_name);
                        };

                        bool param_is_interface = false;
                        if (param->type_info == TYPE_INTERFACE) {
                            param_is_interface = true;
                        } else if (!param->type_name.empty()) {
                            if (interpreter_.find_interface_definition(param->type_name) != nullptr) {
                                param_is_interface = true;
                            }
                        }

                        if (param_is_interface) {
                            if (arg->node_type == ASTNodeType::AST_VARIABLE || arg->node_type == ASTNodeType::AST_IDENTIFIER) {
                                std::string source_name = arg->name;
                                Variable* source_var = interpreter_.find_variable(source_name);
                                if (!source_var) {
                                    throw std::runtime_error("Source variable not found: " + source_name);
                                }
                                if (!is_interface_compatible(source_var)) {
                                    throw std::runtime_error("Cannot pass non-struct/non-primitive to interface parameter '" + param->name + "'");
                                }
                                assign_interface_argument(*source_var, source_name);
                            } else if (arg->node_type == ASTNodeType::AST_STRING_LITERAL) {
                                Variable temp;
                                temp.type = TYPE_STRING;
                                temp.str_value = arg->str_value;
                                temp.is_assigned = true;
                                temp.struct_type_name = "string";
                                assign_interface_argument(temp, "");
                            } else {
                                auto build_temp_from_primitive = [&](TypeInfo value_type, int64_t numeric_value, const std::string& string_value) {
                                    Variable temp;
                                    temp.type = value_type;
                                    temp.is_assigned = true;
                                    if (!arg->type_name.empty()) {
                                        temp.struct_type_name = arg->type_name;
                                    } else {
                                        temp.struct_type_name = type_info_to_string(value_type);
                                    }
                                    if (value_type == TYPE_STRING) {
                                        temp.str_value = string_value;
                                    } else {
                                        temp.value = numeric_value;
                                    }
                                    return temp;
                                };

                                try {
                                    int64_t numeric_value = evaluate_expression(arg.get());
                                    TypeInfo resolved_type = arg->type_info != TYPE_UNKNOWN ? arg->type_info : TYPE_INT;
                                    if (resolved_type == TYPE_STRING) {
                                        Variable temp = build_temp_from_primitive(TYPE_STRING, 0, arg->str_value);
                                        assign_interface_argument(temp, "");
                                    } else {
                                        Variable temp = build_temp_from_primitive(resolved_type, numeric_value, "");
                                        assign_interface_argument(temp, "");
                                    }
                                } catch (const ReturnException& ret) {
                                    if (ret.is_array) {
                                        throw std::runtime_error("Cannot pass array return value to interface parameter '" + param->name + "'");
                                    }
                                    if (!ret.is_struct && ret.type == TYPE_STRING) {
                                        Variable temp = build_temp_from_primitive(TYPE_STRING, 0, ret.str_value);
                                        assign_interface_argument(temp, "");
                                    } else if (!ret.is_struct) {
                                        Variable temp = build_temp_from_primitive(ret.type, ret.value, ret.str_value);
                                        assign_interface_argument(temp, "");
                                    } else {
                                        assign_interface_argument(ret.struct_value, "");
                                    }
                                }
                            }
                            continue;
                        }

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
                                // 元の構造体定義から type_name 情報を取得
                                const StructDefinition* struct_def = interpreter_.find_struct_definition(resolved_struct_type);
                                for (const auto& member_pair : sync_source_var->struct_members) {
                                    std::string full_member_name = param->name + "." + member_pair.first;
                                    Variable member_var = member_pair.second;
                                    // 値を確実に設定
                                    member_var.is_assigned = true;
                                    
                                    // 元の構造体定義から type_name を取得して設定
                                    if (struct_def) {
                                        for (const auto& member : struct_def->members) {
                                            if (member.name == member_pair.first) {
                                                member_var.type_name = member.type_alias;
                                                member_var.is_pointer = member.is_pointer;
                                                member_var.pointer_depth = member.pointer_depth;
                                                member_var.pointer_base_type_name = member.pointer_base_type_name;
                                                member_var.pointer_base_type = member.pointer_base_type;
                                                break;
                                            }
                                        }
                                    }
                                    
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
                
                // メソッド実行後、selfの変更をレシーバーに同期
                if (has_receiver) {
                    Variable* receiver_var = interpreter_.find_variable(receiver_name);
                    if (receiver_var && (receiver_var->type == TYPE_STRUCT || receiver_var->type == TYPE_INTERFACE)) {
                        // selfの変更を元の構造体/interface変数に同期
                        Variable* self_var = interpreter_.find_variable("self");
                        if (self_var) {
                            // 構造体メンバーを同期
                            for (auto& receiver_member : receiver_var->struct_members) {
                                std::string self_member_path = "self." + receiver_member.first;
                                Variable* self_member_var = interpreter_.find_variable(self_member_path);
                                if (self_member_var) {
                                    // selfメンバーの値をレシーバーメンバーにコピー
                                    receiver_member.second.value = self_member_var->value;
                                    receiver_member.second.str_value = self_member_var->str_value;
                                    receiver_member.second.is_assigned = self_member_var->is_assigned;
                                    
                                    // ダイレクトアクセス変数も更新
                                    std::string direct_member_path = receiver_name + "." + receiver_member.first;
                                    Variable* direct_member_var = interpreter_.find_variable(direct_member_path);
                                    if (direct_member_var) {
                                        direct_member_var->value = self_member_var->value;
                                        direct_member_var->str_value = self_member_var->str_value;
                                        direct_member_var->is_assigned = self_member_var->is_assigned;
                                    }
                                }
                            }
                        }
                    }
                }
                
                cleanup_method_context();
                interpreter_.pop_scope();
                method_scope_active = false;
                interpreter_.current_function_name = prev_function_name;
                return 0;
            } catch (const ReturnException &ret) {
                // return文で戻り値がある場合
                
                // メソッド実行後、selfの変更をレシーバーに同期
                if (has_receiver) {
                    Variable* receiver_var = interpreter_.find_variable(receiver_name);
                    if (receiver_var && (receiver_var->type == TYPE_STRUCT || receiver_var->type == TYPE_INTERFACE)) {
                        // selfの変更を元の構造体/interface変数に同期
                        Variable* self_var = interpreter_.find_variable("self");
                        if (self_var) {
                            // 構造体メンバーを同期
                            for (auto& receiver_member : receiver_var->struct_members) {
                                std::string self_member_path = "self." + receiver_member.first;
                                Variable* self_member_var = interpreter_.find_variable(self_member_path);
                                if (self_member_var) {
                                    // selfメンバーの値をレシーバーメンバーにコピー
                                    receiver_member.second.value = self_member_var->value;
                                    receiver_member.second.str_value = self_member_var->str_value;
                                    receiver_member.second.is_assigned = self_member_var->is_assigned;
                                    
                                    // ダイレクトアクセス変数も更新
                                    std::string direct_member_path = receiver_name + "." + receiver_member.first;
                                    Variable* direct_member_var = interpreter_.find_variable(direct_member_path);
                                    if (direct_member_var) {
                                        direct_member_var->value = self_member_var->value;
                                        direct_member_var->str_value = self_member_var->str_value;
                                        direct_member_var->is_assigned = self_member_var->is_assigned;
                                    }
                                }
                            }
                        }
                    }
                }
                
                cleanup_method_context();
                interpreter_.pop_scope();
                method_scope_active = false;
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
            }
        } catch (const ReturnException &ret) {
            // 再投げされたReturnExceptionを処理
            cleanup_method_context();
            if (method_scope_active) {
                interpreter_.pop_scope();
                method_scope_active = false;
            }
            interpreter_.current_function_name = prev_function_name;
            throw ret;
        } catch (...) {
            cleanup_method_context();
            if (method_scope_active) {
                interpreter_.pop_scope();
                method_scope_active = false;
            }
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
        // メンバアクセス: obj.member または array[index].member または self.member
        std::string var_name;
        std::string member_name = node->name;

        // ネストしたメンバーアクセスの場合
        if (!node->member_chain.empty() && node->member_chain.size() > 1) {
            // 現在は基本機能に制限して、最初の2レベルのみサポート
            if (node->member_chain.size() > 2) {
                throw std::runtime_error("Deep nesting (>2 levels) not yet supported");
            }
            
            // ベース変数を取得
            Variable base_var;
            if (node->left->node_type == ASTNodeType::AST_VARIABLE) {
                Variable* var = interpreter_.find_variable(node->left->name);
                if (!var || var->type != TYPE_STRUCT) {
                    throw std::runtime_error("Base variable for nested access is not a struct: " + node->left->name);
                }
                base_var = *var;
            } else {
                throw std::runtime_error("Complex base types for nested access not yet supported");
            }
            
            // 段階的アクセス: base.member1.member2
            try {
                Variable intermediate_var = get_struct_member_from_variable(base_var, node->member_chain[0]);
                
                if (intermediate_var.type != TYPE_STRUCT) {
                    throw std::runtime_error("Intermediate member is not a struct: " + node->member_chain[0]);
                }
                
                Variable final_var = get_struct_member_from_variable(intermediate_var, node->member_chain[1]);
                
                if (final_var.type == TYPE_STRING) {
                    last_typed_result_ = TypedValue(final_var.str_value, InferredType(TYPE_STRING, "string"));
                    return 0;
                } else {
                    return final_var.value;
                }
            } catch (const std::exception& e) {
                throw std::runtime_error("Nested member access failed: " + std::string(e.what()));
            }
        }
        
        if (node->left->node_type == ASTNodeType::AST_VARIABLE) {
            // 通常のstruct変数: obj.member
            var_name = node->left->name;
        } else if (node->left->node_type == ASTNodeType::AST_IDENTIFIER && node->left->name == "self") {
            // selfメンバアクセス: self.member
            var_name = "self";
            debug_msg(DebugMsgId::SELF_MEMBER_ACCESS_START, member_name.c_str());
            
            // selfメンバーアクセスの特別処理
            std::string self_member_path = "self." + member_name;
            Variable* self_member = interpreter_.find_variable(self_member_path);
            if (self_member) {
                debug_msg(DebugMsgId::SELF_MEMBER_ACCESS_FOUND, self_member_path.c_str());
                if (self_member->type == TYPE_STRING) {
                    return 0; // 文字列の場合は別途処理
                }
                debug_msg(DebugMsgId::SELF_MEMBER_ACCESS_VALUE, self_member->value);
                return self_member->value;
            }
        } else if (node->left->node_type == ASTNodeType::AST_ARRAY_REF) {
            // struct配列要素: array[index].member
            std::string array_name = node->left->left->name;
            int64_t index = evaluate_expression(node->left->array_index.get());
            var_name = array_name + "[" + std::to_string(index) + "]";
        } else if (node->left->node_type == ASTNodeType::AST_FUNC_CALL) {
            // 関数呼び出し結果でのメンバアクセス: func().member
            debug_msg(DebugMsgId::EXPR_EVAL_START, "Function call member access");
            
            try {
                // 関数を実行してReturnExceptionを捕捉
                evaluate_typed_expression(node->left.get());
                // 通常の戻り値の場合はエラー
                throw std::runtime_error("Function did not return a struct for member access");
            } catch (const ReturnException& ret_ex) {
                // 構造体戻り値からメンバーを取得
                if (ret_ex.is_struct_array && ret_ex.struct_array_3d.size() > 0) {
                    // 構造体配列の場合（将来拡張）
                    throw std::runtime_error("Struct array function return member access not yet fully supported");
                } else {
                    // 単一構造体の場合
                    Variable struct_var = ret_ex.struct_value;
                    Variable member_var = get_struct_member_from_variable(struct_var, member_name);
                    
                    if (member_var.type == TYPE_STRING) {
                        // 文字列の場合は別途処理が必要（呼び出し元で処理される）
                        TypedValue typed_result(0, InferredType(TYPE_STRING, "string"));
                        typed_result.string_value = member_var.str_value;
                        typed_result.is_numeric_result = false;
                        last_typed_result_ = typed_result;
                        return 0;
                    } else {
                        return member_var.value;
                    }
                }
            }
        } else if (node->left->node_type == ASTNodeType::AST_ARRAY_REF && 
                   node->left->left && node->left->left->node_type == ASTNodeType::AST_FUNC_CALL) {
            // 関数配列戻り値でのメンバアクセス: func()[index].member
            debug_msg(DebugMsgId::EXPR_EVAL_START, "Function array member access");
            
            try {
                // 関数を実行してReturnExceptionを捕捉
                evaluate_expression(node->left->left.get());
                throw std::runtime_error("Function did not return an array for indexed member access");
            } catch (const ReturnException& ret_ex) {
                if (ret_ex.is_struct_array && ret_ex.struct_array_3d.size() > 0) {
                    // インデックスを評価
                    int64_t index = evaluate_expression(node->left->array_index.get());
                    
                    // 配列境界チェック
                    if (index < 0 || index >= (int64_t)ret_ex.struct_array_3d.size()) {
                        throw std::runtime_error("Array index out of bounds in function struct array member access");
                    }
                    
                    // 指定インデックスの構造体からメンバーを取得
                    if (ret_ex.struct_array_3d.size() > 0 && 
                        ret_ex.struct_array_3d[0].size() > 0 &&
                        ret_ex.struct_array_3d[0][0].size() > index) {
                        Variable struct_var = ret_ex.struct_array_3d[0][0][index];
                        Variable member_var = get_struct_member_from_variable(struct_var, member_name);
                        
                        if (member_var.type == TYPE_STRING) {
                            TypedValue typed_result(0, InferredType(TYPE_STRING, "string"));
                            typed_result.string_value = member_var.str_value;
                            typed_result.is_numeric_result = false;
                            last_typed_result_ = typed_result;
                            return 0;
                        } else {
                            return member_var.value;
                        }
                    } else {
                        throw std::runtime_error("Invalid struct array structure");
                    }
                } else {
                    throw std::runtime_error("Function did not return a struct array for indexed member access");
                }
            }
        } else {
            throw std::runtime_error("Invalid member access");
        }
        
        // 個別変数として直接アクセスを試す（構造体配列の場合）
        std::string full_member_path = var_name + "." + member_name;

        interpreter_.sync_struct_members_from_direct_access(var_name);
        interpreter_.ensure_struct_member_access_allowed(var_name, member_name);
        Variable* member_var = interpreter_.find_variable(full_member_path);
        
        if (!member_var) {
            // struct_membersから探す（通常の構造体の場合）
            member_var = interpreter_.get_struct_member(var_name, member_name);
        }
        
        if (!member_var) {
            throw std::runtime_error("Member not found: " + var_name + "." + member_name);
        }
        
        if (member_var->type == TYPE_STRING) {
            // 文字列メンバは別途処理が必要（呼び出し元で処理される）
            return 0; // 文字列の場合は0を返すが、実際の文字列は別途取得される
        }
        return member_var->value;
    }
    
    case ASTNodeType::AST_MEMBER_ARRAY_ACCESS: {
        // メンバの配列アクセス: obj.member[index] または func().member[index]
        std::string obj_name;
        Variable base_struct;
        bool is_function_call = false;
        
        if (node->left->node_type == ASTNodeType::AST_VARIABLE) {
            obj_name = node->left->name;
        } else if (node->left->node_type == ASTNodeType::AST_FUNC_CALL) {
            // 関数呼び出し結果でのメンバー配列アクセス: func().member[index]
            is_function_call = true;
            debug_msg(DebugMsgId::EXPR_EVAL_START, "Function call member array access");
            
            try {
                evaluate_expression(node->left.get());
                throw std::runtime_error("Function did not return a struct for member array access");
            } catch (const ReturnException& ret_ex) {
                if (ret_ex.is_struct_array && ret_ex.struct_array_3d.size() > 0) {
                    throw std::runtime_error("Struct array function return member array access not yet supported");
                } else {
                    base_struct = ret_ex.struct_value;
                    obj_name = "func_result"; // 仮の名前
                }
            }
        } else {
            throw std::runtime_error("Invalid object reference in member array access");
        }
        
        std::string member_name = node->name;
        
        std::cerr << "DEBUG_MEMBER_ARRAY: obj=" << obj_name << ", member=" << member_name 
                  << ", is_function_call=" << is_function_call << std::endl;
        
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
        Variable member_var_copy; // 関数呼び出しの場合用のコピー
        Variable *member_var;
        
        if (is_function_call) {
            // 関数戻り値からメンバーを取得
            member_var_copy = get_struct_member_from_variable(base_struct, member_name);
            member_var = &member_var_copy;
            
            // デバッグ情報
            std::cerr << "DEBUG: Function call member array access - member found" << std::endl;
            std::cerr << "DEBUG: Member is_array: " << member_var->is_array << std::endl;
            std::cerr << "DEBUG: Member array_values.size(): " << member_var->array_values.size() << std::endl;
        } else {
            member_var = interpreter_.get_struct_member(obj_name, member_name);
            if (!member_var) {
                throw std::runtime_error("Struct member not found: " + member_name);
            }
        }

        // 多次元配列の場合
        if (member_var->is_multidimensional && indices.size() > 1) {
            std::cerr << "DEBUG: Using getMultidimensionalArrayElement - indices.size()=" << indices.size() << std::endl;
            if (is_function_call) {
                // 関数戻り値の場合は直接配列要素を取得
                if (!member_var->is_array || member_var->array_values.empty()) {
                    throw std::runtime_error("Member is not a valid array for multi-dimensional access");
                }
                // 多次元インデックス計算（簡易版）
                int64_t flat_index = indices[0];
                if (indices.size() > 1 && member_var->is_multidimensional) {
                    // 簡易的な多次元計算（正確には別の実装が必要）
                    flat_index = indices[0] * 10 + indices[1]; // 仮の計算
                }
                if (flat_index >= 0 && flat_index < (int64_t)member_var->array_values.size()) {
                    return member_var->array_values[flat_index];
                } else {
                    throw std::runtime_error("Array index out of bounds in function member array access");
                }
            } else {
                return interpreter_.getMultidimensionalArrayElement(*member_var, indices);
            }
        }
        
        std::cerr << "DEBUG: Using 1D access - is_multidimensional=" << member_var->is_multidimensional 
                  << ", indices.size()=" << indices.size() << std::endl;
        
        // 1次元配列の場合
        int64_t index = indices[0];
        if (is_function_call) {
            // 関数戻り値の場合
            if (!member_var->is_array || member_var->array_values.empty()) {
                throw std::runtime_error("Member is not a valid array");
            }
            if (index >= 0 && index < (int64_t)member_var->array_values.size()) {
                return member_var->array_values[index];
            } else {
                throw std::runtime_error("Array index out of bounds in function member array access");
            }
        } else {
            std::cerr << "DEBUG: Calling interpreter_.get_struct_member_array_element with obj=" 
                      << obj_name << ", member=" << member_name << ", index=" << index << std::endl;
            return interpreter_.get_struct_member_array_element(obj_name, member_name, static_cast<int>(index));
        }
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

// 型をインターフェース用の文字列に変換するヘルパー関数
std::string ExpressionEvaluator::type_info_to_string(TypeInfo type) {
    const char* name = ::type_info_to_string(type);
    if (name && *name) {
        return std::string(name);
    }
    return "unknown";
}

void ExpressionEvaluator::sync_self_changes_to_receiver(const std::string& receiver_name, Variable* receiver_var) {
    debug_print("SELF_SYNC: Syncing self changes back to %s\n", receiver_name.c_str());
    
    // 構造体の各メンバーについて、selfから元の変数に同期
    for (const auto& member_pair : receiver_var->struct_members) {
        const std::string& member_name = member_pair.first;
        std::string self_member_path = "self." + member_name;
        std::string receiver_member_path = receiver_name + "." + member_name;
        
        // selfメンバーの変数を取得
        Variable* self_member = interpreter_.find_variable(self_member_path);
        Variable* receiver_member = interpreter_.find_variable(receiver_member_path);
        
        if (self_member && receiver_member) {
            // selfメンバーの値を元の変数に同期
            receiver_member->value = self_member->value;
            receiver_member->str_value = self_member->str_value;
            receiver_member->type = self_member->type;
            receiver_member->is_assigned = self_member->is_assigned;
            
            debug_print("SELF_SYNC: %s.%s = %lld (\"%s\")\n", 
                       receiver_name.c_str(), member_name.c_str(), 
                       (long long)receiver_member->value, 
                       receiver_member->str_value.c_str());
        }
    }
}

// 型推論対応の式評価
TypedValue ExpressionEvaluator::evaluate_typed_expression(const ASTNode* node) {
    if (!node) {
        return TypedValue(0, InferredType());
    }
    
    // ReturnExceptionをキャッチして構造体を処理
    try {
        return evaluate_typed_expression_internal(node);
    } catch (const ReturnException& ret_ex) {
        if (ret_ex.struct_value.type == TYPE_STRUCT) {
            // 構造体の場合、ReturnExceptionを再スロー（メンバアクセスで処理される）
            throw;
        } else {
            // 通常の値の場合
            return TypedValue(ret_ex.value, InferredType(TYPE_INT, "int"));
        }
    }
}

// 実際の型推論対応の式評価（内部実装）
TypedValue ExpressionEvaluator::evaluate_typed_expression_internal(const ASTNode* node) {
    if (!node) {
        return TypedValue(0, InferredType());
    }
    
    // まず型を推論
    InferredType inferred_type = type_engine_.infer_type(node);
    
    switch (node->node_type) {
        case ASTNodeType::AST_TERNARY_OP:
            return evaluate_ternary_typed(node);
            
        case ASTNodeType::AST_STRING_LITERAL:
            return TypedValue(node->str_value, InferredType(TYPE_STRING, "string"));
            
        case ASTNodeType::AST_NUMBER:
            return TypedValue(node->int_value, InferredType(TYPE_INT, "int"));
            
        case ASTNodeType::AST_ARRAY_LITERAL: {
            // 配列リテラルの場合、プレースホルダーとして0を返し、型情報を保持
            InferredType array_type = type_engine_.infer_type(node);
            return TypedValue(0, array_type);
        }
        
        case ASTNodeType::AST_FUNC_CALL: {
            // 関数呼び出しの場合、型推論を使って正確な型を決定
            try {
                // まず関数の戻り値型を推論
                InferredType function_return_type = type_engine_.infer_function_return_type(node->name, {});
                
                // 関数を実行して結果を取得
                int64_t numeric_result = evaluate_expression(node);
                
                // 推論された型に基づいて適切なTypedValueを返す
                if (function_return_type.type_info == TYPE_STRING) {
                    // 文字列戻り値の場合（実際の文字列は evaluate_expression では取得困難）
                    return TypedValue("", InferredType(TYPE_STRING, "string"));
                } else if (function_return_type.type_info == TYPE_STRUCT) {
                    // 構造体戻り値の場合は例外をキャッチして処理
                    throw std::runtime_error("Struct return should be caught as exception");
                } else {
                    // 数値戻り値の場合
                    return TypedValue(numeric_result, function_return_type);
                }
            } catch (const ReturnException& ret) {
                if (ret.is_struct || ret.type == TYPE_STRUCT) {
                    // 構造体の場合
                    Variable struct_var = ret.struct_value;
                    InferredType struct_type(TYPE_STRUCT, struct_var.struct_type_name);
                    return TypedValue(struct_var, struct_type);
                } else if (ret.type == TYPE_STRING) {
                    return TypedValue(ret.str_value, InferredType(TYPE_STRING, "string"));
                } else {
                    return TypedValue(ret.value, InferredType(ret.type, type_info_to_string(ret.type)));
                }
            }
        }
        
        case ASTNodeType::AST_VARIABLE: {
            // 変数参照の場合、変数の型に応じて適切なTypedValueを返す
            Variable *var = interpreter_.find_variable(node->name);
            if (!var) {
                std::string error_message = (debug_language == DebugLanguage::JAPANESE) ? 
                    "未定義の変数です: " + node->name : "Undefined variable: " + node->name;
                interpreter_.throw_runtime_error_with_location(error_message, node);
            }
            
            // 変数の型に基づいて適切なTypedValueを作成
            if (var->type == TYPE_STRING) {
                return TypedValue(var->str_value, InferredType(TYPE_STRING, "string"));
            } else if (var->type == TYPE_UNION) {
                if (var->current_type == TYPE_STRING) {
                    return TypedValue(var->str_value, InferredType(TYPE_STRING, "string"));
                } else {
                    return TypedValue(var->value, InferredType(var->current_type, type_info_to_string(var->current_type)));
                }
            } else {
                return TypedValue(var->value, InferredType(var->type, type_info_to_string(var->type)));
            }
        }
        
        case ASTNodeType::AST_MEMBER_ACCESS: {
            // func()[index].member パターンをチェック
            if (node->left && node->left->node_type == ASTNodeType::AST_ARRAY_REF &&
                node->left->left && node->left->left->node_type == ASTNodeType::AST_FUNC_CALL) {
                
                debug_print("Processing func()[index].member pattern: %s[].%s\n", 
                           node->left->left->name.c_str(), node->name.c_str());
                
                try {
                    // まず func()[index] を評価して構造体を取得
                    TypedValue array_element = evaluate_typed_expression(node->left.get());
                    
                    // この時点で array_element は構造体要素への参照のはずだが、
                    // 実際には ReturnException が投げられるはず
                    throw std::runtime_error("Expected struct return exception");
                    
                } catch (const ReturnException& struct_ret) {
                    if (struct_ret.is_struct) {
                        // 構造体からメンバーを取得
                        Variable member_var = get_struct_member_from_variable(struct_ret.struct_value, node->name);
                        
                        if (member_var.type == TYPE_STRING) {
                            return TypedValue(member_var.str_value, InferredType(TYPE_STRING, "string"));
                        } else {
                            return TypedValue(member_var.value, InferredType(TYPE_INT, "int"));
                        }
                    } else {
                        throw std::runtime_error("Expected struct element from function array access");
                    }
                }
            }
            
            // 構造体メンバアクセスの場合
            if (inferred_type.type_info == TYPE_STRING) {
                // 文字列型のメンバアクセス - 直接構造体メンバにアクセス
                if (node->left && node->left->node_type == ASTNodeType::AST_VARIABLE) {
                    std::string struct_name = node->left->name;
                    std::string member_name = node->name;
                    
                    // interpreter_.get_struct_member を使用する代わりに、直接値を取得
                    std::string member_var_name = struct_name + "." + member_name;
                    Variable* member_var = interpreter_.find_variable(member_var_name);
                    if (member_var && member_var->type == TYPE_STRING) {
                        return TypedValue(member_var->str_value, InferredType(TYPE_STRING, "string"));
                    }
                }
            }
            // 数値型やその他の型の場合は従来の評価を使用
            int64_t numeric_result = evaluate_expression(node);
            return TypedValue(numeric_result, inferred_type);
        }
        
        case ASTNodeType::AST_ARRAY_REF: {
            // 関数呼び出しの戻り値に対する配列アクセス: func()[index]
            if (node->left && node->left->node_type == ASTNodeType::AST_FUNC_CALL) {
                debug_print("Processing typed function call array access: %s\n", node->left->name.c_str());
                
                // インデックスを評価
                int64_t index = evaluate_expression(node->array_index.get());
                
                try {
                    // 関数を実行して戻り値を取得（副作用のため実行）
                    (void)evaluate_expression(node->left.get());
                    throw std::runtime_error("Function did not return an array via exception");
                } catch (const ReturnException& ret) {
                    if (ret.is_array) {
                        // 構造体配列の戻り値の場合
                        if (ret.is_struct_array && !ret.struct_array_3d.empty() && 
                            !ret.struct_array_3d[0].empty() && !ret.struct_array_3d[0][0].empty()) {
                            
                            if (index >= 0 && index < static_cast<int64_t>(ret.struct_array_3d[0][0].size())) {
                                // 構造体要素をReturnExceptionとして投げる
                                throw ReturnException(ret.struct_array_3d[0][0][index]);
                            } else {
                                throw std::runtime_error("Array index out of bounds");
                            }
                        }
                        // 文字列配列の戻り値の場合
                        else if (!ret.str_array_3d.empty() && 
                            !ret.str_array_3d[0].empty() && !ret.str_array_3d[0][0].empty()) {
                            
                            if (index >= 0 && index < static_cast<int64_t>(ret.str_array_3d[0][0].size())) {
                                return TypedValue(ret.str_array_3d[0][0][index], TYPE_STRING);
                            } else {
                                throw std::runtime_error("Array index out of bounds");
                            }
                        }
                        // 数値配列の戻り値の場合
                        else if (!ret.int_array_3d.empty() && 
                                 !ret.int_array_3d[0].empty() && !ret.int_array_3d[0][0].empty()) {
                            
                            if (index >= 0 && index < static_cast<int64_t>(ret.int_array_3d[0][0].size())) {
                                return TypedValue(ret.int_array_3d[0][0][index], TYPE_INT);
                            } else {
                                throw std::runtime_error("Array index out of bounds");
                            }
                        } else {
                            throw std::runtime_error("Empty array returned from function");
                        }
                    } else {
                        throw std::runtime_error("Function does not return an array");
                    }
                }
            }
            
            // 通常の配列要素アクセスの場合は直接評価
            int64_t numeric_result = evaluate_expression(node);
            return TypedValue(numeric_result, inferred_type);
        }
            
        default: {
            // デフォルトは従来の評価結果を数値として返す
            int64_t numeric_result = evaluate_expression(node);
            return TypedValue(numeric_result, inferred_type);
        }
    }
}

// 型推論対応の三項演算子評価
TypedValue ExpressionEvaluator::evaluate_ternary_typed(const ASTNode* node) {
    debug_msg(DebugMsgId::TERNARY_EVAL_START);
    
    // 条件式を評価
    int64_t condition = evaluate_expression(node->left.get());
    
    // 条件に基づいて選択されるノードを決定
    const ASTNode* selected_node = condition ? node->right.get() : node->third.get();
    
    // 選択されたノードの型を推論
    InferredType selected_type = type_engine_.infer_type(selected_node);
    
    debug_msg(DebugMsgId::TERNARY_NODE_TYPE, static_cast<int>(selected_node->node_type), static_cast<int>(selected_type.type_info));
    debug_msg(DebugMsgId::TERNARY_TYPE_INFERENCE, static_cast<int>(selected_type.type_info), selected_type.type_name.c_str());
    
    // 単純な型（数値、文字列）の場合は直接評価
    if (selected_type.type_info == TYPE_INT || selected_type.type_info == TYPE_BOOL) {
        TypedValue result = evaluate_typed_expression(selected_node);
        last_typed_result_ = result;
        return result;
    } else if (selected_type.type_info == TYPE_STRING && 
               selected_node->node_type == ASTNodeType::AST_STRING_LITERAL) {
        TypedValue result = evaluate_typed_expression(selected_node);
        last_typed_result_ = result;
        return result;
    } else if (selected_type.type_info == TYPE_STRING && 
               selected_node->node_type == ASTNodeType::AST_VARIABLE) {
        // 文字列変数参照の場合
        TypedValue result = evaluate_typed_expression(selected_node);
        last_typed_result_ = result;
        return result;
    } else if (selected_type.type_info == TYPE_STRING && 
               selected_node->node_type == ASTNodeType::AST_FUNC_CALL) {
        // 文字列を返す関数呼び出しの場合
        try {
            // 関数を実行（副作用のため実行）
            (void)evaluate_expression(selected_node);
            TypedValue result = TypedValue("", InferredType(TYPE_STRING, "string"));
            last_typed_result_ = result;
            return result;
        } catch (const ReturnException& ret) {
            if (ret.type == TYPE_STRING) {
                TypedValue result = TypedValue(ret.str_value, InferredType(TYPE_STRING, "string"));
                last_typed_result_ = result;
                return result;
            } else {
                TypedValue result = TypedValue("", InferredType(TYPE_STRING, "string"));
                last_typed_result_ = result;
                return result;
            }
        }
    } else if (selected_node->node_type == ASTNodeType::AST_ARRAY_REF) {
        // 配列アクセスの場合（関数呼び出し配列アクセスを含む）
        TypedValue result = evaluate_typed_expression(selected_node);
        last_typed_result_ = result;
        return result;
    } else if (selected_node->node_type == ASTNodeType::AST_TERNARY_OP) {
        // ネストした三項演算子の場合は再帰的に評価
        TypedValue result = evaluate_ternary_typed(selected_node);
        last_typed_result_ = result;
        return result;
    } else if (selected_node->node_type == ASTNodeType::AST_ARRAY_REF) {
        // 配列要素アクセスの場合は直接評価
        int64_t numeric_result = evaluate_expression(selected_node);
        TypedValue result = TypedValue(numeric_result, selected_type);
        last_typed_result_ = result;
        return result;
    } else if (selected_node->node_type == ASTNodeType::AST_MEMBER_ACCESS) {
        // 構造体メンバアクセスの場合 - 型に基づいて処理を分岐
        if (selected_type.type_info == TYPE_STRING) {
            debug_msg(DebugMsgId::TERNARY_STRING_MEMBER_ACCESS);
            // 文字列型のメンバアクセスの場合 - 直接構造体メンバにアクセス
            if (selected_node->left && selected_node->left->node_type == ASTNodeType::AST_VARIABLE) {
                std::string struct_name = selected_node->left->name;
                std::string member_name = selected_node->name;
                
                // interpreter_.get_struct_member を使用する代わりに、直接変数名で検索
                std::string member_var_name = struct_name + "." + member_name;
                Variable* member_var = interpreter_.find_variable(member_var_name);
                
                if (member_var && member_var->type == TYPE_STRING) {
                    debug_msg(DebugMsgId::TERNARY_STRING_EVAL, member_var->str_value.c_str());
                    TypedValue result = TypedValue(member_var->str_value, InferredType(TYPE_STRING, "string"));
                    last_typed_result_ = result;
                    return result;
                }
            }
        }
        // 数値型の場合は従来の評価を使用
        int64_t numeric_result = evaluate_expression(selected_node);
        TypedValue result = TypedValue(numeric_result, selected_type);
        last_typed_result_ = result;
        return result;
    } else if (selected_node->node_type == ASTNodeType::AST_FUNC_CALL) {
        // 関数呼び出し（メソッド呼び出し含む）の場合
        try {
            int64_t numeric_result = evaluate_expression(selected_node);
            debug_msg(DebugMsgId::TERNARY_NUMERIC_EVAL, numeric_result);
            TypedValue result = TypedValue(numeric_result, selected_type);
            last_typed_result_ = result;
            return result;
        } catch (const ReturnException& ret) {
            if (ret.type == TYPE_STRING) {
                TypedValue result = TypedValue(ret.str_value, InferredType(TYPE_STRING, "string"));
                last_typed_result_ = result;
                return result;
            } else {
                TypedValue result = TypedValue(ret.value, selected_type);
                last_typed_result_ = result;
                return result;
            }
        }
    }
    
    // 複雑な型（配列、構造体、関数呼び出しなど）の場合は遅延評価
    TypedValue result = TypedValue::deferred(selected_node, selected_type);
    last_typed_result_ = result;
    return result;
}

// 遅延評価されたTypedValueを実際に評価する
TypedValue ExpressionEvaluator::resolve_deferred_evaluation(const TypedValue& deferred_value) {
    if (!deferred_value.needs_deferred_evaluation() || !deferred_value.deferred_node) {
        return deferred_value; // 遅延評価が不要または無効
    }
    
    const ASTNode* node = deferred_value.deferred_node;
    
    switch (node->node_type) {
        case ASTNodeType::AST_ARRAY_LITERAL:
            // 配列リテラルの場合、ノード参照を返す（代入処理で使用）
            return TypedValue::deferred(node, deferred_value.type);
            
        case ASTNodeType::AST_STRUCT_LITERAL:
            // 構造体リテラルの場合、ノード参照を返す（代入処理で使用）
            return TypedValue::deferred(node, deferred_value.type);
            
        case ASTNodeType::AST_FUNC_CALL:
            // 関数呼び出しの場合、実際に実行して結果を取得
            return evaluate_typed_expression(node);
            
        default:
            // その他の場合は通常の評価
            return evaluate_typed_expression(node);
    }
}

// 構造体メンバー取得関数の実装
Variable ExpressionEvaluator::get_struct_member_from_variable(const Variable& struct_var, const std::string& member_name) {
    if (struct_var.type != TYPE_STRUCT) {
        throw std::runtime_error("Variable is not a struct");
    }
    
    auto enforce_privacy = [&](const Variable& member_var) -> Variable {
        if (!member_var.is_private_member) {
            return member_var;
        }

        std::string struct_type = struct_var.struct_type_name;
        if (struct_type.empty() && !struct_var.implementing_struct.empty()) {
            struct_type = struct_var.implementing_struct;
        }

        if (!interpreter_.is_current_impl_context_for(struct_type)) {
            std::string type_label = struct_type.empty() ? std::string("<anonymous>") : struct_type;
            throw std::runtime_error("Cannot access private struct member: " + type_label + "." + member_name);
        }

        return member_var;
    };

    // まず struct_members から直接検索
    auto member_it = struct_var.struct_members.find(member_name);
    if (member_it != struct_var.struct_members.end()) {
        return enforce_privacy(member_it->second);
    }
    
    // 構造体の識別子（struct_type_name）を使用してメンバーを検索
    std::string member_var_name = struct_var.struct_type_name + "." + member_name;
    Variable* member_var = interpreter_.find_variable(member_var_name);
    
    if (member_var) {
        return enforce_privacy(*member_var);
    }
    
    // インタープリターの get_struct_member を使用
    try {
        std::string temp_struct_name = "temp_struct_" + struct_var.struct_type_name;
        member_var = interpreter_.get_struct_member(temp_struct_name, member_name);
        if (member_var) {
            return enforce_privacy(*member_var);
        }
    } catch (...) {
        // 失敗した場合は続行
    }
    
    throw std::runtime_error("Struct member not found: " + member_name);
}

// 関数戻り値からのメンバーアクセス処理
TypedValue ExpressionEvaluator::evaluate_function_member_access(const ASTNode* func_node, const std::string& member_name) {
    debug_msg(DebugMsgId::EXPR_EVAL_START, "evaluate_function_member_access");
    
    try {
        // 関数を実行してReturnExceptionを捕捉
        evaluate_expression(func_node);
        throw std::runtime_error("Function did not return a struct for member access");
    } catch (const ReturnException& ret_ex) {
        debug_print("FUNC_MEMBER_ACCESS: ReturnException caught - type=%d, is_struct=%d\n", 
                   ret_ex.type, ret_ex.is_struct);
        debug_print("FUNC_MEMBER_ACCESS: struct_value type=%d, is_struct=%d, members=%zu\n",
                   ret_ex.struct_value.type, ret_ex.struct_value.is_struct, 
                   ret_ex.struct_value.struct_members.size());
        
        if (ret_ex.is_struct_array && ret_ex.struct_array_3d.size() > 0) {
            throw std::runtime_error("Struct array function return member access requires index");
        } else {
            // 単一構造体の場合
            Variable struct_var = ret_ex.struct_value;
            debug_print("FUNC_MEMBER_ACCESS: Looking for member %s in struct\n", member_name.c_str());
            Variable member_var = get_struct_member_from_variable(struct_var, member_name);
            
            if (member_var.type == TYPE_STRING) {
                TypedValue result(member_var.str_value, InferredType(TYPE_STRING, "string"));
                last_typed_result_ = result;
                return result;
            } else {
                TypedValue result(member_var.value, InferredType(TYPE_INT, "int"));
                last_typed_result_ = result;
                return result;
            }
        }
    }
}

// 関数戻り値からの配列アクセス処理
TypedValue ExpressionEvaluator::evaluate_function_array_access(const ASTNode* func_node, const ASTNode* index_node) {
    debug_msg(DebugMsgId::EXPR_EVAL_START, "evaluate_function_array_access");
    
    // インデックスを評価
    int64_t index = evaluate_expression(index_node);
    
    try {
        // 関数を実行して戻り値を取得
        evaluate_expression(func_node);
        throw std::runtime_error("Function did not return an array via exception");
    } catch (const ReturnException& ret) {
        if (!ret.is_array) {
            throw std::runtime_error("Function does not return an array");
        }
        
        if (ret.is_struct_array && !ret.struct_array_3d.empty() && 
            !ret.struct_array_3d[0].empty() && !ret.struct_array_3d[0][0].empty()) {
            // 構造体配列の場合
            if (index >= 0 && index < static_cast<int64_t>(ret.struct_array_3d[0][0].size())) {
                Variable struct_element = ret.struct_array_3d[0][0][index];
                // 構造体として返す（後でメンバーアクセス可能）
                TypedValue result(0, InferredType(TYPE_STRUCT, struct_element.struct_type_name));
                result.is_struct_result = true;
                result.struct_data = std::make_shared<Variable>(struct_element);  // 構造体データを保持
                last_typed_result_ = result;
                return result;
            } else {
                throw std::runtime_error("Array index out of bounds");
            }
        } else if (!ret.int_array_3d.empty() && 
                   !ret.int_array_3d[0].empty() && !ret.int_array_3d[0][0].empty()) {
            // 数値配列の場合
            if (index >= 0 && index < static_cast<int64_t>(ret.int_array_3d[0][0].size())) {
                return TypedValue(ret.int_array_3d[0][0][index], InferredType(TYPE_INT, "int"));
            } else {
                throw std::runtime_error("Array index out of bounds");
            }
        } else {
            throw std::runtime_error("Unsupported array type in function return");
        }
    }
}

// 関数戻り値からの複合アクセス処理（func()[index].member）
TypedValue ExpressionEvaluator::evaluate_function_compound_access(const ASTNode* func_node, const ASTNode* index_node, const std::string& member_name) {
    debug_msg(DebugMsgId::EXPR_EVAL_START, "evaluate_function_compound_access");
    
    // まず配列アクセスを実行
    TypedValue array_result = evaluate_function_array_access(func_node, index_node);
    
    if (!array_result.is_struct_result || !array_result.struct_data) {
        throw std::runtime_error("Array element is not a struct for member access");
    }
    
    // 構造体データからメンバーを取得
    Variable member_var = get_struct_member_from_variable(*array_result.struct_data, member_name);
    
    if (member_var.type == TYPE_STRING) {
        TypedValue result(member_var.str_value, InferredType(TYPE_STRING, "string"));
        last_typed_result_ = result;
        return result;
    } else {
        TypedValue result(member_var.value, InferredType(TYPE_INT, "int"));
        last_typed_result_ = result;
        return result;
    }
}

// 再帰的メンバーアクセス処理（将来のネスト構造体対応）
TypedValue ExpressionEvaluator::evaluate_recursive_member_access(const Variable& base_var, const std::vector<std::string>& member_path) {
    debug_msg(DebugMsgId::EXPR_EVAL_START, "evaluate_recursive_member_access");
    
    if (member_path.empty()) {
        throw std::runtime_error("Empty member path for recursive access");
    }
    
    std::cerr << "DEBUG_RECURSIVE: Starting recursive access with " << member_path.size() << " levels" << std::endl;
    for (size_t i = 0; i < member_path.size(); ++i) {
        std::cerr << "DEBUG_RECURSIVE: Path[" << i << "] = " << member_path[i] << std::endl;
    }
    
    Variable current_var = base_var;
    
    // 各レベルでのメンバーアクセスを再帰的に処理
    for (size_t i = 0; i < member_path.size(); ++i) {
        const std::string& member_name = member_path[i];
        std::cerr << "DEBUG_RECURSIVE: Accessing member[" << i << "] = " << member_name << std::endl;
        std::cerr << "DEBUG_RECURSIVE: Current var type = " << static_cast<int>(current_var.type) << std::endl;
        
        // 現在の変数が構造体でない場合はエラー
        if (current_var.type != TYPE_STRUCT) {
            throw std::runtime_error("Cannot access member '" + member_name + "' on non-struct type");
        }
        
        // メンバーを取得
        try {
            current_var = get_struct_member_from_variable(current_var, member_name);
            std::cerr << "DEBUG_RECURSIVE: Successfully accessed member, new type = " << static_cast<int>(current_var.type) << std::endl;
        } catch (const std::exception& e) {
            std::cerr << "DEBUG_RECURSIVE: Failed to access member '" << member_name << "': " << e.what() << std::endl;
            throw;
        }
        
        // 最後のレベルでない場合、構造体である必要がある（将来の拡張用）
        if (i < member_path.size() - 1 && current_var.type != TYPE_STRUCT) {
            throw std::runtime_error("Intermediate member '" + member_name + "' is not a struct for further nesting");
        }
    }
    
    // 最終結果を TypedValue に変換
    std::cerr << "DEBUG_RECURSIVE: Final result type = " << static_cast<int>(current_var.type) << std::endl;
    if (current_var.type == TYPE_STRING) {
        TypedValue result(0, InferredType(TYPE_STRING, "string"));
        result.string_value = current_var.str_value;
        result.is_numeric_result = false;
        return result;
    } else if (current_var.type == TYPE_STRUCT) {
        // 構造体の場合、完全なデータを保持
        TypedValue result(current_var, InferredType(TYPE_STRUCT, current_var.struct_type_name));
        std::cerr << "DEBUG_RECURSIVE: Returning struct TypedValue" << std::endl;
        return result;
    } else {
        return TypedValue(current_var.value, InferredType(TYPE_INT, "int"));
    }
}

ExpressionEvaluator::MethodReceiverResolution ExpressionEvaluator::resolve_array_receiver(const ASTNode* array_node) {
    MethodReceiverResolution result;
    if (!array_node || array_node->node_type != ASTNodeType::AST_ARRAY_REF) {
        return result;
    }

    // シンプルな変数配列の場合は直接参照を試みる
    if (array_node->left && array_node->left->node_type == ASTNodeType::AST_VARIABLE && array_node->array_index) {
        std::string base_name = array_node->left->name;
        try {
            int64_t index_value = evaluate_expression(array_node->array_index.get());
            std::string element_name = base_name + "[" + std::to_string(index_value) + "]";
            Variable* element_var = interpreter_.find_variable(element_name);
            if (element_var) {
                result.kind = MethodReceiverResolution::Kind::Direct;
                result.canonical_name = element_name;
                result.variable_ptr = element_var;
                return result;
            }
        } catch (const ReturnException&) {
            // インデックス評価で構造体等が返った場合はチェーン扱い
        }
    }

    return create_chain_receiver_from_expression(array_node);
}

ExpressionEvaluator::MethodReceiverResolution ExpressionEvaluator::resolve_member_receiver(const ASTNode* member_node) {
    MethodReceiverResolution result;
    if (!member_node || member_node->node_type != ASTNodeType::AST_MEMBER_ACCESS) {
        return result;
    }

    const ASTNode* base_node = member_node->left.get();
    if (!base_node) {
        return result;
    }

    const std::string member_name = member_node->name;

    std::function<std::string(const ASTNode*)> build_canonical_name = [&](const ASTNode* node) -> std::string {
        if (!node) {
            return "";
        }
        switch (node->node_type) {
        case ASTNodeType::AST_VARIABLE:
        case ASTNodeType::AST_IDENTIFIER:
            return node->name;
        case ASTNodeType::AST_MEMBER_ACCESS: {
            std::string base = build_canonical_name(node->left.get());
            if (base.empty()) {
                return "";
            }
            return base + "." + node->name;
        }
        default:
            return "";
        }
    };

    MethodReceiverResolution base_resolution = resolve_method_receiver(base_node);

    auto create_chain_from_struct = [&](const Variable& struct_var) {
        try {
            Variable member_var = get_struct_member_from_variable(struct_var, member_name);
            auto chain_ret = std::make_shared<ReturnException>(member_var);
            result.kind = MethodReceiverResolution::Kind::Chain;
            result.chain_value = chain_ret;
            return true;
        } catch (const std::exception&) {
            return false;
        }
    };

    if (base_resolution.kind == MethodReceiverResolution::Kind::Direct && base_resolution.variable_ptr) {
        Variable* base_var = base_resolution.variable_ptr;
        std::string base_name = base_resolution.canonical_name;
        if (base_name.empty()) {
            base_name = build_canonical_name(base_node);
        }

        if (!base_name.empty()) {
            std::string member_path = base_name + "." + member_name;
            Variable* member_var = interpreter_.find_variable(member_path);
            if (!member_var) {
                try {
                    member_var = interpreter_.get_struct_member(base_name, member_name);
                } catch (...) {
                    member_var = nullptr;
                }
            }

            if (member_var) {
                result.kind = MethodReceiverResolution::Kind::Direct;
                result.canonical_name = member_path;
                result.variable_ptr = member_var;
                return result;
            }
        }

        if ((base_var->type == TYPE_STRUCT || base_var->is_struct || base_var->type == TYPE_INTERFACE) &&
            create_chain_from_struct(*base_var)) {
            return result;
        }
    }

    if (base_resolution.kind == MethodReceiverResolution::Kind::Chain && base_resolution.chain_value) {
        const ReturnException& chain_ret = *base_resolution.chain_value;
        if (chain_ret.is_struct || chain_ret.type == TYPE_STRUCT) {
            if (create_chain_from_struct(chain_ret.struct_value)) {
                return result;
            }
        }
    }

    // 直接解決できない場合は式全体をチェーンとして扱う
    return create_chain_receiver_from_expression(member_node);
}

ExpressionEvaluator::MethodReceiverResolution ExpressionEvaluator::create_chain_receiver_from_expression(const ASTNode* node) {
    MethodReceiverResolution result;
    if (!node) {
        return result;
    }

    try {
        int64_t primitive_value = evaluate_expression(node);
        InferredType inferred_type = type_engine_.infer_type(node);
        TypeInfo chain_type = inferred_type.type_info;
        if (chain_type == TYPE_UNKNOWN) {
            chain_type = TYPE_INT;
        }
        ReturnException chain_ret(primitive_value, chain_type);
        result.kind = MethodReceiverResolution::Kind::Chain;
        result.chain_value = std::make_shared<ReturnException>(chain_ret);
        return result;
    } catch (const ReturnException& ret) {
        result.kind = MethodReceiverResolution::Kind::Chain;
        result.chain_value = std::make_shared<ReturnException>(ret);
        return result;
    }
}
