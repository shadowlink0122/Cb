#include "simple_assignment.h"
#include "../../../../common/debug.h"
#include "../../../../common/type_helpers.h"
#include "../../core/error_handler.h"
#include "../../core/interpreter.h"
#include "../../core/pointer_metadata.h"
#include "../../managers/variables/manager.h"
#include "../statement_executor.h"
#include "const_check_helpers.h"

namespace AssignmentHandlers {

void execute_assignment(StatementExecutor *executor, Interpreter &interpreter,
                        const ASTNode *node) {

    // 関数アドレスの代入（関数ポインタ）をチェック
    // ADDRESS_OFで、かつ対象が関数である場合
    // AST_ASSIGNノードでは変数名はnode->nameに入っている
    if (node->right && node->right->node_type == ASTNodeType::AST_UNARY_OP &&
        node->right->op == "ADDRESS_OF" && !node->name.empty()) {

        // 関数アドレスかどうかを実行時に判定
        std::string func_name;
        bool is_func_addr = false;

        if (node->right->is_function_address) {
            // パーサーがフラグを設定している場合
            func_name = node->right->function_address_name;
            // 実際に関数が存在するかを確認
            const ASTNode *func_node = interpreter.find_function(func_name);
            if (func_node) {
                is_func_addr = true;
            }
            // 関数が見つからない場合はis_func_addrをfalseのままにして通常処理へ
        } else if (node->right->left &&
                   node->right->left->node_type == ASTNodeType::AST_VARIABLE) {
            // 実行時に関数かどうかをチェック
            func_name = node->right->left->name;
            const ASTNode *func_node = interpreter.find_function(func_name);
            if (func_node) {
                is_func_addr = true;
            }
        }

        if (is_func_addr) {
            std::string var_name =
                node->name; // AST_ASSIGNではnode->nameに変数名が入っている

            const ASTNode *func_node = interpreter.find_function(func_name);
            // この時点ではfunc_nodeは必ず存在する（上のチェックでis_func_addr=trueになった場合のみここに来る）
            if (!func_node) {
                throw std::runtime_error("Undefined function: " + func_name);
            }

            // 関数ポインタを登録（または更新）
            FunctionPointer func_ptr(func_node, func_name,
                                     func_node->type_info);
            interpreter.current_scope().function_pointers[var_name] = func_ptr;

            // グローバルスコープでも確認して更新
            auto &global_func_ptrs =
                interpreter.get_global_scope().function_pointers;
            if (global_func_ptrs.find(var_name) != global_func_ptrs.end()) {
                global_func_ptrs[var_name] = func_ptr;
            }

            // 変数にも関数ポインタ情報を設定
            Variable *var = interpreter.find_variable(var_name);
            if (!var) {
                throw std::runtime_error("Variable not found: " + var_name);
            }

            var->is_function_pointer = true;
            var->function_pointer_name = func_name;
            var->is_assigned = true;
            var->type = TYPE_POINTER; // ポインタ型として扱う
            // 関数ノードの実際のメモリアドレスを値として格納
            var->value = reinterpret_cast<int64_t>(func_node);

            if (debug_mode) {
                std::cerr << "[FUNC_PTR] Assigned function pointer: "
                          << var_name << " = &" << func_name << std::endl;
            }

            return;
        }
    }

    // 間接参照への代入 (*ptr = value)
    if (node->left && node->left->node_type == ASTNodeType::AST_UNARY_OP &&
        node->left->op == "DEREFERENCE") {
        // ポインタを評価
        int64_t ptr_value = interpreter.evaluate(node->left->left.get());
        if (ptr_value == 0) {
            throw std::runtime_error("Null pointer dereference in assignment");
        }

        // constポインタへの間接代入チェック (*ptr = value で、ptrが const T*
        // の場合)
        AssignmentHelpers::check_const_pointer_modification(
            interpreter, node->left->left.get());

        // 右辺を型付きで評価（float/doubleにも対応）
        TypedValue typed_value =
            interpreter.evaluate_typed_expression(node->right.get());

        // ポインタがメタデータを持つかチェック
        if (ptr_value & (1LL << 63)) {
            // メタデータポインタの場合
            int64_t clean_ptr = ptr_value & ~(1LL << 63);
            using namespace PointerSystem;
            PointerMetadata *meta =
                reinterpret_cast<PointerMetadata *>(clean_ptr);

            if (!meta) {
                throw std::runtime_error(
                    "Invalid pointer metadata in assignment");
            }

            if (debug_mode) {
                std::cerr << "[POINTER_METADATA] Assignment through pointer: "
                          << meta->to_string() << std::endl;
            }

            // 型に応じてメタデータを通じて値を書き込み
            if (typed_value.is_floating()) {
                double float_val = typed_value.as_double();
                if (meta->pointed_type == TYPE_FLOAT ||
                    meta->pointed_type == TYPE_DOUBLE ||
                    meta->pointed_type == TYPE_QUAD) {
                    meta->write_float_value(float_val);
                } else {
                    // 整数型へのfloat代入は切り捨て
                    meta->write_int_value(static_cast<int64_t>(float_val));
                }
            } else {
                meta->write_int_value(typed_value.as_numeric());
            }
        } else {
            // 従来の方式（変数ポインタ）
            Variable *var = reinterpret_cast<Variable *>(ptr_value);

            // 型に応じて値を設定
            if (typed_value.is_floating()) {
                double float_val = typed_value.as_double();
                if (var->type == TYPE_FLOAT) {
                    var->float_value = static_cast<float>(float_val);
                } else if (var->type == TYPE_DOUBLE) {
                    var->double_value = float_val;
                } else if (var->type == TYPE_QUAD) {
                    var->quad_value = static_cast<long double>(float_val);
                } else {
                    // 整数型への代入は切り捨て
                    var->value = static_cast<int64_t>(float_val);
                }
            } else {
                var->value = typed_value.as_numeric();
            }
            var->is_assigned = true;
        }
        return;
    }

    // 右辺が三項演算子の場合の特別処理
    if (node->right && node->right->node_type == ASTNodeType::AST_TERNARY_OP) {
        executor->execute_ternary_assignment(node);
        return;
    }

    // 右辺が配列リテラルの場合の特別処理
    if (node->right &&
        node->right->node_type == ASTNodeType::AST_ARRAY_LITERAL) {
        if (node->left && node->left->node_type == ASTNodeType::AST_VARIABLE) {
            // 通常の変数への配列リテラル代入
            std::string var_name = node->left->name;
            interpreter.assign_array_literal(var_name, node->right.get());
            return;
        } else if (node->left &&
                   node->left->node_type == ASTNodeType::AST_MEMBER_ACCESS) {
            // 構造体メンバーへの配列リテラル代入 (obj.member = [1, 2, 3])
            executor->execute_member_array_literal_assignment(node);
            return;
        } else if (!node->name.empty()) {
            // 名前による直接代入
            interpreter.assign_array_literal(node->name, node->right.get());
            return;
        } else {
            throw std::runtime_error("Array literal can only be assigned to "
                                     "variables or struct members");
        }
    }

    // 右辺が構造体リテラルの場合の特別処理
    if (node->right &&
        node->right->node_type == ASTNodeType::AST_STRUCT_LITERAL) {
        if (!node->left) {
            // leftがnullの場合、nameに変数名が入っている可能性がある
            if (!node->name.empty()) {
                interpreter.assign_struct_literal(node->name,
                                                  node->right.get());
                return;
            }
            throw std::runtime_error(
                "Assignment left side is null and name is empty");
        }

        if (node->left->node_type == ASTNodeType::AST_VARIABLE) {
            // 変数への構造体リテラル代入
            Variable *var = interpreter.get_variable(node->left->name);

            if (var && var->is_array) {
                throw std::runtime_error(
                    "Array assignment must use [] syntax, not {}");
            }

            interpreter.assign_struct_literal(node->left->name,
                                              node->right.get());
            return;
        } else if (node->left->node_type == ASTNodeType::AST_ARRAY_REF) {
            // 配列要素への構造体リテラル代入 (team[0] = {})
            if (debug_mode) {
                std::cerr << "DEBUG: Struct literal assignment to array element"
                          << std::endl;
            }
            std::string element_name =
                interpreter.extract_array_element_name(node->left.get());
            if (debug_mode) {
                std::cerr << "DEBUG: Array element name: " << element_name
                          << std::endl;
            }
            interpreter.assign_struct_literal(element_name, node->right.get());
            return;
        } else {
            throw std::runtime_error("Struct literal can only be assigned to "
                                     "variables or array elements");
        }
    }

    if (node->left && node->left->node_type == ASTNodeType::AST_ARRAY_REF) {
        // 配列要素への代入

        // 右辺が構造体戻り値関数の場合の特別処理
        if (node->right &&
            node->right->node_type == ASTNodeType::AST_FUNC_CALL) {
            try {
                // 関数呼び出し結果を評価（戻り値は現在未使用だが、副作用のため実行）
                interpreter.evaluate(node->right.get());
                // 通常の数値戻り値の場合は通常処理を継続
            } catch (const ReturnException &ret) {
                if (ret.is_struct) {
                    // 構造体戻り値を配列要素に代入
                    std::string element_name =
                        interpreter.extract_array_element_name(
                            node->left.get());
                    debug_msg(
                        DebugMsgId::INTERPRETER_STRUCT_REGISTERED,
                        "Assigning struct return value to array element: %s",
                        element_name.c_str());

                    debug_print("ReturnException struct_value: "
                                "struct_members.size() = %zu\n",
                                ret.struct_value.struct_members.size());

                    // 構造体変数を作成・代入
                    interpreter.current_scope().variables[element_name] =
                        ret.struct_value;

                    Variable &assigned_var =
                        interpreter.current_scope().variables[element_name];
                    debug_print(
                        "Assigned variable: struct_members.size() = %zu\n",
                        assigned_var.struct_members.size());

                    // 個別メンバー変数も更新する必要がある
                    for (const auto &member : assigned_var.struct_members) {
                        std::string member_path =
                            element_name + "." + member.first;
                        Variable *member_var =
                            interpreter.find_variable(member_path);
                        if (member_var) {
                            member_var->value = member.second.value;
                            member_var->str_value = member.second.str_value;
                            member_var->is_assigned = true;
                            debug_print("Updated member variable: %s = %lld\n",
                                        member_path.c_str(),
                                        member.second.value);
                        }
                    }

                    return;
                } else {
                    // その他の戻り値は再投げ
                    throw;
                }
            }
        }

        // 右辺の評価（構造体変数やその他の式）
        // TypedValueで評価して、float/doubleにも対応
        int64_t rvalue = 0;
        bool is_floating = false;
        double float_rvalue = 0.0;

        try {
            TypedValue typed_rvalue =
                interpreter.evaluate_typed_expression(node->right.get());
            if (typed_rvalue.is_floating()) {
                is_floating = true;
                float_rvalue = typed_rvalue.as_double();
                rvalue = static_cast<int64_t>(float_rvalue);
            } else {
                rvalue = typed_rvalue.as_numeric();
            }
        } catch (const ReturnException &ret) {
            if (ret.is_struct) {
                // 構造体変数または構造体戻り値を配列要素に代入
                std::string element_name =
                    interpreter.extract_array_element_name(node->left.get());
                debug_msg(DebugMsgId::INTERPRETER_STRUCT_REGISTERED,
                          "Assigning struct variable/return value to array "
                          "element: %s",
                          element_name.c_str());

                std::cerr << "DEBUG: Struct assignment to array element: "
                          << element_name << std::endl;
                std::cerr << "DEBUG: struct_type_name="
                          << ret.struct_value.struct_type_name << std::endl;
                std::cerr << "DEBUG: struct_members.size()="
                          << ret.struct_value.struct_members.size()
                          << std::endl;

                // 構造体データをデバッグ
                for (const auto &member : ret.struct_value.struct_members) {
                    std::cerr << "DEBUG: member[" << member.first
                              << "] = " << member.second.value
                              << " (assigned=" << member.second.is_assigned
                              << ")" << std::endl;
                }

                // 構造体変数を作成・代入
                interpreter.current_scope().variables[element_name] =
                    ret.struct_value;

                Variable &assigned_var =
                    interpreter.current_scope().variables[element_name];

                // 個別メンバー変数も更新する必要がある
                for (const auto &member : assigned_var.struct_members) {
                    std::string member_path = element_name + "." + member.first;
                    Variable *member_var =
                        interpreter.find_variable(member_path);
                    if (member_var) {
                        member_var->value = member.second.value;
                        member_var->str_value = member.second.str_value;
                        member_var->is_assigned = member.second.is_assigned;
                        std::cerr
                            << "DEBUG: Updated member variable: " << member_path
                            << " = " << member.second.value
                            << " (assigned=" << member.second.is_assigned << ")"
                            << std::endl;
                    }
                }

                return;
            } else {
                // その他の戻り値は再投げ
                throw;
            }
        }

        // 構造体メンバーの1次元配列アクセスかチェック: obj.member[i] = value
        if (node->left->left &&
            node->left->left->node_type == ASTNodeType::AST_MEMBER_ACCESS) {
            // obj.member[index] = value のケース
            std::string obj_name;
            if (node->left->left->left && (node->left->left->left->node_type ==
                                               ASTNodeType::AST_VARIABLE ||
                                           node->left->left->left->node_type ==
                                               ASTNodeType::AST_IDENTIFIER)) {
                obj_name = node->left->left->left->name;
            } else {
                if (node->left->left->left) {
                    debug_print(
                        "ERROR: Invalid node type for object: %d\n",
                        static_cast<int>(node->left->left->left->node_type));
                } else {
                    debug_print("ERROR: node->left->left->left is null\n");
                }
                throw std::runtime_error(
                    "Invalid object reference in member array access");
            }
            std::string member_name = node->left->left->name;
            int64_t index = interpreter.evaluate(node->left->array_index.get());

            // 右辺を評価
            if (node->right->node_type == ASTNodeType::AST_STRING_LITERAL) {
                interpreter.assign_struct_member_array_element(
                    obj_name, member_name, index, node->right->str_value);
            } else {
                TypedValue typed_value =
                    interpreter.evaluate_typed(node->right.get());
                if (typed_value.is_floating()) {
                    interpreter.assign_struct_member_array_element(
                        obj_name, member_name, index, typed_value.as_double());
                } else {
                    interpreter.assign_struct_member_array_element(
                        obj_name, member_name, index, typed_value.as_numeric());
                }
            }
            return;
        }

        // 多次元配列アクセスかチェック
        if (node->left->left &&
            node->left->left->node_type == ASTNodeType::AST_ARRAY_REF) {
            // 構造体メンバーの多次元配列かチェック
            ASTNode *deepest_left = node->left.get();
            while (deepest_left->left && deepest_left->left->node_type ==
                                             ASTNodeType::AST_ARRAY_REF) {
                deepest_left = deepest_left->left.get();
            }

            if (deepest_left->left && deepest_left->left->node_type ==
                                          ASTNodeType::AST_MEMBER_ACCESS) {
                // 構造体メンバーの多次元配列への代入: obj.member[i][j] = value
                debug_msg(DebugMsgId::MULTIDIM_ARRAY_ASSIGNMENT_DETECTED);
                std::string obj_name = deepest_left->left->left->name;
                std::string member_name = deepest_left->left->name;
                debug_msg(DebugMsgId::VAR_MANAGER_STRUCT_CREATE,
                          obj_name.c_str(), member_name.c_str());

                // インデックスを収集（ネストしたAST_ARRAY_REFから）
                std::vector<int64_t> indices;
                ASTNode *current_ref = node->left.get();

                // 逆順でインデックスを収集（最外層から最内層へ）
                std::vector<const ASTNode *> refs;
                while (current_ref &&
                       current_ref->node_type == ASTNodeType::AST_ARRAY_REF) {
                    refs.push_back(current_ref);
                    current_ref = current_ref->left.get();
                }

                // 順序を逆にして、インデックスを正しい順序で収集
                for (auto it = refs.rbegin(); it != refs.rend(); ++it) {
                    if (!(*it)->array_index) {
                        std::cerr << "[ERROR] Null array_index in nested "
                                     "AST_ARRAY_REF"
                                  << std::endl;
                        std::cerr
                            << "[ERROR] Node type: " << (int)(*it)->node_type
                            << std::endl;
                        std::cerr << "[ERROR] This suggests parser failed to "
                                     "properly construct AST for "
                                     "multidimensional access"
                                  << std::endl;
                        throw std::runtime_error(
                            "Null array_index in multidimensional access");
                    }
                    debug_msg(DebugMsgId::ARRAY_ELEMENT_EVAL_START);
                    int64_t index =
                        interpreter.evaluate((*it)->array_index.get());
                    std::string index_str = std::to_string(index);
                    debug_msg(DebugMsgId::ARRAY_ELEMENT_EVAL_VALUE,
                              index_str.c_str());
                    indices.push_back(index);
                }

                // 構造体メンバー変数を取得
                Variable *member_var =
                    interpreter.get_struct_member(obj_name, member_name);
                if (!member_var) {
                    throw std::runtime_error("Struct member not found: " +
                                             member_name);
                }

                // 多次元配列の要素に代入
                if (member_var->is_multidimensional && indices.size() > 1) {
                    // float/doubleの場合は対応するオーバーロードを使用
                    TypeInfo base_type = member_var->array_type_info.base_type;
                    if (is_floating &&
                        (base_type == TYPE_FLOAT || base_type == TYPE_DOUBLE ||
                         base_type == TYPE_QUAD)) {
                        interpreter.setMultidimensionalArrayElement(
                            *member_var, indices, float_rvalue);
                    } else {
                        interpreter.setMultidimensionalArrayElement(
                            *member_var, indices, rvalue);
                    }
                } else {
                    throw std::runtime_error(
                        "Invalid multidimensional member array access");
                }
            } else {
                // 通常の多次元配列要素への代入
                std::string var_name =
                    interpreter.extract_array_name(node->left.get());
                std::vector<int64_t> indices =
                    interpreter.extract_array_indices(node->left.get());

                Variable *var = interpreter.find_variable(var_name);
                if (!var) {
                    throw std::runtime_error("Variable not found: " + var_name);
                }

                if (!var->is_multidimensional) {
                    throw std::runtime_error(
                        "Variable is not a multidimensional array: " +
                        var_name);
                }

                // float/doubleの場合は対応するオーバーロードを使用
                TypeInfo base_type = var->array_type_info.base_type;
                if (is_floating &&
                    (base_type == TYPE_FLOAT || base_type == TYPE_DOUBLE ||
                     base_type == TYPE_QUAD)) {
                    interpreter.setMultidimensionalArrayElement(*var, indices,
                                                                float_rvalue);
                } else {
                    interpreter.setMultidimensionalArrayElement(*var, indices,
                                                                rvalue);
                }
            }
        } else {
            // 単一次元配列要素への代入
            int64_t index_value =
                interpreter.evaluate(node->left->array_index.get());
            int index = static_cast<int>(index_value);

            std::string var_name;
            if (node->left->left &&
                node->left->left->node_type == ASTNodeType::AST_VARIABLE) {
                var_name = node->left->left->name;
            } else if (!node->left->name.empty()) {
                var_name = node->left->name;
            } else {
                throw std::runtime_error(
                    "Invalid array reference in assignment");
            }

            Variable *var = interpreter.find_variable(var_name);
            if (!var) {
                // 詳細なエラー表示
                print_error_with_ast_location(
                    "Undefined variable '" + var_name + "'", node);

                throw DetailedErrorException("Undefined variable: " + var_name);
            }

            if (var->type == TYPE_STRING) {
                interpreter.assign_string_element(
                    var_name, index, std::string(1, static_cast<char>(rvalue)));
            } else {
                // float/double/quad配列の場合はfloat値を使用
                TypeInfo base_type =
                    (var->type >= TYPE_ARRAY_BASE)
                        ? static_cast<TypeInfo>(var->type - TYPE_ARRAY_BASE)
                        : var->type;
                if (is_floating &&
                    (base_type == TYPE_FLOAT || base_type == TYPE_DOUBLE ||
                     base_type == TYPE_QUAD)) {
                    interpreter.assign_array_element_float(var_name, index,
                                                           float_rvalue);
                } else {
                    interpreter.assign_array_element(var_name, index, rvalue);
                }
            }
        }
    } else if (node->left &&
               node->left->node_type == ASTNodeType::AST_MEMBER_ARRAY_ACCESS) {
        // メンバの配列アクセスへの代入 (obj.member[index] = value)
        debug_print("DEBUG: Detected AST_MEMBER_ARRAY_ACCESS assignment\n");
        executor->execute_member_array_assignment(node);
    } else if (node->left &&
               node->left->node_type == ASTNodeType::AST_MEMBER_ACCESS) {
        // メンバアクセスへの代入 (obj.member = value)
        executor->execute_member_assignment(node);
    } else if (node->left &&
               node->left->node_type == ASTNodeType::AST_ARROW_ACCESS) {
        // アロー演算子アクセスへの代入 (ptr->member = value)
        executor->execute_arrow_assignment(node);
    } else {
        // 通常の変数代入
        // Union型変数への代入の特別処理
        if (node->left && node->left->node_type == ASTNodeType::AST_VARIABLE) {
            Variable *var = interpreter.find_variable(node->left->name);
            if (var && var->type == TYPE_UNION) {
                interpreter.assign_union_variable(node->left->name,
                                                  node->right.get());
                return;
            }
        }

        // node->nameベースの代入でもUnion型チェック
        if (!node->name.empty()) {
            Variable *var = interpreter.find_variable(node->name);
            if (var && var->type == TYPE_UNION) {
                interpreter.assign_union_variable(node->name,
                                                  node->right.get());
                return;
            }
        }

        std::string target_name =
            !node->name.empty() ? node->name
                                : (node->left && node->left->node_type ==
                                                     ASTNodeType::AST_VARIABLE
                                       ? node->left->name
                                       : "");

        if (target_name.empty()) {
            throw std::runtime_error("Invalid assignment target");
        }

        Variable *target_var = interpreter.find_variable(target_name);

        // constポインタ自体への代入チェック (T* const ptr の場合、ptr = ...
        // は不可)
        AssignmentHelpers::check_const_pointer_reassignment(target_var);

        // Interface型変数（ポインタを除く）への代入処理
        if (target_var &&
            (target_var->type == TYPE_INTERFACE ||
             !target_var->interface_name.empty()) &&
            target_var->type != TYPE_POINTER) {
            auto assign_from_source = [&](const Variable &source,
                                          const std::string &source_name) {
                interpreter.get_variable_manager()->assign_interface_view(
                    target_name, *target_var, source, source_name);
            };

            auto create_temp_from_typed = [&](const TypedValue &typed,
                                              TypeInfo type_hint) -> Variable {
                Variable temp;
                temp.is_assigned = true;
                temp.struct_type_name.clear();

                if (typed.is_struct()) {
                    if (typed.struct_data) {
                        temp = *typed.struct_data;
                        temp.is_assigned = true;
                    }
                    return temp;
                }

                if (typed.is_string()) {
                    temp.type = TYPE_STRING;
                    temp.str_value = typed.string_value;
                    temp.struct_type_name = type_info_to_string(TYPE_STRING);
                    temp.value = 0;
                    temp.float_value = 0.0f;
                    temp.double_value = 0.0;
                    temp.quad_value = 0.0L;
                    return temp;
                }

                TypeInfo resolved =
                    typed.numeric_type != TYPE_UNKNOWN
                        ? typed.numeric_type
                        : (typed.type.type_info != TYPE_UNKNOWN
                               ? typed.type.type_info
                               : (type_hint != TYPE_UNKNOWN ? type_hint
                                                            : TYPE_INT));

                if (resolved == TYPE_STRING && !typed.is_numeric()) {
                    temp.type = TYPE_STRING;
                    temp.str_value = typed.as_string();
                    temp.struct_type_name = type_info_to_string(TYPE_STRING);
                    return temp;
                }

                if (resolved == TYPE_FLOAT) {
                    long double quad = typed.as_quad();
                    float f = static_cast<float>(quad);
                    temp.type = TYPE_FLOAT;
                    temp.float_value = f;
                    temp.double_value = static_cast<double>(f);
                    temp.quad_value = static_cast<long double>(f);
                    temp.value = static_cast<int64_t>(f);
                } else if (resolved == TYPE_DOUBLE) {
                    long double quad = typed.as_quad();
                    double d = static_cast<double>(quad);
                    temp.type = TYPE_DOUBLE;
                    temp.float_value = static_cast<float>(d);
                    temp.double_value = d;
                    temp.quad_value = static_cast<long double>(d);
                    temp.value = static_cast<int64_t>(d);
                } else if (resolved == TYPE_QUAD) {
                    long double quad = typed.as_quad();
                    temp.type = TYPE_QUAD;
                    temp.float_value = static_cast<float>(quad);
                    temp.double_value = static_cast<double>(quad);
                    temp.quad_value = quad;
                    temp.value = static_cast<int64_t>(quad);
                } else {
                    int64_t numeric_value = typed.as_numeric();
                    if (resolved == TYPE_BOOL) {
                        numeric_value = (numeric_value != 0) ? 1 : 0;
                    }
                    temp.type = resolved;
                    temp.value = numeric_value;
                    temp.float_value = static_cast<float>(numeric_value);
                    temp.double_value = static_cast<double>(numeric_value);
                    temp.quad_value = static_cast<long double>(numeric_value);
                }

                temp.struct_type_name = type_info_to_string(temp.type);
                return temp;
            };

            const ASTNode *rhs = node->right.get();
            try {
                if (rhs->node_type == ASTNodeType::AST_VARIABLE ||
                    rhs->node_type == ASTNodeType::AST_IDENTIFIER) {
                    std::string source_var_name = rhs->name;
                    Variable *source_var =
                        interpreter.find_variable(source_var_name);
                    if (!source_var) {
                        throw std::runtime_error("Source variable not found: " +
                                                 source_var_name);
                    }
                    assign_from_source(*source_var, source_var_name);
                    return;
                }

                if (rhs->node_type == ASTNodeType::AST_STRING_LITERAL) {
                    TypedValue typed_value(rhs->str_value,
                                           InferredType(TYPE_STRING, "string"));
                    Variable temp =
                        create_temp_from_typed(typed_value, TYPE_STRING);
                    assign_from_source(temp, "");
                    return;
                }

                TypedValue typed_value =
                    interpreter.evaluate_typed_expression(rhs);
                TypeInfo resolved_type = rhs->type_info != TYPE_UNKNOWN
                                             ? rhs->type_info
                                             : typed_value.type.type_info;
                Variable temp =
                    create_temp_from_typed(typed_value, resolved_type);
                assign_from_source(temp, "");
                return;
            } catch (const ReturnException &ret) {
                if (ret.is_array) {
                    throw std::runtime_error("Cannot assign array return value "
                                             "to interface variable '" +
                                             target_name + "'");
                }

                if (!ret.is_struct) {
                    if (TypeHelpers::isString(ret.type)) {
                        TypedValue typed_value(
                            ret.str_value, InferredType(TYPE_STRING, "string"));
                        Variable temp =
                            create_temp_from_typed(typed_value, TYPE_STRING);
                        assign_from_source(temp, "");
                        return;
                    }

                    TypeInfo resolved_type =
                        ret.type != TYPE_UNKNOWN ? ret.type : TYPE_INT;
                    TypedValue typed_value =
                        (ret.type == TYPE_FLOAT)
                            ? TypedValue(ret.double_value,
                                         InferredType(TYPE_FLOAT, "float"))
                        : (ret.type == TYPE_DOUBLE)
                            ? TypedValue(ret.double_value,
                                         InferredType(TYPE_DOUBLE, "double"))
                        : (ret.type == TYPE_QUAD)
                            ? TypedValue(ret.quad_value,
                                         InferredType(TYPE_QUAD, "quad"))
                            : TypedValue(ret.value,
                                         InferredType(resolved_type,
                                                      type_info_to_string(
                                                          resolved_type)));
                    Variable temp =
                        create_temp_from_typed(typed_value, resolved_type);
                    assign_from_source(temp, "");
                    return;
                }

                assign_from_source(ret.struct_value, "");
                return;
            }
        }

        if (node->right &&
            node->right->node_type == ASTNodeType::AST_FUNC_CALL) {
            try {
                TypedValue typed_value =
                    interpreter.evaluate_typed_expression(node->right.get());
                // ポインタ型の場合はTYPE_POINTERをヒントとして渡す
                TypeInfo type_hint = (typed_value.numeric_type == TYPE_POINTER)
                                         ? TYPE_POINTER
                                         : TYPE_UNKNOWN;
                interpreter.assign_variable(target_name, typed_value, type_hint,
                                            false);
            } catch (const ReturnException &ret) {
                if (ret.is_struct) {
                    interpreter.current_scope().variables[target_name] =
                        ret.struct_value;
                    interpreter.sync_direct_access_from_struct_value(
                        target_name,
                        interpreter.current_scope().variables[target_name]);
                } else {
                    throw;
                }
            }
        } else {
            // const安全性チェック:
            // const変数のアドレスを非constポインタに代入しようとしていないか確認
            if (node->right->node_type == ASTNodeType::AST_UNARY_OP &&
                node->right->op == "ADDRESS_OF" && node->right->left &&
                node->right->left->node_type == ASTNodeType::AST_VARIABLE) {
                Variable *target_var =
                    interpreter.find_variable(node->right->left->name);
                Variable *ptr_var = interpreter.find_variable(target_name);

                // ケース1: const変数のアドレスを非constポインタに代入
                if (target_var && target_var->is_const && ptr_var &&
                    ptr_var->type == TYPE_POINTER &&
                    !ptr_var->is_pointee_const) {
                    throw std::runtime_error(
                        "Cannot assign address of const variable '" +
                        node->right->left->name + "' to non-const pointer '" +
                        target_name + "'. Use 'const " +
                        type_info_to_string(ptr_var->pointer_base_type) +
                        "*' instead of '" +
                        type_info_to_string(ptr_var->pointer_base_type) + "*'");
                }

                // ケース2: const pointer (const T*) のアドレスを非const double
                // pointer (T**) に代入
                if (target_var && target_var->type == TYPE_POINTER &&
                    target_var->is_pointee_const && ptr_var &&
                    ptr_var->type == TYPE_POINTER &&
                    ptr_var->pointer_depth >= 2 && !ptr_var->is_pointee_const) {
                    throw std::runtime_error(
                        "Cannot assign address of pointer to const (const T*) "
                        "'" +
                        node->right->left->name +
                        "' to non-const double pointer '" + target_name +
                        "'. The pointee should be 'const T**', not 'T**'");
                }

                // ケース3: const pointer (T* const)
                // のアドレスを取得する場合も同様
                if (target_var && target_var->type == TYPE_POINTER &&
                    target_var->is_pointer_const && ptr_var &&
                    ptr_var->type == TYPE_POINTER &&
                    ptr_var->pointer_depth >= 2 && !ptr_var->is_pointee_const) {
                    throw std::runtime_error(
                        "Cannot assign address of const pointer (T* const) '" +
                        node->right->left->name +
                        "' to non-const double pointer '" + target_name +
                        "'. Use 'const' qualifier appropriately");
                }
            }

            TypedValue typed_value =
                interpreter.evaluate_typed_expression(node->right.get());
            // ポインタ型の場合はTYPE_POINTERをヒントとして渡す
            TypeInfo type_hint = (typed_value.numeric_type == TYPE_POINTER)
                                     ? TYPE_POINTER
                                     : TYPE_UNKNOWN;
            interpreter.assign_variable(target_name, typed_value, type_hint,
                                        false);
        }
    }
}

} // namespace AssignmentHandlers
