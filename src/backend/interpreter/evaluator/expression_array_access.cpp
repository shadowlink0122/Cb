#include "expression_array_access.h"
#include "core/interpreter.h"
#include "../../../common/ast.h"
#include "../../../common/debug.h"
#include "../../../common/utf8_utils.h"
#include "core/error_handler.h"
#include <iostream>
#include <stdexcept>

namespace ArrayAccessHelpers {

// ========================================================================
// 配列アクセス（AST_ARRAY_REF）の評価
// ========================================================================
int64_t evaluate_array_ref(
    const ASTNode* node,
    Interpreter& interpreter,
    std::function<int64_t(const ASTNode*)> evaluate_expression_func,
    std::function<Variable(const Variable&, const std::string&)> get_struct_member_func
) {
    bool debug_mode = interpreter.is_debug_mode();
    
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
            int64_t index = evaluate_expression_func(current_node->array_index.get());
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
        Variable *member_var = interpreter.get_struct_member(obj_name, member_name);
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
            return interpreter.get_struct_member_multidim_array_element(obj_name, member_name, indices);
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
        int64_t index = evaluate_expression_func(node->array_index.get());
        
        // 関数呼び出しの場合
        if (node->left->left && node->left->left->node_type == ASTNodeType::AST_FUNC_CALL) {
            try {
                evaluate_expression_func(node->left->left.get());
                throw std::runtime_error("Function did not return a struct for member array access");
            } catch (const ReturnException& ret_ex) {
                Variable base_struct = ret_ex.struct_value;
                Variable member_var_copy = get_struct_member_func(base_struct, member_name);
                
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
                return interpreter.get_struct_member_array_element(obj_name, member_name, static_cast<int>(index));
            } catch (const std::exception& e) {
                // 失敗した場合は従来の方式を試す
                std::string member_array_element_name = obj_name + "." + member_name + "[" + std::to_string(index) + "]";
                
                Variable *var = interpreter.find_variable(member_array_element_name);
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
        int64_t index = evaluate_expression_func(node->array_index.get());
        
        try {
            // 関数を実行して戻り値を取得（副作用のため実行）
            (void)evaluate_expression_func(node->left.get());
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

    std::string array_name = interpreter.extract_array_name(node);
    if (array_name.empty()) {
        throw std::runtime_error("Cannot determine array name");
    }
    
    std::vector<int64_t> indices = interpreter.extract_array_indices(node);
    
    Variable *var = interpreter.find_variable(array_name);
    if (!var) {
        throw std::runtime_error("Undefined array: " + array_name);
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
        int64_t result = interpreter.getMultidimensionalArrayElement(*var, indices);
        if (interpreter.is_debug_mode()) {
            debug_print("[DBG multidim] %s dims=%zu value=%lld\n",
                        array_name.c_str(), indices.size(),
                        static_cast<long long>(result));
        }
        return result;
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
    
    // 1次元float配列のアクセス
    if (var->is_array && !var->array_float_values.empty() && indices.size() == 1) {
        int64_t array_index = indices[0];
        
        if (array_index < 0 || array_index >= static_cast<int64_t>(var->array_float_values.size())) {
            throw std::runtime_error("Array index out of bounds");
        }
        
        // float値を整数に変換して返す（int64_tを要求される場面用）
        // 注意: 精度が失われるため、型付き評価(evaluate_typed_expression)を使うべき
        return static_cast<int64_t>(var->array_float_values[array_index]);
    }
    
    // 1次元double配列のアクセス
    if (var->is_array && !var->array_double_values.empty() && indices.size() == 1) {
        int64_t array_index = indices[0];
        
        if (array_index < 0 || array_index >= static_cast<int64_t>(var->array_double_values.size())) {
            throw std::runtime_error("Array index out of bounds");
        }
        
        // double値を整数に変換して返す（int64_tを要求される場面用）
        // 注意: 精度が失われるため、型付き評価(evaluate_typed_expression)を使うべき
        return static_cast<int64_t>(var->array_double_values[array_index]);
    }
    
    // 1次元quad配列のアクセス
    if (var->is_array && !var->array_quad_values.empty() && indices.size() == 1) {
        int64_t array_index = indices[0];
        
        if (array_index < 0 || array_index >= static_cast<int64_t>(var->array_quad_values.size())) {
            throw std::runtime_error("Array index out of bounds");
        }
        
        // quad値を整数に変換して返す（int64_tを要求される場面用）
        // 注意: 精度が失われるため、型付き評価(evaluate_typed_expression)を使うべき
        return static_cast<int64_t>(var->array_quad_values[array_index]);
    }
    
    if (var->array_values.empty() && var->array_float_values.empty() && 
        var->array_double_values.empty() && var->array_quad_values.empty()) {
        if (!var->is_array) {
            throw std::runtime_error("Variable is not an array");
        }
        return var->value; // スカラー値の場合
    }

    // フラットインデックスの計算
    int64_t flat_index = 0;
    if (indices.size() == 1) {
        flat_index = indices[0];
    } else if (indices.size() > 1 && !var->array_dimensions.empty()) {
        // 多次元配列の場合
        flat_index = indices[0];
        for (size_t i = 1; i < indices.size(); i++) {
            flat_index = flat_index * var->array_dimensions[i] + indices[i];
        }
    }
    
    if (flat_index < 0 || flat_index >= static_cast<int64_t>(var->array_values.size())) {
        throw std::runtime_error("Array index out of bounds");
    }
    
    return var->array_values[flat_index];
}

// ========================================================================
// 配列リテラル（AST_ARRAY_LITERAL）の評価
// ========================================================================
int64_t evaluate_array_literal(const ASTNode* node, Interpreter& interpreter) {
    // 配列リテラルは通常、変数代入やprintf等の文脈で使用される
    // int64_tを返す必要がある場合は0を返す（実際の配列データは別途処理される）
    return 0;
}

} // namespace ArrayAccessHelpers
