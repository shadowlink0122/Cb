#include "member_assignment.h"
#include "../../../../common/type_helpers.h"
#include "../statement_executor.h"
#include "const_check_helpers.h"
#include "core/error_handler.h"
#include "core/interpreter.h"
#include "core/pointer_metadata.h"
#include "managers/variables/manager.h"
#include "recursive_member_resolver.h"

namespace AssignmentHandlers {

void execute_member_assignment(StatementExecutor *executor,
                               Interpreter &interpreter, const ASTNode *node) {
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

        // constポインタチェック（const T*経由でのメンバ変更を禁止）
        AssignmentHelpers::check_const_pointer_modification(
            interpreter, member_access->left->left.get());

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
        // v0.13.1: 参照がある場合はそれを使用
        auto &members = struct_var->get_struct_members();
        members[member_name] = new_value;

        // 個別変数システムとの同期
        interpreter.sync_individual_member_from_struct(struct_var, member_name);

        if (debug_mode) {
            debug_print("DEBUG: Dereference member assignment completed\n");
        }
        return;
    } else if (member_access->left &&
               (member_access->left->node_type ==
                    ASTNodeType::AST_MEMBER_ACCESS ||
                member_access->left->node_type == ASTNodeType::AST_ARRAY_REF)) {
        // ネストメンバアクセス: obj.mid.data.value = 100
        // または配列を含むネスト: container.shapes[0].edges[0].start.x = 10
        debug_print(
            "DEBUG: Nested/Array member access assignment - member=%s\n",
            member_access->name.c_str());

        // 評価用のラムダ関数
        auto evaluate_index =
            [&interpreter](const ASTNode *idx_node) -> int64_t {
            return interpreter.evaluate(idx_node);
        };

        // ルート変数名を取得してconstチェック
        const ASTNode *root_node = member_access->left.get();
        while (root_node &&
               (root_node->node_type == ASTNodeType::AST_MEMBER_ACCESS ||
                root_node->node_type == ASTNodeType::AST_ARRAY_REF)) {
            if (root_node->node_type == ASTNodeType::AST_MEMBER_ACCESS) {
                root_node = root_node->left.get();
            } else if (root_node->node_type == ASTNodeType::AST_ARRAY_REF) {
                root_node = root_node->left.get();
            }
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

        // 再帰的に親構造体と最終メンバー名を解決
        auto [parent_struct, final_member] =
            AssignmentHelpers::resolve_nested_member_for_assignment(
                interpreter, member_access, evaluate_index);

        if (!parent_struct || !parent_struct->is_struct) {
            throw std::runtime_error("Parent is not a struct");
        }

        // v0.13.1: struct_members_refを考慮した参照取得
        auto &members = parent_struct->get_struct_members();

        if (debug_mode) {
            debug_print(
                "DEBUG: parent_struct=%p, final_member=%s, members=%zu\n",
                static_cast<void *>(parent_struct), final_member.c_str(),
                members.size());
        }

        debug_print("DEBUG: Resolved parent struct, final member: %s\n",
                    final_member.c_str());

        // constメンバへの代入チェック
        auto final_member_it = members.find(final_member);
        if (final_member_it != members.end()) {
            if (final_member_it->second.is_const &&
                final_member_it->second.is_assigned) {
                throw std::runtime_error("Cannot assign to const member '" +
                                         final_member +
                                         "' after initialization");
            }
        }

        // 親のstruct_membersに直接代入（参照経由）
        auto &member_ref = members[final_member];

        // 右辺を評価して代入
        if (node->right->node_type == ASTNodeType::AST_STRING_LITERAL) {
            member_ref.str_value = node->right->str_value;
            member_ref.type = TYPE_STRING;
        } else {
            TypedValue typed_value =
                interpreter.evaluate_typed(node->right.get());
            if (typed_value.is_floating()) {
                member_ref.double_value = typed_value.as_double();
                member_ref.type = typed_value.type.type_info;
            } else {
                member_ref.value = typed_value.as_numeric();
                member_ref.type = typed_value.type.type_info;
            }
        }
        member_ref.is_assigned = true;

        if (debug_mode) {
            debug_print("DEBUG: member_ref after assignment -> value=%lld, "
                        "is_assigned=%d\n",
                        member_ref.value, member_ref.is_assigned ? 1 : 0);
        }

        debug_print("DEBUG: Nested member assignment completed: %s = %lld\n",
                    final_member.c_str(), member_ref.value);

        // 個別変数システムとの同期
        // 完全なベースパスを構築 (例: points[0])
        std::function<std::string(const ASTNode *)> build_base_path =
            [&](const ASTNode *base) -> std::string {
            if (!base)
                return "";
            if (base->node_type == ASTNodeType::AST_VARIABLE ||
                base->node_type == ASTNodeType::AST_IDENTIFIER) {
                return base->name;
            } else if (base->node_type == ASTNodeType::AST_ARRAY_REF) {
                std::string left_path = build_base_path(base->left.get());
                int64_t index = interpreter.evaluate(base->array_index.get());
                return left_path + "[" + std::to_string(index) + "]";
            } else if (base->node_type == ASTNodeType::AST_MEMBER_ACCESS) {
                std::string left_path = build_base_path(base->left.get());
                return left_path + "." + base->name;
            }
            return "";
        };

        std::string base_path = build_base_path(member_access->left.get());
        if (!base_path.empty()) {
            std::string full_member_path = base_path + "." + final_member;
            Variable *individual_var =
                interpreter.find_variable(full_member_path);
            if (individual_var) {
                individual_var->value = member_ref.value;
                individual_var->type = member_ref.type;
                individual_var->str_value = member_ref.str_value;
                individual_var->is_assigned = member_ref.is_assigned;
                individual_var->is_const = member_ref.is_const;
                individual_var->is_unsigned = member_ref.is_unsigned;
                if (member_ref.type == TYPE_FLOAT ||
                    member_ref.type == TYPE_DOUBLE ||
                    member_ref.type == TYPE_QUAD) {
                    individual_var->float_value = member_ref.float_value;
                    individual_var->double_value = member_ref.double_value;
                    individual_var->quad_value = member_ref.quad_value;
                }
                debug_print("DEBUG: Synced individual variable: %s = %lld\n",
                            full_member_path.c_str(), individual_var->value);
            }
        }

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

        // デリファレンスの対象がメンバーアクセスか単純な変数かを判定
        ASTNode *deref_target = member_access->left->left.get();

        // constポインタチェック（const T*経由でのメンバ変更を禁止）
        AssignmentHelpers::check_const_pointer_modification(interpreter,
                                                            deref_target);

        // ポインタを評価（デリファレンスの左側を完全に評価）
        // これにより (*o.middle).inner や o.middle
        // などのネストした式も処理される
        int64_t ptr_value = 0;

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
        // v0.13.1: 参照がある場合はそれを使用
        auto &members = struct_var->get_struct_members();
        members[member_name] = new_value;

        // 個別変数システムとの同期
        interpreter.sync_individual_member_from_struct(struct_var, member_name);

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
        // v0.13.1: struct_members_refを考慮
        auto &members = target_var->get_struct_members();
        auto member_it = members.find(member_name);
        if (member_it != members.end()) {
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

        // v0.13.1: 参照の場合: actual_varのメンバーに直接代入
        auto &actual_members = actual_var->get_struct_members();
        auto member_it = actual_members.find(member_name);
        if (member_it == actual_members.end()) {
            throw std::runtime_error("Struct member not found: " + member_name);
        }

        Variable &member_var = member_it->second;

        // 右辺を評価して代入
        if (node->right->node_type == ASTNodeType::AST_STRING_LITERAL) {
            member_var.str_value = node->right->str_value;
            member_var.type = TYPE_STRING;
            member_var.is_assigned = true;

            // v0.13.1: 参照変数自体のstruct_membersも更新（エイリアシング）
            auto &base_members = base_var->get_struct_members();
            auto ref_member_it = base_members.find(member_name);
            if (ref_member_it != base_members.end()) {
                ref_member_it->second.str_value = node->right->str_value;
                ref_member_it->second.type = TYPE_STRING;
                ref_member_it->second.is_assigned = true;
            }

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

            // v0.13.1: 参照変数自体のstruct_membersも更新（エイリアシング）
            auto &base_members = base_var->get_struct_members();
            auto ref_member_it = base_members.find(member_name);
            if (ref_member_it != base_members.end()) {
                ref_member_it->second.value = typed_value.value;
                ref_member_it->second.type = typed_value.numeric_type;
                ref_member_it->second.is_assigned = true;
            }

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
                if (TypeHelpers::isStruct(ret_ex.struct_value.type)) {
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
                              Interpreter &interpreter, const ASTNode *node) {
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

    // v0.11.0 Week 2 Day 3: ptr[index]->member = value パターン対応
    // 左側がポインタ配列アクセス (ptr[0])
    // の場合、ReturnExceptionで構造体が返される
    Variable *struct_var = nullptr;
    std::string
        struct_element_name; // 構造体配列要素の変数名（例: "points[0]"）

    // ポインタ戻り値の型情報を保持
    bool is_generic_pointer_return = false;
    std::string pointer_base_type_name;

    try {
        // まず、ポインタ変数自体を取得（arrow_access->leftが変数参照の場合）
        if (arrow_access->left &&
            arrow_access->left->node_type == ASTNodeType::AST_VARIABLE) {
            // 変数名からVariable*を取得
            std::string var_name = arrow_access->left->name;
            struct_var = interpreter.find_variable(var_name);

            if (!struct_var) {
                throw std::runtime_error(
                    "Variable not found in arrow assignment: " + var_name);
            }

            if (debug_mode) {
                debug_print(
                    "[ARROW_ASSIGN] Found variable '%s': is_pointer=%d, "
                    "pointer_base_type_name='%s', value=0x%llx\n",
                    var_name.c_str(), struct_var->is_pointer ? 1 : 0,
                    struct_var->pointer_base_type_name.c_str(),
                    (unsigned long long)struct_var->value);
            }

            if (struct_var->value == 0) {
                throw std::runtime_error(
                    "Null pointer dereference in arrow assignment");
            }
        } else {
            // ポインタを評価（メソッド呼び出しや配列アクセスなど）
            int64_t ptr_value = 0;

            // ReturnExceptionをキャッチしてポインタ型情報を取得
            try {
                ptr_value = interpreter.evaluate(arrow_access->left.get());
            } catch (const ReturnException &ret_ex) {
                if (debug_mode) {
                    debug_print("DEBUG: execute_arrow_assignment - caught "
                                "ReturnException: "
                                "is_pointer=%d, pointer_base_type_name='%s', "
                                "value=0x%llx\n",
                                ret_ex.is_pointer ? 1 : 0,
                                ret_ex.pointer_base_type_name.c_str(),
                                (unsigned long long)ret_ex.value);
                }

                // ポインタ情報を保存
                if (ret_ex.is_pointer) {
                    is_generic_pointer_return = true;
                    pointer_base_type_name = ret_ex.pointer_base_type_name;
                }

                ptr_value = ret_ex.value;
            }

            if (ptr_value == 0) {
                throw std::runtime_error(
                    "Null pointer dereference in arrow assignment");
            }

            // メタデータポインタかどうかをチェック（最上位ビットが1）
            bool has_metadata = (ptr_value & (1LL << 63)) != 0;

            if (has_metadata) {
                // メタデータポインタの場合、最上位ビットをクリア
                int64_t meta_ptr = ptr_value & ~(1LL << 63);
                PointerSystem::PointerMetadata *metadata =
                    reinterpret_cast<PointerSystem::PointerMetadata *>(
                        meta_ptr);

                if (!metadata) {
                    throw std::runtime_error(
                        "Invalid metadata pointer in arrow assignment");
                }

                // メタデータの種類に応じて処理
                if (metadata->target_type ==
                        PointerSystem::PointerTargetType::VARIABLE &&
                    metadata->var_ptr) {
                    struct_var = metadata->var_ptr;
                } else if (metadata->target_type ==
                           PointerSystem::PointerTargetType::ARRAY_ELEMENT) {
                    // 配列要素の場合、array_name[index]という変数名を構築
                    if (!metadata->array_var) {
                        throw std::runtime_error(
                            "Invalid array metadata in arrow assignment");
                    }

                    if (debug_mode) {
                        debug_print("DEBUG: ARROW_ASSIGN metadata - "
                                    "array_name='%s', element_index=%zu\n",
                                    metadata->array_name.c_str(),
                                    metadata->element_index);
                    }

                    struct_element_name =
                        metadata->array_name + "[" +
                        std::to_string(metadata->element_index) + "]";
                    struct_var = interpreter.find_variable(struct_element_name);

                    if (!struct_var) {
                        throw std::runtime_error(
                            "Struct array element not found: " +
                            struct_element_name);
                    }
                } else {
                    throw std::runtime_error(
                        "Unsupported metadata type in arrow assignment");
                }
            } else {
                // 通常のポインタの場合
                struct_var = reinterpret_cast<Variable *>(ptr_value);
            }

            if (!struct_var) {
                throw std::runtime_error("Invalid pointer in arrow assignment");
            }
        }
    } catch (const ReturnException &ret) {
        // 構造体が返された場合（ptr[index]からの構造体）
        if (ret.is_struct) {
            // ptr[index]から返された構造体は一時的なコピーなので、
            // 元の配列要素の変数名を取得する必要がある
            // AST構造から配列名とインデックスを抽出
            if (arrow_access->left &&
                arrow_access->left->node_type == ASTNodeType::AST_ARRAY_REF) {
                std::string ptr_var_name;
                if (arrow_access->left->left &&
                    arrow_access->left->left->node_type ==
                        ASTNodeType::AST_VARIABLE) {
                    ptr_var_name = arrow_access->left->left->name;
                } else if (!arrow_access->left->name.empty()) {
                    ptr_var_name = arrow_access->left->name;
                }

                // ポインタ変数を取得し、メタデータから配列名を取得
                Variable *ptr_var = interpreter.find_variable(ptr_var_name);
                if (!ptr_var || !ptr_var->is_pointer) {
                    throw std::runtime_error(
                        "Invalid pointer variable in arrow assignment");
                }

                std::string array_name;
                int64_t ptr_value = ptr_var->value;
                bool is_metadata_ptr = (ptr_value < 0); // 負の値 = メタデータ

                if (is_metadata_ptr) {
                    // メタデータポインタの場合
                    int64_t clean_ptr = ptr_value & ~(1LL << 63);
                    PointerSystem::PointerMetadata *meta =
                        reinterpret_cast<PointerSystem::PointerMetadata *>(
                            clean_ptr);

                    if (meta && !meta->array_name.empty()) {
                        array_name = meta->array_name;
                    } else {
                        throw std::runtime_error(
                            "Pointer metadata does not contain array name");
                    }
                } else {
                    // 直接のVariable*ポインタの場合は配列変数名を推定できない
                    throw std::runtime_error(
                        "Direct pointer does not have array name information");
                }

                int64_t index =
                    interpreter.evaluate(arrow_access->left->array_index.get());
                struct_element_name =
                    array_name + "[" + std::to_string(index) + "]";

                // 元の配列要素変数を取得
                struct_var = interpreter.find_variable(struct_element_name);
                if (!struct_var) {
                    throw std::runtime_error(
                        "Struct array element not found: " +
                        struct_element_name);
                }
            } else {
                throw std::runtime_error("Cannot determine struct element name "
                                         "from arrow assignment");
            }
        } else {
            // その他のReturnExceptionは再投げ
            throw;
        }
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
        new_value.type = typed_value.type.type_info;

        // 型に応じて適切なフィールドに値を格納
        if (typed_value.type.type_info == TYPE_STRING) {
            new_value.str_value = typed_value.string_value;
        } else if (typed_value.type.type_info == TYPE_FLOAT) {
            float f_val = static_cast<float>(typed_value.as_double());
            new_value.float_value = f_val;
            new_value.value = static_cast<int64_t>(f_val);
        } else if (typed_value.type.type_info == TYPE_DOUBLE) {
            double d_val = typed_value.as_double();
            new_value.double_value = d_val;
            new_value.value = static_cast<int64_t>(d_val);
            if (debug_mode) {
                debug_print(
                    "[ARROW_ASSIGN] Setting double member '%s': "
                    "typed_value.double_value=%f, new_value.double_value=%f\n",
                    member_name.c_str(), typed_value.double_value,
                    new_value.double_value);
            }
        } else if (typed_value.type.type_info == TYPE_QUAD) {
            new_value.quad_value = typed_value.as_quad();
            new_value.value = static_cast<int64_t>(typed_value.as_quad());
        } else {
            new_value.value = typed_value.as_numeric();
        }
    }
    new_value.is_assigned = true;

    // ポインタ経由のアクセスの場合、ポインタ先の構造体レイアウトに従って処理
    // ジェネリック構造体の場合は生メモリに直接書き込む
    // 非ジェネリック構造体の場合はstruct_membersを更新
    if (struct_var->is_pointer && !struct_var->pointer_base_type_name.empty()) {
        // ジェネリック型パラメータを現在のTypeContextで解決
        std::string resolved_type_name = interpreter.resolve_type_in_context(
            struct_var->pointer_base_type_name);

        if (debug_mode) {
            debug_print("[ARROW_ASSIGN] Pointer-to-struct access: "
                        "base_type='%s', resolved='%s', member='%s'\n",
                        struct_var->pointer_base_type_name.c_str(),
                        resolved_type_name.c_str(), member_name.c_str());
        }

        // 構造体定義を取得
        const StructDefinition *struct_def =
            interpreter.get_struct_definition(resolved_type_name);

        // 見つからない場合、ジェネリック構造体のベース定義を取得
        if (!struct_def && resolved_type_name.find('<') != std::string::npos) {
            // MapNode<int, int> から MapNode を抽出
            size_t angle_pos = resolved_type_name.find('<');
            std::string base_struct_name =
                resolved_type_name.substr(0, angle_pos);

            if (debug_mode) {
                debug_print("[ARROW_ASSIGN] Trying base generic struct: "
                            "base='%s', full='%s'\n",
                            base_struct_name.c_str(),
                            resolved_type_name.c_str());
            }

            // ベース定義を取得
            struct_def = interpreter.get_struct_definition(base_struct_name);
        }

        if (!struct_def) {
            throw std::runtime_error(
                "Cannot find struct definition for pointer base type: " +
                resolved_type_name);
        }

        // 非ジェネリック構造体の場合、Variable*を取得してstruct_membersを更新
        // ジェネリック構造体の場合のみ生メモリアクセス
        bool is_generic_struct =
            struct_def->is_generic ||
            (resolved_type_name.find('<') != std::string::npos);

        if (!is_generic_struct) {
            // 非ジェネリック構造体: ポインタ先のVariable*を取得
            Variable *target_var =
                reinterpret_cast<Variable *>(struct_var->value);

            if (!target_var) {
                throw std::runtime_error(
                    "Null pointer dereference in arrow assignment");
            }

            if (debug_mode) {
                debug_print("[ARROW_ASSIGN] Non-generic struct: updating "
                            "struct_members "
                            "for member='%s', target_var=%p\n",
                            member_name.c_str(), (void *)target_var);
            }

            // struct_membersに代入
            auto &members = target_var->get_struct_members();
            members[member_name] = new_value;

            // 個別変数システムとの同期
            interpreter.sync_individual_member_from_struct(target_var,
                                                           member_name);

            if (debug_mode) {
                debug_print("[ARROW_ASSIGN] Updated struct_members for "
                            "non-generic struct\n");
            }

            return; // 処理完了
        }

        // 以下はジェネリック構造体の場合のみ実行
        // メンバーのオフセットを計算
        size_t offset = 0;
        bool member_found = false;
        TypeInfo member_type = TYPE_UNKNOWN;

        for (const auto &member : struct_def->members) {
            // ジェネリック型パラメータを解決
            TypeInfo actual_type = member.type;
            if (actual_type == TYPE_UNKNOWN && !member.type_alias.empty()) {
                std::string resolved =
                    interpreter.resolve_type_in_context(member.type_alias);

                if (resolved == "int")
                    actual_type = TYPE_INT;
                else if (resolved == "long")
                    actual_type = TYPE_LONG;
                else if (resolved == "short")
                    actual_type = TYPE_SHORT;
                else if (resolved == "tiny")
                    actual_type = TYPE_TINY;
                else if (resolved == "char")
                    actual_type = TYPE_CHAR;
                else if (resolved == "bool")
                    actual_type = TYPE_BOOL;
                else if (resolved == "float")
                    actual_type = TYPE_FLOAT;
                else if (resolved == "double")
                    actual_type = TYPE_DOUBLE;
                else if (resolved == "string")
                    actual_type = TYPE_STRING;
                else if (resolved.find('*') != std::string::npos)
                    actual_type = TYPE_POINTER;
            }

            // メンバーのサイズを取得
            size_t member_size = 0;
            if (member.is_pointer || actual_type == TYPE_POINTER) {
                member_size = sizeof(void *);
            } else {
                switch (actual_type) {
                case TYPE_INT:
                    member_size = 4;
                    break;
                case TYPE_LONG:
                    member_size = 8;
                    break;
                case TYPE_SHORT:
                    member_size = 2;
                    break;
                case TYPE_TINY:
                    member_size = 1;
                    break;
                case TYPE_CHAR:
                    member_size = 1;
                    break;
                case TYPE_BOOL:
                    member_size = 1;
                    break;
                case TYPE_STRING:
                    member_size = sizeof(void *);
                    break;
                case TYPE_FLOAT:
                    member_size = 4;
                    break;
                case TYPE_DOUBLE:
                    member_size = 8;
                    break;
                default:
                    member_size = sizeof(void *);
                    break;
                }
            }

            // アライメントを適用
            size_t alignment = member_size;
            if (alignment > 8)
                alignment = 8;
            if (alignment > 0) {
                size_t padding = (alignment - (offset % alignment)) % alignment;
                offset += padding;
            }

            if (member.name == member_name) {
                member_found = true;
                member_type = member.type;

                // ジェネリック型パラメータの場合、TypeContextで解決
                if (member_type == TYPE_UNKNOWN && !member.type_alias.empty()) {
                    std::string resolved_member_type =
                        interpreter.resolve_type_in_context(member.type_alias);

                    if (debug_mode) {
                        debug_print(
                            "[ARROW_ASSIGN] Member '%s' has generic type '%s', "
                            "resolved to '%s'\n",
                            member_name.c_str(), member.type_alias.c_str(),
                            resolved_member_type.c_str());
                    }

                    // 解決された型からTypeInfoを取得
                    if (resolved_member_type == "int")
                        member_type = TYPE_INT;
                    else if (resolved_member_type == "long")
                        member_type = TYPE_LONG;
                    else if (resolved_member_type == "short")
                        member_type = TYPE_SHORT;
                    else if (resolved_member_type == "tiny")
                        member_type = TYPE_TINY;
                    else if (resolved_member_type == "char")
                        member_type = TYPE_CHAR;
                    else if (resolved_member_type == "bool")
                        member_type = TYPE_BOOL;
                    else if (resolved_member_type == "float")
                        member_type = TYPE_FLOAT;
                    else if (resolved_member_type == "double")
                        member_type = TYPE_DOUBLE;
                    else if (resolved_member_type == "string")
                        member_type = TYPE_STRING;
                    else if (resolved_member_type.find('*') !=
                             std::string::npos)
                        member_type = TYPE_POINTER;
                }

                if (debug_mode) {
                    debug_print("[ARROW_ASSIGN] Found member '%s': type=%d, "
                                "is_pointer=%d, type_alias='%s'\n",
                                member_name.c_str(), (int)member_type,
                                member.is_pointer ? 1 : 0,
                                member.type_alias.c_str());
                }
                break;
            }

            // 次のメンバーへ進む
            offset += member_size;
        }

        if (!member_found) {
            throw std::runtime_error("Member '" + member_name +
                                     "' not found in struct " +
                                     resolved_type_name);
        }

        // ポインタ先のメモリアドレスを取得
        char *base_addr = reinterpret_cast<char *>(struct_var->value);
        if (base_addr == nullptr) {
            throw std::runtime_error(
                "Null pointer dereference in arrow assignment");
        }

        // メンバーのアドレスを計算
        char *member_addr = base_addr + offset;

        // 型に応じてメモリに書き込む
        switch (member_type) {
        case TYPE_INT:
            *reinterpret_cast<int32_t *>(member_addr) =
                static_cast<int32_t>(new_value.value);
            break;
        case TYPE_LONG:
            *reinterpret_cast<int64_t *>(member_addr) = new_value.value;
            break;
        case TYPE_SHORT:
            *reinterpret_cast<int16_t *>(member_addr) =
                static_cast<int16_t>(new_value.value);
            break;
        case TYPE_TINY:
        case TYPE_CHAR:
            *reinterpret_cast<int8_t *>(member_addr) =
                static_cast<int8_t>(new_value.value);
            break;
        case TYPE_BOOL:
            *reinterpret_cast<bool *>(member_addr) =
                static_cast<bool>(new_value.value);
            break;
        case TYPE_FLOAT:
            *reinterpret_cast<float *>(member_addr) = new_value.float_value;
            break;
        case TYPE_DOUBLE:
            *reinterpret_cast<double *>(member_addr) = new_value.double_value;
            break;
        case TYPE_POINTER:
            if (debug_mode) {
                debug_print(
                    "[ARROW_ASSIGN] Writing pointer: addr=%p, value=0x%llx\n",
                    (void *)member_addr, (unsigned long long)new_value.value);
            }
            *reinterpret_cast<int64_t *>(member_addr) = new_value.value;
            break;
        case TYPE_STRING: {
            // 文字列の場合、C文字列としてコピーしてポインタを保存
            // 注意: メモリ管理が必要（将来的にはGC or デストラクタで解放）
            // Variable は str_value フィールドを使用

            // デバッグ: 入力値を確認
            debug_print("[ARROW_ASSIGN] String assignment: str_value='%s', "
                        "value=%lld (0x%llx)\n",
                        new_value.str_value.c_str(), (long long)new_value.value,
                        (unsigned long long)new_value.value);

            const char *str_data = strdup(new_value.str_value.c_str());
            *reinterpret_cast<const char **>(member_addr) = str_data;
            debug_print(
                "[ARROW_ASSIGN] Wrote string: addr=%p, ptr=%p, str='%s'\n",
                (void *)member_addr, (void *)str_data, str_data);
            break;
        }
        default:
            throw std::runtime_error(
                "Unsupported member type for pointer-based assignment");
        }

        if (debug_mode) {
            debug_print(
                "[ARROW_ASSIGN] Wrote to memory: addr=0x%llx, offset=%zu, "
                "value=%lld\n",
                (unsigned long long)base_addr, offset,
                (long long)new_value.value);
        }

        // ポインタ経由の場合は struct_members への代入はスキップ
        // （ポインタ変数自体には struct_members がない）
    } else {
        // 通常の構造体変数の場合
        // struct_membersに代入
        // v0.13.1: 参照がある場合はそれを使用
        auto &members = struct_var->get_struct_members();
        members[member_name] = new_value;

        // 個別変数システムとの同期
        interpreter.sync_individual_member_from_struct(struct_var, member_name);
    }

    if (debug_mode) {
        debug_print("DEBUG: execute_arrow_assignment - completed\n");
    }
}

} // namespace AssignmentHandlers
