// ============================================================================
// expression_member_access_impl.cpp
// ============================================================================
// Phase 13: Expression Evaluator Refactoring - Member Access Implementation
//
// AST_MEMBER_ACCESSケースの実装を分離（568行）
// メンバアクセス: obj.member, array[index].member, self.member,
//               ネストしたアクセス、関数戻り値からのアクセス等
// ============================================================================

#include "../../../common/ast.h"
#include "../../../common/debug.h"
#include "../../../common/debug_messages.h"
#include "core/interpreter.h"
#include "expression_evaluator.h"
#include <functional>
#include <stdexcept>

int64_t ExpressionEvaluator::evaluate_member_access_impl(const ASTNode *node) {
    // メンバアクセス: obj.member または array[index].member または
    // self.member
    std::string var_name;
    std::string member_name = node->name;

    // ネストしたメンバーアクセスの場合（再帰的に処理）
    if (!node->member_chain.empty() && node->member_chain.size() > 1) {
        // ベース変数を取得
        Variable base_var;
        if (node->left->node_type == ASTNodeType::AST_VARIABLE) {
            Variable *var = interpreter_.find_variable(node->left->name);
            if (!var || var->type != TYPE_STRUCT) {
                throw std::runtime_error(
                    "Base variable for nested access is not a struct: " +
                    node->left->name);
            }
            base_var = *var;
        } else if (node->left->node_type == ASTNodeType::AST_IDENTIFIER &&
                   node->left->name == "self") {
            // selfの場合
            Variable *var = interpreter_.find_variable("self");
            if (!var ||
                (var->type != TYPE_STRUCT && var->type != TYPE_INTERFACE)) {
                throw std::runtime_error("self is not a struct or interface");
            }
            base_var = *var;
        } else if (node->left->node_type == ASTNodeType::AST_MEMBER_ACCESS ||
                   node->left->node_type == ASTNodeType::AST_ARRAY_REF) {
            // ネストしたメンバーアクセスまたは配列アクセスの場合:
            // scene.triangle.vertices や array[index] のような
            // 完全なパスを構築
            std::function<std::string(const ASTNode *)> build_path;
            build_path = [&](const ASTNode *n) -> std::string {
                if (n->node_type == ASTNodeType::AST_VARIABLE) {
                    return n->name;
                } else if (n->node_type == ASTNodeType::AST_MEMBER_ACCESS) {
                    std::string base = build_path(n->left.get());
                    return base + "." + n->name;
                } else if (n->node_type == ASTNodeType::AST_ARRAY_REF) {
                    // 配列アクセスの場合: base[index]
                    std::string base = build_path(n->left.get());
                    int64_t index = evaluate_expression(n->array_index.get());
                    return base + "[" + std::to_string(index) + "]";
                } else {
                    throw std::runtime_error(
                        "Unsupported node type in nested member access "
                        "path building");
                }
            };
            std::string full_path = build_path(node->left.get());
            Variable *var = interpreter_.find_variable(full_path);
            if (!var || var->type != TYPE_STRUCT) {
                throw std::runtime_error(
                    "Base variable for nested access is not a struct: " +
                    full_path);
            }
            base_var = *var;
        } else {
            throw std::runtime_error(
                "Complex base types for nested access not yet supported");
        }

        // 再帰的にメンバーチェーンをたどる
        try {
            Variable current_var = base_var;

            for (size_t i = 0; i < node->member_chain.size(); ++i) {
                const std::string &member_name_in_chain = node->member_chain[i];

                // 現在の変数から次のメンバーを取得
                current_var = get_struct_member_from_variable(
                    current_var, member_name_in_chain);

                // 最後のメンバーでない場合、次のメンバーにアクセスするために構造体である必要がある
                if (i < node->member_chain.size() - 1) {
                    if (current_var.type != TYPE_STRUCT &&
                        current_var.type != TYPE_INTERFACE) {
                        throw std::runtime_error(
                            "Intermediate member is not a struct: " +
                            member_name_in_chain);
                    }
                }
            }

            // 最終的な値を返す
            if (current_var.type == TYPE_STRING) {
                last_typed_result_ = TypedValue(
                    current_var.str_value, InferredType(TYPE_STRING, "string"));
                return 0;
            } else if (current_var.type == TYPE_POINTER) {
                return current_var.value;
            } else if (current_var.type == TYPE_FLOAT ||
                       current_var.type == TYPE_DOUBLE ||
                       current_var.type == TYPE_QUAD) {
                // float/double/quadの場合はTypedValueに設定して返す
                InferredType float_type(current_var.type, "");
                if (current_var.type == TYPE_QUAD) {
                    last_typed_result_ =
                        TypedValue(current_var.quad_value, float_type);
                } else {
                    last_typed_result_ =
                        TypedValue(current_var.float_value, float_type);
                }
                return static_cast<int64_t>(current_var.float_value);
            } else {
                return current_var.value;
            }
        } catch (const std::exception &e) {
            throw std::runtime_error("Nested member access failed: " +
                                     std::string(e.what()));
        }
    }

    // leftがAST_MEMBER_ACCESSの場合、まずleftを評価して構造体を取得
    if (node->left->node_type == ASTNodeType::AST_MEMBER_ACCESS) {
        debug_msg(DebugMsgId::NESTED_MEMBER_EVAL_START,
                  "left is AST_MEMBER_ACCESS");
        // ネストしたメンバーアクセス: (obj.inner).value
        // leftを評価して中間の構造体を取得
        Variable intermediate_struct;

        // leftのAST_MEMBER_ACCESSを評価
        // この時点でlast_typed_result_に型情報が設定される
        evaluate_typed_expression(node->left.get());

        // last_typed_result_から構造体変数を取得
        if (last_typed_result_.type.type_info == TYPE_STRUCT) {
            // last_typed_result_が構造体の場合、それを使用
            // しかし、evaluate_typed_expressionは数値しか返さないため、
            // 代わりにleftを完全に評価して構造体メンバーの変数パスを構築する必要がある

            // leftから構造体変数のパスを構築
            std::string struct_path;
            const ASTNode *current = node->left.get();

            // 再帰的にパスを構築
            std::function<std::string(const ASTNode *)> build_path;
            build_path = [&](const ASTNode *n) -> std::string {
                if (n->node_type == ASTNodeType::AST_VARIABLE) {
                    return n->name;
                } else if (n->node_type == ASTNodeType::AST_MEMBER_ACCESS) {
                    std::string base = build_path(n->left.get());
                    return base + "." + n->name;
                } else if (n->node_type == ASTNodeType::AST_ARRAY_REF) {
                    // 配列アクセスの場合: base[index]
                    std::string base = build_path(n->left.get());
                    int64_t index = evaluate_expression(n->array_index.get());
                    return base + "[" + std::to_string(index) + "]";
                } else {
                    throw std::runtime_error(
                        "Unsupported node type in nested member access");
                }
            };

            struct_path = build_path(current);

            // 完全パスを構築（最終メンバーまで含む）
            std::string full_member_path = struct_path + "." + member_name;

            debug_msg(DebugMsgId::NESTED_MEMBER_FULL_PATH,
                      full_member_path.c_str());

            // 個別変数を直接検索
            Variable *member_var_ptr =
                interpreter_.find_variable(full_member_path);
            if (member_var_ptr) {
                debug_msg(DebugMsgId::NESTED_MEMBER_INDIVIDUAL_VAR_FOUND,
                          full_member_path.c_str(), member_var_ptr->value);
                // 個別変数が見つかった場合、それを使用
                if (member_var_ptr->type == TYPE_STRING) {
                    last_typed_result_ =
                        TypedValue(member_var_ptr->str_value,
                                   InferredType(TYPE_STRING, "string"));
                    return 0;
                } else if (member_var_ptr->type == TYPE_STRUCT) {
                    last_typed_result_ = TypedValue(
                        member_var_ptr->value,
                        InferredType(TYPE_STRUCT, member_var_ptr->type_name));
                    return member_var_ptr->value;
                } else if (member_var_ptr->type == TYPE_FLOAT ||
                           member_var_ptr->type == TYPE_DOUBLE ||
                           member_var_ptr->type == TYPE_QUAD) {
                    // float/double/quadの場合
                    InferredType float_type(member_var_ptr->type, "");
                    if (member_var_ptr->type == TYPE_QUAD) {
                        last_typed_result_ =
                            TypedValue(member_var_ptr->quad_value, float_type);
                    } else {
                        last_typed_result_ =
                            TypedValue(member_var_ptr->float_value, float_type);
                    }
                    return static_cast<int64_t>(member_var_ptr->float_value);
                } else {
                    last_typed_result_ =
                        TypedValue(member_var_ptr->value,
                                   InferredType(member_var_ptr->type, ""));
                    return member_var_ptr->value;
                }
            }

            // 個別変数が見つからない場合は従来の方法（struct_membersから取得）
            Variable *intermediate_var =
                interpreter_.find_variable(struct_path);
            if (!intermediate_var) {
                throw std::runtime_error("Intermediate struct not found: " +
                                         struct_path);
            }

            if (intermediate_var->type != TYPE_STRUCT) {
                throw std::runtime_error(
                    "Intermediate value is not a struct: " + struct_path);
            }

            intermediate_struct = *intermediate_var;
            Variable member_var = get_struct_member_from_variable(
                intermediate_struct, member_name);

            // 型情報を設定
            if (member_var.type == TYPE_STRING) {
                last_typed_result_ = TypedValue(
                    member_var.str_value, InferredType(TYPE_STRING, "string"));
                return 0;
            } else if (member_var.type == TYPE_STRUCT) {
                last_typed_result_ =
                    TypedValue(member_var.value,
                               InferredType(TYPE_STRUCT, member_var.type_name));
                return member_var.value;
            } else if (member_var.type == TYPE_FLOAT ||
                       member_var.type == TYPE_DOUBLE ||
                       member_var.type == TYPE_QUAD) {
                // float/double/quadの場合
                InferredType float_type(member_var.type, "");
                if (member_var.type == TYPE_QUAD) {
                    last_typed_result_ =
                        TypedValue(member_var.quad_value, float_type);
                } else {
                    last_typed_result_ =
                        TypedValue(member_var.float_value, float_type);
                }
                return static_cast<int64_t>(member_var.float_value);
            } else {
                last_typed_result_ = TypedValue(
                    member_var.value, InferredType(member_var.type, ""));
                return member_var.value;
            }
        } else {
            throw std::runtime_error("Left side of nested member access "
                                     "did not evaluate to a struct");
        }
    } else if (node->left->node_type == ASTNodeType::AST_VARIABLE) {
        // 通常のstruct変数: obj.member
        var_name = node->left->name;
    } else if (node->left->node_type == ASTNodeType::AST_IDENTIFIER &&
               node->left->name == "self") {
        // selfメンバアクセス: self.member
        var_name = "self";
        debug_msg(DebugMsgId::SELF_MEMBER_ACCESS_START, member_name.c_str());

        // selfメンバーアクセスの特別処理
        std::string self_member_path = "self." + member_name;
        Variable *self_member = interpreter_.find_variable(self_member_path);
        if (self_member) {
            debug_msg(DebugMsgId::SELF_MEMBER_ACCESS_FOUND,
                      self_member_path.c_str());
            if (self_member->type == TYPE_STRING) {
                return 0; // 文字列の場合は別途処理
            }
            debug_msg(DebugMsgId::SELF_MEMBER_ACCESS_VALUE, self_member->value);
            return self_member->value;
        }
    } else if (node->left->node_type == ASTNodeType::AST_ARRAY_REF) {
        // struct配列要素: array[index].member または
        // obj.array[index].member
        std::string array_name;

        // 配列のベース名を取得（メンバーアクセスや配列アクセスの場合を考慮）
        if (node->left->left->node_type == ASTNodeType::AST_MEMBER_ACCESS ||
            node->left->left->node_type == ASTNodeType::AST_ARRAY_REF) {
            // obj.array[index].member や obj.array[i][j].member の場合
            // 完全なパスを構築: obj.array
            std::function<std::string(const ASTNode *)> build_path;
            build_path = [&](const ASTNode *n) -> std::string {
                if (n->node_type == ASTNodeType::AST_VARIABLE) {
                    return n->name;
                } else if (n->node_type == ASTNodeType::AST_MEMBER_ACCESS) {
                    std::string base = build_path(n->left.get());
                    return base + "." + n->name;
                } else if (n->node_type == ASTNodeType::AST_ARRAY_REF) {
                    // 配列アクセスの場合: base[index]
                    std::string base = build_path(n->left.get());
                    int64_t index = evaluate_expression(n->array_index.get());
                    return base + "[" + std::to_string(index) + "]";
                } else {
                    throw std::runtime_error(
                        "Unsupported node type in array member access");
                }
            };
            array_name = build_path(node->left->left.get());
        } else {
            // 単純な配列の場合: array[index].member
            array_name = node->left->left->name;
        }

        int64_t index = evaluate_expression(node->left->array_index.get());
        var_name = array_name + "[" + std::to_string(index) + "]";
    } else if (node->left->node_type == ASTNodeType::AST_FUNC_CALL) {
        // 関数呼び出し結果でのメンバーアクセス: func().member
        debug_msg(DebugMsgId::EXPR_EVAL_START, "Function call member access");

        try {
            // 関数を実行してReturnExceptionを捕捉
            evaluate_typed_expression(node->left.get());
            // 通常の戻り値の場合はエラー
            throw std::runtime_error(
                "Function did not return a struct for member access");
        } catch (const ReturnException &ret_ex) {
            // 構造体戻り値からメンバーを取得
            if (ret_ex.is_struct_array && ret_ex.struct_array_3d.size() > 0) {
                // 構造体配列の場合（将来拡張）
                throw std::runtime_error(
                    "Struct array function return member access not yet "
                    "fully supported");
            } else {
                // 単一構造体の場合
                Variable struct_var = ret_ex.struct_value;
                Variable member_var =
                    get_struct_member_from_variable(struct_var, member_name);

                if (member_var.type == TYPE_STRING) {
                    // 文字列の場合は別途処理が必要（呼び出し元で処理される）
                    TypedValue typed_result(
                        static_cast<int64_t>(0),
                        InferredType(TYPE_STRING, "string"));
                    typed_result.string_value = member_var.str_value;
                    typed_result.is_numeric_result = false;
                    last_typed_result_ = typed_result;
                    return 0;
                } else if (member_var.type == TYPE_FLOAT ||
                           member_var.type == TYPE_DOUBLE ||
                           member_var.type == TYPE_QUAD) {
                    InferredType float_type(member_var.type, "");
                    if (member_var.type == TYPE_QUAD) {
                        last_typed_result_ =
                            TypedValue(member_var.quad_value, float_type);
                    } else {
                        last_typed_result_ =
                            TypedValue(member_var.float_value, float_type);
                    }
                    return static_cast<int64_t>(member_var.float_value);
                } else {
                    return member_var.value;
                }
            }
        }
    } else if (node->left->node_type == ASTNodeType::AST_ARRAY_REF &&
               node->left->left &&
               node->left->left->node_type == ASTNodeType::AST_FUNC_CALL) {
        // 関数配列戻り値でのメンバーアクセス: func()[index].member
        debug_msg(DebugMsgId::EXPR_EVAL_START, "Function array member access");

        try {
            // 関数を実行してReturnExceptionを捕捉
            evaluate_expression(node->left->left.get());
            throw std::runtime_error("Function did not return an array for "
                                     "indexed member access");
        } catch (const ReturnException &ret_ex) {
            if (ret_ex.is_struct_array && ret_ex.struct_array_3d.size() > 0) {
                // インデックスを評価
                int64_t index =
                    evaluate_expression(node->left->array_index.get());

                // 配列境界チェック
                if (index < 0 ||
                    index >= (int64_t)ret_ex.struct_array_3d.size()) {
                    throw std::runtime_error(
                        "Array index out of bounds in function struct "
                        "array member access");
                }

                // 指定インデックスの構造体からメンバーを取得
                if (ret_ex.struct_array_3d.size() > 0 &&
                    ret_ex.struct_array_3d[0].size() > 0 &&
                    ret_ex.struct_array_3d[0][0].size() > index) {
                    Variable struct_var = ret_ex.struct_array_3d[0][0][index];
                    Variable member_var = get_struct_member_from_variable(
                        struct_var, member_name);

                    if (member_var.type == TYPE_STRING) {
                        TypedValue typed_result(
                            static_cast<int64_t>(0),
                            InferredType(TYPE_STRING, "string"));
                        typed_result.string_value = member_var.str_value;
                        typed_result.is_numeric_result = false;
                        last_typed_result_ = typed_result;
                        return 0;
                    } else if (member_var.type == TYPE_FLOAT ||
                               member_var.type == TYPE_DOUBLE ||
                               member_var.type == TYPE_QUAD) {
                        InferredType float_type(member_var.type, "");
                        if (member_var.type == TYPE_QUAD) {
                            last_typed_result_ =
                                TypedValue(member_var.quad_value, float_type);
                        } else {
                            last_typed_result_ =
                                TypedValue(member_var.float_value, float_type);
                        }
                        return static_cast<int64_t>(member_var.float_value);
                    } else {
                        return member_var.value;
                    }
                } else {
                    throw std::runtime_error("Invalid struct array structure");
                }
            } else {
                throw std::runtime_error("Function did not return a struct "
                                         "array for indexed member access");
            }
        }
    } else if (node->left->node_type == ASTNodeType::AST_UNARY_OP &&
               node->left->op == "DEREFERENCE") {
        // デリファレンスされたポインタからのメンバーアクセス: (*pp).member
        debug_msg(DebugMsgId::EXPR_EVAL_START,
                  "Pointer dereference member access");

        // デリファレンスを評価して構造体のポインタ値を取得
        int64_t ptr_value = evaluate_expression(node->left.get());

        // ポインタ値から構造体変数を取得
        Variable *struct_var = reinterpret_cast<Variable *>(ptr_value);
        if (!struct_var) {
            throw std::runtime_error(
                "Null pointer dereference in member access");
        }

        // 構造体メンバーを取得
        Variable member_var =
            get_struct_member_from_variable(*struct_var, member_name);

        if (member_var.type == TYPE_STRING) {
            TypedValue typed_result(static_cast<int64_t>(0),
                                    InferredType(TYPE_STRING, "string"));
            typed_result.string_value = member_var.str_value;
            typed_result.is_numeric_result = false;
            last_typed_result_ = typed_result;
            return 0;
        } else if (member_var.type == TYPE_FLOAT ||
                   member_var.type == TYPE_DOUBLE ||
                   member_var.type == TYPE_QUAD) {
            InferredType float_type(member_var.type, "");
            if (member_var.type == TYPE_QUAD) {
                last_typed_result_ =
                    TypedValue(member_var.quad_value, float_type);
            } else {
                last_typed_result_ =
                    TypedValue(member_var.float_value, float_type);
            }
            return static_cast<int64_t>(member_var.float_value);
        } else {
            return member_var.value;
        }
    } else {
        throw std::runtime_error("Invalid member access");
    }

    // 参照型変数の場合、参照先を取得
    Variable *base_var = interpreter_.find_variable(var_name);
    std::string actual_var_name = var_name;

    if (base_var && base_var->is_reference) {
        // 参照の場合、参照先の変数から直接メンバを取得
        debug_print("[DEBUG] Member access on reference variable: %s\n",
                    var_name.c_str());
    }

    // 個別変数として直接アクセスを試す（構造体配列の場合）
    std::string full_member_path = actual_var_name + "." + member_name;

    interpreter_.sync_struct_members_from_direct_access(actual_var_name);
    interpreter_.ensure_struct_member_access_allowed(actual_var_name,
                                                     member_name);
    Variable *member_var = interpreter_.find_variable(full_member_path);

    if (!member_var) {
        // struct_membersから探す（通常の構造体の場合）
        // 参照の場合、直接参照先から取得
        if (base_var && base_var->is_reference) {
            Variable result_member =
                get_struct_member_from_variable(*base_var, member_name);
            // 一時変数として返す必要があるため、last_typed_result_を使用
            if (result_member.type == TYPE_STRING) {
                TypedValue typed_result(static_cast<int64_t>(0),
                                        InferredType(TYPE_STRING, "string"));
                typed_result.string_value = result_member.str_value;
                typed_result.is_numeric_result = false;
                last_typed_result_ = typed_result;
                return 0;
            } else if (result_member.type == TYPE_FLOAT ||
                       result_member.type == TYPE_DOUBLE ||
                       result_member.type == TYPE_QUAD) {
                InferredType float_type(result_member.type, "");
                if (result_member.type == TYPE_QUAD) {
                    last_typed_result_ =
                        TypedValue(result_member.quad_value, float_type);
                } else {
                    last_typed_result_ =
                        TypedValue(result_member.float_value, float_type);
                }
                return static_cast<int64_t>(result_member.float_value);
            } else if (result_member.type == TYPE_STRUCT) {
                // 構造体メンバの場合、ReturnExceptionでラップして返す
                throw ReturnException(result_member);
            } else {
                return result_member.value;
            }
        }

        member_var =
            interpreter_.get_struct_member(actual_var_name, member_name);
    }

    if (!member_var) {
        throw std::runtime_error("Member not found: " + actual_var_name + "." +
                                 member_name);
    }

    if (member_var->type == TYPE_STRING) {
        // 文字列メンバは別途処理が必要（呼び出し元で処理される）
        return 0; // 文字列の場合は0を返すが、実際の文字列は別途取得される
    } else if (member_var->type == TYPE_FLOAT ||
               member_var->type == TYPE_DOUBLE ||
               member_var->type == TYPE_QUAD) {
        // float/double/quadの場合は型情報を保持
        InferredType float_type(member_var->type, "");
        if (member_var->type == TYPE_QUAD) {
            last_typed_result_ = TypedValue(member_var->quad_value, float_type);
        } else {
            last_typed_result_ =
                TypedValue(member_var->float_value, float_type);
        }
        return static_cast<int64_t>(member_var->float_value);
    }
    return member_var->value;
}
