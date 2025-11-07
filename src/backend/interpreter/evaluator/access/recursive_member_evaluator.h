#ifndef RECURSIVE_MEMBER_EVALUATOR_H
#define RECURSIVE_MEMBER_EVALUATOR_H

#include "../../../../common/ast.h"
#include "../../../../common/debug.h"
#include "../../core/interpreter.h"
#include "../../core/pointer_metadata.h"
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

    {
        char dbg_buf[512];
        snprintf(
            dbg_buf, sizeof(dbg_buf),
            "[EVAL_RESOLVER] Resolving member: %s, node_type=%d, left_type=%d",
            final_member.c_str(),
            static_cast<int>(member_access_node->node_type),
            member_access_node->left
                ? static_cast<int>(member_access_node->left->node_type)
                : -1);
        debug_msg(DebugMsgId::GENERIC_DEBUG, dbg_buf);
    }

    if (!member_access_node->left) {
        throw std::runtime_error("Member access node has no left child");
    }

    // ケース4を先にチェック: ARROW_ACCESS自体が渡された場合
    if (member_access_node->node_type == ASTNodeType::AST_ARROW_ACCESS) {
        goto case_4_arrow_access;
    }

    // ケース1: 単純な変数アクセス (obj.member)
    if (member_access_node->left->node_type == ASTNodeType::AST_VARIABLE ||
        member_access_node->left->node_type == ASTNodeType::AST_IDENTIFIER) {
        {
            char dbg_buf[512];
            snprintf(dbg_buf, sizeof(dbg_buf),
                     "[EVAL_RESOLVER] Case 1: Simple variable access for '%s'",
                     final_member.c_str());
            debug_msg(DebugMsgId::GENERIC_DEBUG, dbg_buf);
        }

        std::string var_name = member_access_node->left->name;
        Variable *var = interpreter.find_variable(var_name);

        // v0.11.0: enum型もメンバーアクセスをサポート
        if (!var || (!var->is_struct && !var->is_enum)) {
            throw std::runtime_error("Base variable is not a struct or enum: " +
                                     var_name);
        }

        // enum型の場合は特別処理（member.cppで処理済み）
        if (var->is_enum) {
            // このパスには到達しないはず（member.cppで先に処理）
            throw std::runtime_error(
                "Enum member access should be handled earlier");
        }

        // メンバーを取得
        debug_msg(DebugMsgId::GENERIC_DEBUG,
                  "[EVAL_RESOLVER] Looking for member '%s' in var '%s' ");

        auto it = var->struct_members.find(final_member);
        if (it == var->struct_members.end()) {
            {
                char dbg_buf[512];
                snprintf(
                    dbg_buf, sizeof(dbg_buf),
                    "[EVAL_RESOLVER] Member '%s' not found in struct_members",
                    final_member.c_str());
                debug_msg(DebugMsgId::GENERIC_DEBUG, dbg_buf);
            }
            throw std::runtime_error("Member not found: " + final_member +
                                     " in " + var_name);
        }

        {
            char dbg_buf[512];
            snprintf(dbg_buf, sizeof(dbg_buf),
                     "[EVAL_RESOLVER] Found member '%s'", final_member.c_str());
            debug_msg(DebugMsgId::GENERIC_DEBUG, dbg_buf);
        }
        return &it->second;
    }

    // ケース2: ネストされたメンバーアクセス (obj.mid.member)
    if (member_access_node->left->node_type == ASTNodeType::AST_MEMBER_ACCESS) {
        {
            char dbg_buf[512];
            snprintf(dbg_buf, sizeof(dbg_buf),
                     "[EVAL_RESOLVER] Case 2: Nested member access for '%s'",
                     final_member.c_str());
            debug_msg(DebugMsgId::GENERIC_DEBUG, dbg_buf);
        }

        // 再帰的に親を解決
        Variable *parent_var = resolve_nested_member_for_evaluation(
            interpreter, member_access_node->left.get(), evaluate_index);

        if (!parent_var || !parent_var->is_struct) {
            debug_msg(DebugMsgId::GENERIC_DEBUG,
                      "[EVAL_RESOLVER] Parent is not a struct!");
            throw std::runtime_error("Parent is not a struct");
        }

        debug_msg(DebugMsgId::GENERIC_DEBUG,
                  "[EVAL_RESOLVER] Parent resolved, searching for member ");

        // 親のstruct_membersから最終メンバーを取得
        auto it = parent_var->struct_members.find(final_member);
        if (it == parent_var->struct_members.end()) {
            debug_msg(DebugMsgId::GENERIC_DEBUG,
                      "[EVAL_RESOLVER] Member '%s' not found in parent ");

            // struct_membersが空の場合、これは構造体リテラル初期化による
            // 問題の可能性がある
            // parent_var 自体を返してフォールバック
            debug_msg(DebugMsgId::GENERIC_DEBUG,
                      "[EVAL_RESOLVER] Returning parent_var as fallback");
            return parent_var;
        }

        {
            char dbg_buf[512];
            snprintf(dbg_buf, sizeof(dbg_buf),
                     "[EVAL_RESOLVER] Found member '%s'", final_member.c_str());
            debug_msg(DebugMsgId::GENERIC_DEBUG, dbg_buf);
        }
        return &it->second;
    }

    // ケース3: デリファレンス演算子を含むネスト ((*ptr).val.member)
    if (member_access_node->left->node_type == ASTNodeType::AST_UNARY_OP &&
        member_access_node->left->op == "DEREFERENCE") {
        {
            char dbg_buf[512];
            snprintf(dbg_buf, sizeof(dbg_buf),
                     "[EVAL_RESOLVER] Case 3: Dereference access for '%s'",
                     final_member.c_str());
            debug_msg(DebugMsgId::GENERIC_DEBUG, dbg_buf);
        }

        const ASTNode *deref_node = member_access_node->left.get();

        // デリファレンスの対象を再帰的に評価
        Variable *ptr_var = nullptr;

        if (deref_node->left->node_type == ASTNodeType::AST_VARIABLE ||
            deref_node->left->node_type == ASTNodeType::AST_IDENTIFIER) {
            // 単純なポインタ変数: (*ptr).val.member
            std::string ptr_name = deref_node->left->name;
            ptr_var = interpreter.find_variable(ptr_name);
            if (!ptr_var || ptr_var->type != TYPE_POINTER) {
                throw std::runtime_error("Not a pointer for dereference: " +
                                         ptr_name);
            }
        } else if (deref_node->left->node_type ==
                   ASTNodeType::AST_MEMBER_ACCESS) {
            // ネストされたメンバーからのポインタ: obj.ptr の後にデリファレンス
            ptr_var = resolve_nested_member_for_evaluation(
                interpreter, deref_node->left.get(), evaluate_index);
            if (!ptr_var || ptr_var->type != TYPE_POINTER) {
                throw std::runtime_error(
                    "Not a pointer in nested dereference access");
            }
        } else if (deref_node->left->node_type ==
                   ASTNodeType::AST_ARROW_ACCESS) {
            // Arrow演算子の後にデリファレンス: (*p->ptr).member
            debug_msg(DebugMsgId::GENERIC_DEBUG,
                      "[EVAL_RESOLVER] Dereference after arrow access");
            ptr_var = resolve_nested_member_for_evaluation(
                interpreter, deref_node->left.get(), evaluate_index);
            if (!ptr_var || ptr_var->type != TYPE_POINTER) {
                throw std::runtime_error(
                    "Not a pointer in arrow-then-dereference");
            }
            debug_msg(DebugMsgId::GENERIC_DEBUG,
                      "[EVAL_RESOLVER] Arrow-then-deref resolved");
        } else if (deref_node->left->node_type == ASTNodeType::AST_UNARY_OP &&
                   deref_node->left->op == "DEREFERENCE") {
            // 二重デリファレンス: **ptr または (**ptr)
            // 内側のデリファレンスを再帰的に処理
            debug_msg(DebugMsgId::GENERIC_DEBUG,
                      "[EVAL_RESOLVER] Processing double dereference");

            // 内側のデリファレンスノードを作成して再帰的に処理
            // deref_node->left は内側の UNARY_OP (DEREFERENCE)
            // deref_node->left->left は実際のポインタ変数

            // 内側のデリファレンスの対象を取得
            const ASTNode *inner_deref = deref_node->left.get();
            Variable *inner_ptr_var = nullptr;

            if (inner_deref->left->node_type == ASTNodeType::AST_VARIABLE ||
                inner_deref->left->node_type == ASTNodeType::AST_IDENTIFIER) {
                // **ptr の形式
                std::string ptr_name = inner_deref->left->name;
                inner_ptr_var = interpreter.find_variable(ptr_name);
                if (!inner_ptr_var || inner_ptr_var->type != TYPE_POINTER) {
                    throw std::runtime_error(
                        "Not a pointer for double dereference: " + ptr_name);
                }
            } else if (inner_deref->left->node_type ==
                       ASTNodeType::AST_MEMBER_ACCESS) {
                // obj.ptr の後に二重デリファレンス
                inner_ptr_var = resolve_nested_member_for_evaluation(
                    interpreter, inner_deref->left.get(), evaluate_index);
                if (!inner_ptr_var || inner_ptr_var->type != TYPE_POINTER) {
                    throw std::runtime_error(
                        "Not a pointer in nested double dereference");
                }
            } else if (inner_deref->left->node_type ==
                       ASTNodeType::AST_ARROW_ACCESS) {
                // arrow の後に二重デリファレンス: (**p->ptr)
                inner_ptr_var = resolve_nested_member_for_evaluation(
                    interpreter, inner_deref->left.get(), evaluate_index);
                if (!inner_ptr_var || inner_ptr_var->type != TYPE_POINTER) {
                    throw std::runtime_error(
                        "Not a pointer in arrow-then-double-dereference");
                }
            } else if (inner_deref->left->node_type ==
                           ASTNodeType::AST_UNARY_OP &&
                       inner_deref->left->op == "DEREFERENCE") {
                // 三重以上のデリファレンス: ***ptr
                debug_msg(DebugMsgId::GENERIC_DEBUG,
                          "[EVAL_RESOLVER] Processing triple+ dereference");

                // 三重デリファレンスを段階的に処理
                // inner_deref->left は ***ptr の場合の **ptr部分
                const ASTNode *triple_inner = inner_deref->left.get();

                // 最内側のポインタ変数を取得（再帰的に）
                Variable *level3_ptr = nullptr;
                if (triple_inner->left->node_type ==
                        ASTNodeType::AST_VARIABLE ||
                    triple_inner->left->node_type ==
                        ASTNodeType::AST_IDENTIFIER) {
                    // ***ptr の ptr 部分
                    std::string ptr_name = triple_inner->left->name;
                    level3_ptr = interpreter.find_variable(ptr_name);
                    if (!level3_ptr || level3_ptr->type != TYPE_POINTER) {
                        throw std::runtime_error(
                            "Not a pointer for triple dereference: " +
                            ptr_name);
                    }
                } else {
                    // さらに複雑なパターン（****ptr など）
                    // TODO: 4重以上のデリファレンスは現状未対応
                    throw std::runtime_error(
                        "Quadruple+ dereference not yet supported");
                }

                // 1回目のデリファレンス: ptr*** -> ptr**
                if (level3_ptr->value == 0) {
                    throw std::runtime_error(
                        "Null pointer in triple dereference (first level)");
                }
                Variable *level2_ptr =
                    reinterpret_cast<Variable *>(level3_ptr->value);
                if (!level2_ptr || level2_ptr->type != TYPE_POINTER) {
                    throw std::runtime_error("Triple dereference requires "
                                             "pointer to pointer to pointer");
                }

                // 2回目のデリファレンス: ptr** -> ptr*
                if (level2_ptr->value == 0) {
                    throw std::runtime_error(
                        "Null pointer in triple dereference (second level)");
                }
                inner_ptr_var = reinterpret_cast<Variable *>(level2_ptr->value);
                if (!inner_ptr_var || inner_ptr_var->type != TYPE_POINTER) {
                    throw std::runtime_error(
                        "Triple dereference requires valid pointer chain");
                }

                debug_msg(
                    DebugMsgId::GENERIC_DEBUG,
                    "[EVAL_RESOLVER] Triple dereference resolved to pointer");
            } else {
                throw std::runtime_error(
                    "Unsupported double dereference pattern");
            }

            // 最初のデリファレンス: ポインタのポインタ -> ポインタ
            if (inner_ptr_var->value == 0) {
                throw std::runtime_error(
                    "Null pointer in double dereference (first level)");
            }

            Variable *intermediate_ptr =
                reinterpret_cast<Variable *>(inner_ptr_var->value);
            if (!intermediate_ptr || intermediate_ptr->type != TYPE_POINTER) {
                throw std::runtime_error(
                    "Double dereference requires pointer to pointer");
            }

            // 2回目のデリファレンス: ポインタ -> 構造体
            if (intermediate_ptr->value == 0) {
                throw std::runtime_error(
                    "Null pointer in double dereference (second level)");
            }

            ptr_var =
                intermediate_ptr; // 2回目のデリファレンスで得たポインタをptr_varにセット
        } else {
            throw std::runtime_error("Unsupported dereference pattern");
        }

        // ポインタから構造体を取得
        if (ptr_var->value == 0) {
            throw std::runtime_error("Null pointer dereference");
        }

        Variable *struct_var = reinterpret_cast<Variable *>(ptr_var->value);
        if (!struct_var || (struct_var->type != TYPE_STRUCT &&
                            struct_var->type != TYPE_INTERFACE)) {
            throw std::runtime_error(
                "Dereference requires struct or interface pointer");
        }

        // 構造体から最終メンバーを取得
        auto final_it = struct_var->struct_members.find(final_member);
        if (final_it != struct_var->struct_members.end()) {
            {
                char dbg_buf[512];
                snprintf(
                    dbg_buf, sizeof(dbg_buf),
                    "[EVAL_RESOLVER] Found final member '%s' via dereference",
                    final_member.c_str());
                debug_msg(DebugMsgId::GENERIC_DEBUG, dbg_buf);
            }
            return &final_it->second;
        }

        debug_msg(DebugMsgId::GENERIC_DEBUG,
                  "[EVAL_RESOLVER] Member '%s' not found via dereference, ");
        return struct_var;
    }

