#include "executors/statement_executor.h"
#include "../../../common/debug.h"
#include "../../../common/type_alias.h"
#include "core/error_handler.h"
#include "core/interpreter.h"
#include "core/pointer_metadata.h"
#include "core/type_inference.h"
#include "evaluator/core/evaluator.h"
#include "executors/assignments/simple_assignment.h"
#include "executors/declarations/array_declaration.h"
#include "executors/declarations/variable_declaration.h"
#include "managers/arrays/manager.h"
#include "managers/types/manager.h"
#include "managers/variables/manager.h"
#include "services/array_processing_service.h"

StatementExecutor::StatementExecutor(Interpreter &interpreter)
    : interpreter_(interpreter) {}

void StatementExecutor::execute_statement(const ASTNode *node) {
    execute(node);
}

void StatementExecutor::execute(const ASTNode *node) {
    if (!node)
        return;

    // ASTNodeTypeが異常な値でないことを確認
    int node_type_int = static_cast<int>(node->node_type);
    if (node_type_int < 0 || node_type_int > 100) {
        if (debug_mode) {
            std::cerr << "[CRITICAL] Abnormal node_type detected in "
                         "StatementExecutor: "
                      << node_type_int
                      << " (ptr: " << static_cast<const void *>(node) << ")"
                      << std::endl;
            std::cerr << "[CRITICAL] Node name: '" << node->name << "'"
                      << std::endl;
        }
        return;
    }

    if (debug_mode) {
        std::cerr << "[DEBUG_EXECUTE] Executing node type: " << node_type_int
                  << std::endl;
    }

    switch (node->node_type) {
    case ASTNodeType::AST_ASSIGN: {
        execute_assignment(node);
        break;
    }
    case ASTNodeType::AST_VAR_DECL: {
        // Debug output removed - use --debug option if needed
        execute_variable_declaration(node);
        break;
    }
    case ASTNodeType::AST_MULTIPLE_VAR_DECL: {
        execute_multiple_var_decl(node);
        break;
    }
    case ASTNodeType::AST_ARRAY_DECL: {
        execute_array_decl(node);
        break;
    }
    case ASTNodeType::AST_PRE_INCDEC:
    case ASTNodeType::AST_POST_INCDEC: {
        // インクリメント/デクリメントをステートメントとして実行
        // expression_evaluatorで評価するだけで副作用（変数の変更）が発生する
        interpreter_.evaluate(node);
        break;
    }
    // 他のstatement types（AST_FUNC_DECL, AST_IF_STMT等）は
    // Interpreterクラスで直接処理されるため、ここでは未対応
    default:
        // StatementExecutorが対応していないノード型は
        // Interpreterで処理される想定
        break;
    }
}

void StatementExecutor::execute_assignment(const ASTNode *node) {
    AssignmentHandlers::execute_assignment(this, interpreter_, node);
}

void StatementExecutor::execute_variable_declaration(const ASTNode *node) {
    DeclarationHandlers::execute_variable_declaration(this, interpreter_, node);
}

void StatementExecutor::execute_multiple_var_decl(const ASTNode *node) {
    DeclarationHandlers::execute_multiple_var_decl(this, interpreter_, node);
}

void StatementExecutor::execute_array_decl(const ASTNode *node) {
    DeclarationHandlers::execute_array_decl(this, interpreter_, node);
}

void StatementExecutor::execute_struct_array_literal_init(
    const std::string &array_name, const ASTNode *array_literal,
    const std::string &struct_type) {
    DeclarationHandlers::execute_struct_array_literal_init(
        interpreter_, array_name, array_literal, struct_type);
}

