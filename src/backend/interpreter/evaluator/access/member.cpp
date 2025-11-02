// ============================================================================
// expression_member_access_impl.cpp
// ============================================================================
// Phase 13: Expression Evaluator Refactoring - Member Access Implementation
//
// AST_MEMBER_ACCESSケースの実装を分離（568行）
// メンバアクセス: obj.member, array[index].member, self.member,
//               ネストしたアクセス、関数戻り値からのアクセス等
// ============================================================================

#include "../../../../common/ast.h"
#include "../../../../common/debug.h"
#include "../../../../common/debug_messages.h"
#include "../../../../common/type_helpers.h"
#include "../../core/interpreter.h"
#include "../core/evaluator.h"
#include "recursive_member_evaluator.h"
#include <functional>
#include <stdexcept>

int64_t ExpressionEvaluator::evaluate_member_access_impl(const ASTNode *node) {
    // メンバアクセス: obj.member または array[index].member または
    // self.member
    std::string var_name;
    std::string member_name = node->name;

    debug_print("[MEMBER_EVAL_IMPL] Entry: member_name='%s', "
                "member_chain.size=%zu, left_type=%d\n",
                member_name.c_str(), node->member_chain.size(),
                node->left ? static_cast<int>(node->left->node_type) : -1);

    // v0.11.0: Enum値へのメンバーアクセス
    // Option<int> x = Some(42); の後、x.variantやx.valueへアクセス
    if (node->left && node->left->node_type == ASTNodeType::AST_VARIABLE) {
        Variable *base_var = interpreter_.find_variable(node->left->name);
        debug_print(
            "[MEMBER_EVAL_IMPL] Checking variable '%s': found=%d, is_enum=%d\n",
            node->left->name.c_str(), base_var != nullptr,
            base_var ? base_var->is_enum : 0);

        if (base_var && base_var->is_enum) {
            debug_print("[MEMBER_EVAL_IMPL] Enum member access: member='%s', "
                        "has_associated_value=%d, associated_int_value=%lld\n",
                        member_name.c_str(), base_var->has_associated_value,
                        (long long)base_var->associated_int_value);

            if (member_name == "variant") {
                // variant名を文字列として返す
                // 文字列を返すためにlast_typed_resultを使用
                TypedValue typed_result(static_cast<int64_t>(0),
                                        InferredType(TYPE_STRING, "string"));
                typed_result.string_value = base_var->enum_variant;
                typed_result.is_numeric_result = false;
                set_last_typed_result(typed_result);
                debug_print("[MEMBER_EVAL_IMPL] Returning variant: '%s'\n",
                            base_var->enum_variant.c_str());
                return 0;
            } else if (member_name == "value") {
                // 関連値を返す
                if (base_var->has_associated_value) {
                    int64_t val = base_var->associated_int_value;
                    debug_print(
                        "[MEMBER_EVAL_IMPL] Returning associated value: %lld\n",
                        (long long)val);
                    return val;
                } else {
                    throw std::runtime_error(
                        "Enum variant '" + base_var->enum_variant +
                        "' does not have an associated value");
                }
            } else {
                throw std::runtime_error("Unknown enum member: " + member_name +
                                         ". Available: variant, value");
            }
        }
    }

    // ARROW_ACCESSまたはUNARY_OPが含まれる場合、member_chainパスをスキップ
    // これらは再帰的解決でのみ正しく処理できる
    // 再帰的にチェック：ネストした式の中にARROW/DEREFがある場合も検出
    bool has_arrow_or_deref = false;
    std::function<bool(const ASTNode *)> check_for_arrow_or_deref;
    check_for_arrow_or_deref = [&](const ASTNode *n) -> bool {
        if (!n)
            return false;
        if (n->node_type == ASTNodeType::AST_ARROW_ACCESS)
            return true;
        if (n->node_type == ASTNodeType::AST_UNARY_OP && n->op == "DEREFERENCE")
            return true;
        if (n->left)
            return check_for_arrow_or_deref(n->left.get());
        return false;
    };

    if (node->left) {
        has_arrow_or_deref = check_for_arrow_or_deref(node->left.get());
        if (has_arrow_or_deref) {
            debug_print("[MEMBER_EVAL] Left contains ARROW/DEREF (possibly "
                        "nested), will use recursive resolution\n");
        }
    }

    // ネストしたメンバーアクセスの場合（再帰的に処理）
    // ただし、ARROW_ACCESSやDEREFが含まれる場合はこのパスをスキップ
    if (!has_arrow_or_deref && !node->member_chain.empty() &&
        node->member_chain.size() > 1) {
        // ベース変数を取得
        Variable base_var;
        if (node->left->node_type == ASTNodeType::AST_VARIABLE) {
            Variable *var = interpreter_.find_variable(node->left->name);
            // v0.11.0: enum型もメンバーチェーンをサポート（将来的に）
            if (!var || (var->type != TYPE_STRUCT && !var->is_enum)) {
                throw std::runtime_error("Base variable for nested access is "
                                         "not a struct or enum: " +
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
            // v0.11.0: enum型もサポート
            if (!var || (!var->is_struct && var->type != TYPE_STRUCT &&
                         !var->is_enum)) {
                throw std::runtime_error("Base variable for nested access is "
                                         "not a struct or enum: " +
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
            if (TypeHelpers::isString(current_var.type)) {
                last_typed_result_ = TypedValue(
                    current_var.str_value, InferredType(TYPE_STRING, "string"));
                return 0;
            } else if (TypeHelpers::isPointer(current_var.type)) {
                return current_var.value;
            } else if (TypeHelpers::isFloating(current_var.type) ||
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

    // leftがAST_MEMBER_ACCESS、AST_ARRAY_REF、AST_ARROW_ACCESS、またはUNARY_OP
    // (DEREFERENCE)の場合、再帰的に解決
    debug_print("[MEMBER_EVAL] Checking recursive condition: "
                "left->node_type=%d (MEMBER_ACCESS=%d, ARRAY_REF=%d, "
                "ARROW_ACCESS=%d, UNARY_OP=%d)\n",
                static_cast<int>(node->left->node_type),
                static_cast<int>(ASTNodeType::AST_MEMBER_ACCESS),
                static_cast<int>(ASTNodeType::AST_ARRAY_REF),
                static_cast<int>(ASTNodeType::AST_ARROW_ACCESS),
                static_cast<int>(ASTNodeType::AST_UNARY_OP));

    if (node->left->node_type == ASTNodeType::AST_MEMBER_ACCESS ||
        node->left->node_type == ASTNodeType::AST_ARRAY_REF ||
        node->left->node_type == ASTNodeType::AST_ARROW_ACCESS ||
        (node->left->node_type == ASTNodeType::AST_UNARY_OP &&
         node->left->op == "DEREFERENCE")) {
        debug_msg(DebugMsgId::NESTED_MEMBER_EVAL_START,
                  "left is nested access (AST_MEMBER_ACCESS, AST_ARRAY_REF, "
                  "AST_ARROW_ACCESS, or DEREFERENCE)");

        // 再帰的なヘルパーを使用してメンバーを解決
        auto evaluate_index = [this](const ASTNode *idx_node) -> int64_t {
            return this->evaluate_expression(idx_node);
        };

        try {
            Variable *member_var_ptr =
                MemberEvaluationHelpers::resolve_nested_member_for_evaluation(
                    interpreter_, node, evaluate_index);

            if (member_var_ptr) {
                debug_msg(DebugMsgId::NESTED_MEMBER_INDIVIDUAL_VAR_FOUND,
                          member_name.c_str(), member_var_ptr->value);
                // メンバー変数が見つかった場合、それを使用
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
        } catch (const std::exception &e) {
            debug_print("[EVAL_RESOLVER_ERROR] Exception: %s\n", e.what());
            // フォールバックコードに進む
        }
    }

    // 旧コードとの互換性のため、以下の処理も残す（古い形式のネストアクセス用）
    if (node->left->node_type == ASTNodeType::AST_MEMBER_ACCESS) {
        debug_msg(DebugMsgId::NESTED_MEMBER_EVAL_START,
                  "left is AST_MEMBER_ACCESS (fallback)");
        Variable intermediate_struct;

        // 再帰的にパスを構築（フォールバック）
        std::function<std::string(const ASTNode *)> build_path;
        build_path = [&](const ASTNode *n) -> std::string {
            if (n->node_type == ASTNodeType::AST_VARIABLE) {
                return n->name;
            } else if (n->node_type == ASTNodeType::AST_MEMBER_ACCESS) {
                std::string base = build_path(n->left.get());
                return base + "." + n->name;
            } else if (n->node_type == ASTNodeType::AST_ARRAY_REF) {
                std::string base = build_path(n->left.get());
                int64_t index = evaluate_expression(n->array_index.get());
                return base + "[" + std::to_string(index) + "]";
            } else {
                throw std::runtime_error(
                    "Unsupported node type in nested member access");
            }
        };

        std::string struct_path = build_path(node->left.get());
        std::string full_member_path = struct_path + "." + member_name;

        // 個別変数を直接検索
        Variable *member_var_ptr = interpreter_.find_variable(full_member_path);
        if (member_var_ptr) {
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

        // 個別変数が見つからない場合はstruct_membersから取得
        Variable *intermediate_var = interpreter_.find_variable(struct_path);
        if (intermediate_var && intermediate_var->type == TYPE_STRUCT) {
            Variable member_var =
                get_struct_member_from_variable(*intermediate_var, member_name);

            if (TypeHelpers::isString(member_var.type)) {
                last_typed_result_ = TypedValue(
                    member_var.str_value, InferredType(TYPE_STRING, "string"));
                return 0;
            } else if (TypeHelpers::isStruct(member_var.type)) {
                last_typed_result_ =
                    TypedValue(member_var.value,
                               InferredType(TYPE_STRUCT, member_var.type_name));
                return member_var.value;
            } else if (TypeHelpers::isFloating(member_var.type) ||
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
                last_typed_result_ = TypedValue(
                    member_var.value, InferredType(member_var.type, ""));
                return member_var.value;
            }
        }

        throw std::runtime_error(
            "Nested member access failed: intermediate struct not found");
    }

    if (node->left->node_type == ASTNodeType::AST_VARIABLE) {
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
    } else if (node->left->node_type == ASTNodeType::AST_ARRAY_REF &&
               node->left->left &&
               node->left->left->node_type == ASTNodeType::AST_FUNC_CALL) {
        // 関数配列戻り値でのメンバーアクセス: func()[index].member
        debug_msg(DebugMsgId::EXPR_EVAL_START,
                  "Function array member access (pre-array branch)");

        try {
            evaluate_expression(node->left->left.get());
            throw std::runtime_error("Function did not return an array for "
                                     "indexed member access");
        } catch (const ReturnException &ret_ex) {
            if (ret_ex.is_struct_array && !ret_ex.struct_array_3d.empty() &&
                !ret_ex.struct_array_3d[0].empty()) {
                const auto &array_2d = ret_ex.struct_array_3d[0];
                int64_t index =
                    evaluate_expression(node->left->array_index.get());

                const auto &struct_list = array_2d[0];
                if (index < 0 ||
                    index >= static_cast<int64_t>(struct_list.size())) {
                    throw std::runtime_error(
                        "Array index out of bounds in function struct "
                        "array member access");
                }

                Variable struct_var = struct_list[index];
                Variable member_var =
                    get_struct_member_from_variable(struct_var, member_name);

                if (TypeHelpers::isString(member_var.type)) {
                    TypedValue typed_result(
                        static_cast<int64_t>(0),
                        InferredType(TYPE_STRING, "string"));
                    typed_result.string_value = member_var.str_value;
                    typed_result.is_numeric_result = false;
                    last_typed_result_ = typed_result;
                    return 0;
                } else if (TypeHelpers::isFloating(member_var.type) ||
                           member_var.type == TYPE_DOUBLE ||
                           member_var.type == TYPE_QUAD) {
                    InferredType float_type(member_var.type, "");
                    if (member_var.type == TYPE_QUAD) {
                        last_typed_result_ =
                            TypedValue(member_var.quad_value, float_type);
                        return static_cast<int64_t>(
                            last_typed_result_.quad_value);
                    } else if (member_var.type == TYPE_DOUBLE) {
                        last_typed_result_ =
                            TypedValue(member_var.double_value, float_type);
                        return static_cast<int64_t>(
                            last_typed_result_.double_value);
                    } else {
                        last_typed_result_ =
                            TypedValue(member_var.float_value, float_type);
                        return static_cast<int64_t>(
                            last_typed_result_.double_value);
                    }
                } else if (TypeHelpers::isStruct(member_var.type)) {
                    throw ReturnException(member_var);
                } else {
                    last_typed_result_ = TypedValue(
                        member_var.value, InferredType(member_var.type, ""));
                    return member_var.value;
                }
            }

            throw std::runtime_error(
                "Function did not return a struct array for "
                "indexed member access");
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

                if (TypeHelpers::isString(member_var.type)) {
                    // 文字列の場合は別途処理が必要（呼び出し元で処理される）
                    TypedValue typed_result(
                        static_cast<int64_t>(0),
                        InferredType(TYPE_STRING, "string"));
                    typed_result.string_value = member_var.str_value;
                    typed_result.is_numeric_result = false;
                    last_typed_result_ = typed_result;
                    return 0;
                } else if (TypeHelpers::isFloating(member_var.type) ||
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
    } else if (node->left->node_type == ASTNodeType::AST_UNARY_OP &&
               node->left->op == "DEREFERENCE") {
        // デリファレンスされたポインタからのメンバーアクセス: (*pp).member
        debug_msg(DebugMsgId::EXPR_EVAL_START,
                  "Pointer dereference member access");

        // デリファレンスを型情報付きで評価
        TypedValue deref_result = evaluate_typed_expression(node->left.get());

        debug_print("[DEREF_MEMBER] deref_result: type=%d, value=%lld\n",
                    static_cast<int>(deref_result.type.type_info),
                    (long long)deref_result.value);

        // 構造体ポインタのデリファレンスの場合
        if (deref_result.type.type_info == TYPE_STRUCT) {
            debug_print("[DEREF_MEMBER] Struct pointer dereference detected\n");
            // 生メモリポインタの場合、valueフィールドにアドレスがある
            void *base_ptr = reinterpret_cast<void *>(deref_result.value);
            if (!base_ptr) {
                throw std::runtime_error(
                    "Null pointer dereference in member access");
            }

            // 構造体定義を取得
            const StructDefinition *struct_def =
                interpreter_.find_struct_definition(
                    deref_result.type.type_name);
            if (!struct_def) {
                throw std::runtime_error("Struct definition not found: " +
                                         deref_result.type.type_name);
            }

            // 構造体定義からメンバーを取得
            const std::vector<StructMember> &members = struct_def->members;

            // メンバーのオフセットを計算
            size_t offset = 0;
            TypeInfo member_type = TYPE_UNKNOWN;
            bool found = false;

            for (const auto &member : members) {
                if (member.name == member_name) {
                    member_type = member.type;
                    found = true;
                    break;
                }
                // メンバーのサイズを加算
                if (member.type == TYPE_INT || member.type == TYPE_LONG ||
                    member.type == TYPE_POINTER) {
                    offset += sizeof(int64_t);
                } else if (member.type == TYPE_FLOAT) {
                    offset += sizeof(float);
                } else if (member.type == TYPE_DOUBLE) {
                    offset += sizeof(double);
                } else {
                    throw std::runtime_error(
                        "Unsupported member type in dereference access: " +
                        std::string(type_info_to_string_basic(member.type)));
                }
            }

            if (!found) {
                throw std::runtime_error("Member not found: " + member_name);
            }

            // 生メモリから値を読み取り
            void *member_ptr = static_cast<char *>(base_ptr) + offset;

            if (member_type == TYPE_INT || member_type == TYPE_LONG) {
                int64_t *int_ptr = static_cast<int64_t *>(member_ptr);
                return *int_ptr;
            } else if (member_type == TYPE_FLOAT) {
                float *float_ptr = static_cast<float *>(member_ptr);
                InferredType float_type(TYPE_FLOAT, "float");
                last_typed_result_ =
                    TypedValue(static_cast<double>(*float_ptr), float_type);
                return static_cast<int64_t>(*float_ptr);
            } else if (member_type == TYPE_DOUBLE) {
                double *double_ptr = static_cast<double *>(member_ptr);
                InferredType double_type(TYPE_DOUBLE, "double");
                last_typed_result_ = TypedValue(*double_ptr, double_type);
                return static_cast<int64_t>(*double_ptr);
            } else if (member_type == TYPE_POINTER) {
                int64_t *ptr_ptr = static_cast<int64_t *>(member_ptr);
                return *ptr_ptr;
            } else {
                throw std::runtime_error(
                    "Unsupported member type in dereference access: " +
                    std::string(type_info_to_string_basic(member_type)));
            }
        }

        // 従来の方式（変数ポインタ）
        int64_t ptr_value = deref_result.value;
        Variable *struct_var = reinterpret_cast<Variable *>(ptr_value);
        if (!struct_var) {
            throw std::runtime_error(
                "Null pointer dereference in member access");
        }

        // 構造体メンバーを取得
        Variable member_var =
            get_struct_member_from_variable(*struct_var, member_name);

        if (TypeHelpers::isString(member_var.type)) {
            TypedValue typed_result(static_cast<int64_t>(0),
                                    InferredType(TYPE_STRING, "string"));
            typed_result.string_value = member_var.str_value;
            typed_result.is_numeric_result = false;
            last_typed_result_ = typed_result;
            return 0;
        } else if (TypeHelpers::isStruct(member_var.type)) {
            // メンバーが構造体の場合、その構造体へのポインタを返す
            // これにより、(*ptr).val.x のようなパターンをサポート
            // v0.13.1: 参照がある場合はそれを使用
            auto& members = struct_var->get_struct_members();
            auto member_it = members.find(member_name);
            if (member_it != members.end()) {
                return reinterpret_cast<int64_t>(&member_it->second);
            }
            // フォールバック: member_varのvalueを返す（構造体の場合は通常0）
            return member_var.value;
        } else if (TypeHelpers::isFloating(member_var.type) ||
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

    // v0.10.0: 参照型変数の場合、参照先を取得
    // TODO v0.10.1: メンバーアクセスでの参照解決を完全実装
    // 現在find_variable()は参照を辿るが、actual_var_nameの更新が必要
    std::string actual_var_name = var_name;

    // 実際の変数を取得（find_variable()は参照チェーンを辿る）
    Variable *base_var = interpreter_.find_variable(var_name);

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
            if (TypeHelpers::isString(result_member.type)) {
                TypedValue typed_result(static_cast<int64_t>(0),
                                        InferredType(TYPE_STRING, "string"));
                typed_result.string_value = result_member.str_value;
                typed_result.is_numeric_result = false;
                last_typed_result_ = typed_result;
                return 0;
            } else if (TypeHelpers::isFloating(result_member.type) ||
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
            } else if (TypeHelpers::isStruct(result_member.type)) {
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
