#include "../../../../common/debug.h"
#include "../../../../common/debug_messages.h"
#include "../../../../common/type_helpers.h"
#include "../../core/interpreter.h"
#include "../../services/debug_service.h"
#include "core/type_inference.h"
#include "evaluator/core/evaluator.h"
#include "managers/arrays/manager.h"
#include "managers/common/operations.h"
#include "managers/types/enums.h"
#include "managers/types/manager.h"
#include "managers/variables/manager.h"
#include <algorithm>
#include <cstdio>
#include <cstring> // for strdup
#include <numeric>
#include <utility>

namespace {

std::string getPrimitiveTypeNameForImpl(TypeInfo type) {
    return std::string(type_info_to_string(type));
}

} // namespace

void VariableManager::process_variable_assignment(const ASTNode *node) {
    // 変数代入の処理

    // 配列リテラル代入の特別処理
    if (node->right &&
        node->right->node_type == ASTNodeType::AST_ARRAY_LITERAL) {
        std::string var_name;
        if (node->left && node->left->node_type == ASTNodeType::AST_VARIABLE) {
            var_name = node->left->name;
        } else if (!node->name.empty()) {
            var_name = node->name;
        } else {
            throw std::runtime_error(
                "Array literal can only be assigned to simple variables");
        }

        interpreter_->assign_array_literal(var_name, node->right.get());
        return;
    }

    if (!node->name.empty() && node->right) {
        // node->nameを使った代入（通常の変数代入）
        std::string var_name = node->name;

        Variable *var = find_variable(var_name);
        if (!var) {
            throw std::runtime_error("Undefined variable: " + var_name);
        }

        if (var->is_const && var->is_assigned) {
            error_msg(DebugMsgId::CONST_REASSIGN_ERROR, var_name.c_str());
            throw std::runtime_error("Cannot reassign const variable: " +
                                     var_name);
        }

        // Union型変数への代入の特別処理
        if (var->type == TYPE_UNION) {
            if (debug_mode) {
                debug_msg(DebugMsgId::GENERIC_DEBUG,
                          "UNION_ASSIGN_DEBUG: Processing union ");
            }
            assign_union_value(*var, var->type_name, node->right.get());
            return; // Union型代入処理完了後は早期リターン
        }

        // Interface型の変数（ポインタを除く）の代入処理
        if (debug_mode) {
            debug_msg(DebugMsgId::GENERIC_DEBUG,
                      "VAR_ASSIGN_DEBUG: var_name=%s, var->type=%d, ");
        }
        if ((var->type == TYPE_INTERFACE || !var->interface_name.empty()) &&
            var->type != TYPE_POINTER) {
            auto assign_from_source = [&](const Variable &source,
                                          const std::string &source_name) {
                assign_interface_view(var_name, *var, source, source_name);
            };

            auto create_temp_primitive = [&](TypeInfo value_type,
                                             int64_t numeric_value,
                                             const std::string &string_value) {
                Variable temp;
                temp.is_assigned = true;
                temp.type = value_type;
                if (value_type == TYPE_STRING) {
                    temp.str_value = string_value;
                    // value フィールドに文字列のコピーのポインタを保存（generic
                    // 型で使用される）
                    temp.value = reinterpret_cast<int64_t>(
                        strdup(temp.str_value.c_str()));
                } else {
                    temp.value = numeric_value;
                }
                temp.struct_type_name = getPrimitiveTypeNameForImpl(value_type);
                return temp;
            };

            try {
                const ASTNode *rhs = node->right.get();
                if (rhs->node_type == ASTNodeType::AST_VARIABLE ||
                    rhs->node_type == ASTNodeType::AST_IDENTIFIER) {
                    std::string source_var_name = rhs->name;
                    Variable *source_var = find_variable(source_var_name);
                    if (!source_var) {
                        throw std::runtime_error("Source variable not found: " +
                                                 source_var_name);
                    }
                    assign_from_source(*source_var, source_var_name);
                    return;
                }

                if (rhs->node_type == ASTNodeType::AST_STRING_LITERAL) {
                    Variable temp =
                        create_temp_primitive(TYPE_STRING, 0, rhs->str_value);
                    assign_from_source(temp, "");
                    return;
                }

                int64_t numeric_value =
                    interpreter_->expression_evaluator_->evaluate_expression(
                        rhs);
                TypeInfo resolved_type =
                    rhs->type_info != TYPE_UNKNOWN ? rhs->type_info : TYPE_INT;
                Variable temp =
                    create_temp_primitive(resolved_type, numeric_value, "");
                assign_from_source(temp, "");
                return;
            } catch (const ReturnException &ret) {
                if (ret.is_array) {
                    throw std::runtime_error(
                        "Cannot assign array return value to interface "
                        "variable '" +
                        var_name + "'");
                }

                if (!ret.is_struct) {
                    if (TypeHelpers::isString(ret.type)) {
                        Variable temp = create_temp_primitive(TYPE_STRING, 0,
                                                              ret.str_value);
                        assign_from_source(temp, "");
                        return;
                    }

                    TypeInfo resolved_type =
                        ret.type != TYPE_UNKNOWN ? ret.type : TYPE_INT;
                    Variable temp = create_temp_primitive(
                        resolved_type, ret.value, ret.str_value);
                    assign_from_source(temp, "");
                    return;
                }

                assign_from_source(ret.struct_value, "");
                return;
            }
        }

        // Union型変数への代入の特別処理
        if (var->type == TYPE_UNION) {
            if (debug_mode) {
                debug_msg(DebugMsgId::GENERIC_DEBUG,
                          "UNION_ASSIGN_DEBUG: Processing union ");
            }
            assign_union_value(*var, var->type_name, node->right.get());
            return; // Union型代入処理完了後は早期リターン
        }

        // 文字列変数への代入で、右辺が多次元配列アクセスの場合の特別処理
        if (var->type == TYPE_STRING &&
            node->right->node_type == ASTNodeType::AST_ARRAY_REF) {
            // 配列名を取得
            std::string array_name;
            const ASTNode *base_node = node->right.get();
            while (base_node &&
                   base_node->node_type == ASTNodeType::AST_ARRAY_REF &&
                   base_node->left) {
                base_node = base_node->left.get();
            }
            if (base_node &&
                base_node->node_type == ASTNodeType::AST_VARIABLE) {
                array_name = base_node->name;
            }

            Variable *array_var = find_variable(array_name);
            if (array_var && array_var->is_array &&
                array_var->array_type_info.base_type == TYPE_STRING) {
                debug_msg(DebugMsgId::MULTIDIM_STRING_ARRAY_ACCESS,
                          array_name.c_str());

                // 多次元インデックスを収集
                std::vector<int64_t> indices;
                const ASTNode *current_node = node->right.get();
                while (current_node &&
                       current_node->node_type == ASTNodeType::AST_ARRAY_REF) {
                    int64_t index = interpreter_->expression_evaluator_
                                        ->evaluate_expression(
                                            current_node->array_index.get());
                    indices.insert(indices.begin(),
                                   index); // 先頭に挿入（逆順になるため）
                    current_node = current_node->left.get();
                }

                // インデックス情報をデバッグ出力
                std::string indices_str;
                for (size_t i = 0; i < indices.size(); ++i) {
                    if (i > 0)
                        indices_str += ", ";
                    indices_str += std::to_string(indices[i]);
                }
                debug_msg(DebugMsgId::MULTIDIM_STRING_ARRAY_INDICES,
                          indices_str.c_str());

                try {
                    std::string str_value =
                        interpreter_->getMultidimensionalStringArrayElement(
                            *array_var, indices);
                    debug_msg(DebugMsgId::MULTIDIM_STRING_ARRAY_VALUE,
                              str_value.c_str());
                    var->str_value = str_value;
                    var->is_assigned = true;
                    return;
                } catch (const std::exception &e) {
                    var->str_value = "";
                    var->is_assigned = true;
                    return;
                }
            }
        }

        int64_t value =
            interpreter_->expression_evaluator_->evaluate_expression(
                node->right.get());

        clamp_unsigned_value(*var, value, "received assignment", node);

        // varは既に上で定義済み
        if (var->is_const && var->is_assigned) {
            error_msg(DebugMsgId::CONST_REASSIGN_ERROR, var_name.c_str());
            throw std::runtime_error("Cannot reassign const variable: " +
                                     var_name);
        }

        // 型範囲チェック（代入前に実行）
        // ポインタ型の場合はスキップ
        if (!var->is_pointer) {
            interpreter_->type_manager_->check_type_range(
                var->type, value, var_name, var->is_unsigned);
        }

        var->value = value;
        var->is_assigned = true;

    } else if (node->left &&
               node->left->node_type == ASTNodeType::AST_VARIABLE) {
        // 通常の変数代入
        std::string var_name = node->left->name;

        Variable *var = find_variable(var_name);
        if (!var) {
            throw std::runtime_error("Undefined variable: " + var_name);
        }

        if (var->is_const && var->is_assigned) {
            error_msg(DebugMsgId::CONST_REASSIGN_ERROR, var_name.c_str());
            throw std::runtime_error("Cannot reassign const variable: " +
                                     var_name);
        }

        // Union型変数への代入の特別処理
        if (var->type == TYPE_UNION) {
            if (debug_mode) {
                debug_msg(DebugMsgId::GENERIC_DEBUG,
                          "UNION_ASSIGN_DEBUG: Processing union ");
            }
            assign_union_value(*var, var->type_name, node->right.get());
            return; // Union型代入処理完了後は早期リターン
        }

        int64_t value =
            interpreter_->expression_evaluator_->evaluate_expression(
                node->right.get());

        clamp_unsigned_value(*var, value, "received assignment", node);

        // 型範囲チェック（代入前に実行）
        // ポインタ型の場合はスキップ
        if (!var->is_pointer) {
            interpreter_->type_manager_->check_type_range(
                var->type, value, var_name, var->is_unsigned);
        }

        var->value = value;
        var->is_assigned = true;
    } else if (node->left &&
               node->left->node_type == ASTNodeType::AST_ARRAY_REF) {
        // 配列要素代入の処理（N次元対応）
        std::string array_name = extract_array_name(node->left.get());
        if (array_name.empty()) {
            throw std::runtime_error("Cannot determine array name");
        }

        std::vector<int64_t> indices = extract_array_indices(node->left.get());
        int64_t value =
            interpreter_->expression_evaluator_->evaluate_expression(
                node->right.get());

        Variable *var = find_variable(array_name);
        if (!var) {
            throw std::runtime_error("Undefined array: " + array_name);
        }

        // 文字列要素への代入の場合
        if (var->type == TYPE_STRING && !var->is_array) {
            if (indices.size() != 1) {
                throw std::runtime_error("Invalid string element access");
            }

            if (var->is_const) {
                throw std::runtime_error(
                    "Cannot assign to const string element: " + array_name);
            }

            int64_t index = indices[0];
            // 文字列要素代入は interpreter_.assign_string_element を使用
            interpreter_->assign_string_element(
                array_name, index, std::string(1, static_cast<char>(value)));
            return;
        }

        if (!var->is_array) {
            throw std::runtime_error("Not an array: " + array_name);
        }

        // 多次元配列の場合
        if (var->is_multidimensional && indices.size() > 1) {
            interpreter_->array_manager_->setMultidimensionalArrayElement(
                *var, indices, value);
        } else if (indices.size() == 1) {
            // 1次元配列の場合
            // const配列への書き込みチェック
            if (var->is_const && var->is_assigned) {
                throw std::runtime_error("Cannot assign to const array: " +
                                         array_name);
            }

            int64_t index = indices[0];
            if (index < 0 ||
                index >= static_cast<int64_t>(var->array_values.size())) {
                throw std::runtime_error("Array index out of bounds");
            }
            var->array_values[index] = value;
        } else {
            throw std::runtime_error("Invalid array access");
        }
    } else if (node->left &&
               node->left->node_type == ASTNodeType::AST_MEMBER_ACCESS) {
        // struct メンバー代入の処理: obj.member = value または
        // array[index].member = value
        std::string member_name = node->left->name;
        Variable *struct_var = nullptr;
        std::string struct_name;

        if (node->left->left->node_type == ASTNodeType::AST_VARIABLE) {
            // 通常のstruct変数: obj.member = value
            struct_name = node->left->left->name;
            struct_var = find_variable(struct_name);
        } else if (node->left->left->node_type == ASTNodeType::AST_ARRAY_REF) {
            // struct配列要素: array[index].member = value
            std::string array_name = node->left->left->left->name;
            int64_t index =
                interpreter_->expression_evaluator_->evaluate_expression(
                    node->left->left->array_index.get());
            struct_name = array_name + "[" + std::to_string(index) + "]";
            struct_var = find_variable(struct_name);
        }

        if (!struct_var) {
            throw std::runtime_error("Undefined struct variable: " +
                                     struct_name);
        }

        if (!struct_var->is_struct) {
            throw std::runtime_error(struct_name + " is not a struct");
        }

        // メンバーが存在するかチェック
        auto member_it = struct_var->struct_members.find(member_name);
        if (member_it == struct_var->struct_members.end()) {
            throw std::runtime_error("Struct " + struct_name +
                                     " has no member: " + member_name);
        }

        // 右辺の値を評価
        Variable &member = member_it->second;

        if (TypeHelpers::isString(member.type)) {
            // 文字列型メンバーの場合
            if (node->right->node_type == ASTNodeType::AST_STRING_LITERAL) {
                member.str_value = node->right->str_value;
            } else {
                // 数値を文字列に変換
                int64_t value =
                    interpreter_->expression_evaluator_->evaluate_expression(
                        node->right.get());
                member.str_value = std::to_string(value);
            }
        } else {
            // 数値型メンバーの場合
            int64_t value =
                interpreter_->expression_evaluator_->evaluate_expression(
                    node->right.get());
            member.value = value;
        }
        member.is_assigned = true;
    } else if (node->left &&
               node->left->node_type == ASTNodeType::AST_MEMBER_ARRAY_ACCESS) {
        // struct メンバー配列要素代入の処理: obj.member[index] = value
        // または func().member[index] = value
        std::string member_name = node->left->name;

        if (!node->left->left) {
            throw std::runtime_error("Invalid struct member array access");
        }

        // 関数呼び出しの場合と通常の変数の場合を分岐
        if (node->left->left->node_type == ASTNodeType::AST_FUNC_CALL) {
            // 関数呼び出しの場合: func().member[index] = value

            try {
                interpreter_->expression_evaluator_->evaluate_expression(
                    node->left->left.get());
                throw std::runtime_error("Function did not return a struct "
                                         "for member array assignment");
            } catch (const ReturnException &ret_ex) {
                Variable base_struct = ret_ex.struct_value;

                // メンバー配列を取得
                auto member_it = base_struct.struct_members.find(member_name);
                if (member_it == base_struct.struct_members.end()) {
                    throw std::runtime_error("Struct member not found: " +
                                             member_name);
                }

                Variable &member_var = member_it->second;
                if (!member_var.is_array) {
                    throw std::runtime_error("Member is not an array: " +
                                             member_name);
                }

                // インデックスを評価
                std::vector<int64_t> indices;
                if (node->left->array_indices.empty() &&
                    node->left->arguments.empty()) {
                    throw std::runtime_error(
                        "No indices found for array access");
                }

                if (!node->left->array_indices.empty()) {
                    for (const auto &arg : node->left->array_indices) {
                        int64_t index = interpreter_->expression_evaluator_
                                            ->evaluate_expression(arg.get());
                        indices.push_back(index);
                    }
                } else {
                    for (const auto &arg : node->left->arguments) {
                        int64_t index = interpreter_->expression_evaluator_
                                            ->evaluate_expression(arg.get());
                        indices.push_back(index);
                    }
                }

                // 1次元配列の場合
                if (indices.size() == 1) {
                    int64_t index = indices[0];
                    if (index < 0 ||
                        index >=
                            static_cast<int>(member_var.array_values.size())) {
                        throw std::runtime_error("Array index out of bounds");
                    }

                    // 右辺の値を評価（副作用のため実行）
                    (void)interpreter_->expression_evaluator_
                        ->evaluate_expression(node->right.get());

                    // 値を代入（関数戻り値なので実際の代入はできないが、エラーを避けるため）
                    throw std::runtime_error("Cannot assign to function "
                                             "return value member array");
                } else {
                    throw std::runtime_error(
                        "Multi-dimensional function return member array "
                        "assignment not supported");
                }
            }
            return;
        } else if (node->left->left->node_type != ASTNodeType::AST_VARIABLE) {
            throw std::runtime_error("Invalid struct member array access");
        }

        std::string struct_name = node->left->left->name;
        Variable *struct_var = find_variable(struct_name);

        if (!struct_var) {
            throw std::runtime_error("Undefined struct variable: " +
                                     struct_name);
        }

        if (!struct_var->is_struct) {
            throw std::runtime_error(struct_name + " is not a struct");
        }

        // インデックスを評価（多次元対応）
        std::vector<int64_t> indices;
        if (node->left->right) {
            // 1次元の場合（従来通り）
            int64_t index =
                interpreter_->expression_evaluator_->evaluate_expression(
                    node->left->right.get());
            indices.push_back(index);
        } else if (!node->left->arguments.empty()) {
            // 多次元の場合
            for (const auto &arg : node->left->arguments) {
                int64_t index =
                    interpreter_->expression_evaluator_->evaluate_expression(
                        arg.get());
                indices.push_back(index);
            }
        } else {
            throw std::runtime_error("No indices found for array access");
        }

        // 構造体メンバー変数を取得
        Variable *member_var =
            interpreter_->get_struct_member(struct_name, member_name);
        if (!member_var) {
            throw std::runtime_error("Struct member not found: " + member_name);
        }

        // 多次元配列の場合の処理
        if (member_var->is_multidimensional && indices.size() > 1) {
            // 多次元配列の要素への代入
            if (node->right->node_type == ASTNodeType::AST_STRING_LITERAL) {
                std::string value = node->right->str_value;
                interpreter_->setMultidimensionalStringArrayElement(
                    *member_var, indices, value);
            } else {
                int64_t value =
                    interpreter_->expression_evaluator_->evaluate_expression(
                        node->right.get());
                interpreter_->setMultidimensionalArrayElement(*member_var,
                                                              indices, value);
            }
            return;
        }

        // 1次元配列または多次元配列の1次元アクセスの場合（従来処理）
        int64_t index = indices[0];

        // メンバー配列要素の変数名を生成: s.grades[0]
        std::string element_name =
            struct_name + "." + member_name + "[" + std::to_string(index) + "]";
        Variable *element_var = find_variable(element_name);

        if (!element_var) {
            throw std::runtime_error("Member array element not found: " +
                                     element_name);
        }

        // 右辺の値を評価
        int64_t value =
            interpreter_->expression_evaluator_->evaluate_expression(
                node->right.get());

        // 型範囲チェック
        interpreter_->type_manager_->check_type_range(
            element_var->type, value, element_name, element_var->is_unsigned);

        // 値を代入
        element_var->value = value;
        element_var->is_assigned = true;

        if (interpreter_->debug_mode) {
            {
                char dbg_buf[512];
                snprintf(dbg_buf, sizeof(dbg_buf),
                         "Assigned %lld to struct member array element: %s",
                         (long long)value, element_name.c_str());
                debug_msg(DebugMsgId::GENERIC_DEBUG, dbg_buf);
            }
        }
    }
    // 他の複雑なケースは後で実装
}