void StatementExecutor::execute_member_array_assignment(const ASTNode *node) {
    debug_print("DEBUG: execute_member_array_assignment called\n");
    // obj.member[index] = value の処理
    const ASTNode *member_array_access = node->left.get();

    if (!member_array_access || member_array_access->node_type !=
                                    ASTNodeType::AST_MEMBER_ARRAY_ACCESS) {
        debug_print("DEBUG: Not AST_MEMBER_ARRAY_ACCESS, node_type=%d\n",
                    member_array_access
                        ? static_cast<int>(member_array_access->node_type)
                        : -1);
        throw std::runtime_error("Invalid member array access in assignment");
    }

    // オブジェクト名を取得
    std::string obj_name;
    std::string array_member_name; // obj.array[idx].member の "array" 部分
    bool is_nested_struct_array_access = false;

    if (member_array_access->left &&
        (member_array_access->left->node_type == ASTNodeType::AST_VARIABLE ||
         member_array_access->left->node_type == ASTNodeType::AST_IDENTIFIER)) {
        obj_name = member_array_access->left->name;
    } else if (member_array_access->left &&
               member_array_access->left->node_type ==
                   ASTNodeType::AST_MEMBER_ACCESS) {
        // 2つのケースをチェック:
        // 1. s.grades[0] = 85 (構造体メンバーの配列へのアクセス)
        // 2. triangle.points[0].x = 1
        // (構造体配列メンバーの要素へのメンバーアクセス)

        if (member_array_access->left->left &&
            (member_array_access->left->left->node_type ==
                 ASTNodeType::AST_VARIABLE ||
             member_array_access->left->left->node_type ==
                 ASTNodeType::AST_IDENTIFIER)) {
            obj_name = member_array_access->left->left->name;
            array_member_name = member_array_access->left->name;

            // member_array_access->name が設定されている場合、これは
            // obj.array[idx].member のパターン
            if (!member_array_access->name.empty() &&
                member_array_access->name != array_member_name) {
                is_nested_struct_array_access = true;
                debug_print("DEBUG: Detected nested struct array member "
                            "access: %s.%s[idx].%s\n",
                            obj_name.c_str(), array_member_name.c_str(),
                            member_array_access->name.c_str());
            }
        } else {
            debug_print("ERROR: Nested "
                        "member_array_access->left->left->node_type = %d\n",
                        member_array_access->left->left
                            ? static_cast<int>(
                                  member_array_access->left->left->node_type)
                            : -1);
            throw std::runtime_error(
                "Invalid nested object reference in member array access");
        }
    } else {
        if (member_array_access->left) {
            debug_print("ERROR: member_array_access->left->node_type = %d\n",
                        static_cast<int>(member_array_access->left->node_type));
        } else {
            debug_print("ERROR: member_array_access->left is null\n");
        }
        throw std::runtime_error(
            "Invalid object reference in member array access");
    }

    // メンバ名を取得
    std::string member_name;
    if (is_nested_struct_array_access) {
        // triangle.points[0].x = 1 の場合
        member_name = member_array_access->name; // "x"
    } else if (member_array_access->left &&
               member_array_access->left->node_type ==
                   ASTNodeType::AST_MEMBER_ACCESS) {
        // ネストされた場合: s.grades[0] の "grades" 部分
        member_name = member_array_access->left->name;
    } else {
        // 直接の場合
        member_name = member_array_access->name;
    }

    debug_print("DEBUG: obj_name='%s', member_name='%s', array_member='%s', "
                "is_nested=%d\n",
                obj_name.c_str(), member_name.c_str(),
                array_member_name.c_str(), is_nested_struct_array_access);

    // インデックス値を評価（多次元対応）
    std::vector<int64_t> indices;
    if (member_array_access->right) {
        // 1次元の場合（従来通り）
        int64_t index = interpreter_.evaluate(member_array_access->right.get());
        indices.push_back(index);
    } else if (!member_array_access->arguments.empty()) {
        // 多次元の場合
        for (const auto &arg : member_array_access->arguments) {
            int64_t index = interpreter_.evaluate(arg.get());
            indices.push_back(index);
        }
    } else {
        throw std::runtime_error(
            "No indices found for array access in member array assignment");
    }

    // ネストされた構造体配列メンバーアクセスの処理: obj.array[idx].member =
    // value
    if (is_nested_struct_array_access) {
        debug_print(
            "DEBUG: Processing nested struct array member assignment\n");
        int array_index = static_cast<int>(indices[0]);

        // array_member_name の配列から要素を取得
        Variable *array_member =
            interpreter_.get_struct_member(obj_name, array_member_name);
        if (!array_member) {
            throw std::runtime_error("Struct member not found: " +
                                     array_member_name);
        }

        if (!array_member->is_array) {
            throw std::runtime_error("Member is not an array: " +
                                     array_member_name);
        }

        // 配列インデックスの境界チェック
        if (array_index < 0 || array_index >= array_member->array_size) {
            throw std::runtime_error("Array index out of bounds: " +
                                     std::to_string(array_index));
        }

        // struct_members内で配列要素にアクセス
        // 構造体配列の要素は "array_member_name[index]"
        // という名前で個別に格納されている まず array_member の struct_members
        // を調べる
        std::string element_key =
            array_member_name + "[" + std::to_string(array_index) + "]";

        debug_print("DEBUG: Looking for struct array element: %s\n",
                    element_key.c_str());

        // 親構造体から配列要素を探す（最初に親のstruct_membersを確認）
        Variable *parent_struct = interpreter_.find_variable(obj_name);
        if (!parent_struct || !parent_struct->is_struct) {
            throw std::runtime_error("Parent variable is not a struct: " +
                                     obj_name);
        }

        // 配列要素の構造体を探す - まず親のstruct_membersから
        auto element_it = parent_struct->struct_members.find(element_key);
        if (element_it == parent_struct->struct_members.end()) {
            // 配列メンバー自体のstruct_membersから探す
            element_it = array_member->struct_members.find(element_key);
            if (element_it == array_member->struct_members.end()) {
                debug_print(
                    "DEBUG: Available keys in parent struct_members:\n");
                for (const auto &pair : parent_struct->struct_members) {
                    debug_print("  - %s\n", pair.first.c_str());
                }
                debug_print(
                    "DEBUG: Available keys in array_member struct_members:\n");
                for (const auto &pair : array_member->struct_members) {
                    debug_print("  - %s\n", pair.first.c_str());
                }
                throw std::runtime_error("Struct array element not found: " +
                                         element_key);
            }
        }

        Variable &struct_element = element_it->second;
        debug_print("DEBUG: Found struct array element, is_struct=%s, "
                    "struct_members.size()=%zu\n",
                    struct_element.is_struct ? "true" : "false",
                    struct_element.struct_members.size());
        if (!struct_element.is_struct) {
            throw std::runtime_error("Array element is not a struct");
        }

        // 構造体要素のメンバーに値を代入
        auto member_it = struct_element.struct_members.find(member_name);
        if (member_it == struct_element.struct_members.end()) {
            throw std::runtime_error(
                "Struct member not found in array element: " + member_name);
        }

        // 右辺の値を評価して代入
        if (node->right->node_type == ASTNodeType::AST_STRING_LITERAL) {
            member_it->second.str_value = node->right->str_value;
            member_it->second.type = TYPE_STRING;
            debug_print("DEBUG_ASSIGN: Assigned string '%s' to %s.%s[%d].%s\n",
                        node->right->str_value.c_str(), obj_name.c_str(),
                        array_member_name.c_str(), array_index,
                        member_name.c_str());
        } else {
            TypedValue typed_value =
                interpreter_.evaluate_typed(node->right.get());
            if (typed_value.is_floating()) {
                member_it->second.double_value = typed_value.as_double();
                member_it->second.type = typed_value.type.type_info;
                debug_print(
                    "DEBUG_ASSIGN: Assigned double %f to %s.%s[%d].%s\n",
                    typed_value.as_double(), obj_name.c_str(),
                    array_member_name.c_str(), array_index,
                    member_name.c_str());
            } else {
                int64_t value = typed_value.as_numeric();
                member_it->second.value = value;
                member_it->second.type = typed_value.type.type_info;
                debug_print(
                    "DEBUG_ASSIGN: Assigned integer %lld to %s.%s[%d].%s\n",
                    value, obj_name.c_str(), array_member_name.c_str(),
                    array_index, member_name.c_str());
            }
        }
        member_it->second.is_assigned = true;

        // ダイレクトアクセス変数も更新
        std::string direct_access_name =
            obj_name + "." + element_key + "." + member_name;
        Variable *direct_var = interpreter_.find_variable(direct_access_name);
        if (direct_var) {
            if (member_it->second.type == TYPE_STRING) {
                direct_var->str_value = member_it->second.str_value;
            } else if (member_it->second.type == TYPE_FLOAT ||
                       member_it->second.type == TYPE_DOUBLE) {
                direct_var->double_value = member_it->second.double_value;
            } else {
                direct_var->value = member_it->second.value;
            }
            direct_var->type = member_it->second.type;
            direct_var->is_assigned = true;
            debug_print(
                "DEBUG_ASSIGN: Updated direct access variable: %s = %lld\n",
                direct_access_name.c_str(), direct_var->value);
        }

        // 構造体配列要素変数自体の struct_members も更新
        std::string element_var_name = obj_name + "." + element_key;
        Variable *element_variable =
            interpreter_.find_variable(element_var_name);
        if (element_variable && element_variable->is_struct) {
            auto elem_member_it =
                element_variable->struct_members.find(member_name);
            if (elem_member_it != element_variable->struct_members.end()) {
                elem_member_it->second = member_it->second;
                debug_print("DEBUG_ASSIGN: Updated element variable "
                            "struct_members: %s.%s = %lld\n",
                            element_var_name.c_str(), member_name.c_str(),
                            elem_member_it->second.value);
            }
        }

        debug_print(
            "DEBUG: Nested struct array member assigned: %s.%s[%d].%s\n",
            obj_name.c_str(), array_member_name.c_str(), array_index,
            member_name.c_str());
        return;
    }

    // 右辺の値を評価して構造体メンバー配列要素に代入
    debug_print("DEBUG: execute_member_array_assignment - right type=%d, "
                "indices count=%zu\n",
                static_cast<int>(node->right->node_type), indices.size());

    if (indices.size() > 1) {
        // 多次元配列の場合
        Variable *member_var =
            interpreter_.get_struct_member(obj_name, member_name);
        if (!member_var) {
            throw std::runtime_error("Struct member not found: " + member_name);
        }

        if (node->right->node_type == ASTNodeType::AST_STRING_LITERAL) {
            interpreter_.setMultidimensionalStringArrayElement(
                *member_var, indices, node->right->str_value);
        } else {
            int64_t value = interpreter_.evaluate(node->right.get());
            interpreter_.setMultidimensionalArrayElement(*member_var, indices,
                                                         value);
        }
        return; // 多次元処理完了
    }

    // 1次元配列の場合（従来処理）
    int index = static_cast<int>(indices[0]);
    if (node->right->node_type == ASTNodeType::AST_STRING_LITERAL) {
        interpreter_.assign_struct_member_array_element(
            obj_name, member_name, index, node->right->str_value);
    } else if (node->right->node_type == ASTNodeType::AST_ARRAY_REF) {
        // 構造体メンバ配列アクセスがAST_ARRAY_REFとして解析される場合
        debug_print("DEBUG: Processing AST_ARRAY_REF on right-hand side in "
                    "array assignment\n");
        if (node->right->left &&
            node->right->left->node_type == ASTNodeType::AST_MEMBER_ACCESS) {
            // original.tags[0] の形式
            std::string right_obj_name = node->right->left->left->name;
            std::string right_member_name = node->right->left->name;
            int64_t array_index =
                interpreter_.evaluate(node->right->array_index.get());

            Variable *right_member_var = interpreter_.get_struct_member(
                right_obj_name, right_member_name);
            debug_print(
                "DEBUG: AST_ARRAY_REF right_member_var type=%d, is_array=%d\n",
                static_cast<int>(right_member_var->type),
                right_member_var->is_array ? 1 : 0);
            if ((right_member_var->type == TYPE_STRING &&
                 right_member_var->is_array) ||
                right_member_var->type ==
                    static_cast<TypeInfo>(TYPE_ARRAY_BASE + TYPE_STRING)) {
                debug_print("DEBUG: Using string array element access via "
                            "AST_ARRAY_REF\n");
                std::string str_value =
                    interpreter_.get_struct_member_array_string_element(
                        right_obj_name, right_member_name,
                        static_cast<int>(array_index));
                interpreter_.assign_struct_member_array_element(
                    obj_name, member_name, index, str_value);
            } else {
                debug_print("DEBUG: Using numeric array element access via "
                            "AST_ARRAY_REF\n");
                int64_t value = interpreter_.get_struct_member_array_element(
                    right_obj_name, right_member_name,
                    static_cast<int>(array_index));
                interpreter_.assign_struct_member_array_element(
                    obj_name, member_name, index, value);
            }
        } else {
            // 通常の配列参照として処理
            int64_t value = interpreter_.evaluate(node->right.get());
            interpreter_.assign_struct_member_array_element(
                obj_name, member_name, index, value);
        }
    } else if (node->right->node_type == ASTNodeType::AST_MEMBER_ARRAY_ACCESS) {
        // 構造体メンバ配列アクセスの場合（original.tags[0]等）
        debug_print("DEBUG: Processing AST_MEMBER_ARRAY_ACCESS on right-hand "
                    "side in array assignment\n");
        std::string right_obj_name;
        std::string right_member_name = node->right->name;

        if (node->right->left->node_type == ASTNodeType::AST_VARIABLE) {
            right_obj_name = node->right->left->name;
        } else if (node->right->left->node_type == ASTNodeType::AST_ARRAY_REF) {
            // struct配列要素の場合
            std::string array_name = node->right->left->left->name;
            int64_t idx =
                interpreter_.evaluate(node->right->left->array_index.get());
            right_obj_name = array_name + "[" + std::to_string(idx) + "]";
        } else {
            throw std::runtime_error("Invalid right-hand member array access");
        }

        // インデックスを評価
        int64_t array_index = interpreter_.evaluate(node->right->right.get());

        // 右辺の構造体メンバ配列要素を取得
        Variable *right_member_var =
            interpreter_.get_struct_member(right_obj_name, right_member_name);
        debug_print("DEBUG: right_member_var type=%d, is_array=%d in array "
                    "assignment\n",
                    static_cast<int>(right_member_var->type),
                    right_member_var->is_array ? 1 : 0);
        if ((right_member_var->type == TYPE_STRING &&
             right_member_var->is_array) ||
            right_member_var->type ==
                static_cast<TypeInfo>(TYPE_ARRAY_BASE + TYPE_STRING)) {
            debug_print("DEBUG: Using string array element access in array "
                        "assignment\n");
            std::string str_value =
                interpreter_.get_struct_member_array_string_element(
                    right_obj_name, right_member_name,
                    static_cast<int>(array_index));
            interpreter_.assign_struct_member_array_element(
                obj_name, member_name, index, str_value);
        } else {
            debug_print("DEBUG: Using numeric array element access in array "
                        "assignment\n");
            int64_t value = interpreter_.get_struct_member_array_element(
                right_obj_name, right_member_name,
                static_cast<int>(array_index));
            interpreter_.assign_struct_member_array_element(
                obj_name, member_name, index, value);
        }
    } else {
        int64_t value = interpreter_.evaluate(node->right.get());
        interpreter_.assign_struct_member_array_element(obj_name, member_name,
                                                        index, value);
    }
}

