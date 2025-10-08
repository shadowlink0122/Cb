#include "member_assignment.h"
#include "../statement_executor.h"
#include "core/interpreter.h"
#include "core/error_handler.h"
#include "managers/variables/manager.h"

namespace AssignmentHandlers {

void execute_member_assignment(StatementExecutor *executor,
                                Interpreter &interpreter,
                                const ASTNode *node) {
    // obj.member = value または array[index].member = value の処理
    const ASTNode *member_access = node->left.get();

    if (debug_mode) {
        debug_print("DEBUG: execute_member_assignment - starting\n");
    }
    debug_print(
        "DEBUG: execute_member_assignment - left type=%d, right type=%d\n",
        static_cast<int>(node->left->node_type),
        static_cast<int>(node->right->node_type));

    if (member_access->left) {
        debug_print("DEBUG: member_access->left->node_type=%d, name='%s'\n",
                    static_cast<int>(member_access->left->node_type),
                    member_access->left->name.c_str());
    } else {
        debug_print("DEBUG: member_access->left is null\n");
    }

    if (!member_access ||
        member_access->node_type != ASTNodeType::AST_MEMBER_ACCESS) {
        throw std::runtime_error("Invalid member access in assignment");
    }

    // オブジェクト名を取得
    std::string obj_name;
    if (member_access->left &&
        (member_access->left->node_type == ASTNodeType::AST_VARIABLE ||
         member_access->left->node_type == ASTNodeType::AST_IDENTIFIER)) {
        // 通常の構造体変数: obj.member または self.member
        obj_name = member_access->left->name;

        // selfの場合は特別処理
        if (obj_name == "self") {
            debug_msg(DebugMsgId::SELF_MEMBER_ACCESS_START,
                      member_access->name.c_str());
            // selfへの代入処理を実行
            executor->execute_self_member_assignment(member_access->name,
                                           node->right.get());
            return;
        }

        if (debug_mode) {
            debug_print("DEBUG: Struct member access - variable: %s\n",
                        obj_name.c_str());
        }
    } else if (member_access->left &&
               member_access->left->node_type == ASTNodeType::AST_UNARY_OP &&
               member_access->left->op == "*") {
        // デリファレンスされたポインタへのメンバアクセス: (*ptr).member = value
        debug_print("DEBUG: Dereference member access assignment - member=%s\n",
                    member_access->name.c_str());

        // ポインタを評価
        int64_t ptr_value =
            interpreter.evaluate(member_access->left->left.get());

        if (ptr_value == 0) {
            throw std::runtime_error(
                "Null pointer dereference in member assignment");
        }

        // ポインタから構造体変数を取得
        Variable *struct_var = reinterpret_cast<Variable *>(ptr_value);

        if (!struct_var) {
            throw std::runtime_error(
                "Invalid pointer in dereference member assignment");
        }

        // メンバ名を取得
        std::string member_name = member_access->name;

        // 右辺を評価
        Variable new_value;
        if (node->right->node_type == ASTNodeType::AST_STRING_LITERAL) {
            new_value.str_value = node->right->str_value;
            new_value.type = TYPE_STRING;
        } else {
            TypedValue typed_value =
                interpreter.evaluate_typed(node->right.get());
            new_value.value = typed_value.as_numeric();
            new_value.type = typed_value.type.type_info;
        }
        new_value.is_assigned = true;

        // struct_membersに代入
        struct_var->struct_members[member_name] = new_value;

        // 個別変数システムとの同期
        interpreter.sync_individual_member_from_struct(struct_var,
                                                        member_name);

        if (debug_mode) {
            debug_print("DEBUG: Dereference member assignment completed\n");
        }
        return;
    } else if (member_access->left && member_access->left->node_type ==
                                          ASTNodeType::AST_MEMBER_ACCESS) {
        // ネストメンバアクセス: obj.mid.data.value = 100
        // member_accessの左側のメンバアクセスチェーンを評価して、最後の構造体変数を取得
        debug_print("DEBUG: Nested member access assignment - member=%s\n",
                    member_access->name.c_str());

        // ルート変数名を取得
        const ASTNode *root_node = member_access->left.get();
        while (root_node &&
               root_node->node_type == ASTNodeType::AST_MEMBER_ACCESS &&
               root_node->left) {
            root_node = root_node->left.get();
        }
        std::string root_var_name;
        if (root_node &&
            (root_node->node_type == ASTNodeType::AST_VARIABLE ||
             root_node->node_type == ASTNodeType::AST_IDENTIFIER)) {
            root_var_name = root_node->name;
        }

        // ルート変数がconstかチェック
        if (!root_var_name.empty()) {
            Variable *root_var = interpreter.find_variable(root_var_name);
            if (root_var && root_var->is_const) {
                throw std::runtime_error(
                    "Cannot assign to member of const struct: " +
                    root_var_name);
            }
        }

        // ネストメンバアクセスを評価して対象の構造体変数を取得
        Variable *target_struct =
            executor->evaluate_nested_member_access(member_access->left.get());

        if (!target_struct) {
            throw std::runtime_error("Cannot resolve nested member access");
        }

        // 左側のメンバアクセスのメンバ名を取得
        std::string parent_member = member_access->left->name;
        debug_print("DEBUG: Parent member: %s\n", parent_member.c_str());

        // parent_memberが構造体メンバかどうか確認
        auto parent_it = target_struct->struct_members.find(parent_member);
        if (parent_it == target_struct->struct_members.end()) {
            throw std::runtime_error("Parent member not found: " +
                                     parent_member);
        }

        Variable &parent_member_var = parent_it->second;
        if (parent_member_var.type != TYPE_STRUCT) {
            throw std::runtime_error("Parent member is not a struct: " +
                                     parent_member);
        }

        // 最終的なメンバ名を取得
        std::string member_name = member_access->name;
        debug_print("DEBUG: Final member: %s\n", member_name.c_str());

        // constメンバへの代入チェック
        auto final_member_it =
            parent_member_var.struct_members.find(member_name);
        if (final_member_it != parent_member_var.struct_members.end()) {
            debug_print("DEBUG: Nested member const check: %s.%s - "
                        "is_const=%d, is_assigned=%d\n",
                        parent_member.c_str(), member_name.c_str(),
                        final_member_it->second.is_const ? 1 : 0,
                        final_member_it->second.is_assigned ? 1 : 0);
            if (final_member_it->second.is_const &&
                final_member_it->second.is_assigned) {
                throw std::runtime_error("Cannot assign to const member '" +
                                         member_name +
                                         "' after initialization");
            }
        }

        //  完全なパスを構築
        std::function<std::string(const ASTNode *)> build_full_path;
        build_full_path = [&](const ASTNode *n) -> std::string {
            if (!n)
                return "";
            if (n->node_type == ASTNodeType::AST_VARIABLE ||
                n->node_type == ASTNodeType::AST_IDENTIFIER) {
                return n->name;
            } else if (n->node_type == ASTNodeType::AST_MEMBER_ACCESS) {
                std::string base = build_full_path(n->left.get());
                return base.empty() ? n->name : base + "." + n->name;
            }
            return "";
        };
        std::string full_member_path = build_full_path(member_access);

        if (debug_mode) {
            debug_print("DEBUG: Nested member assignment - full_path='%s'\n",
                        full_member_path.c_str());
        }

        // 完全パスで個別変数を直接更新
        if (!full_member_path.empty()) {
            Variable *individual_var =
                interpreter.find_variable(full_member_path);
            if (individual_var) {
                if (node->right->node_type == ASTNodeType::AST_STRING_LITERAL) {
                    individual_var->str_value = node->right->str_value;
                    individual_var->type = TYPE_STRING;
                } else {
                    TypedValue typed_value =
                        interpreter.evaluate_typed(node->right.get());
                    individual_var->value = typed_value.as_numeric();
                    individual_var->type = typed_value.type.type_info;
                }
                individual_var->is_assigned = true;

                if (debug_mode) {
                    debug_print(
                        "DEBUG: Updated individual variable '%s' = %lld\n",
                        full_member_path.c_str(), individual_var->value);
                }
            } else {
                if (debug_mode) {
                    debug_print("DEBUG: Individual variable '%s' not found!\n",
                                full_member_path.c_str());
                }
            }
        }

        // struct_members階層も更新（互換性のため）
        if (node->right->node_type == ASTNodeType::AST_STRING_LITERAL) {
            parent_member_var.struct_members[member_name].str_value =
                node->right->str_value;
            parent_member_var.struct_members[member_name].type = TYPE_STRING;
        } else {
            TypedValue typed_value =
                interpreter.evaluate_typed(node->right.get());
            parent_member_var.struct_members[member_name].value =
                typed_value.as_numeric();
            parent_member_var.struct_members[member_name].type =
                typed_value.type.type_info;
        }
        parent_member_var.struct_members[member_name].is_assigned = true;

        return;
    } else if (member_access->left &&
               member_access->left->node_type == ASTNodeType::AST_ARRAY_REF) {
        // 構造体配列要素のメンバ: struct.array[index].member
        // または単純な配列要素: array[index].member

        const ASTNode *array_ref = member_access->left.get();

        // 配列参照の左側を評価して完全な名前を構築
        std::string array_base_name;
        if (array_ref->left->node_type == ASTNodeType::AST_MEMBER_ACCESS) {
            // struct.array[index].member の場合
            std::string struct_name = array_ref->left->left->name;
            std::string array_member = array_ref->left->name;
            array_base_name = struct_name + "." + array_member;
        } else {
            // array[index].member の場合
            array_base_name = array_ref->left->name;
        }

        int64_t index = interpreter.evaluate(array_ref->array_index.get());
        obj_name = array_base_name + "[" + std::to_string(index) + "]";

        if (debug_mode) {
            debug_print(
                "DEBUG: Struct array element member assignment: %s.%s\n",
                obj_name.c_str(), member_access->name.c_str());
        }
    } else if (member_access->left &&
               member_access->left->node_type == ASTNodeType::AST_UNARY_OP &&
               member_access->left->op == "DEREFERENCE") {
        // デリファレンスされたポインタ: (*pp).member or (*(*p).inner).value
        debug_print("DEBUG: Dereference pointer member assignment\n");

        // ポインタを評価（デリファレンスの左側を完全に評価）
        // これにより (*o.middle).inner や o.middle
        // などのネストした式も処理される
        int64_t ptr_value = 0;

        // デリファレンスの対象がメンバーアクセスか単純な変数かを判定
        ASTNode *deref_target = member_access->left->left.get();

        if (debug_mode) {
            debug_print("DEBUG: deref_target node_type=%d (MEMBER_ACCESS=%d)\n",
                        static_cast<int>(deref_target->node_type),
                        static_cast<int>(ASTNodeType::AST_MEMBER_ACCESS));
        }

        if (deref_target->node_type == ASTNodeType::AST_MEMBER_ACCESS) {
            // ネストしたメンバーアクセス: (*o.middle).inner の場合
            // 完全に評価してポインタ値を取得
            ptr_value = interpreter.evaluate(deref_target);

            if (debug_mode) {
                debug_print("DEBUG: Nested member access evaluated to "
                            "ptr_value=%lld (0x%llx)\n",
                            ptr_value, static_cast<uint64_t>(ptr_value));
            }
        } else {
            // 単純な変数: (*ptr).member の場合
            ptr_value = interpreter.evaluate(deref_target);

            if (debug_mode) {
                debug_print("DEBUG: Simple variable evaluated to "
                            "ptr_value=%lld (0x%llx)\n",
                            ptr_value, static_cast<uint64_t>(ptr_value));
            }
        }

        Variable *struct_var = reinterpret_cast<Variable *>(ptr_value);

        if (!struct_var || ptr_value == 0) {
            throw std::runtime_error(
                "Null pointer dereference in member assignment");
        }

        // メンバ名を取得
        std::string member_name = member_access->name;

        // 右辺を評価
        Variable new_value;
        if (node->right->node_type == ASTNodeType::AST_STRING_LITERAL) {
            new_value.str_value = node->right->str_value;
            new_value.type = TYPE_STRING;
        } else {
            TypedValue typed_value =
                interpreter.evaluate_typed(node->right.get());
            new_value.value = typed_value.as_numeric();
            new_value.type = typed_value.type.type_info;
        }
        new_value.is_assigned = true;

        // struct_membersに代入
        struct_var->struct_members[member_name] = new_value;

        // 個別変数システムとの同期
        interpreter.sync_individual_member_from_struct(struct_var,
                                                        member_name);

        if (debug_mode) {
            debug_print("DEBUG: Dereference member assignment completed: "
                        "struct_var=%p, member=%s, value=%lld\n",
                        static_cast<void *>(struct_var), member_name.c_str(),
                        new_value.value);
        }

        return;
    } else {
        throw std::runtime_error("Invalid object reference in member access");
    }

    // メンバ名を取得
    std::string member_name = member_access->name;

    // constメンバへの代入チェック
    Variable *target_var = interpreter.find_variable(obj_name);

    // 参照の場合は実際の変数を取得
    if (target_var && target_var->is_reference) {
        Variable *actual_var = reinterpret_cast<Variable *>(target_var->value);
        if (!actual_var) {
            throw std::runtime_error("Invalid reference in member assignment");
        }
        target_var = actual_var;
    }

    if (target_var && target_var->is_struct) {
        auto member_it = target_var->struct_members.find(member_name);
        if (member_it != target_var->struct_members.end()) {
            if (member_it->second.is_const && member_it->second.is_assigned) {
                throw std::runtime_error("Cannot assign to const member '" +
                                         member_name + "' of struct '" +
                                         obj_name + "' after initialization");
            }
        }
    }

    // ネストしたメンバーの場合、最上位の親変数のconstもチェック
    std::string root_obj_name = obj_name;
    size_t dot_pos = obj_name.find('.');
    if (dot_pos != std::string::npos) {
        root_obj_name = obj_name.substr(0, dot_pos);
        if (Variable *root_var = interpreter.find_variable(root_obj_name)) {
            if (root_var->is_const) {
                throw std::runtime_error(
                    "Cannot assign to member of const struct: " + obj_name +
                    "." + member_name);
            }
        }
    }

    // 参照の場合は、実際の変数のメンバーに直接代入
    Variable *base_var = interpreter.find_variable(obj_name);
    if (base_var && base_var->is_reference) {
        Variable *actual_var = reinterpret_cast<Variable *>(base_var->value);
        if (!actual_var || !actual_var->is_struct) {
            throw std::runtime_error(
                "Invalid reference or non-struct in member assignment");
        }

        // 参照の場合: actual_varのメンバーに直接代入
        auto member_it = actual_var->struct_members.find(member_name);
        if (member_it == actual_var->struct_members.end()) {
            throw std::runtime_error("Struct member not found: " + member_name);
        }

        Variable &member_var = member_it->second;

        // 右辺を評価して代入
        if (node->right->node_type == ASTNodeType::AST_STRING_LITERAL) {
            member_var.str_value = node->right->str_value;
            member_var.type = TYPE_STRING;
            member_var.is_assigned = true;

            // ダイレクトアクセス変数も更新
            std::string actual_var_name =
                interpreter.find_variable_name_by_address(actual_var);
            if (!actual_var_name.empty()) {
                std::string direct_var_name =
                    actual_var_name + "." + member_name;
                Variable *direct_var =
                    interpreter.find_variable(direct_var_name);
                if (direct_var) {
                    direct_var->str_value = node->right->str_value;
                    direct_var->type = TYPE_STRING;
                    direct_var->is_assigned = true;
                }
            }
        } else {
            TypedValue typed_value =
                interpreter.evaluate_typed(node->right.get());
            member_var.value = typed_value.value;
            member_var.type = typed_value.numeric_type;
            member_var.is_assigned = true;

            // ダイレクトアクセス変数も更新
            std::string actual_var_name =
                interpreter.find_variable_name_by_address(actual_var);
            if (!actual_var_name.empty()) {
                std::string direct_var_name =
                    actual_var_name + "." + member_name;
                Variable *direct_var =
                    interpreter.find_variable(direct_var_name);
                if (direct_var) {
                    direct_var->value = typed_value.value;
                    direct_var->type = typed_value.numeric_type;
                    direct_var->is_assigned = true;
                }
            }
        }
        return;
    }

    // struct変数のメンバに直接代入
    if (node->right->node_type == ASTNodeType::AST_STRING_LITERAL) {
        interpreter.assign_struct_member(obj_name, member_name,
                                          node->right->str_value);
    } else if (node->right->node_type == ASTNodeType::AST_VARIABLE) {
        // 変数参照の場合、構造体変数か数値/文字列変数か判断
        Variable *right_var = interpreter.find_variable(node->right->name);

        if (!right_var) {
            throw std::runtime_error("Right-hand variable not found: " +
                                     node->right->name);
        }

        // 構造体変数の場合、ReturnExceptionをキャッチして構造体を代入
        if (right_var->type == TYPE_STRUCT) {
            try {
                // 構造体変数を評価（ReturnExceptionが投げられる）
                interpreter.evaluate(node->right.get());
                throw std::runtime_error(
                    "Expected struct variable to throw ReturnException");
            } catch (const ReturnException &ret_ex) {
                if (ret_ex.struct_value.type == TYPE_STRUCT) {
                    std::cerr
                        << "DEBUG: Assigning struct to member: " << obj_name
                        << "." << member_name << std::endl;
                    std::cerr << "DEBUG: Source struct type: "
                              << ret_ex.struct_value.struct_type_name
                              << std::endl;

                    // 構造体全体をメンバーに代入
                    interpreter.assign_struct_member_struct(
                        obj_name, member_name, ret_ex.struct_value);
                } else {
                    throw std::runtime_error("Variable is not a struct for "
                                             "struct member assignment");
                }
            }
        } else if (right_var->type == TYPE_STRING) {
            interpreter.assign_struct_member(obj_name, member_name,
                                              right_var->str_value);
        } else {
            // TypedValueを使用して型情報を保持
            TypedValue typed_value =
                interpreter.evaluate_typed(node->right.get());
            interpreter.assign_struct_member(obj_name, member_name,
                                              typed_value);
        }
    } else if (node->right->node_type == ASTNodeType::AST_MEMBER_ACCESS) {
        // 構造体メンバアクセスの場合（original.name等）
        std::string right_obj_name;
        std::string right_member_name = node->right->name;

        if (node->right->left->node_type == ASTNodeType::AST_VARIABLE) {
            right_obj_name = node->right->left->name;
        } else if (node->right->left->node_type == ASTNodeType::AST_ARRAY_REF) {
            // struct配列要素の場合
            std::string array_name = node->right->left->left->name;
            int64_t index =
                interpreter.evaluate(node->right->left->array_index.get());
            right_obj_name = array_name + "[" + std::to_string(index) + "]";
        } else {
            throw std::runtime_error("Invalid right-hand member access");
        }

        // 右辺の構造体メンバを取得
        Variable *right_member_var =
            interpreter.get_struct_member(right_obj_name, right_member_name);
        if (right_member_var->type == TYPE_STRING) {
            interpreter.assign_struct_member(obj_name, member_name,
                                              right_member_var->str_value);
        } else if (right_member_var->type == TYPE_FLOAT ||
                   right_member_var->type == TYPE_DOUBLE ||
                   right_member_var->type == TYPE_QUAD) {
            // 浮動小数点型の場合はTypedValueを作成
            InferredType inferred;
            inferred.type_info = right_member_var->type;
            if (right_member_var->type == TYPE_FLOAT) {
                TypedValue typed_value(
                    static_cast<double>(right_member_var->float_value),
                    inferred);
                typed_value.numeric_type = TYPE_FLOAT;
                interpreter.assign_struct_member(obj_name, member_name,
                                                  typed_value);
            } else if (right_member_var->type == TYPE_DOUBLE) {
                TypedValue typed_value(right_member_var->double_value,
                                       inferred);
                typed_value.numeric_type = TYPE_DOUBLE;
                interpreter.assign_struct_member(obj_name, member_name,
                                                  typed_value);
            } else {
                TypedValue typed_value(right_member_var->quad_value, inferred);
                typed_value.numeric_type = TYPE_QUAD;
                interpreter.assign_struct_member(obj_name, member_name,
                                                  typed_value);
            }
        } else {
            interpreter.assign_struct_member(obj_name, member_name,
                                              right_member_var->value);
        }
    } else if (node->right->node_type == ASTNodeType::AST_MEMBER_ARRAY_ACCESS) {
        // 構造体メンバ配列アクセスの場合（original.tags[0]等）
        debug_print(
            "DEBUG: Processing AST_MEMBER_ARRAY_ACCESS on right-hand side\n");
        std::string right_obj_name;
        std::string right_member_name = node->right->name;

        if (node->right->left->node_type == ASTNodeType::AST_VARIABLE) {
            right_obj_name = node->right->left->name;
        } else if (node->right->left->node_type == ASTNodeType::AST_ARRAY_REF) {
            // struct配列要素の場合
            std::string array_name = node->right->left->left->name;
            int64_t index =
                interpreter.evaluate(node->right->left->array_index.get());
            right_obj_name = array_name + "[" + std::to_string(index) + "]";
        } else {
            throw std::runtime_error("Invalid right-hand member array access");
        }

        // インデックスを評価
        int64_t array_index = interpreter.evaluate(node->right->right.get());

        // 右辺の構造体メンバ配列要素を取得
        Variable *right_member_var =
            interpreter.get_struct_member(right_obj_name, right_member_name);
        debug_print("DEBUG: right_member_var type=%d, is_array=%d\n",
                    static_cast<int>(right_member_var->type),
                    right_member_var->is_array ? 1 : 0);
        if ((right_member_var->type == TYPE_STRING &&
             right_member_var->is_array) ||
            right_member_var->type ==
                static_cast<TypeInfo>(TYPE_ARRAY_BASE + TYPE_STRING)) {
            debug_print("DEBUG: Using string array element access\n");
            std::string str_value =
                interpreter.get_struct_member_array_string_element(
                    right_obj_name, right_member_name,
                    static_cast<int>(array_index));
            interpreter.assign_struct_member(obj_name, member_name, str_value);
        } else {
            debug_print("DEBUG: Using numeric array element access\n");
            int64_t value = interpreter.get_struct_member_array_element(
                right_obj_name, right_member_name,
                static_cast<int>(array_index));
            interpreter.assign_struct_member(obj_name, member_name, value);
        }
    } else {
        // TypedValueを使用して型情報を保持
        TypedValue typed_value = interpreter.evaluate_typed(node->right.get());
        interpreter.assign_struct_member(obj_name, member_name, typed_value);
    }
}

void execute_arrow_assignment(StatementExecutor *executor,
                               Interpreter &interpreter,
                               const ASTNode *node) {
    // ptr->member = value の処理（アロー演算子は (*ptr).member と等価）
    const ASTNode *arrow_access = node->left.get();

    if (debug_mode) {
        debug_print("DEBUG: execute_arrow_assignment - starting\n");
    }
    debug_print(
        "DEBUG: execute_arrow_assignment - left type=%d, right type=%d\n",
        static_cast<int>(node->left->node_type),
        static_cast<int>(node->right->node_type));

    if (!arrow_access ||
        arrow_access->node_type != ASTNodeType::AST_ARROW_ACCESS) {
        throw std::runtime_error("Invalid arrow access in assignment");
    }

    // ポインタを評価
    int64_t ptr_value = interpreter.evaluate(arrow_access->left.get());

    if (ptr_value == 0) {
        throw std::runtime_error(
            "Null pointer dereference in arrow assignment");
    }

    // ポインタから構造体変数を取得
    Variable *struct_var = reinterpret_cast<Variable *>(ptr_value);

    if (!struct_var) {
        throw std::runtime_error("Invalid pointer in arrow assignment");
    }

    // メンバ名を取得
    std::string member_name = arrow_access->name;

    // 右辺を評価
    Variable new_value;
    if (node->right->node_type == ASTNodeType::AST_STRING_LITERAL) {
        new_value.str_value = node->right->str_value;
        new_value.type = TYPE_STRING;
    } else {
        TypedValue typed_value = interpreter.evaluate_typed(node->right.get());
        new_value.value = typed_value.as_numeric();
        new_value.type = typed_value.type.type_info;
    }
    new_value.is_assigned = true;

    // struct_membersに代入
    struct_var->struct_members[member_name] = new_value;

    // 個別変数システムとの同期:
    // struct_var が指す実体の個別メンバ変数も更新する必要がある
    // interpreter の sync_struct_member 関数を使用（存在する場合）
    // もしくは、struct_members の変更を反映するヘルパー関数を呼び出す
    interpreter.sync_individual_member_from_struct(struct_var, member_name);

    if (debug_mode) {
        debug_print("DEBUG: execute_arrow_assignment - completed\n");
    }
}

} // namespace AssignmentHandlers
