#pragma once

#include "../../../../common/ast.h"
#include "../../core/interpreter.h"
#include <functional>
#include <string>
#include <vector>

namespace AssignmentHelpers {

// ネストしたメンバーアクセスを再帰的に解決する
// 例: container.shapes[0].edges[0].start.x
// 戻り値: {親構造体のVariable*, 最終メンバー名}
inline std::pair<Variable *, std::string> resolve_nested_member_for_assignment(
    Interpreter &interpreter, const ASTNode *member_access_node,
    std::function<int64_t(const ASTNode *)> evaluate_index) {

    if (!member_access_node ||
        member_access_node->node_type != ASTNodeType::AST_MEMBER_ACCESS) {
        throw std::runtime_error("Invalid member access node");
    }

    // 最終メンバー名
    std::string final_member = member_access_node->name;

    // 左側がVARIABLEの場合: obj.member
    if (member_access_node->left->node_type == ASTNodeType::AST_VARIABLE ||
        member_access_node->left->node_type == ASTNodeType::AST_IDENTIFIER) {

        std::string obj_name = member_access_node->left->name;
        Variable *parent_var = interpreter.find_variable(obj_name);

        if (!parent_var) {
            throw std::runtime_error("Variable not found: " + obj_name);
        }

        return {parent_var, final_member};
    }

    // 左側がMEMBER_ACCESSの場合: obj.mid.member
    // 再帰的に左側を解決
    if (member_access_node->left->node_type == ASTNodeType::AST_MEMBER_ACCESS) {
        auto [grandparent_var, parent_member] =
            resolve_nested_member_for_assignment(
                interpreter, member_access_node->left.get(), evaluate_index);

        if (!grandparent_var || !grandparent_var->is_struct) {
            throw std::runtime_error("Grandparent is not a struct");
        }

        // 祖父母のstruct_membersから親メンバーを取得
        auto it = grandparent_var->struct_members.find(parent_member);
        if (it == grandparent_var->struct_members.end()) {
            // メンバーが見つからない場合、作成を試みる（構造体メンバーの場合）
            throw std::runtime_error(
                "Parent member not found: " + parent_member + " in struct");
        }

        Variable *parent_var = &it->second;

        // 親メンバーが構造体でない場合はエラー
        if (!parent_var->is_struct) {
            throw std::runtime_error("Parent member is not a struct: " +
                                     parent_member);
        }

        // 最終的に、親構造体から最終メンバーにアクセス
        return {parent_var, final_member};
    }

    // 左側がARRAY_REFの場合: obj.array[0].member または array[0].member
    if (member_access_node->left->node_type == ASTNodeType::AST_ARRAY_REF) {
        const ASTNode *array_ref = member_access_node->left.get();

        // 配列のインデックスを評価
        int64_t index = evaluate_index(array_ref->array_index.get());

        // 配列の左側を解決（再帰的）
        Variable *array_parent = nullptr;
        std::string array_member_name;

        if (array_ref->left->node_type == ASTNodeType::AST_MEMBER_ACCESS) {
            // obj.array[0] の場合
            auto [parent, member] = resolve_nested_member_for_assignment(
                interpreter, array_ref->left.get(), evaluate_index);

            if (!parent || !parent->is_struct) {
                throw std::runtime_error("Array parent is not a struct");
            }

            // struct_membersから配列メンバーを取得
            auto it = parent->struct_members.find(member);
            if (it == parent->struct_members.end()) {
                throw std::runtime_error("Array member not found: " + member);
            }

            array_parent = &it->second;
            array_member_name = member;

            debug_print("DEBUG_RESOLVER_EARLY: Found array member '%s', "
                        "is_struct=%d, is_array=%d, struct_type='%s'\n",
                        member.c_str(), array_parent->is_struct ? 1 : 0,
                        array_parent->is_array ? 1 : 0,
                        array_parent->struct_type_name.c_str());

        } else if (array_ref->left->node_type == ASTNodeType::AST_VARIABLE ||
                   array_ref->left->node_type == ASTNodeType::AST_IDENTIFIER) {
            // array[0] の場合
            std::string var_name = array_ref->left->name;
            array_parent = interpreter.find_variable(var_name);
            array_member_name = var_name;

        } else if (array_ref->left->node_type == ASTNodeType::AST_ARRAY_REF) {
            // array[0][1] のようなケース - 再帰的に処理
            // ここでは配列要素から構造体メンバーを取得する必要がある
            throw std::runtime_error(
                "Nested array access in member assignment not yet supported");
        } else {
            throw std::runtime_error("Unsupported array reference type");
        }

        if (!array_parent || !array_parent->is_array) {
            throw std::runtime_error("Not an array: " + array_member_name);
        }

        // 構造体配列の要素の場合
        // array_parentは配列メンバー自体（例: edges配列）
        // 構造体配列の要素を取得する

        // デバッグ情報
        debug_print("DEBUG_RESOLVER: array_member_name=%s, is_struct=%d, "
                    "is_array=%d, struct_type_name='%s'\n",
                    array_member_name.c_str(), array_parent->is_struct ? 1 : 0,
                    array_parent->is_array ? 1 : 0,
                    array_parent->struct_type_name.c_str());

        // まず、配列自体が構造体配列かチェック
        // struct_type_nameが設定されていれば構造体配列として扱う
        if (array_parent->struct_type_name.empty()) {
            // 非構造体配列へのメンバーアクセスはエラー
            std::string error_msg =
                "Cannot access member of non-struct array element: " +
                array_member_name;
            error_msg +=
                " (struct_type='" + array_parent->struct_type_name + "')";
            throw std::runtime_error(error_msg);
        }

        // 配列要素のキーを構築
        std::string element_key =
            array_member_name + "[" + std::to_string(index) + "]";

        // struct_membersから配列要素を取得
        auto elem_it = array_parent->struct_members.find(element_key);

        auto resolve_element = [&]() -> Variable * {
            if (elem_it != array_parent->struct_members.end()) {
                Variable *stored = &elem_it->second;
                if (stored->is_reference && stored->value != 0) {
                    return reinterpret_cast<Variable *>(stored->value);
                }

                if (stored->struct_members.empty()) {
                    if (Variable *direct =
                            interpreter.find_variable(element_key)) {
                        return direct;
                    }
                }

                if (stored->is_struct) {
                    return stored;
                }
            }

            if (Variable *direct = interpreter.find_variable(element_key)) {
                return direct;
            }
            return nullptr;
        };

        if (Variable *resolved = resolve_element()) {
            Variable ref_entry;
            ref_entry.type = TYPE_STRUCT;
            ref_entry.is_struct = true;
            ref_entry.struct_type_name = resolved->struct_type_name;
            ref_entry.is_reference = true;
            ref_entry.value = reinterpret_cast<int64_t>(resolved);
            ref_entry.is_assigned = true;
            array_parent->struct_members[element_key] = ref_entry;
            return {resolved, final_member};
        }

        if (!array_parent->struct_type_name.empty()) {
            interpreter.create_struct_variable(element_key,
                                               array_parent->struct_type_name);
            if (Variable *created = interpreter.find_variable(element_key)) {
                Variable ref_entry;
                ref_entry.type = TYPE_STRUCT;
                ref_entry.is_struct = true;
                ref_entry.struct_type_name =
                    created->struct_type_name.empty()
                        ? array_parent->struct_type_name
                        : created->struct_type_name;
                ref_entry.is_reference = true;
                ref_entry.value = reinterpret_cast<int64_t>(created);
                ref_entry.is_assigned = true;
                array_parent->struct_members[element_key] = ref_entry;
                return {created, final_member};
            }
        }

        throw std::runtime_error("Failed to create array element: " +
                                 element_key);
    }

    throw std::runtime_error("Unsupported member access pattern");
}

} // namespace AssignmentHelpers