void StatementExecutor::execute_member_assignment(const ASTNode *node) {
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
            execute_self_member_assignment(member_access->name,
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
            interpreter_.evaluate(member_access->left->left.get());

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
                interpreter_.evaluate_typed(node->right.get());
            new_value.value = typed_value.as_numeric();
            new_value.type = typed_value.type.type_info;
        }
        new_value.is_assigned = true;

        // struct_membersに代入
        struct_var->struct_members[member_name] = new_value;

        // 個別変数システムとの同期
        interpreter_.sync_individual_member_from_struct(struct_var,
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
            Variable *root_var = interpreter_.find_variable(root_var_name);
            if (root_var && root_var->is_const) {
                throw std::runtime_error(
                    "Cannot assign to member of const struct: " +
                    root_var_name);
            }
        }

        // ネストメンバアクセスを評価して対象の構造体変数を取得
        Variable *target_struct =
            evaluate_nested_member_access(member_access->left.get());

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
                interpreter_.find_variable(full_member_path);
            if (individual_var) {
                if (node->right->node_type == ASTNodeType::AST_STRING_LITERAL) {
                    individual_var->str_value = node->right->str_value;
                    individual_var->type = TYPE_STRING;
                } else {
                    TypedValue typed_value =
                        interpreter_.evaluate_typed(node->right.get());
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
                interpreter_.evaluate_typed(node->right.get());
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

        int64_t index = interpreter_.evaluate(array_ref->array_index.get());
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
            ptr_value = interpreter_.evaluate(deref_target);

            if (debug_mode) {
                debug_print("DEBUG: Nested member access evaluated to "
                            "ptr_value=%lld (0x%llx)\n",
                            ptr_value, static_cast<uint64_t>(ptr_value));
            }
        } else {
            // 単純な変数: (*ptr).member の場合
            ptr_value = interpreter_.evaluate(deref_target);

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
                interpreter_.evaluate_typed(node->right.get());
            new_value.value = typed_value.as_numeric();
            new_value.type = typed_value.type.type_info;
        }
        new_value.is_assigned = true;

        // struct_membersに代入
        struct_var->struct_members[member_name] = new_value;

        // 個別変数システムとの同期
        interpreter_.sync_individual_member_from_struct(struct_var,
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
    Variable *target_var = interpreter_.find_variable(obj_name);

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
        if (Variable *root_var = interpreter_.find_variable(root_obj_name)) {
            if (root_var->is_const) {
                throw std::runtime_error(
                    "Cannot assign to member of const struct: " + obj_name +
                    "." + member_name);
            }
        }
    }

    // 参照の場合は、実際の変数のメンバーに直接代入
    Variable *base_var = interpreter_.find_variable(obj_name);
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
                interpreter_.find_variable_name_by_address(actual_var);
            if (!actual_var_name.empty()) {
                std::string direct_var_name =
                    actual_var_name + "." + member_name;
                Variable *direct_var =
                    interpreter_.find_variable(direct_var_name);
                if (direct_var) {
                    direct_var->str_value = node->right->str_value;
                    direct_var->type = TYPE_STRING;
                    direct_var->is_assigned = true;
                }
            }
        } else {
            TypedValue typed_value =
                interpreter_.evaluate_typed(node->right.get());
            member_var.value = typed_value.value;
            member_var.type = typed_value.numeric_type;
            member_var.is_assigned = true;

            // ダイレクトアクセス変数も更新
            std::string actual_var_name =
                interpreter_.find_variable_name_by_address(actual_var);
            if (!actual_var_name.empty()) {
                std::string direct_var_name =
                    actual_var_name + "." + member_name;
                Variable *direct_var =
                    interpreter_.find_variable(direct_var_name);
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
        interpreter_.assign_struct_member(obj_name, member_name,
                                          node->right->str_value);
    } else if (node->right->node_type == ASTNodeType::AST_VARIABLE) {
        // 変数参照の場合、構造体変数か数値/文字列変数か判断
        Variable *right_var = interpreter_.find_variable(node->right->name);

        if (!right_var) {
            throw std::runtime_error("Right-hand variable not found: " +
                                     node->right->name);
        }

        // 構造体変数の場合、ReturnExceptionをキャッチして構造体を代入
        if (right_var->type == TYPE_STRUCT) {
            try {
                // 構造体変数を評価（ReturnExceptionが投げられる）
                interpreter_.evaluate(node->right.get());
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
                    interpreter_.assign_struct_member_struct(
                        obj_name, member_name, ret_ex.struct_value);
                } else {
                    throw std::runtime_error("Variable is not a struct for "
                                             "struct member assignment");
                }
            }
        } else if (right_var->type == TYPE_STRING) {
            interpreter_.assign_struct_member(obj_name, member_name,
                                              right_var->str_value);
        } else {
            // TypedValueを使用して型情報を保持
            TypedValue typed_value =
                interpreter_.evaluate_typed(node->right.get());
            interpreter_.assign_struct_member(obj_name, member_name,
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
                interpreter_.evaluate(node->right->left->array_index.get());
            right_obj_name = array_name + "[" + std::to_string(index) + "]";
        } else {
            throw std::runtime_error("Invalid right-hand member access");
        }

        // 右辺の構造体メンバを取得
        Variable *right_member_var =
            interpreter_.get_struct_member(right_obj_name, right_member_name);
        if (right_member_var->type == TYPE_STRING) {
            interpreter_.assign_struct_member(obj_name, member_name,
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
                interpreter_.assign_struct_member(obj_name, member_name,
                                                  typed_value);
            } else if (right_member_var->type == TYPE_DOUBLE) {
                TypedValue typed_value(right_member_var->double_value,
                                       inferred);
                typed_value.numeric_type = TYPE_DOUBLE;
                interpreter_.assign_struct_member(obj_name, member_name,
                                                  typed_value);
            } else {
                TypedValue typed_value(right_member_var->quad_value, inferred);
                typed_value.numeric_type = TYPE_QUAD;
                interpreter_.assign_struct_member(obj_name, member_name,
                                                  typed_value);
            }
        } else {
            interpreter_.assign_struct_member(obj_name, member_name,
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
                interpreter_.evaluate(node->right->left->array_index.get());
            right_obj_name = array_name + "[" + std::to_string(index) + "]";
        } else {
            throw std::runtime_error("Invalid right-hand member array access");
        }

        // インデックスを評価
        int64_t array_index = interpreter_.evaluate(node->right->right.get());

        // 右辺の構造体メンバ配列要素を取得
        Variable *right_member_var =
            interpreter_.get_struct_member(right_obj_name, right_member_name);
        debug_print("DEBUG: right_member_var type=%d, is_array=%d\n",
                    static_cast<int>(right_member_var->type),
                    right_member_var->is_array ? 1 : 0);
        if ((right_member_var->type == TYPE_STRING &&
             right_member_var->is_array) ||
            right_member_var->type ==
                static_cast<TypeInfo>(TYPE_ARRAY_BASE + TYPE_STRING)) {
            debug_print("DEBUG: Using string array element access\n");
            std::string str_value =
                interpreter_.get_struct_member_array_string_element(
                    right_obj_name, right_member_name,
                    static_cast<int>(array_index));
            interpreter_.assign_struct_member(obj_name, member_name, str_value);
        } else {
            debug_print("DEBUG: Using numeric array element access\n");
            int64_t value = interpreter_.get_struct_member_array_element(
                right_obj_name, right_member_name,
                static_cast<int>(array_index));
            interpreter_.assign_struct_member(obj_name, member_name, value);
        }
    } else {
        // TypedValueを使用して型情報を保持
        TypedValue typed_value = interpreter_.evaluate_typed(node->right.get());
        interpreter_.assign_struct_member(obj_name, member_name, typed_value);
    }
}

void StatementExecutor::execute_arrow_assignment(const ASTNode *node) {
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
    int64_t ptr_value = interpreter_.evaluate(arrow_access->left.get());

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
        TypedValue typed_value = interpreter_.evaluate_typed(node->right.get());
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
    interpreter_.sync_individual_member_from_struct(struct_var, member_name);

    if (debug_mode) {
        debug_print("DEBUG: execute_arrow_assignment - completed\n");
    }
}

void StatementExecutor::execute_member_array_literal_assignment(
    const ASTNode *node) {
    // obj.member = [1, 2, 3] または array[index].member = [1, 2, 3] の処理
    const ASTNode *member_access = node->left.get();

    if (!member_access ||
        member_access->node_type != ASTNodeType::AST_MEMBER_ACCESS) {
        throw std::runtime_error(
            "Invalid member access in array literal assignment");
    }

    // オブジェクト名を取得
    std::string obj_name;
    if (member_access->left &&
        member_access->left->node_type == ASTNodeType::AST_VARIABLE) {
        // 通常の構造体変数: obj.member
        obj_name = member_access->left->name;
    } else if (member_access->left &&
               member_access->left->node_type == ASTNodeType::AST_ARRAY_REF) {
        // 構造体配列要素: array[index].member
        std::string array_name = member_access->left->left->name;
        int64_t index =
            interpreter_.evaluate(member_access->left->array_index.get());
        obj_name = array_name + "[" + std::to_string(index) + "]";
    } else {
        throw std::runtime_error(
            "Invalid object reference in member array literal assignment");
    }

    // メンバ名を取得
    std::string member_name = member_access->name;

    if (debug_mode) {
        std::cerr << "DEBUG: Member array literal assignment: " << obj_name
                  << "." << member_name << std::endl;
    }

    // 構造体メンバー配列への配列リテラル代入
    interpreter_.assign_struct_member_array_literal(obj_name, member_name,
                                                    node->right.get());
}

void StatementExecutor::execute_union_assignment(const std::string &var_name,
                                                 const ASTNode *value_node) {
    // union型変数への代入を実行
    auto &var = interpreter_.current_scope().variables[var_name];

    if (var.type != TYPE_UNION) {
        throw std::runtime_error("Variable is not a union type: " + var_name);
    }

    std::string union_type_name = var.type_name;

    // 値の型に応じて検証と代入を実行
    if (value_node->node_type == ASTNodeType::AST_STRING_LITERAL) {
        // 文字列値
        std::string str_value = value_node->str_value;
        if (interpreter_.get_type_manager()->is_value_allowed_for_union(
                union_type_name, str_value)) {
            var.str_value = str_value;
            var.current_type = TYPE_STRING;
        } else {
            throw std::runtime_error("String value '" + str_value +
                                     "' is not allowed for union type " +
                                     union_type_name);
        }
    } else if (value_node->node_type == ASTNodeType::AST_NUMBER) {
        // 数値
        int64_t int_value = value_node->int_value;
        if (interpreter_.get_type_manager()->is_value_allowed_for_union(
                union_type_name, int_value)) {
            var.value = int_value;
            var.current_type = TYPE_INT;
        } else {
            throw std::runtime_error(
                "Integer value " + std::to_string(int_value) +
                " is not allowed for union type " + union_type_name);
        }
    } else {
        // 式の評価
        try {
            // まず文字列として評価してみる
            if (value_node->node_type == ASTNodeType::AST_VARIABLE) {
                // 変数参照の場合、変数の値を取得
                auto &source_var =
                    interpreter_.current_scope().variables[value_node->name];
                if (source_var.current_type == TYPE_STRING) {
                    if (interpreter_.get_type_manager()
                            ->is_value_allowed_for_union(
                                union_type_name, source_var.str_value)) {
                        var.str_value = source_var.str_value;
                        var.current_type = TYPE_STRING;
                        return;
                    }
                } else {
                    int64_t int_value = source_var.value;
                    if (interpreter_.get_type_manager()
                            ->is_value_allowed_for_union(union_type_name,
                                                         int_value)) {
                        var.value = int_value;
                        var.current_type = TYPE_INT;
                        return;
                    }
                }
            }

            // 数値として評価
            int64_t int_value = interpreter_.evaluate(value_node);
            if (interpreter_.get_type_manager()->is_value_allowed_for_union(
                    union_type_name, int_value)) {
                var.value = int_value;
                var.current_type = TYPE_INT;
            } else {
                throw std::runtime_error("Value " + std::to_string(int_value) +
                                         " is not allowed for union type " +
                                         union_type_name);
            }
        } catch (const std::exception &e) {
            throw std::runtime_error(
                "Failed to assign value to union variable " + var_name + ": " +
                e.what());
        }
    }
}

void StatementExecutor::execute_self_member_assignment(
    const std::string &member_name, const ASTNode *value_node) {
    debug_msg(DebugMsgId::SELF_MEMBER_ACCESS_START, member_name.c_str());

    // selfメンバーへのパスを構築
    std::string self_member_path = "self." + member_name;

    // selfメンバー変数を検索
    Variable *self_member = interpreter_.find_variable(self_member_path);
    if (!self_member) {
        throw std::runtime_error("Self member not found: " + member_name);
    }

    // constメンバへの代入チェック
    if (self_member->is_const && self_member->is_assigned) {
        error_msg(DebugMsgId::CONST_REASSIGN_ERROR, self_member_path.c_str());
        throw std::runtime_error("Cannot assign to const self member: " +
                                 member_name);
    }

    debug_msg(DebugMsgId::SELF_MEMBER_ACCESS_FOUND, member_name.c_str());

    // 元のレシーバー変数からselfメンバーのパスを取得
    Variable *self_var = interpreter_.find_variable("self");
    Variable *receiver_info = interpreter_.find_variable("__self_receiver__");
    std::string original_receiver_path;

    if (self_var && receiver_info && !receiver_info->str_value.empty()) {
        original_receiver_path = receiver_info->str_value + "." + member_name;
        debug_print("SELF_ASSIGN_DEBUG: Original receiver path: %s\n",
                    original_receiver_path.c_str());
    }

    // 値の型に応じて代入処理
    if (value_node->node_type == ASTNodeType::AST_STRING_LITERAL) {
        self_member->str_value = value_node->str_value;
        self_member->type = TYPE_STRING;
        self_member->is_assigned = true;

        // 元の変数のメンバーも同時に更新
        if (!original_receiver_path.empty()) {
            debug_print("SELF_ASSIGN_DEBUG: Looking for original member: %s\n",
                        original_receiver_path.c_str());
            Variable *original_member =
                interpreter_.find_variable(original_receiver_path);
            if (original_member) {
                debug_print("SELF_ASSIGN_DEBUG: Found original member, "
                            "updating string value\n");
                original_member->str_value = value_node->str_value;
                original_member->type = TYPE_STRING;
                original_member->is_assigned = true;
                debug_print("SELF_ASSIGN_SYNC: %s = \"%s\"\n",
                            original_receiver_path.c_str(),
                            value_node->str_value.c_str());
            } else {
                debug_print(
                    "SELF_ASSIGN_DEBUG: Could not find original member: %s\n",
                    original_receiver_path.c_str());
            }
        }

        debug_print("SELF_ASSIGN: %s = \"%s\"\n", member_name.c_str(),
                    value_node->str_value.c_str());
    } else if (value_node->node_type == ASTNodeType::AST_VARIABLE) {
        // 変数参照の場合
        Variable *source_var = interpreter_.find_variable(value_node->name);
        if (source_var && source_var->type == TYPE_STRING) {
            self_member->str_value = source_var->str_value;
            self_member->type = TYPE_STRING;

            // 元の変数のメンバーも同時に更新
            if (!original_receiver_path.empty()) {
                debug_print(
                    "SELF_ASSIGN_DEBUG: Looking for original member: %s\n",
                    original_receiver_path.c_str());
                Variable *original_member =
                    interpreter_.find_variable(original_receiver_path);
                if (original_member) {
                    debug_print("SELF_ASSIGN_DEBUG: Found original member, "
                                "updating string value from variable\n");
                    original_member->str_value = source_var->str_value;
                    original_member->type = TYPE_STRING;
                    original_member->is_assigned = true;
                    debug_print(
                        "SELF_ASSIGN_SYNC: %s = \"%s\" (from variable)\n",
                        original_receiver_path.c_str(),
                        source_var->str_value.c_str());
                } else {
                    debug_print("SELF_ASSIGN_DEBUG: Could not find original "
                                "member: %s\n",
                                original_receiver_path.c_str());
                }
            }

            debug_print("SELF_ASSIGN: %s = \"%s\" (from variable)\n",
                        member_name.c_str(), source_var->str_value.c_str());
        } else {
            int64_t value = interpreter_.evaluate(value_node);
            self_member->value = value;
            if (self_member->type != TYPE_STRING) {
                self_member->type = TYPE_INT; // デフォルトはint型
            }

            // 元の変数のメンバーも同時に更新
            if (!original_receiver_path.empty()) {
                debug_print(
                    "SELF_ASSIGN_DEBUG: Looking for original member: %s\n",
                    original_receiver_path.c_str());
                Variable *original_member =
                    interpreter_.find_variable(original_receiver_path);
                if (original_member) {
                    debug_print("SELF_ASSIGN_DEBUG: Found original member, "
                                "updating numeric value from variable\n");
                    original_member->value = value;
                    if (original_member->type != TYPE_STRING) {
                        original_member->type = TYPE_INT;
                    }
                    original_member->is_assigned = true;
                    debug_print("SELF_ASSIGN_SYNC: %s = %lld (from variable)\n",
                                original_receiver_path.c_str(),
                                (long long)value);
                } else {
                    debug_print("SELF_ASSIGN_DEBUG: Could not find original "
                                "member: %s\n",
                                original_receiver_path.c_str());
                }
            }

            debug_print("SELF_ASSIGN: %s = %lld (from variable)\n",
                        member_name.c_str(), (long long)value);
        }
        self_member->is_assigned = true;
    } else {
        // 式の評価
        int64_t value = interpreter_.evaluate(value_node);

        // 複合代入演算子の処理
        if (value_node->node_type == ASTNodeType::AST_BINARY_OP) {
            // += -= *= /= などの複合代入かチェック
            if (value_node->name == "+=" || value_node->name == "-=" ||
                value_node->name == "*=" || value_node->name == "/=") {
                // 複合代入は既に評価済みの値として処理
                debug_print("SELF_COMPOUND_ASSIGN: %s %s= %lld\n",
                            member_name.c_str(), value_node->name.c_str(),
                            (long long)value);
            }
        }

        self_member->value = value;
        if (self_member->type != TYPE_STRING) {
            self_member->type = TYPE_INT;
        }
        self_member->is_assigned = true;

        // 元の変数のメンバーも同時に更新
        if (!original_receiver_path.empty()) {
            debug_print("SELF_ASSIGN_DEBUG: Looking for original member: %s\n",
                        original_receiver_path.c_str());
            Variable *original_member =
                interpreter_.find_variable(original_receiver_path);
            if (original_member) {
                debug_print("SELF_ASSIGN_DEBUG: Found original member, "
                            "updating numeric value\n");
                original_member->value = value;
                if (original_member->type != TYPE_STRING) {
                    original_member->type = TYPE_INT;
                }
                original_member->is_assigned = true;
                debug_print("SELF_ASSIGN_SYNC: %s = %lld\n",
                            original_receiver_path.c_str(), (long long)value);
            } else {
                debug_print(
                    "SELF_ASSIGN_DEBUG: Could not find original member: %s\n",
                    original_receiver_path.c_str());
            }
        }

        debug_print("SELF_ASSIGN: %s = %lld\n", member_name.c_str(),
                    (long long)value);
    }

    std::string self_value_str = std::to_string(self_member->value);
    debug_msg(DebugMsgId::SELF_MEMBER_ACCESS_VALUE, self_value_str.c_str());
}

void StatementExecutor::execute_ternary_assignment(const ASTNode *node) {
    // 三項演算子の条件を評価
    int64_t condition = interpreter_.evaluate(node->right->left.get());

    // 条件に基づいて選択される分岐を決定
    const ASTNode *selected_branch =
        condition ? node->right->right.get() : node->right->third.get();

    // 選択された分岐の型に基づいて処理を分岐
    if (selected_branch->node_type == ASTNodeType::AST_ARRAY_LITERAL) {
        // 配列リテラルの代入
        if (!node->name.empty()) {
            interpreter_.assign_array_literal(node->name, selected_branch);
            return;
        }
    } else if (selected_branch->node_type == ASTNodeType::AST_STRUCT_LITERAL) {
        // 構造体リテラルの代入
        if (!node->name.empty()) {
            interpreter_.assign_struct_literal(node->name, selected_branch);
            return;
        }
    } else if (selected_branch->node_type == ASTNodeType::AST_STRING_LITERAL) {
        // 文字列リテラルの代入
        if (!node->name.empty()) {
            Variable *var = interpreter_.get_variable(node->name);
            if (var) {
                var->str_value = selected_branch->str_value;
                var->type = TYPE_STRING;
                var->is_assigned = true;
            }
        }
        return;
    } else {
        // その他（数値、関数呼び出しなど）の場合は通常の評価
        try {
            TypedValue typed_value =
                interpreter_.evaluate_typed_expression(selected_branch);
            interpreter_.assign_variable(node->name, typed_value,
                                         typed_value.type.type_info, false);
        } catch (const ReturnException &ret) {
            if (!node->name.empty()) {
                if (ret.type == TYPE_STRING) {
                    TypedValue typed_value(ret.str_value,
                                           InferredType(TYPE_STRING, "string"));
                    interpreter_.assign_variable(node->name, typed_value,
                                                 TYPE_STRING, false);
                } else if (ret.type == TYPE_FLOAT || ret.type == TYPE_DOUBLE ||
                           ret.type == TYPE_QUAD) {
                    TypeInfo numeric_type = ret.type;
                    long double quad_value =
                        (ret.type == TYPE_FLOAT)
                            ? static_cast<long double>(ret.double_value)
                        : (ret.type == TYPE_DOUBLE)
                            ? static_cast<long double>(ret.double_value)
                            : ret.quad_value;
                    TypedValue typed_value(
                        quad_value,
                        InferredType(numeric_type,
                                     type_info_to_string(numeric_type)));
                    interpreter_.assign_variable(node->name, typed_value,
                                                 numeric_type, false);
                } else if (ret.is_struct) {
                    Variable struct_var = ret.struct_value;
                    TypedValue typed_value(
                        struct_var,
                        InferredType(TYPE_STRUCT, struct_var.struct_type_name));
                    interpreter_.assign_variable(node->name, typed_value,
                                                 TYPE_STRUCT, false);
                } else {
                    TypedValue typed_value(
                        ret.value,
                        InferredType(ret.type, type_info_to_string(ret.type)));
                    interpreter_.assign_variable(node->name, typed_value,
                                                 ret.type, false);
                }
            }
        }
    }
}

void StatementExecutor::execute_ternary_variable_initialization(
    const ASTNode *var_decl_node, const ASTNode *ternary_node) {
    printf("DEBUG: execute_ternary_variable_initialization called\n");

    // 三項演算子の条件を評価
    int64_t condition = interpreter_.evaluate(ternary_node->left.get());
    printf("DEBUG: Ternary condition = %lld\n", condition);

    // 条件に基づいて選択される分岐を決定
    const ASTNode *selected_branch =
        condition ? ternary_node->right.get() : ternary_node->third.get();
    printf("DEBUG: Selected branch node_type = %d\n",
           static_cast<int>(selected_branch->node_type));

    std::string var_name = var_decl_node->name;
    Variable *var = interpreter_.get_variable(var_name);

    if (!var) {
        throw std::runtime_error(
            "Variable not found during ternary initialization: " + var_name);
    }

    // 選択された分岐の型に基づいて処理
    if (selected_branch->node_type == ASTNodeType::AST_ARRAY_LITERAL) {
        // 配列リテラルの初期化
        interpreter_.assign_array_literal(var_name, selected_branch);
        var->is_assigned = true;
    } else if (selected_branch->node_type == ASTNodeType::AST_STRUCT_LITERAL) {
        // 構造体リテラルの初期化
        interpreter_.assign_struct_literal(var_name, selected_branch);
        var->is_assigned = true;
    } else if (selected_branch->node_type == ASTNodeType::AST_STRING_LITERAL) {
        // 文字列リテラルの初期化
        var->str_value = selected_branch->str_value;
        var->type = TYPE_STRING;
        var->is_assigned = true;
    } else {
        try {
            TypedValue typed_value =
                interpreter_.evaluate_typed_expression(selected_branch);
            interpreter_.assign_variable(var_name, typed_value,
                                         typed_value.type.type_info, false);
        } catch (const ReturnException &ret) {
            if (ret.type == TYPE_STRING) {
                TypedValue typed_value(ret.str_value,
                                       InferredType(TYPE_STRING, "string"));
                interpreter_.assign_variable(var_name, typed_value, TYPE_STRING,
                                             false);
            } else if (ret.type == TYPE_FLOAT || ret.type == TYPE_DOUBLE ||
                       ret.type == TYPE_QUAD) {
                TypeInfo numeric_type = ret.type;
                long double quad_value =
                    (ret.type == TYPE_FLOAT)
                        ? static_cast<long double>(ret.double_value)
                    : (ret.type == TYPE_DOUBLE)
                        ? static_cast<long double>(ret.double_value)
                        : ret.quad_value;
                TypedValue typed_value(
                    quad_value, InferredType(numeric_type, type_info_to_string(
                                                               numeric_type)));
                interpreter_.assign_variable(var_name, typed_value,
                                             numeric_type, false);
            } else if (ret.is_struct) {
                Variable struct_var = ret.struct_value;
                TypedValue typed_value(
                    struct_var,
                    InferredType(TYPE_STRUCT, struct_var.struct_type_name));
                interpreter_.assign_variable(var_name, typed_value, TYPE_STRUCT,
                                             false);
            } else {
                TypedValue typed_value(
                    ret.value,
                    InferredType(ret.type, type_info_to_string(ret.type)));
                interpreter_.assign_variable(var_name, typed_value, ret.type,
                                             false);
            }
        }
    }
}

Variable *StatementExecutor::evaluate_nested_member_access(
    const ASTNode *member_access_node) {
    // ネストメンバアクセス (obj.mid.data)
    // を再帰的に評価して、最終的なメンバを含む親構造体を返す
    if (!member_access_node ||
        member_access_node->node_type != ASTNodeType::AST_MEMBER_ACCESS) {
        return nullptr;
    }

    // 左側を取得
    if (!member_access_node->left) {
        return nullptr;
    }

    Variable *parent_struct = nullptr;

    if (member_access_node->left->node_type == ASTNodeType::AST_VARIABLE ||
        member_access_node->left->node_type == ASTNodeType::AST_IDENTIFIER) {
        // 基底オブジェクト: obj または self
        std::string obj_name = member_access_node->left->name;
        parent_struct = interpreter_.find_variable(obj_name);

        if (!parent_struct || parent_struct->type != TYPE_STRUCT) {
            throw std::runtime_error("Base object is not a struct: " +
                                     obj_name);
        }
    } else if (member_access_node->left->node_type ==
               ASTNodeType::AST_MEMBER_ACCESS) {
        // ネストメンバアクセス: obj.mid (さらに再帰)
        // 左側のメンバアクセスを評価して、その親構造体を取得
        Variable *intermediate_struct =
            evaluate_nested_member_access(member_access_node->left.get());
        if (!intermediate_struct) {
            return nullptr;
        }

        // 左側のメンバ名を取得
        std::string intermediate_member = member_access_node->left->name;

        // 親構造体から中間メンバを取得
        auto it = intermediate_struct->struct_members.find(intermediate_member);
        if (it == intermediate_struct->struct_members.end()) {
            throw std::runtime_error("Intermediate member not found: " +
                                     intermediate_member);
        }

        parent_struct = &it->second;
        if (parent_struct->type != TYPE_STRUCT) {
            throw std::runtime_error("Intermediate member is not a struct: " +
                                     intermediate_member);
        }
    } else {
        throw std::runtime_error(
            "Unsupported nested member access left node type");
    }

    return parent_struct;
}