// ケース4: アロー演算子を含むネスト (ptr->val.member)
// または member_access_node 自体が ARROW_ACCESS の場合
case_4_arrow_access:
    if (member_access_node->node_type == ASTNodeType::AST_ARROW_ACCESS ||
        member_access_node->left->node_type == ASTNodeType::AST_ARROW_ACCESS) {
        {
            char dbg_buf[512];
            snprintf(dbg_buf, sizeof(dbg_buf),
                     "[EVAL_RESOLVER] Case 4: Arrow access for '%s'",
                     final_member.c_str());
            debug_msg(DebugMsgId::GENERIC_DEBUG, dbg_buf);
        }

        const ASTNode *arrow_node =
            (member_access_node->node_type == ASTNodeType::AST_ARROW_ACCESS)
                ? member_access_node
                : member_access_node->left.get();

        // ポインタを評価して値を取得
        // arrow_nodeのleftを再帰的に評価する必要がある
        Variable *ptr_var = nullptr;

        if (arrow_node->left->node_type == ASTNodeType::AST_VARIABLE ||
            arrow_node->left->node_type == ASTNodeType::AST_IDENTIFIER) {
            // 単純なポインタ変数: ptr->val.member
            std::string ptr_name = arrow_node->left->name;
            ptr_var = interpreter.find_variable(ptr_name);
            if (!ptr_var || ptr_var->type != TYPE_POINTER) {
                throw std::runtime_error("Not a pointer: " + ptr_name);
            }
        } else if (arrow_node->left->node_type ==
                   ASTNodeType::AST_MEMBER_ACCESS) {
            // ネストされたメンバーからのポインタ: obj.ptr->val.member
            ptr_var = resolve_nested_member_for_evaluation(
                interpreter, arrow_node->left.get(), evaluate_index);
            if (!ptr_var || ptr_var->type != TYPE_POINTER) {
                throw std::runtime_error(
                    "Not a pointer in nested arrow access");
            }
        } else if (arrow_node->left->node_type ==
                   ASTNodeType::AST_ARROW_ACCESS) {
            // ポインタからポインタへ: ptr1->ptr2->val.member
            // この場合、arrow_node->leftを評価して構造体メンバー(ポインタ)を取得する必要がある
            // しかし、resolve_nested_member_for_evaluationは最終メンバーを返すため、
            // ここでは評価関数を使って値を取得する必要がある

            // arrow_nodeのleftを再帰的に評価してポインタ値を取得
            // ここでは評価関数が必要だが、この関数には渡されていない
            // 代替案: arrow_nodeのleftが指す構造体を直接たどる

            // 簡易実装: arrow_nodeのleftを分解して手動で処理
            // 例: obj.ptr1->ptr2 の場合
            //   - arrow_node->left = obj.ptr1 (ARROW_ACCESS)
            //   - arrow_node->left->left = obj (VARIABLE)
            //   - arrow_node->left->name = ptr1

            const ASTNode *nested_arrow = arrow_node->left.get();
            Variable *nested_struct = nullptr;

            if (nested_arrow->left->node_type == ASTNodeType::AST_VARIABLE ||
                nested_arrow->left->node_type == ASTNodeType::AST_IDENTIFIER) {
                // obj.ptr1->ptr2 の obj.ptr1 部分
                std::string base_name = nested_arrow->left->name;
                Variable *base_var = interpreter.find_variable(base_name);
                if (!base_var || base_var->type != TYPE_POINTER) {
                    throw std::runtime_error("Not a pointer in nested arrow: " +
                                             base_name);
                }

                // ptr1が指す構造体を取得
                if (base_var->value == 0) {
                    throw std::runtime_error(
                        "Null pointer in nested arrow access");
                }
                nested_struct = reinterpret_cast<Variable *>(base_var->value);

            } else if (nested_arrow->left->node_type ==
                       ASTNodeType::AST_MEMBER_ACCESS) {
                // obj.mid.ptr1->ptr2 のような場合
                Variable *base_member = resolve_nested_member_for_evaluation(
                    interpreter, nested_arrow->left.get(), evaluate_index);
                if (!base_member || base_member->type != TYPE_POINTER) {
                    throw std::runtime_error(
                        "Not a pointer in deeply nested arrow");
                }

                if (base_member->value == 0) {
                    throw std::runtime_error(
                        "Null pointer in deeply nested arrow access");
                }
                nested_struct =
                    reinterpret_cast<Variable *>(base_member->value);
            } else if (nested_arrow->left->node_type ==
                       ASTNodeType::AST_ARROW_ACCESS) {
                // さらに深いネスト: ptr1->ptr2->ptr3->val.member
                // nested_arrowのleftを再帰的に解決
                debug_msg(DebugMsgId::GENERIC_DEBUG,
                          "[EVAL_RESOLVER] Recursive arrow: ");
                Variable *base_member = resolve_nested_member_for_evaluation(
                    interpreter, nested_arrow->left.get(), evaluate_index);
                if (!base_member || base_member->type != TYPE_POINTER) {
                    throw std::runtime_error(
                        "Not a pointer in deeply nested arrow chain");
                }

                if (base_member->value == 0) {
                    throw std::runtime_error(
                        "Null pointer in deeply nested arrow chain");
                }
                nested_struct =
                    reinterpret_cast<Variable *>(base_member->value);
                debug_msg(DebugMsgId::GENERIC_DEBUG,
                          "[EVAL_RESOLVER] Recursive arrow resolved, got ");
            } else {
                throw std::runtime_error(
                    "Complex nested arrow pattern not supported");
            }

            // nested_structからnested_arrow->nameのメンバー(ポインタ)を取得
            if (!nested_struct || (nested_struct->type != TYPE_STRUCT &&
                                   nested_struct->type != TYPE_INTERFACE)) {
                throw std::runtime_error("Invalid struct in nested arrow");
            }

            std::string nested_member = nested_arrow->name;
            auto nested_it = nested_struct->struct_members.find(nested_member);
            if (nested_it == nested_struct->struct_members.end()) {
                throw std::runtime_error("Member not found in nested arrow: " +
                                         nested_member);
            }

            ptr_var = &nested_it->second;
            if (ptr_var->type != TYPE_POINTER) {
                throw std::runtime_error(
                    "Not a pointer in nested arrow result: " + nested_member);
            }
        } else {
            throw std::runtime_error("Unsupported left side in arrow access");
        }

        // ポインタから構造体を取得
        if (ptr_var->value == 0) {
            throw std::runtime_error(
                "Null pointer dereference in arrow access");
        }

        Variable *struct_var = reinterpret_cast<Variable *>(ptr_var->value);
        if (!struct_var || (struct_var->type != TYPE_STRUCT &&
                            struct_var->type != TYPE_INTERFACE)) {
            throw std::runtime_error(
                "Arrow access requires struct or interface pointer");
        }

        // arrow_nodeのメンバー名を取得
        std::string arrow_member = arrow_node->name;

        {
            char dbg_buf[512];
            snprintf(dbg_buf, sizeof(dbg_buf),
                     "[EVAL_RESOLVER] Arrow: getting member '%s' from struct",
                     arrow_member.c_str());
            debug_msg(DebugMsgId::GENERIC_DEBUG, dbg_buf);
        }

        // arrow_memberとfinal_memberが同じ場合、これはp->ptrのような単純なアローアクセス
        if (arrow_member == final_member) {
            debug_msg(DebugMsgId::GENERIC_DEBUG,
                      "[EVAL_RESOLVER] Arrow: simple arrow access (p->member)");
            // 構造体から最終メンバーを直接取得
            auto member_it = struct_var->struct_members.find(arrow_member);
            if (member_it == struct_var->struct_members.end()) {
                debug_msg(
                    DebugMsgId::GENERIC_DEBUG,
                    "[EVAL_RESOLVER] Member '%s' not found in struct_members ");
                return struct_var;
            }
            {
                char dbg_buf[512];
                snprintf(dbg_buf, sizeof(dbg_buf),
                         "[EVAL_RESOLVER] Found member '%s' via simple arrow",
                         arrow_member.c_str());
                debug_msg(DebugMsgId::GENERIC_DEBUG, dbg_buf);
            }
            return &member_it->second;
        }

        // arrow_memberとfinal_memberが異なる場合、中間メンバーを経由する
        // 例: p->mid.final （arrow_member="mid", final_member="final"）

        // 構造体から中間メンバーを取得
        auto arrow_it = struct_var->struct_members.find(arrow_member);
        if (arrow_it == struct_var->struct_members.end()) {
            debug_msg(DebugMsgId::GENERIC_DEBUG,
                      "[EVAL_RESOLVER] arrow_member '%s' not found in "
                      "struct_members ");
            // 構造体リテラル初期化の問題：struct_membersが空の場合のフォールバック
            return struct_var;
        }

        Variable *intermediate_var = &arrow_it->second;

        debug_msg(
            DebugMsgId::GENERIC_DEBUG,
            "[EVAL_RESOLVER] Arrow: arrow_member='%s', final_member='%s', ");

        // 中間メンバーが構造体の場合、最終メンバーを取得
        if (intermediate_var->type == TYPE_STRUCT ||
            intermediate_var->is_struct) {
            // まずstruct_membersから探す
            auto final_it = intermediate_var->struct_members.find(final_member);
            if (final_it != intermediate_var->struct_members.end()) {
                {
                    char dbg_buf[512];
                    snprintf(
                        dbg_buf, sizeof(dbg_buf),
                        "[EVAL_RESOLVER] Found final member '%s' via arrow",
                        final_member.c_str());
                    debug_msg(DebugMsgId::GENERIC_DEBUG, dbg_buf);
                }
                return &final_it->second;
            }

            debug_msg(DebugMsgId::GENERIC_DEBUG,
                      "[EVAL_RESOLVER] final_member '%s' not found in ");

            // struct_membersが空の場合、これは構造体リテラル初期化の問題
            // 個別変数として管理されている可能性がある
            // intermediate_var自体を構造体として返し、上位レベルで処理させる
            //
            // 実は、これは intermediate_var が「構造体の値」であり、
            // そのメンバーが struct_members に展開されていない場合
            // この場合、intermediate_var 自体を返すことで、
            // 呼び出し元がさらにメンバーアクセスを試みることができる
            //
            // ただし、このヘルパー関数の契約は「final_memberの変数を返す」なので、
            // intermediate_var を返すと、呼び出し元は final_member として
            // intermediate_var を受け取ることになる
            //
            // これは正しくない。実際には、intermediate_var は final_member の
            // 「親」構造体である
            //
            // 解決策: intermediate_var 自体を返すが、呼び出し元に対して
            // 「これは最終メンバーではなく、親構造体である」と示す必要がある
            // しかし、現在のインターフェースではそれができない
            //
            // 別の解決策: エラーを返さずに、intermediate_var を返す
            // これにより、少なくともクラッシュは避けられる
            // 呼び出し元が適切に処理することを期待

            debug_msg(DebugMsgId::GENERIC_DEBUG,
                      "[EVAL_RESOLVER] Returning intermediate_var as fallback");
            return intermediate_var;
        } else {
            // 中間メンバーが構造体でない場合はエラー
            throw std::runtime_error(
                "Cannot access member '" + final_member +
                "' of non-struct member '" + arrow_member +
                "' (type=" + std::to_string(intermediate_var->type) + ")");
        }
    }

    // ケース5: 配列アクセスを含むネスト (obj.arr[0].member または
    // container.shapes[0].edges[0].start)
    if (member_access_node->left->node_type == ASTNodeType::AST_ARRAY_REF) {
        {
            char dbg_buf[512];
            snprintf(
                dbg_buf, sizeof(dbg_buf),
                "[EVAL_RESOLVER] Case 5: Array access for final_member '%s'",
                final_member.c_str());
            debug_msg(DebugMsgId::GENERIC_DEBUG, dbg_buf);
        }

        const ASTNode *array_ref = member_access_node->left.get();

        // インデックスを評価
        int64_t index = evaluate_index(array_ref->array_index.get());
        {
            char dbg_buf[512];
            snprintf(dbg_buf, sizeof(dbg_buf),
                     "[EVAL_RESOLVER] Array index: %lld", index);
            debug_msg(DebugMsgId::GENERIC_DEBUG, dbg_buf);
        }

        Variable *array_parent = nullptr;
        std::string array_member_name;

        // 配列の親を解決
        if (array_ref->left->node_type == ASTNodeType::AST_MEMBER_ACCESS) {
            debug_msg(DebugMsgId::GENERIC_DEBUG,
                      "[EVAL_RESOLVER] Array parent is MEMBER_ACCESS");
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

            debug_msg(DebugMsgId::GENERIC_DEBUG,
                      "[EVAL_RESOLVER] Resolved array parent '%s' ");

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

        if (!array_parent ||
            (!array_parent->is_array && !array_parent->is_pointer)) {
            throw std::runtime_error("Not an array: " + array_member_name);
        }

        // ポインタの場合は、実際の配列を取得
        if (array_parent->is_pointer) {
            // ポインタの値を取得
            int64_t ptr_value = array_parent->value;
            bool is_metadata_ptr = (ptr_value & (1LL << 63)) != 0;

            if (is_metadata_ptr) {
                // メタデータポインタの場合
                int64_t clean_ptr = ptr_value & ~(1LL << 63);
                PointerSystem::PointerMetadata *meta =
                    reinterpret_cast<PointerSystem::PointerMetadata *>(
                        clean_ptr);

                if (meta && meta->array_var) {
                    // 実際の配列と調整されたインデックスを使用
                    array_parent = meta->array_var;
                    index += meta->element_index;
                    if (!meta->array_name.empty()) {
                        array_member_name = meta->array_name;
                    }
                } else {
                    throw std::runtime_error("Invalid pointer metadata");
                }
            } else {
                // 直接ポインタの場合（後方互換性）
                throw std::runtime_error(
                    "Direct pointer array evaluation not supported");
            }
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