// N次元配列の配列名を抽出する汎用関数
std::string VariableManager::extract_array_name(const ASTNode *node) {
    if (!node)
        return "";

    if (node->node_type == ASTNodeType::AST_VARIABLE) {
        return node->name;
    } else if (node->node_type == ASTNodeType::AST_ARRAY_REF) {
        if (!node->name.empty()) {
            return node->name; // 直接名前がある場合
        } else if (node->left) {
            return extract_array_name(node->left.get()); // 再帰的に探索
        }
    } else if (node->node_type == ASTNodeType::AST_MEMBER_ACCESS) {
        // メンバアクセスの場合: obj.member
        std::string obj_name;
        if (node->left && node->left->node_type == ASTNodeType::AST_VARIABLE) {
            obj_name = node->left->name;
        } else {
            return "";
        }
        std::string member_name = node->name;
        return obj_name + "." + member_name;
    }
    return "";
}

// N次元配列のインデックスを抽出する汎用関数
std::vector<int64_t>
VariableManager::extract_array_indices(const ASTNode *node) {
    std::vector<int64_t> indices;

    if (!node || node->node_type != ASTNodeType::AST_ARRAY_REF) {
        return indices;
    }

    // 現在のインデックスを評価
    if (node->array_index) {
        int64_t index =
            interpreter_->expression_evaluator_->evaluate_expression(
                node->array_index.get());
        indices.push_back(index);
    }

    // 左側に更なる配列アクセスがあるかチェック
    if (node->left && node->left->node_type == ASTNodeType::AST_ARRAY_REF) {
        std::vector<int64_t> left_indices =
            extract_array_indices(node->left.get());
        // 左側のインデックスを先頭に挿入
        indices.insert(indices.begin(), left_indices.begin(),
                       left_indices.end());
    }

    return indices;
}

// ============================================================================
// Helper Methods
// ============================================================================
