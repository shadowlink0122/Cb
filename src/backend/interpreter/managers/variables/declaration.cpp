#include "../../../../common/debug.h"
#include "../../../../common/debug_messages.h"
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
#include <cctype>
#include <cstdio>
#include <numeric>
#include <utility>

namespace {

std::string trim(const std::string &str) {
    auto begin = std::find_if_not(str.begin(), str.end(), [](unsigned char ch) {
        return std::isspace(ch);
    });
    auto end = std::find_if_not(str.rbegin(), str.rend(), [](unsigned char ch) {
                   return std::isspace(ch);
               }).base();
    if (begin >= end) {
        return "";
    }
    return std::string(begin, end);
}

void setNumericFields(Variable &var, long double quad_value) {
    var.quad_value = quad_value;
    var.double_value = static_cast<double>(quad_value);
    var.float_value = static_cast<float>(quad_value);
    var.value = static_cast<int64_t>(quad_value);
}

} // namespace

void VariableManager::process_variable_declaration(const ASTNode *node) {
    // 変数宣言の処理
    Variable var;
    // ポインタ型の場合はTYPE_POINTERを設定
    if (node->is_pointer) {
        var.type = TYPE_POINTER;
        var.is_pointer = true;
        var.pointer_depth = node->pointer_depth;
        var.pointer_base_type = node->pointer_base_type;
        var.pointer_base_type_name = node->pointer_base_type_name;
    } else {
        var.type = node->type_info;
    }
    var.is_const = node->is_const;
    var.is_assigned = false;
    var.is_array = false;
    var.array_size = 0;
    var.is_unsigned = node->is_unsigned;
    // ポインタのconst修飾子
    var.is_pointer_const = node->is_pointer_const_qualifier;
    var.is_pointee_const = node->is_pointee_const_qualifier;

    // struct変数の場合の追加設定
    if (node->type_info == TYPE_STRUCT && !node->type_name.empty()) {
        var.is_struct = true;
        var.struct_type_name = node->type_name;
    }

    // interface変数の場合の追加設定
    if ((node->type_info == TYPE_INTERFACE ||
         (node->is_pointer && node->pointer_base_type == TYPE_INTERFACE)) &&
        !node->type_name.empty()) {
        // ポインタの場合は、ベース型名を使用
        if (node->is_pointer && !node->pointer_base_type_name.empty()) {
            var.interface_name = node->pointer_base_type_name;
        } else {
            var.interface_name = node->type_name;
        }
    }

    // 新しいArrayTypeInfoが設定されている場合の処理
    if (handle_array_type_info_declaration(node, var)) {
        // ArrayTypeInfoで処理完了、次の分岐へ
    }
    // typedef解決処理（ArrayTypeInfoが設定されていない場合）
    else if (handle_typedef_resolution(node, var)) {
        // Union型の場合は既に処理完了しているので早期リターン
        if (var.type == TYPE_UNION) {
            return; // Union型は handle_union_typedef_declaration 内で全て完了
        }
        // typedef解決完了、初期化処理へ
        if (node->right || node->init_expr) {
            ASTNode *init_node =
                node->init_expr ? node->init_expr.get() : node->right.get();

            // 三項演算子による初期化の新しい処理（型推論使用）
            if (init_node->node_type == ASTNodeType::AST_TERNARY_OP) {
                TypedValue ternary_result =
                    interpreter_->evaluate_ternary_typed(init_node);

                if (ternary_result.is_string()) {
                    var.str_value = ternary_result.string_value;
                    var.value = 0;
                } else {
                    var.value = ternary_result.value;
                    var.str_value = "";
                }

                interpreter_->current_scope().variables[node->name] = var;
                return; // 早期リターン
            }

            // 型チェック: typedef変数の初期化値が適切な型かチェック
            if (var.type == TYPE_STRING &&
                init_node->node_type == ASTNodeType::AST_NUMBER) {
                throw std::runtime_error(
                    "Type mismatch: Cannot assign integer value " +
                    std::to_string(init_node->int_value) + " to string type '" +
                    node->type_name + "'");
            } else if ((var.type == TYPE_INT || var.type == TYPE_LONG ||
                        var.type == TYPE_SHORT || var.type == TYPE_TINY) &&
                       init_node->node_type ==
                           ASTNodeType::AST_STRING_LITERAL) {
                throw std::runtime_error(
                    "Type mismatch: Cannot assign string value '" +
                    init_node->str_value + "' to numeric type '" +
                    node->type_name + "'");
            } else if (var.type == TYPE_BOOL &&
                       init_node->node_type == ASTNodeType::AST_NUMBER &&
                       init_node->int_value != 0 && init_node->int_value != 1) {
                throw std::runtime_error(
                    "Type mismatch: Cannot assign integer value " +
                    std::to_string(init_node->int_value) +
                    " to boolean type '" + node->type_name + "'");
            }

            // カスタム型（typedef）変数代入の型チェック
            if (init_node->node_type == ASTNodeType::AST_VARIABLE) {
                Variable *source_var = find_variable(init_node->name);
                if (source_var && !source_var->type_name.empty()) {
                    // 代入元がカスタム型を持つ場合、型名の整合性をチェック
                    std::string source_resolved =
                        interpreter_->type_manager_->resolve_typedef(
                            source_var->type_name);
                    std::string target_resolved =
                        interpreter_->type_manager_->resolve_typedef(
                            node->type_name);

                    // 基本型は同じだが、カスタム型名が異なる場合
                    if (source_resolved == target_resolved &&
                        source_var->type_name != node->type_name) {
                        // 再帰的typedefでは同じ基本型に解決される場合は互換性がある
                        // 例: ID=int, UserID=ID
                        // の場合、IDとUserIDは互換性がある
                        // この場合は型チェックを通す（TypeScriptの型エイリアス的動作）
                        if (interpreter_->is_debug_mode()) {
                            debug_print(
                                "RECURSIVE_TYPEDEF_DEBUG: %s and %s both "
                                "resolve to %s - allowing assignment\n",
                                source_var->type_name.c_str(),
                                node->type_name.c_str(),
                                source_resolved.c_str());
                        }
                        // 互換性があるものとして処理を続行
                    }
                }
            }

            if (var.type == TYPE_STRING &&
                init_node->node_type == ASTNodeType::AST_STRING_LITERAL) {
                // 文字列リテラル初期化
                var.str_value = init_node->str_value;
                var.value = 0; // プレースホルダー
                var.is_assigned = true;
            } else if (var.type == TYPE_STRING &&
                       init_node->node_type == ASTNodeType::AST_ARRAY_REF) {
                // 文字列配列アクセス初期化
                // 配列名を取得
                std::string array_name;
                const ASTNode *base_node = init_node;
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
                    const ASTNode *current_node = init_node;
                    while (current_node && current_node->node_type ==
                                               ASTNodeType::AST_ARRAY_REF) {
                        int64_t index =
                            interpreter_->expression_evaluator_
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
                        var.str_value = str_value;
                        var.value = 0; // プレースホルダー
                        var.is_assigned = true;
                    } catch (const std::exception &e) {
                        var.str_value = "";
                        var.value = 0;
                        var.is_assigned = true;
                    }
                } else {
                    // 配列アクセスではない場合は通常の処理にフォールバック
                    int64_t value = interpreter_->expression_evaluator_
                                        ->evaluate_expression(init_node);
                    var.str_value = std::to_string(value);
                    var.value = value;
                    var.is_assigned = true;
                }
            } else if (var.type == TYPE_STRING &&
                       init_node->node_type == ASTNodeType::AST_BINARY_OP &&
                       init_node->op == "+") {
                // 文字列連結の処理
                std::string left_str, right_str;
                bool success = true;

                // 左オペランドを取得
                if (init_node->left->node_type == ASTNodeType::AST_VARIABLE) {
                    Variable *left_var = find_variable(init_node->left->name);
                    if (left_var && (left_var->type == TYPE_STRING ||
                                     left_var->current_type == TYPE_STRING)) {
                        left_str = left_var->str_value;
                    } else {
                        success = false;
                    }
                } else if (init_node->left->node_type ==
                           ASTNodeType::AST_STRING_LITERAL) {
                    left_str = init_node->left->str_value;
                } else {
                    success = false;
                }

                // 右オペランドを取得
                if (success) {
                    if (init_node->right->node_type ==
                        ASTNodeType::AST_VARIABLE) {
                        Variable *right_var =
                            find_variable(init_node->right->name);
                        if (right_var &&
                            (right_var->type == TYPE_STRING ||
                             right_var->current_type == TYPE_STRING)) {
                            right_str = right_var->str_value;
                        } else {
                            success = false;
                        }
                    } else if (init_node->right->node_type ==
                               ASTNodeType::AST_STRING_LITERAL) {
                        right_str = init_node->right->str_value;
                    } else {
                        success = false;
                    }
                }

                if (success) {
                    // 文字列連結を実行
                    var.str_value = left_str + right_str;
                    var.value = 0; // プレースホルダー
                    var.is_assigned = true;
                } else {
                    // 文字列連結に失敗した場合は通常の処理にフォールバック
                    throw std::runtime_error("String concatenation failed "
                                             "for typedef variable '" +
                                             node->name + "'");
                }
            } else {
                // その他の初期化
                try {
                    int64_t value = interpreter_->expression_evaluator_
                                        ->evaluate_expression(init_node);
                    clamp_unsigned_value(var, value,
                                         "initialized with expression", node);
                    var.value = value;
                    var.is_assigned = true;

                    if (interpreter_->is_debug_mode() && node->name == "ptr") {
                        std::cerr
                            << "[VAR_MANAGER] Pointer variable initialized:"
                            << std::endl;
                        std::cerr << "  value=" << value << " (0x" << std::hex
                                  << value << std::dec << ")" << std::endl;
                        std::cerr << "  var.value=" << var.value << " (0x"
                                  << std::hex << var.value << std::dec << ")"
                                  << std::endl;
                        std::cerr << "  var.type=" << static_cast<int>(var.type)
                                  << std::endl;
                    }

                    // 型範囲チェック（ポインタ型・ポインタ配列は除外）
                    if (var.type != TYPE_STRING && var.type != TYPE_POINTER &&
                        !(var.is_pointer && var.is_array)) {
                        interpreter_->type_manager_->check_type_range(
                            var.type, var.value, node->name, var.is_unsigned);
                    }
                } catch (const ReturnException &ret) {
                    // 関数戻り値の処理
                    if (ret.is_function_pointer) {
                        // 関数ポインタ戻り値の場合
                        if (debug_mode) {
                            std::cerr
                                << "[VAR_MANAGER] Function pointer return: "
                                << ret.function_pointer_name << " -> "
                                << ret.value << std::endl;
                        }
                        var.value = ret.value;
                        var.is_assigned = true;
                        var.is_function_pointer = true;

                        // function_pointersマップに登録
                        FunctionPointer func_ptr(
                            ret.function_pointer_node,
                            ret.function_pointer_name,
                            ret.function_pointer_node->type_info);
                        interpreter_->current_scope()
                            .function_pointers[node->name] = func_ptr;
                    } else if (var.type == TYPE_STRING &&
                               ret.type == TYPE_STRING) {
                        // 文字列戻り値の場合
                        var.str_value = ret.str_value;
                        var.is_assigned = true;
                    } else if (ret.is_struct && var.type == TYPE_STRUCT) {
                        // struct戻り値の場合
                        var = ret.struct_value;
                        var.is_assigned = true;
                    } else if (ret.is_struct && var.type == TYPE_UNION) {
                        // union型変数への構造体代入の場合
                        if (interpreter_->get_type_manager()
                                ->is_custom_type_allowed_for_union(
                                    var.type_name,
                                    ret.struct_value.struct_type_name)) {
                            var.value = ret.struct_value.value;
                            var.str_value = ret.struct_value.str_value;
                            var.current_type = TYPE_STRUCT;
                            var.is_struct = true;
                            var.struct_type_name =
                                ret.struct_value.struct_type_name;
                            var.struct_members =
                                ret.struct_value.struct_members;
                            var.is_assigned = true;
                        } else {
                            throw std::runtime_error(
                                "Struct type '" +
                                ret.struct_value.struct_type_name +
                                "' is not allowed for union type " +
                                var.type_name);
                        }
                    } else if (ret.is_array) {
                        auto &scope_vars = current_scope().variables;
                        bool inserted_temp = false;
                        auto it = scope_vars.find(node->name);
                        if (it == scope_vars.end()) {
                            scope_vars[node->name] = var;
                            inserted_temp = true;
                        } else {
                            it->second = var;
                        }

                        try {
                            interpreter_->assign_array_from_return(node->name,
                                                                   ret);
                            var = scope_vars[node->name];
                            var.is_assigned = true;
                        } catch (...) {
                            if (inserted_temp) {
                                scope_vars.erase(node->name);
                            }
                            throw;
                        }
                    } else if (!ret.is_array && !ret.is_struct) {
                        // 数値戻り値の場合
                        int64_t numeric_value = ret.value;
                        clamp_unsigned_value(var, numeric_value,
                                             "initialized with function return",
                                             node);
                        var.value = numeric_value;
                        var.is_assigned = true;

                        // 型範囲チェック（ポインタ型・ポインタ配列は除外）
                        if (var.type != TYPE_STRING &&
                            var.type != TYPE_POINTER &&
                            !(var.is_pointer && var.is_array)) {
                            interpreter_->type_manager_->check_type_range(
                                var.type, var.value, node->name,
                                var.is_unsigned);
                        }
                    } else {
                        throw std::runtime_error("Incompatible return type "
                                                 "for typedef variable '" +
                                                 node->name + "'");
                    }
                } catch (const std::exception &e) {
                    throw std::runtime_error(
                        "Failed to initialize typedef variable '" + node->name +
                        "': " + e.what());
                }
            }
        }
    }

    // struct型メンバーの初期化
    handle_struct_member_initialization(node, var);

    // 配列タイプチェック（直接配列宣言の場合）
    if (!var.is_array && node->type_name.find("[") != std::string::npos) {
        var.is_array = true;

        // 配列サイズを解析
        size_t bracket_pos = node->type_name.find("[");

        if (bracket_pos != std::string::npos) {
            std::string base = trim(node->type_name.substr(0, bracket_pos));
            std::string array_part = node->type_name.substr(bracket_pos);

            TypeInfo base_type =
                interpreter_->type_manager_->string_to_type_info(base);
            var.type = static_cast<TypeInfo>(TYPE_ARRAY_BASE + base_type);
            var.type_name = node->type_name;

            auto dimensions = parse_array_dimensions(array_part, node);
            initialize_array_from_dimensions(var, base_type, dimensions);
        }
    }

    // 初期化式がある場合
    if (node->init_expr) {
        if (var.is_struct &&
            node->init_expr->node_type == ASTNodeType::AST_STRUCT_LITERAL) {
            // struct literal初期化の処理: Person p = {25, "Bob"};

            // まず変数を登録
            current_scope().variables[node->name] = var;

            // struct literal代入を実行
            interpreter_->assign_struct_literal(node->name,
                                                node->init_expr.get());

            // 代入完了
            current_scope().variables[node->name].is_assigned = true;

            return; // struct literal処理完了後は早期リターン

        }
        // Interface型変数の初期化処理
        else if (handle_interface_initialization(node, var)) {
            return; // Interface初期化完了（早期return）
        }
        // 配列リテラル初期化処理
        else if (handle_array_literal_initialization(node, var)) {
            return; // 配列リテラル初期化完了（早期return）
        } else if (var.is_struct &&
                   node->init_expr->node_type == ASTNodeType::AST_VARIABLE) {
            // struct to struct代入の処理: Person p2 = p1;
            std::string source_var_name = node->init_expr->name;
            Variable *source_var = find_variable(source_var_name);
            if (!source_var) {
                throw std::runtime_error("Source variable not found: " +
                                         source_var_name);
            }

            if (!source_var->is_struct) {
                throw std::runtime_error(
                    "Cannot assign non-struct to struct variable");
            }

            if (source_var->struct_type_name != var.struct_type_name) {
                throw std::runtime_error(
                    "Cannot assign struct of different type");
            }

            // まず変数を登録
            current_scope().variables[node->name] = var;

            // 全メンバをコピー
            for (const auto &member : source_var->struct_members) {
                current_scope()
                    .variables[node->name]
                    .struct_members[member.first] = member.second;

                // 直接アクセス変数もコピー
                std::string source_member_name =
                    source_var_name + "." + member.first;
                std::string dest_member_name = node->name + "." + member.first;
                Variable *source_member_var = find_variable(source_member_name);
                if (source_member_var) {
                    Variable member_copy = *source_member_var;
                    current_scope().variables[dest_member_name] = member_copy;

                    // 配列メンバの場合、個別要素変数もコピー
                    if (source_member_var->is_array) {
                        for (int i = 0; i < source_member_var->array_size;
                             i++) {
                            std::string source_element_name =
                                source_member_name + "[" + std::to_string(i) +
                                "]";
                            std::string dest_element_name =
                                dest_member_name + "[" + std::to_string(i) +
                                "]";
                            Variable *source_element_var =
                                find_variable(source_element_name);
                            if (source_element_var) {
                                Variable element_copy = *source_element_var;
                                current_scope().variables[dest_element_name] =
                                    element_copy;

                                if (interpreter_->debug_mode) {
                                    if (source_element_var->type ==
                                        TYPE_STRING) {
                                        debug_print("STRUCT_COPY: Copied array "
                                                    "element %s = '%s' to %s\n",
                                                    source_element_name.c_str(),
                                                    source_element_var
                                                        ->str_value.c_str(),
                                                    dest_element_name.c_str());
                                    } else {
                                        debug_print(
                                            "STRUCT_COPY: Copied array "
                                            "element %s = %lld to %s\n",
                                            source_element_name.c_str(),
                                            (long long)
                                                source_element_var->value,
                                            dest_element_name.c_str());
                                    }
                                }
                            }
                        }
                    }
                }
            }

            // 代入完了
            current_scope().variables[node->name].is_assigned = true;

            return; // struct代入処理完了後は早期リターン

        } else if (var.is_struct &&
                   node->init_expr->node_type == ASTNodeType::AST_FUNC_CALL) {
            // 構造体変数の関数呼び出し初期化: Calculator add_result =
            // math.add(10, 5);

            try {
                // 構造体戻り値を期待した関数実行（副作用のため実行）
                (void)interpreter_->expression_evaluator_->evaluate_expression(
                    node->init_expr.get());
                // 通常の数値戻り値の場合はエラー
                throw std::runtime_error(
                    "Expected struct return but got numeric value");
            } catch (const ReturnException &ret) {
                if (ret.is_struct) {
                    // 構造体戻り値を変数に代入
                    var = ret.struct_value;
                    var.is_assigned = true;

                    if (interpreter_->debug_mode && node->name == "student1") {
                        debug_print("FUNC_RETURN_RECEIVED: "
                                    "ret.struct_value has %zu members\n",
                                    ret.struct_value.struct_members.size());
                        auto scores_it =
                            ret.struct_value.struct_members.find("scores");
                        if (scores_it !=
                                ret.struct_value.struct_members.end() &&
                            scores_it->second.is_array) {
                            debug_print("FUNC_RETURN_RECEIVED: "
                                        "scores.array_size=%d, "
                                        "array_values.size()=%zu\n",
                                        scores_it->second.array_size,
                                        scores_it->second.array_values.size());
                            if (scores_it->second.array_values.size() >= 3) {
                                debug_print(
                                    "FUNC_RETURN_RECEIVED: "
                                    "scores.array_values = [%lld, %lld, "
                                    "%lld]\n",
                                    scores_it->second.array_values[0],
                                    scores_it->second.array_values[1],
                                    scores_it->second.array_values[2]);
                            }
                        }
                    }

                    // マップの再ハッシュを防ぐため、全ての変数を一時マップに収集してから一括登録
                    std::map<std::string, Variable> vars_batch;

                    // 個別メンバー変数を収集
                    for (const auto &member : ret.struct_value.struct_members) {
                        // scores[0]のような配列要素キーはスキップ（後で個別に処理される）
                        if (member.first.find('[') != std::string::npos) {
                            if (interpreter_->debug_mode &&
                                node->name == "student1") {
                                debug_print(
                                    "FUNC_RETURN: Skipping array element "
                                    "key from struct_members: '%s'\n",
                                    member.first.c_str());
                            }
                            continue;
                        }

                        std::string member_path =
                            node->name + "." + member.first;
                        // 一時マップに追加
                        vars_batch[member_path] = member.second;

                        // 配列メンバーの場合、個別要素変数も収集
                        if (member.second.is_array) {
                            for (int i = 0; i < member.second.array_size; i++) {
                                std::string element_name =
                                    member_path + "[" + std::to_string(i) + "]";
                                std::string element_key = member.first + "[" +
                                                          std::to_string(i) +
                                                          "]";

                                // 構造体配列要素の場合
                                auto element_it =
                                    ret.struct_value.struct_members.find(
                                        element_key);
                                if (element_it !=
                                        ret.struct_value.struct_members.end() &&
                                    element_it->second.is_struct) {
                                    Variable element_var = element_it->second;
                                    element_var.is_assigned = true;
                                    vars_batch[element_name] = element_var;

                                    // 構造体要素のメンバー変数も収集
                                    for (const auto &sub_member :
                                         element_var.struct_members) {
                                        std::string sub_member_path =
                                            element_name + "." +
                                            sub_member.first;
                                        vars_batch[sub_member_path] =
                                            sub_member.second;
                                    }
                                } else {
                                    // プリミティブ型配列の要素
                                    if (interpreter_->debug_mode &&
                                        node->name == "student1") {
                                        debug_print(
                                            "FUNC_RETURN_ELEMENT: "
                                            "member.second.type=%d, "
                                            "array_values.size()=%zu, "
                                            "i=%d\n",
                                            (int)member.second.type,
                                            member.second.array_values.size(),
                                            i);
                                    }

                                    Variable element_var;
                                    element_var.type =
                                        member.second.type >= TYPE_ARRAY_BASE
                                            ? static_cast<TypeInfo>(
                                                  member.second.type -
                                                  TYPE_ARRAY_BASE)
                                            : member.second.type;
                                    element_var.is_assigned = true;

                                    if (element_var.type == TYPE_STRING &&
                                        i < static_cast<int>(
                                                member.second.array_strings
                                                    .size())) {
                                        element_var.str_value =
                                            member.second.array_strings[i];
                                    } else if (element_var.type !=
                                                   TYPE_STRING &&
                                               i < static_cast<int>(
                                                       member.second
                                                           .array_values
                                                           .size())) {
                                        element_var.value =
                                            member.second.array_values[i];
                                    }

                                    if (interpreter_->debug_mode &&
                                        node->name == "student1") {
                                        debug_print(
                                            "FUNC_RETURN_BATCH: Created "
                                            "element_var for %s: type=%d, "
                                            "value=%lld, is_assigned=%d\n",
                                            element_name.c_str(),
                                            (int)element_var.type,
                                            (long long)element_var.value,
                                            element_var.is_assigned);
                                    }

                                    if (interpreter_->debug_mode &&
                                        node->name == "student1") {
                                        auto existing =
                                            vars_batch.find(element_name);
                                        if (existing != vars_batch.end()) {
                                            debug_print(
                                                "FUNC_RETURN_BATCH: KEY "
                                                "ALREADY EXISTS! '%s' "
                                                "current: type=%d, "
                                                "value=%lld\n",
                                                element_name.c_str(),
                                                (int)existing->second.type,
                                                (long long)
                                                    existing->second.value);
                                        }
                                    }

                                    vars_batch[element_name] = element_var;

                                    if (interpreter_->debug_mode &&
                                        node->name == "student1") {
                                        debug_print(
                                            "FUNC_RETURN_BATCH: Set %s: "
                                            "type=%d, value=%lld, "
                                            "is_assigned=%d\n",
                                            element_name.c_str(),
                                            (int)element_var.type,
                                            (long long)element_var.value,
                                            element_var.is_assigned);
                                    }
                                }
                            }
                        }
                    }

                    // バッチ内容を確認（親構造体追加前）
                    if (interpreter_->debug_mode && node->name == "student1") {
                        debug_print("FUNC_RETURN: Batch size before adding "
                                    "parent: %zu variables\n",
                                    vars_batch.size());
                        debug_print("FUNC_RETURN: All keys in batch "
                                    "(BEFORE parent):\n");
                        for (const auto &var_pair : vars_batch) {
                            if (var_pair.first.find("scores[") !=
                                std::string::npos) {
                                debug_print("  '%s': type=%d, value=%lld, "
                                            "is_assigned=%d\n",
                                            var_pair.first.c_str(),
                                            (int)var_pair.second.type,
                                            (long long)var_pair.second.value,
                                            var_pair.second.is_assigned);
                            }
                        }
                    }

                    // 親構造体変数も追加（その前にstruct_membersを確認）
                    if (interpreter_->debug_mode && node->name == "student1") {
                        debug_print("FUNC_RETURN: Parent "
                                    "var.struct_members has %zu members\n",
                                    var.struct_members.size());
                        for (const auto &sm : var.struct_members) {
                            debug_print("  struct_member key: '%s', "
                                        "type=%d, is_array=%d\n",
                                        sm.first.c_str(), (int)sm.second.type,
                                        sm.second.is_array);
                        }
                    }
                    vars_batch[node->name] = var;

                    // バッチ内容をデバッグ出力（親構造体追加後）
                    if (interpreter_->debug_mode && node->name == "student1") {
                        debug_print("FUNC_RETURN: Batch size after adding "
                                    "parent: %zu variables\n",
                                    vars_batch.size());
                        debug_print("FUNC_RETURN: All keys in batch (AFTER "
                                    "parent):\n");
                        for (const auto &var_pair : vars_batch) {
                            if (var_pair.first.find("scores[") !=
                                std::string::npos) {
                                debug_print("  '%s': type=%d, value=%lld, "
                                            "is_assigned=%d\n",
                                            var_pair.first.c_str(),
                                            (int)var_pair.second.type,
                                            (long long)var_pair.second.value,
                                            var_pair.second.is_assigned);
                            }
                        }
                    }

                    // 一括登録: std::mapに順次追加
                    // std::mapは再バランスで要素が移動する可能性があるが、
                    // 全ての変数をここで一度に登録するため、後続の参照は安全
                    for (const auto &var_pair : vars_batch) {
                        current_scope().variables[var_pair.first] =
                            var_pair.second;

                        if (interpreter_->debug_mode &&
                            node->name == "student1" &&
                            var_pair.first.find("scores[") !=
                                std::string::npos) {
                            debug_print("FUNC_RETURN: Registered %s = %lld\n",
                                        var_pair.first.c_str(),
                                        (long long)var_pair.second.value);
                        }
                    }

                    if (interpreter_->debug_mode && node->name == "student1") {
                        debug_print(
                            "FUNC_RETURN: Batch registered %zu variables\n",
                            vars_batch.size());
                        Variable *final_check =
                            find_variable("student1.scores[0]");
                        if (final_check) {
                            debug_print("FUNC_RETURN: Final check - "
                                        "student1.scores[0] = %lld, "
                                        "is_assigned=%d\n",
                                        (long long)final_check->value,
                                        final_check->is_assigned);
                        } else {
                            debug_print("FUNC_RETURN: Final check - "
                                        "student1.scores[0] NOT FOUND\n");
                        }
                    }

                    return; // 構造体関数呼び出し処理完了後は早期リターン
                } else {
                    throw std::runtime_error(
                        "Function did not return expected struct type");
                }
            }

        } else if (var.is_array &&
                   node->init_expr->node_type == ASTNodeType::AST_ARRAY_REF) {
            // 配列スライス代入の処理
            std::string source_var_name = node->init_expr->name;
            Variable *source_var = find_variable(source_var_name);
            if (!source_var) {
                throw std::runtime_error("Source variable not found: " +
                                         source_var_name);
            }

            // インデックスを評価
            std::vector<int64_t> indices;
            for (const auto &index_expr : node->init_expr->arguments) {
                int64_t index =
                    interpreter_->expression_evaluator_->evaluate_expression(
                        index_expr.get());
                indices.push_back(index);
            }

            // 配列スライスをコピー
            interpreter_->array_manager_->copyArraySlice(var, *source_var,
                                                         indices);

        } else if (var.is_array &&
                   node->init_expr->node_type == ASTNodeType::AST_VARIABLE) {
            // 配列全体のコピー
            std::string source_var_name = node->init_expr->name;
            Variable *source_var = find_variable(source_var_name);
            if (!source_var) {
                throw std::runtime_error("Source variable not found: " +
                                         source_var_name);
            }

            // 配列をコピー
            interpreter_->array_manager_->copyArray(var, *source_var);

        } else if (var.type == TYPE_STRING &&
                   node->init_expr->node_type ==
                       ASTNodeType::AST_STRING_LITERAL) {
            // 文字列初期化の処理
            var.str_value = node->init_expr->str_value;
            var.value = 0; // プレースホルダー
            var.is_assigned = true;
        } else if (var.is_array && !var.is_assigned &&
                   node->init_expr->node_type == ASTNodeType::AST_FUNC_CALL) {
            // 配列を返す関数呼び出し
            try {
                int64_t value =
                    interpreter_->expression_evaluator_->evaluate_expression(
                        node->init_expr.get());
                var.value = value;
                var.is_assigned = true;
            } catch (const ReturnException &ret) {
                if (ret.is_array) {
                    // 配列戻り値の場合
                    if (ret.type == TYPE_STRING) {
                        // 文字列配列
                        if (!ret.str_array_3d.empty() &&
                            !ret.str_array_3d[0].empty() &&
                            !ret.str_array_3d[0][0].empty()) {
                            var.array_strings = ret.str_array_3d[0][0];
                            var.array_size = var.array_strings.size();
                            var.type = static_cast<TypeInfo>(TYPE_ARRAY_BASE +
                                                             TYPE_STRING);
                        }
                    } else if (ret.type == TYPE_FLOAT ||
                               ret.type == TYPE_DOUBLE ||
                               ret.type == TYPE_QUAD) {
                        // float/double/quad配列
                        if (!ret.double_array_3d.empty() &&
                            !ret.double_array_3d[0].empty()) {

                            // typedef配列名から多次元配列かどうかを判定
                            std::string actual_type =
                                interpreter_->type_manager_->resolve_typedef(
                                    ret.array_type_name);
                            bool is_multidim =
                                (actual_type.find("[][]") !=
                                     std::string::npos ||
                                 ret.array_type_name.find("[][]") !=
                                     std::string::npos ||
                                 ret.double_array_3d.size() > 1 ||
                                 (ret.double_array_3d.size() == 1 &&
                                  ret.double_array_3d[0].size() > 1));

                            if (is_multidim) {
                                // 多次元float/double配列の場合 -
                                // 全要素を展開
                                if (ret.type == TYPE_FLOAT) {
                                    var.multidim_array_float_values.clear();
                                    for (const auto &plane :
                                         ret.double_array_3d) {
                                        for (const auto &row : plane) {
                                            for (const auto &element : row) {
                                                var.multidim_array_float_values
                                                    .push_back(
                                                        static_cast<float>(
                                                            element));
                                            }
                                        }
                                    }
                                    var.array_size =
                                        var.multidim_array_float_values.size();
                                } else if (ret.type == TYPE_DOUBLE) {
                                    var.multidim_array_double_values.clear();
                                    for (const auto &plane :
                                         ret.double_array_3d) {
                                        for (const auto &row : plane) {
                                            for (const auto &element : row) {
                                                var.multidim_array_double_values
                                                    .push_back(element);
                                            }
                                        }
                                    }
                                    var.array_size =
                                        var.multidim_array_double_values.size();
                                } else { // TYPE_QUAD
                                    var.multidim_array_quad_values.clear();
                                    for (const auto &plane :
                                         ret.double_array_3d) {
                                        for (const auto &row : plane) {
                                            for (const auto &element : row) {
                                                var.multidim_array_quad_values
                                                    .push_back(static_cast<
                                                               long double>(
                                                        element));
                                            }
                                        }
                                    }
                                    var.array_size =
                                        var.multidim_array_quad_values.size();
                                }
                                var.is_multidimensional = true;
                                var.array_values.clear();

                                // 配列の次元情報を設定
                                if (!ret.double_array_3d[0].empty()) {
                                    var.array_dimensions.clear();
                                    var.array_dimensions.push_back(
                                        ret.double_array_3d[0].size()); // 行数
                                    var.array_dimensions.push_back(
                                        ret.double_array_3d[0][0]
                                            .size()); // 列数
                                }
                            } else if (!ret.double_array_3d[0][0].empty()) {
                                // 1次元float/double配列の場合
                                if (ret.type == TYPE_FLOAT) {
                                    var.array_float_values.clear();
                                    for (const auto &element :
                                         ret.double_array_3d[0][0]) {
                                        var.array_float_values.push_back(
                                            static_cast<float>(element));
                                    }
                                    var.array_size =
                                        var.array_float_values.size();
                                } else if (ret.type == TYPE_DOUBLE) {
                                    var.array_double_values.clear();
                                    for (const auto &element :
                                         ret.double_array_3d[0][0]) {
                                        var.array_double_values.push_back(
                                            element);
                                    }
                                    var.array_size =
                                        var.array_double_values.size();
                                } else { // TYPE_QUAD
                                    var.array_quad_values.clear();
                                    for (const auto &element :
                                         ret.double_array_3d[0][0]) {
                                        var.array_quad_values.push_back(
                                            static_cast<long double>(element));
                                    }
                                    var.array_size =
                                        var.array_quad_values.size();
                                }
                            }
                            var.type = static_cast<TypeInfo>(TYPE_ARRAY_BASE +
                                                             ret.type);
                        }
                    } else {
                        // 整数型配列
                        if (!ret.int_array_3d.empty() &&
                            !ret.int_array_3d[0].empty()) {

                            // typedef配列名から多次元配列かどうかを判定
                            // typedefの場合、実際の型を解決して確認
                            std::string actual_type =
                                interpreter_->type_manager_->resolve_typedef(
                                    ret.array_type_name);
                            bool is_multidim =
                                (actual_type.find("[][]") !=
                                     std::string::npos ||
                                 ret.array_type_name.find("[][]") !=
                                     std::string::npos ||
                                 ret.int_array_3d.size() > 1 ||
                                 (ret.int_array_3d.size() == 1 &&
                                  ret.int_array_3d[0].size() > 1));

                            if (is_multidim) {
                                // 多次元配列の場合 - 全要素を展開
                                var.multidim_array_values.clear();
                                for (const auto &plane : ret.int_array_3d) {
                                    for (const auto &row : plane) {
                                        for (const auto &element : row) {
                                            var.multidim_array_values.push_back(
                                                element);
                                        }
                                    }
                                }
                                var.array_size =
                                    var.multidim_array_values.size();
                                var.is_multidimensional = true;
                                var.array_values.clear();

                                // 配列の次元情報を設定
                                if (!ret.int_array_3d[0].empty()) {
                                    var.array_dimensions.clear();
                                    var.array_dimensions.push_back(
                                        ret.int_array_3d[0].size()); // 行数
                                    var.array_dimensions.push_back(
                                        ret.int_array_3d[0][0].size()); // 列数
                                }
                            } else if (!ret.int_array_3d[0][0].empty()) {
                                // 1次元配列の場合
                                var.array_values = ret.int_array_3d[0][0];
                                var.array_size = var.array_values.size();
                            }
                            var.type = static_cast<TypeInfo>(TYPE_ARRAY_BASE +
                                                             ret.type);
                        }
                    }
                    var.is_assigned = true;
                } else if (ret.is_struct) {
                    // struct戻り値の場合
                    debug_print("STRUCT_RETURN_DEBUG: Processing struct "
                                "return value for %s\n",
                                node->name.c_str());
                    var = ret.struct_value;
                    var.is_assigned = true;

                    // struct変数を登録
                    current_scope().variables[node->name] = var;

                    // 構造体定義を取得して配列メンバの個別要素変数を作成
                    const StructDefinition *struct_def =
                        interpreter_->find_struct_definition(
                            interpreter_->type_manager_->resolve_typedef(
                                var.struct_type_name));
                    if (struct_def) {
                        for (const auto &member_def : struct_def->members) {
                            // 直接アクセス変数を作成
                            std::string member_name =
                                node->name + "." + member_def.name;
                            Variable member_var;

                            auto struct_member_it =
                                var.struct_members.find(member_def.name);
                            if (struct_member_it != var.struct_members.end()) {
                                member_var = struct_member_it->second;
                                current_scope().variables[member_name] =
                                    member_var;

                                // 配列メンバの場合、個別要素変数も作成
                                if (member_var.is_array) {
                                    for (int i = 0; i < member_var.array_size;
                                         i++) {
                                        std::string element_name =
                                            member_name + "[" +
                                            std::to_string(i) + "]";
                                        Variable element_var;
                                        element_var.type =
                                            member_def.array_info.base_type;
                                        element_var.is_assigned = true;

                                        if (element_var.type == TYPE_STRING) {
                                            if (i < static_cast<int>(
                                                        member_var.array_strings
                                                            .size())) {
                                                element_var.str_value =
                                                    member_var.array_strings[i];
                                            }
                                        } else {
                                            if (i < static_cast<int>(
                                                        member_var.array_values
                                                            .size())) {
                                                element_var.value =
                                                    member_var.array_values[i];
                                            }
                                        }

                                        current_scope()
                                            .variables[element_name] =
                                            element_var;

                                        if (interpreter_->debug_mode) {
                                            if (element_var.type ==
                                                TYPE_STRING) {
                                                debug_print(
                                                    "STRUCT_RETURN: "
                                                    "Created array element "
                                                    "%s = '%s'\n",
                                                    element_name.c_str(),
                                                    element_var.str_value
                                                        .c_str());
                                            } else {
                                                debug_print(
                                                    "STRUCT_RETURN: "
                                                    "Created array element "
                                                    "%s = %lld\n",
                                                    element_name.c_str(),
                                                    (long long)
                                                        element_var.value);
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }

                    return; // struct戻り値処理完了後は早期リターン
                } else {
                    // 非配列戻り値の場合
                    if (ret.type == TYPE_STRING) {
                        var.str_value = ret.str_value;
                    } else {
                        int64_t numeric_value = ret.value;
                        clamp_unsigned_value(
                            var, numeric_value,
                            "  initialized with function return", node);
                        var.value = numeric_value;
                    }
                    var.is_assigned = true;
                }
            }
        } else {
            if (var.type == TYPE_STRING &&
                node->init_expr->node_type == ASTNodeType::AST_ARRAY_REF) {
                // 文字列配列アクセス初期化
                // 配列名を取得
                std::string array_name;
                const ASTNode *base_node = node->init_expr.get();
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
                    const ASTNode *current_node = node->init_expr.get();
                    while (current_node && current_node->node_type ==
                                               ASTNodeType::AST_ARRAY_REF) {
                        int64_t index =
                            interpreter_->expression_evaluator_
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
                        var.str_value = str_value;
                        var.value = 0; // プレースホルダー
                        var.is_assigned = true;
                    } catch (const std::exception &e) {
                        var.str_value = "";
                        var.value = 0;
                        var.is_assigned = true;
                    }
                } else {
                    // 配列アクセスではない場合は通常の処理にフォールバック
                    int64_t value =
                        interpreter_->expression_evaluator_
                            ->evaluate_expression(node->init_expr.get());
                    var.str_value = std::to_string(value);
                    var.value = value;
                    var.is_assigned = true;
                }
            } else if (node->init_expr->node_type ==
                       ASTNodeType::AST_FUNC_CALL) {
                // 関数呼び出しの場合、型推論対応評価を使用
                try {
                    TypedValue typed_result =
                        interpreter_->expression_evaluator_
                            ->evaluate_typed_expression(node->init_expr.get());

                    if (typed_result.is_string()) {
                        var.str_value = typed_result.string_value;
                        var.value = 0;
                    } else if (typed_result.numeric_type == TYPE_FLOAT ||
                               typed_result.numeric_type == TYPE_DOUBLE ||
                               typed_result.numeric_type == TYPE_QUAD) {
                        // float/double/quad戻り値の場合
                        long double quad_val = typed_result.as_quad();

                        if (typed_result.numeric_type == TYPE_FLOAT) {
                            float f = static_cast<float>(quad_val);
                            var.float_value = f;
                            var.double_value = static_cast<double>(f);
                            var.quad_value = static_cast<long double>(f);
                            var.value = static_cast<int64_t>(f);
                        } else if (typed_result.numeric_type == TYPE_DOUBLE) {
                            double d = static_cast<double>(quad_val);
                            var.float_value = static_cast<float>(d);
                            var.double_value = d;
                            var.quad_value = static_cast<long double>(d);
                            var.value = static_cast<int64_t>(d);
                        } else { // TYPE_QUAD
                            var.float_value = static_cast<float>(quad_val);
                            var.double_value = static_cast<double>(quad_val);
                            var.quad_value = quad_val;
                            var.value = static_cast<int64_t>(quad_val);
                        }
                        var.str_value = "";
                    } else {
                        int64_t numeric_value = typed_result.value;
                        clamp_unsigned_value(var, numeric_value,
                                             "  initialized with expression",
                                             node);
                        var.value = numeric_value;
                        var.str_value = "";
                    }
                    var.is_assigned = true;
                } catch (const ReturnException &ret) {
                    if (ret.is_struct) {
                        debug_print("STRUCT_RETURN_DEBUG_2: Processing "
                                    "struct return value for %s\n",
                                    node->name.c_str());
                        var = ret.struct_value;
                        var.is_assigned = true;

                        // 構造体の場合、直接アクセス変数も作成
                        current_scope().variables[node->name] = var;

                        // 構造体定義を取得して配列メンバの個別要素変数を作成
                        const StructDefinition *struct_def =
                            interpreter_->find_struct_definition(
                                interpreter_->type_manager_->resolve_typedef(
                                    var.struct_type_name));
                        if (struct_def) {
                            for (const auto &member_def : struct_def->members) {
                                std::string member_name =
                                    node->name + "." + member_def.name;

                                auto struct_member_it =
                                    var.struct_members.find(member_def.name);
                                if (struct_member_it !=
                                    var.struct_members.end()) {
                                    Variable member_var =
                                        struct_member_it->second;
                                    current_scope().variables[member_name] =
                                        member_var;

                                    // 配列メンバの場合、個別要素変数も作成
                                    if (member_var.is_array) {
                                        for (int i = 0;
                                             i < member_var.array_size; i++) {
                                            std::string element_name =
                                                member_name + "[" +
                                                std::to_string(i) + "]";
                                            Variable element_var;

                                            if (member_var.type ==
                                                TYPE_STRING) {
                                                element_var.type = TYPE_STRING;
                                                if (i <
                                                    static_cast<int>(
                                                        member_var.array_strings
                                                            .size())) {
                                                    element_var.str_value =
                                                        member_var
                                                            .array_strings[i];
                                                } else {
                                                    element_var.str_value = "";
                                                }
                                            } else {
                                                element_var.type =
                                                    member_var.type;
                                                if (i <
                                                    static_cast<int>(
                                                        member_var.array_values
                                                            .size())) {
                                                    element_var.value =
                                                        member_var
                                                            .array_values[i];
                                                } else {
                                                    element_var.value = 0;
                                                }
                                            }
                                            element_var.is_assigned = true;
                                            current_scope()
                                                .variables[element_name] =
                                                element_var;

                                            if (interpreter_->debug_mode) {
                                                if (element_var.type ==
                                                    TYPE_STRING) {
                                                    debug_print(
                                                        "STRUCT_RETURN_2: "
                                                        "Created array "
                                                        "element %s = "
                                                        "'%s'\n",
                                                        element_name.c_str(),
                                                        element_var.str_value
                                                            .c_str());
                                                } else {
                                                    debug_print(
                                                        "STRUCT_RETURN_2: "
                                                        "Created array "
                                                        "element %s = "
                                                        "%lld\n",
                                                        element_name.c_str(),
                                                        (long long)
                                                            element_var.value);
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                        }
                        return; // 構造体処理完了後は早期リターン
                    } else if (ret.type == TYPE_STRING) {
                        var.str_value = ret.str_value;
                        var.type = TYPE_STRING;
                    } else {
                        int64_t numeric_value = ret.value;
                        clamp_unsigned_value(
                            var, numeric_value,
                            "  initialized with function return", node);
                        var.value = numeric_value;
                    }
                    var.is_assigned = true;
                }

                // 型チェック（ReturnExceptionがキャッチされた場合はスキップ）
                if (!var.is_assigned && var.type == TYPE_STRING) {
                    throw std::runtime_error(
                        "Type mismatch: expected string but got numeric "
                        "value");
                }
            } else {
                // 型推論対応の式評価を使用して文字列・数値を取得
                TypedValue typed_result =
                    interpreter_->expression_evaluator_
                        ->evaluate_typed_expression(node->init_expr.get());

                if (typed_result.is_string()) {
                    var.type = TYPE_STRING;
                    var.str_value = typed_result.string_value;
                    setNumericFields(var, 0.0L);
                } else if (typed_result.is_numeric()) {
                    var.str_value.clear();

                    TypeInfo inferred_type = var.type;
                    if (inferred_type == TYPE_UNKNOWN &&
                        typed_result.numeric_type != TYPE_UNKNOWN) {
                        inferred_type = typed_result.numeric_type;
                        var.type = inferred_type;
                    }

                    const long double quad_value = typed_result.as_quad();
                    auto assign_from_quad = [&](long double value) {
                        setNumericFields(var, value);
                    };

                    switch (inferred_type) {
                    case TYPE_FLOAT: {
                        float f = static_cast<float>(quad_value);
                        assign_from_quad(static_cast<long double>(f));
                        break;
                    }
                    case TYPE_DOUBLE: {
                        double d = static_cast<double>(quad_value);
                        assign_from_quad(static_cast<long double>(d));
                        break;
                    }
                    case TYPE_QUAD:
                        assign_from_quad(quad_value);
                        break;
                    default: {
                        int64_t numeric_value = typed_result.as_numeric();
                        clamp_unsigned_value(var, numeric_value,
                                             "  initialized with expression",
                                             node);
                        assign_from_quad(
                            static_cast<long double>(numeric_value));

                        if (var.type == TYPE_UNKNOWN) {
                            if (typed_result.numeric_type != TYPE_UNKNOWN) {
                                var.type = typed_result.numeric_type;
                            } else {
                                var.type = TYPE_INT;
                            }
                        }
                        break;
                    }
                    }
                } else {
                    // 非数値かつ非文字列の場合は0初期化
                    setNumericFields(var, 0.0L);
                    var.str_value.clear();
                }
                var.is_assigned = true;
            }

            // 型範囲チェック（ポインタ型・ポインタ配列は除外）
            if (var.type != TYPE_STRING && var.type != TYPE_POINTER &&
                !(var.is_pointer && var.is_array)) {
                interpreter_->type_manager_->check_type_range(
                    var.type, var.value, node->name, var.is_unsigned);
            }
        }
    }

    if (var.is_assigned && !var.is_array && !var.is_struct &&
        var.type != TYPE_STRING) {
        clamp_unsigned_value(var, var.value,
                             "  initialized with negative value", node);
    }

    // static変数の場合は特別処理
    if (node->is_static) {
        // static変数として登録
        Variable *existing_static =
            interpreter_->find_static_variable(node->name);
        if (existing_static) {
            // 既にstatic変数が存在する場合は何もしない（初期化は最初の1回のみ）
            return;
        } else {
            // 新しいstatic変数を作成
            interpreter_->create_static_variable(node->name, node);
            return;
        }
    }

    // 未定義型のチェック（基本的な変数宣言の場合）
    if (!node->type_name.empty() && node->type_info == TYPE_UNKNOWN) {
        // type_nameが指定されているがtype_infoがUNKNOWNの場合、未定義型の可能性
        std::string resolved =
            interpreter_->type_manager_->resolve_typedef(node->type_name);
        bool is_union =
            interpreter_->type_manager_->is_union_type(node->type_name);
        bool is_struct =
            (interpreter_->find_struct_definition(node->type_name) != nullptr);
        bool is_enum =
            (interpreter_->get_enum_manager() &&
             interpreter_->get_enum_manager()->enum_exists(node->type_name));

        // typedef、union、struct、enumのいずれでもない場合はエラー
        if (resolved == node->type_name && !is_union && !is_struct &&
            !is_enum) {
            throw std::runtime_error("Undefined type: " + node->type_name);
        }
    }

    if (interpreter_->is_debug_mode() && node->name == "ptr") {
        std::cerr << "[VAR_MANAGER] Registering variable ptr to scope:"
                  << std::endl;
        std::cerr << "  var.value=" << var.value << std::endl;
        std::cerr << "  var.type=" << static_cast<int>(var.type) << std::endl;
        std::cerr << "  node->type_info=" << static_cast<int>(node->type_info)
                  << std::endl;
    }

    // ポインタ型の場合、型情報を確実に設定
    if (node->type_info == TYPE_POINTER) {
        // 初期化式がある場合、関数ポインタかどうか先にチェック
        if (node->init_expr || node->right) {
            ASTNode *init_node =
                node->init_expr ? node->init_expr.get() : node->right.get();

            if (interpreter_->debug_mode) {
                std::cerr << "[VAR_MANAGER] Checking pointer init: node_type="
                          << static_cast<int>(init_node->node_type)
                          << ", op=" << init_node->op
                          << ", is_function_address="
                          << init_node->is_function_address << std::endl;
            }

            // 関数ポインタかチェック
            if (init_node->node_type == ASTNodeType::AST_UNARY_OP &&
                init_node->op == "ADDRESS_OF" &&
                init_node->is_function_address) {

                std::string func_name = init_node->function_address_name;
                const ASTNode *func_node =
                    interpreter_->find_function(func_name);

                // 関数が見つかった場合のみ関数ポインタとして処理
                if (func_node) {
                    // 関数ポインタとして登録
                    var.is_function_pointer = true;
                    var.function_pointer_name = func_name;
                    var.type = TYPE_POINTER; // ポインタ型として扱う
                    var.is_assigned = true;
                    // 関数ノードの実際のメモリアドレスを値として格納
                    var.value = reinterpret_cast<int64_t>(func_node);

                    // FunctionPointerを登録
                    FunctionPointer func_ptr(func_node, func_name,
                                             func_node->type_info);
                    interpreter_->current_scope()
                        .function_pointers[node->name] = func_ptr;

                    if (interpreter_->debug_mode) {
                        std::cerr
                            << "[VAR_MANAGER] Registered function pointer: "
                            << node->name << " -> " << func_name << std::endl;
                    }

                    // 関数ポインタの場合は変数を登録してreturn
                    current_scope().variables[node->name] = var;
                    return;
                }
                // 関数が見つからない場合は通常の変数アドレスとして処理を継続
                if (interpreter_->debug_mode) {
                    std::cerr << "[VAR_MANAGER] Not a function, treating "
                                 "as variable address: "
                              << func_name << std::endl;
                }
            }
        }

        // 通常のポインタ処理
        var.type = TYPE_POINTER;

        // ポインタ型の初期化式がある場合は評価して代入
        if (node->init_expr || node->right) {
            ASTNode *init_node =
                node->init_expr ? node->init_expr.get() : node->right.get();
            if (interpreter_->debug_mode) {
                std::cerr << "[VAR_MANAGER] Evaluating normal pointer "
                             "initialization expression"
                          << std::endl;
            }

            // 関数呼び出しの場合、ReturnExceptionをキャッチする
            if (init_node->node_type == ASTNodeType::AST_FUNC_CALL) {
                try {
                    // evaluate_typed_expressionを使って型情報も取得
                    TypedValue typed_value =
                        interpreter_->expression_evaluator_
                            ->evaluate_typed_expression(init_node);

                    // TypedValueに関数ポインタ情報がある場合
                    if (typed_value.is_function_pointer) {
                        if (interpreter_->debug_mode) {
                            std::cerr << "[VAR_MANAGER] Function returned "
                                         "function pointer: "
                                      << typed_value.function_pointer_name
                                      << " -> " << typed_value.value
                                      << std::endl;
                        }
                        var.value = typed_value.value;
                        var.is_assigned = true;
                        var.is_function_pointer = true;
                        var.function_pointer_name =
                            typed_value.function_pointer_name;

                        // function_pointersマップに登録
                        FunctionPointer func_ptr(
                            typed_value.function_pointer_node,
                            typed_value.function_pointer_name,
                            typed_value.function_pointer_node->type_info);
                        interpreter_->current_scope()
                            .function_pointers[node->name] = func_ptr;
                    } else {
                        // 通常の戻り値（ポインタを含む）
                        var.value = typed_value.value;
                        var.is_assigned = true;
                    }
                } catch (const ReturnException &ret) {
                    // 例外で返される場合（配列など）
                    if (ret.is_function_pointer) {
                        var.value = ret.value;
                        var.is_assigned = true;
                        var.is_function_pointer = true;
                        var.function_pointer_name = ret.function_pointer_name;

                        FunctionPointer func_ptr(
                            ret.function_pointer_node,
                            ret.function_pointer_name,
                            ret.function_pointer_node->type_info);
                        interpreter_->current_scope()
                            .function_pointers[node->name] = func_ptr;
                    } else {
                        var.value = ret.value;
                        var.is_assigned = true;
                    }
                }
            } else {
                TypedValue typed_value =
                    interpreter_->expression_evaluator_
                        ->evaluate_typed_expression(init_node);

                // TypedValueに関数ポインタ情報がある場合
                if (typed_value.is_function_pointer) {
                    if (interpreter_->debug_mode) {
                        std::cerr << "[VAR_MANAGER] TypedValue contains "
                                     "function pointer: "
                                  << typed_value.function_pointer_name << " -> "
                                  << typed_value.value << std::endl;
                    }
                    var.value = typed_value.value;
                    var.is_assigned = true;
                    var.is_function_pointer = true;
                    var.function_pointer_name =
                        typed_value.function_pointer_name;

                    // function_pointersマップに登録
                    FunctionPointer func_ptr(
                        typed_value.function_pointer_node,
                        typed_value.function_pointer_name,
                        typed_value.function_pointer_node->type_info);
                    interpreter_->current_scope()
                        .function_pointers[node->name] = func_ptr;
                } else {
                    var.value = typed_value.value;
                    var.is_assigned = true;
                }
            }

            if (interpreter_->debug_mode) {
                std::cerr << "[VAR_MANAGER] Pointer initialized: value="
                          << var.value << " (0x" << std::hex << var.value
                          << std::dec << ")" << std::endl;
            }
        }
    }

    current_scope().variables[node->name] = var;

    if (interpreter_->is_debug_mode() && node->name == "ptr") {
        Variable *registered = find_variable(node->name);
        if (registered) {
            std::cerr << "[VAR_MANAGER] After registration, ptr value="
                      << registered->value << std::endl;
        }
    }
    // std::cerr << "DEBUG: Variable created: " << node->name << ",
    // is_array=" << var.is_array << std::endl;
}

// ============================================================================
// Variable Assignment Processing (AST_ASSIGN)
// ============================================================================
