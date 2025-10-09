#ifndef RECURSIVE_MEMBER_EVALUATOR_H
#define RECURSIVE_MEMBER_EVALUATOR_H

#include "../../../../common/ast.h"
#include "../../../../common/debug.h"
#include "../../core/interpreter.h"
#include <functional>
#include <stdexcept>
#include <string>

namespace MemberEvaluationHelpers {

// 再帰的にネストされたメンバーアクセスを評価して、最終的な変数を返す
// 例: container.shapes[0].edges[0].start.x
inline Variable *resolve_nested_member_for_evaluation(
    Interpreter &interpreter, const ASTNode *member_access_node,
    std::function<int64_t(const ASTNode *)> evaluate_index) {
    if (!member_access_node) {
        throw std::runtime_error("Null member access node");
    }

    // 最終メンバー名を取得
    std::string final_member = member_access_node->name;

    debug_print(
        "[EVAL_RESOLVER] Resolving member: %s, node_type=%d, left_type=%d\n",
        final_member.c_str(), static_cast<int>(member_access_node->node_type),
        member_access_node->left
            ? static_cast<int>(member_access_node->left->node_type)
            : -1);

    if (!member_access_node->left) {
        throw std::runtime_error("Member access node has no left child");
    }

    // [DEV DEBUG] Print actual enum values before conditional check
    debug_print("[EVAL_RESOLVER] Before Case 1: left->node_type=%d, "
                "AST_VARIABLE=%d, AST_IDENTIFIER=%d\n",
                static_cast<int>(member_access_node->left->node_type),
                static_cast<int>(ASTNodeType::AST_VARIABLE),
                static_cast<int>(ASTNodeType::AST_IDENTIFIER));

    // ケース1: 単純な変数アクセス (obj.member)
    if (member_access_node->left->node_type == ASTNodeType::AST_VARIABLE ||
        member_access_node->left->node_type == ASTNodeType::AST_IDENTIFIER) {
        debug_print("[EVAL_RESOLVER] Case 1: Simple variable access for '%s'\n",
                    final_member.c_str());

        std::string var_name = member_access_node->left->name;
        Variable *var = interpreter.find_variable(var_name);

        if (!var || !var->is_struct) {
            throw std::runtime_error("Base variable is not a struct: " +
                                     var_name);
        }

        // メンバーを取得
        debug_print("[EVAL_RESOLVER] Looking for member '%s' in var '%s' "
                    "(struct_members.size=%zu)\n",
                    final_member.c_str(), var_name.c_str(),
                    var->struct_members.size());

        auto it = var->struct_members.find(final_member);
        if (it == var->struct_members.end()) {
            debug_print(
                "[EVAL_RESOLVER] Member '%s' not found in struct_members\n",
                final_member.c_str());
            throw std::runtime_error("Member not found: " + final_member +
                                     " in " + var_name);
        }

        debug_print("[EVAL_RESOLVER] Found member '%s'\n",
                    final_member.c_str());
        return &it->second;
    }

    // ケース2: ネストされたメンバーアクセス (obj.mid.member)
    if (member_access_node->left->node_type == ASTNodeType::AST_MEMBER_ACCESS) {
        debug_print("[EVAL_RESOLVER] Case 2: Nested member access for '%s'\n",
                    final_member.c_str());

        // 再帰的に親を解決
        Variable *parent_var = resolve_nested_member_for_evaluation(
            interpreter, member_access_node->left.get(), evaluate_index);

        if (!parent_var || !parent_var->is_struct) {
            debug_print("[EVAL_RESOLVER] Parent is not a struct!\n");
            throw std::runtime_error("Parent is not a struct");
        }

        debug_print("[EVAL_RESOLVER] Parent resolved, searching for member "
                    "'%s' (struct_members.size=%zu)\n",
                    final_member.c_str(), parent_var->struct_members.size());

        // 親のstruct_membersから最終メンバーを取得
        auto it = parent_var->struct_members.find(final_member);
        if (it == parent_var->struct_members.end()) {
            debug_print("[EVAL_RESOLVER] Member '%s' not found in parent!\n",
                        final_member.c_str());
            throw std::runtime_error("Member not found: " + final_member);
        }

        debug_print("[EVAL_RESOLVER] Found member '%s'\n",
                    final_member.c_str());
        return &it->second;
    }

    // ケース3: 配列アクセスを含むネスト (obj.arr[0].member または
    // container.shapes[0].edges[0].start)
    if (member_access_node->left->node_type == ASTNodeType::AST_ARRAY_REF) {
        debug_print(
            "[EVAL_RESOLVER] Case 3: Array access for final_member '%s'\n",
            final_member.c_str());

        const ASTNode *array_ref = member_access_node->left.get();

        // インデックスを評価
        int64_t index = evaluate_index(array_ref->array_index.get());
        debug_print("[EVAL_RESOLVER] Array index: %lld\n", index);

        Variable *array_parent = nullptr;
        std::string array_member_name;

        // 配列の親を解決
        if (array_ref->left->node_type == ASTNodeType::AST_MEMBER_ACCESS) {
            debug_print("[EVAL_RESOLVER] Array parent is MEMBER_ACCESS\n");
            // struct.array[index].member の場合
            // 再帰的に配列メンバー自体を取得
            std::string member = array_ref->left->name;

            // 再帰呼び出しで配列メンバー自体のVariable*を取得
            // 例: shape.edges の場合、edges配列のVariable*が返される
            array_parent = resolve_nested_member_for_evaluation(
                interpreter, array_ref->left.get(), evaluate_index);

            if (!array_parent) {
                throw std::runtime_error("Failed to resolve array member: " +
                                         member);
            }

            debug_print("[EVAL_RESOLVER] Resolved array parent '%s' "
                        "(is_array=%d, struct_type='%s')\n",
                        member.c_str(), array_parent->is_array,
                        array_parent->struct_type_name.c_str());

            array_member_name = member;

        } else if (array_ref->left->node_type == ASTNodeType::AST_VARIABLE ||
                   array_ref->left->node_type == ASTNodeType::AST_IDENTIFIER) {
            // array[index].member の場合
            std::string var_name = array_ref->left->name;
            array_parent = interpreter.find_variable(var_name);
            array_member_name = var_name;

        } else if (array_ref->left->node_type == ASTNodeType::AST_ARRAY_REF) {
            // array[0][1].member のようなケース - 再帰的に処理
            throw std::runtime_error(
                "Nested array access not yet fully supported in evaluation");
        } else {
            throw std::runtime_error(
                "Unsupported array reference type in evaluation");
        }

        if (!array_parent || !array_parent->is_array) {
            throw std::runtime_error("Not an array: " + array_member_name);
        }

        // 配列が構造体配列かチェック
        if (array_parent->struct_type_name.empty()) {
            throw std::runtime_error(
                "Cannot access member of non-struct array element: " +
                array_member_name);
        }

        // 配列要素のキーを構築
        std::string element_key =
            array_member_name + "[" + std::to_string(index) + "]";

        // struct_membersから配列要素を取得
        auto elem_it = array_parent->struct_members.find(element_key);
        Variable *stored_entry = elem_it != array_parent->struct_members.end()
                                     ? &elem_it->second
                                     : nullptr;

        Variable *elem_var = nullptr;

        if (stored_entry) {
            if (stored_entry->is_reference && stored_entry->value != 0) {
                elem_var = reinterpret_cast<Variable *>(stored_entry->value);
            } else if (stored_entry->is_struct) {
                elem_var = stored_entry;
            }
        }

        if (!elem_var) {
            elem_var = interpreter.find_variable(element_key);
        }

        if (!elem_var && !array_parent->struct_type_name.empty()) {
            interpreter.create_struct_variable(element_key,
                                               array_parent->struct_type_name);
            elem_var = interpreter.find_variable(element_key);
        }

        if (!elem_var) {
            throw std::runtime_error("Array element not found: " + element_key);
        }

        if (stored_entry != elem_var) {
            Variable ref_entry;
            ref_entry.type = elem_var->type;
            ref_entry.is_struct = true;
            ref_entry.struct_type_name = elem_var->struct_type_name;
            ref_entry.is_reference = true;
            ref_entry.value = reinterpret_cast<int64_t>(elem_var);
            ref_entry.is_assigned = true;
            array_parent->struct_members[element_key] = ref_entry;
        }

        // 配列要素を取得したので、そこから残りのメンバーにアクセス
        // elem_varはedges[0] (Line構造体)
        // member_access_nodeは edges[0].start.x 全体
        // member_access_nodeのleftはarray_refなので、その後のメンバーアクセスを処理する必要がある

        // 解決策: member_access_nodeのleftがarray_refの場合、
        // array_refより後のメンバーアクセスチェーンを構築して、
        // elem_varからそのチェーンをたどる

        // しかし、元のnodeは edges[0].start.x 全体を表しているが、
        // leftはarray_refで、そこから先のメンバーアクセス情報は失われていない
        // 実は、nodeのleftはarray_refではなく、edges[0].startかもしれない

        // 確認: member_access_nodeのleftの型を見る
        // もしleftがMEMBER_ACCESSなら、再帰的に処理されているはず

        // 実際、ケース2で handle: leftがMEMBER_ACCESSの場合は再帰処理されている
        // ケース3はleftがARRAY_REFの場合

        // つまり、edges[0].start.xの場合、
        // - 最上位: name="x", left=edges[0].start (MEMBER_ACCESS) → ケース2
        // - 中間: name="start", left=edges[0] (ARRAY_REF) → ケース3

        // ケース3に来るのは、leftがARRAY_REFの時なので、
        // これは edges[0].start (中間ノード)の処理
        // つまり、final_member="start"で、elem_var=edges[0]

        // そして、最上位ノード(x)はケース2で処理され、
        // その時parent_var=edges[0].startを返す必要がある

        // つまり、ケース3では final_memberを elem_varから取得して返せば良い
        auto member_it = elem_var->struct_members.find(final_member);
        if (member_it != elem_var->struct_members.end()) {
            return &member_it->second;
        }

        // struct_membersに見つからない場合、個別変数として探す
        std::string full_path = element_key + "." + final_member;
        Variable *full_var = interpreter.find_variable(full_path);
        if (full_var) {
            return full_var;
        }

        throw std::runtime_error("Member not found: " + final_member +
                                 " in array element " + element_key);
    }

    throw std::runtime_error("Unsupported member access pattern in evaluation");
}

} // namespace MemberEvaluationHelpers

#endif // RECURSIVE_MEMBER_EVALUATOR_H
