#include "evaluator/core/evaluator.h"
#include "../../../../common/debug.h"
#include "../../../../common/debug_messages.h"
#include "../../../../common/utf8_utils.h"
#include "../../common/type_helpers.h"
#include "../../core/error_handler.h"
#include "../../core/interpreter.h"
#include "../../core/pointer_metadata.h" // ポインタメタデータシステム
#include "../../managers/arrays/manager.h"
#include "../../managers/types/enums.h"   // EnumManager定義が必要
#include "../../managers/types/manager.h" // TypeManager定義が必要
#include "../../services/array_processing_service.h"
#include "../../services/debug_service.h"
#include "evaluator/access/address_ops.h" // アドレス演算子と間接参照演算子のヘルパー
#include "evaluator/access/array.h" // 配列アクセスのヘルパー
#include "evaluator/access/member_helpers.h" // メンバーアクセス関連のヘルパー
#include "evaluator/access/receiver_resolution.h" // Phase 12: Method Receiver Resolution
#include "evaluator/access/special.h" // 特殊アクセス（アロー、メンバー配列、Enum）のヘルパー
#include "evaluator/core/dispatcher.h" // Phase 13: Expression Dispatcher
#include "evaluator/core/helpers.h" // Tier 2リファクタリング: ヘルパー関数群
#include "evaluator/functions/call.h" // 関数呼び出しのヘルパー
#include "evaluator/literals/eval.h" // リテラル評価（数値、文字列、nullptr、変数）のヘルパー
#include "evaluator/operators/assignment.h" // 代入演算子のヘルパー
#include "evaluator/operators/binary_unary.h" // 二項/単項演算子（typed版）のヘルパー
#include "evaluator/operators/incdec.h" // インクリメント/デクリメントのヘルパー
#include "evaluator/operators/ternary.h" // 三項演算子（?:）のヘルパー
#include <cstdio>
#include <functional>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <stdexcept>

ExpressionEvaluator::ExpressionEvaluator(Interpreter &interpreter)
    : interpreter_(interpreter), type_engine_(interpreter),
      last_typed_result_(static_cast<int64_t>(0), InferredType()),
      last_captured_function_value_(std::nullopt) {}

ExpressionEvaluator::~ExpressionEvaluator() = default;

// ============================================================================
// evaluate_expression - 式評価のメインメソッド
// ============================================================================
// このメソッドは3,933行の巨大switch文です。
// 全ての式（リテラル、変数、演算子、関数呼び出し等）の評価を担当します。
//
// 【主なセクション】:
// - Line 86-88:    リテラル値（NUMBER, NULLPTR, STRING_LITERAL）
// - Line 89-169:   変数参照（IDENTIFIER, VARIABLE）
// - Line 170-519:  配列アクセス・リテラル（ARRAY_REF, ARRAY_LITERAL）
// - Line 520-668:  二項演算子（BINARY_OP: +, -, *, /, %, <, >, ==, &&, || 等）
// - Line 669-681:  三項演算子（TERNARY_OP: ? : ）
// - Line 682-958:  単項演算子（UNARY_OP: !, -, ~, ADDRESS_OF, DEREFERENCE）
// - Line 959-1438: インクリメント/デクリメント（PRE_INCDEC, POST_INCDEC）
// - Line 1439-1587: 関数ポインタ呼び出し（FUNC_PTR_CALL）
// - Line 1588-3135: 関数呼び出し（FUNC_CALL）
// - Line 3136-3297: 代入式（ASSIGN）
// - Line 3298-3744: メンバーアクセス（MEMBER_ACCESS）
// - Line 3745-3795: アロー演算子（ARROW_ACCESS: ptr->member）
// - Line 3796-3897: メンバー配列アクセス（MEMBER_ARRAY_ACCESS）
// - Line 3898-3903: 構造体リテラル（STRUCT_LITERAL）
// - Line 3904-3929: Enum値アクセス（ENUM_ACCESS）
//
// 【TODO - 将来の改善】:
// このメソッドは以下のように分割すべき:
// 1. evaluate_literal() - リテラル値評価
// 2. evaluate_variable() - 変数参照評価
// 3. evaluate_array_access() - 配列アクセス評価
// 4. evaluate_binary_operation() - 二項演算評価
// 5. evaluate_unary_operation() - 単項演算評価
// 6. evaluate_function_call() - 関数呼び出し評価
// 7. evaluate_member_access() - メンバーアクセス評価
//
// パーサーリファクタリング（parseStatement: 1,452行→64行）の成功例を参考に、
// 段階的な分割を検討すべき。
// ============================================================================

int64_t ExpressionEvaluator::evaluate_expression(const ASTNode *node) {
    // Phase 13: Expression Dispatcherへの完全委譲
    // 巨大なswitch文(322行)をExpressionDispatcherクラスに移動
    // これにより、expression_evaluator.cppが大幅に簡素化される
    ExpressionDispatcher dispatcher(*this);
    return dispatcher.dispatch_expression(node);
}
// ============================================================================
// evaluate_expression メソッド終了
// ============================================================================

// 型推論対応の式評価
TypedValue ExpressionEvaluator::evaluate_typed_expression(const ASTNode *node) {
    if (!node) {
        return TypedValue(static_cast<int64_t>(0), InferredType());
    }

    debug_msg(DebugMsgId::TYPED_EVAL_ENTRY, static_cast<int>(node->node_type));

    // ReturnExceptionをキャッチして構造体を処理
    try {
        return evaluate_typed_expression_internal(node);
    } catch (const ReturnException &ret_ex) {
        if (debug_mode) {
            debug_msg(DebugMsgId::GENERIC_DEBUG,
                      "TYPED_EVAL_RETURN: is_struct=%d type=%d is_array=%d ");
        }
        if (ret_ex.is_function_pointer) {
            // 関数ポインタの場合、ReturnExceptionを再スロー
            if (debug_mode) {
                std::cerr << "[TYPED_EVAL] Re-throwing function pointer "
                             "ReturnException"
                          << std::endl;
            }
            throw;
        }

        if (ret_ex.is_struct || TypeHelpers::isStruct(ret_ex.type)) {
            // 構造体の場合、ReturnExceptionを再スロー（メンバアクセスで処理される）
            throw;
        }

        if (ret_ex.is_array) {
            // 配列戻り値は呼び出し元で特別処理する
            throw;
        }

        if (TypeHelpers::isString(ret_ex.type)) {
            return TypedValue(ret_ex.str_value,
                              InferredType(TYPE_STRING, "string"));
        }

        if (ret_ex.type == TYPE_FLOAT) {
            return TypedValue(ret_ex.double_value,
                              InferredType(TYPE_FLOAT, "float"));
        }
        if (ret_ex.type == TYPE_DOUBLE) {
            return TypedValue(ret_ex.double_value,
                              InferredType(TYPE_DOUBLE, "double"));
        }
        if (ret_ex.type == TYPE_QUAD) {
            return TypedValue(ret_ex.quad_value,
                              InferredType(TYPE_QUAD, "quad"));
        }

        // 通常の数値の場合（ポインタを含む）
        TypedValue tv(
            ret_ex.value,
            InferredType(ret_ex.type,
                         ExpressionHelpers::type_info_to_string(ret_ex.type)));

        // ポインタの場合、const情報を保持する（Phase 2: v0.9.2）
        if (ret_ex.type == TYPE_POINTER || ret_ex.is_pointer) {
            tv.is_pointer = true;
            tv.is_pointee_const = ret_ex.is_pointee_const;
            tv.is_pointer_const = ret_ex.is_pointer_const;
            tv.pointer_depth = ret_ex.pointer_depth;
            tv.pointer_base_type = ret_ex.pointer_base_type;
            tv.pointer_base_type_name = ret_ex.pointer_base_type_name;
        }

        return tv;
    }
}

// 実際の型推論対応の式評価（内部実装）
TypedValue
ExpressionEvaluator::evaluate_typed_expression_internal(const ASTNode *node) {
    if (!node) {
        return TypedValue(static_cast<int64_t>(0), InferredType());
    }

    debug_msg(DebugMsgId::TYPED_EVAL_INTERNAL_ENTRY,
              static_cast<int>(node->node_type));

    // まず型を推論
    InferredType inferred_type = type_engine_.infer_type(node);

    switch (node->node_type) {
    case ASTNodeType::AST_TERNARY_OP:
        return evaluate_ternary_typed(node);

    case ASTNodeType::AST_STRING_LITERAL: {
        // 文字列リテラルの評価はLiteralEvalHelpersに移動（6行）
        return LiteralEvalHelpers::evaluate_string_literal_typed(node,
                                                                 inferred_type);
    }

    case ASTNodeType::AST_INTERPOLATED_STRING: {
        // v0.11.0 補間文字列の評価
        return evaluate_interpolated_string(node);
    }

    case ASTNodeType::AST_NUMBER: {
        // 数値リテラルの評価はLiteralEvalHelpersに移動（18行）
        return LiteralEvalHelpers::evaluate_number_literal_typed(node,
                                                                 inferred_type);
    }

    case ASTNodeType::AST_NULLPTR: {
        // nullptrの評価はLiteralEvalHelpersに移動（4行）
        return LiteralEvalHelpers::evaluate_nullptr_literal_typed();
    }

    case ASTNodeType::AST_CAST_EXPR: {
        // キャスト式の評価
        int64_t value = evaluate_expression(node);

        // string型へのキャストの場合、ポインタ値を保持する
        if (node->cast_type_info == TYPE_STRING) {
            TypedValue result(static_cast<int64_t>(0),
                              InferredType(TYPE_STRING, "string"));
            result.value = value;     // ポインタ値を保存
            result.string_value = ""; // 空の文字列
            return result;
        }

        // その他のキャストは通常の処理
        return consume_numeric_typed_value(node, value, inferred_type);
    }

    case ASTNodeType::AST_BINARY_OP: {
        // 二項演算子の評価（typed版）はBinaryUnaryTypedHelpersに移動（318行）
        auto evaluate_typed_lambda = [this](const ASTNode *n) {
            return this->evaluate_typed_expression(n);
        };
        return BinaryUnaryTypedHelpers::evaluate_binary_op_typed(
            node, interpreter_, inferred_type, evaluate_typed_lambda);
    }

    case ASTNodeType::AST_UNARY_OP: {
        // 単項演算子の評価（typed版）はBinaryUnaryTypedHelpersに移動（177行）
        auto evaluate_typed_lambda = [this](const ASTNode *n) {
            return this->evaluate_typed_expression(n);
        };
        auto evaluate_expression_lambda = [this](const ASTNode *n) {
            return this->evaluate_expression(n);
        };
        return BinaryUnaryTypedHelpers::evaluate_unary_op_typed(
            node, interpreter_, inferred_type, evaluate_typed_lambda,
            evaluate_expression_lambda);
    }

    case ASTNodeType::AST_ARRAY_LITERAL: {
        // 配列リテラルの場合、プレースホルダーとして0を返し、型情報を保持
        InferredType array_type = type_engine_.infer_type(node);
        return TypedValue(static_cast<int64_t>(0), array_type);
    }

    case ASTNodeType::AST_FUNC_CALL: {
        // 関数呼び出しの場合、型推論を使って正確な型を決定
        try {
            // まず関数の戻り値型を推論
            InferredType function_return_type =
                type_engine_.infer_function_return_type(node->name, {});

            // array_get_double は特別扱い: ビット表現からdoubleに変換
            if (node->name == "array_get_double") {
                int64_t bits = evaluate_expression(node);
                union {
                    double d;
                    int64_t i;
                } converter;
                converter.i = bits;
                return TypedValue(converter.d,
                                  InferredType(TYPE_DOUBLE, "double"));
            }

            // 関数を実行して結果を取得
            int64_t numeric_result = evaluate_expression(node);

            // 推論された型に基づいて適切なTypedValueを返す
            if (function_return_type.type_info == TYPE_STRING) {
                // 文字列戻り値の場合（実際の文字列は evaluate_expression
                // では取得困難）
                return TypedValue("", InferredType(TYPE_STRING, "string"));
            } else if (function_return_type.type_info == TYPE_STRUCT) {
                // 構造体戻り値の場合は例外をキャッチして処理
                throw std::runtime_error(
                    "Struct return should be caught as exception");
            } else {
                // 数値戻り値の場合
                return consume_numeric_typed_value(node, numeric_result,
                                                   function_return_type);
            }
        } catch (const ReturnException &ret) {
            if (ret.is_function_pointer) {
                // 関数ポインタの場合、ReturnExceptionを再スロー
                throw;
            }
            if (ret.is_array || ret.is_struct_array) {
                throw;
            }
            if (ret.is_struct || TypeHelpers::isStruct(ret.type)) {
                // 構造体の場合
                Variable struct_var = ret.struct_value;
                InferredType struct_type(TYPE_STRUCT,
                                         struct_var.struct_type_name);
                return TypedValue(struct_var, struct_type);
            } else if (TypeHelpers::isString(ret.type)) {
                return TypedValue(ret.str_value,
                                  InferredType(TYPE_STRING, "string"));
            } else if (ret.type == TYPE_FLOAT) {
                return TypedValue(ret.double_value,
                                  InferredType(TYPE_FLOAT, "float"));
            } else if (ret.type == TYPE_DOUBLE) {
                return TypedValue(ret.double_value,
                                  InferredType(TYPE_DOUBLE, "double"));
            } else if (ret.type == TYPE_QUAD) {
                return TypedValue(ret.quad_value,
                                  InferredType(TYPE_QUAD, "quad"));
            } else {
                return TypedValue(
                    ret.value,
                    InferredType(
                        ret.type,
                        ExpressionHelpers::type_info_to_string(ret.type)));
            }
        }
    }

    case ASTNodeType::AST_VARIABLE: {
        // 変数参照の評価はLiteralEvalHelpersに移動（66行）
        return LiteralEvalHelpers::evaluate_variable_typed(node, interpreter_,
                                                           inferred_type);
    }

    // ========================================================================
    // 無名変数（DISCARD_VARIABLE）v0.10.0新機能
    // 無名変数の参照は許可されない
    // ========================================================================
    case ASTNodeType::AST_DISCARD_VARIABLE: {
        throw std::runtime_error("Cannot reference discard variable '_'");
    }

    // ========================================================================
    // 無名関数（LAMBDA_EXPR）v0.10.0新機能
    // 無名関数を評価して関数ポインタとして返す
    // ========================================================================
    case ASTNodeType::AST_LAMBDA_EXPR: {
        // 無名関数を通常の関数として登録
        // 1. 内部識別子を使用して関数として登録
        std::string lambda_name = node->internal_name;

        // 2. ラムダ本体を関数宣言として構築
        // ラムダは既にASTノードとして存在するので、それを関数として登録
        interpreter_.register_function_to_global(lambda_name, node);

        // 3. 関数ポインタとして返す
        // ReturnExceptionを使用することで、変数宣言時に正しく処理される
        ReturnException ret(static_cast<int64_t>(0));
        ret.is_function_pointer = true;
        ret.function_pointer_name = lambda_name;
        ret.function_pointer_node = node;
        ret.type = node->lambda_return_type;
        throw ret;
    }

    case ASTNodeType::AST_MEMBER_ACCESS: {
        debug_msg(DebugMsgId::TYPED_MEMBER_ACCESS_CASE, node->name.c_str(),
                  node->member_chain.size());

        // 修飾アクセスのチェック: module.constant
        if (node->left && node->left->node_type == ASTNodeType::AST_VARIABLE) {
            std::string potential_module = node->left->name;
            bool is_variable =
                (interpreter_.find_variable(potential_module) != nullptr);
            bool is_module = interpreter_.is_module_imported(potential_module);

            if (!is_variable && is_module) {
                // module.name の形式で変数を検索
                std::string qualified_name =
                    potential_module + "." + node->name;
                Variable *var = interpreter_.find_variable(qualified_name);
                if (var) {
                    if (debug_mode) {
                        std::cerr << "[QUALIFIED_ACCESS] Found variable: "
                                  << qualified_name << std::endl;
                    }

                    // 変数の型に応じてTypedValueを返す
                    if (var->type == TYPE_STRING) {
                        // mallocで取得したポインタを文字列に変換
                        if (var->str_value.empty() && var->value != 0) {
                            const char *ptr =
                                reinterpret_cast<const char *>(var->value);
                            return TypedValue(
                                std::string(ptr),
                                InferredType(TYPE_STRING, "string"));
                        }
                        return TypedValue(var->str_value,
                                          InferredType(TYPE_STRING, "string"));
                    } else if (var->type == TYPE_FLOAT) {
                        return TypedValue(static_cast<double>(var->float_value),
                                          InferredType(TYPE_FLOAT, "float"));
                    } else if (var->type == TYPE_DOUBLE) {
                        return TypedValue(var->double_value,
                                          InferredType(TYPE_DOUBLE, "double"));
                    } else if (var->type == TYPE_QUAD) {
                        return TypedValue(var->quad_value,
                                          InferredType(TYPE_QUAD, "quad"));
                    } else if (var->type == TYPE_STRUCT || var->is_struct) {
                        return TypedValue(
                            *var,
                            InferredType(TYPE_STRUCT, var->struct_type_name));
                    } else if (var->is_enum) {
                        // v0.11.0: enum型を返す
                        // 古いスタイルのenum（整数値）の場合はTYPE_ENUMとして返す
                        if (var->has_associated_value) {
                            return TypedValue(
                                *var,
                                InferredType(TYPE_ENUM, var->enum_type_name));
                        } else {
                            // 古いスタイルenum: 整数値を返す
                            return TypedValue(
                                var->value,
                                InferredType(TYPE_ENUM, var->enum_type_name));
                        }
                    } else {
                        return TypedValue(
                            var->value,
                            InferredType(var->type,
                                         ::type_info_to_string(var->type)));
                    }
                }
            }
        }

        // member_chainが2つ以上ある場合（ネストメンバアクセス）
        if (!node->member_chain.empty() && node->member_chain.size() > 1) {
            // ベース変数を取得
            Variable base_var;
            if (node->left->node_type == ASTNodeType::AST_VARIABLE) {
                Variable *var = interpreter_.find_variable(node->left->name);
                // v0.11.0: enum型も許可
                if (!var || (!var->is_struct && var->type != TYPE_STRUCT &&
                             !var->is_enum)) {
                    throw std::runtime_error("Base variable for nested access "
                                             "is not a struct or enum: " +
                                             node->left->name);
                }
                base_var = *var;
            } else {
                throw std::runtime_error(
                    "Complex base types for nested access not yet supported in "
                    "typed evaluation");
            }

            // 再帰的にメンバーチェーンをたどる
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

            // 最終的な値をTypedValueとして返す
            if (TypeHelpers::isString(current_var.type)) {
                return TypedValue(current_var.str_value,
                                  InferredType(TYPE_STRING, "string"));
            } else if (TypeHelpers::isStruct(current_var.type)) {
                return TypedValue(
                    current_var,
                    InferredType(TYPE_STRUCT, current_var.struct_type_name));
            } else if (current_var.type == TYPE_FLOAT) {
                return TypedValue(static_cast<double>(current_var.float_value),
                                  InferredType(TYPE_FLOAT, "float"));
            } else if (current_var.type == TYPE_DOUBLE) {
                return TypedValue(current_var.double_value,
                                  InferredType(TYPE_DOUBLE, "double"));
            } else if (current_var.type == TYPE_QUAD) {
                return TypedValue(current_var.quad_value,
                                  InferredType(TYPE_QUAD, "quad"));
            } else {
                return TypedValue(
                    current_var.value,
                    InferredType(current_var.type,
                                 ExpressionHelpers::type_info_to_string(
                                     current_var.type)));
            }
        }

        auto convert_member_to_typed = [&](const Variable &member_var,
                                           TypedValue &out) -> bool {
            switch (member_var.type) {
            case TYPE_STRING:
                out = TypedValue(member_var.str_value,
                                 InferredType(TYPE_STRING, "string"));
                return true;
            case TYPE_FLOAT:
                out = TypedValue(static_cast<double>(member_var.float_value),
                                 InferredType(TYPE_FLOAT, "float"));
                return true;
            case TYPE_DOUBLE:
                out = TypedValue(member_var.double_value,
                                 InferredType(TYPE_DOUBLE, "double"));
                return true;
            case TYPE_QUAD:
                out = TypedValue(member_var.quad_value,
                                 InferredType(TYPE_QUAD, "quad"));
                return true;
            case TYPE_STRUCT:
                out = TypedValue(
                    member_var,
                    InferredType(TYPE_STRUCT, member_var.struct_type_name));
                return true;
            case TYPE_UNION: {
                TypeInfo active = member_var.current_type;
                if (active == TYPE_STRING) {
                    out = TypedValue(member_var.str_value,
                                     InferredType(TYPE_STRING, "string"));
                    return true;
                }
                if (active == TYPE_FLOAT) {
                    out =
                        TypedValue(static_cast<double>(member_var.float_value),
                                   InferredType(TYPE_FLOAT, "float"));
                    return true;
                }
                if (active == TYPE_DOUBLE) {
                    out = TypedValue(member_var.double_value,
                                     InferredType(TYPE_DOUBLE, "double"));
                    return true;
                }
                if (active == TYPE_QUAD) {
                    out = TypedValue(member_var.quad_value,
                                     InferredType(TYPE_QUAD, "quad"));
                    return true;
                }
                if (active != TYPE_UNKNOWN) {
                    out = TypedValue(
                        member_var.value,
                        InferredType(
                            active,
                            ExpressionHelpers::type_info_to_string(active)));
                    return true;
                }
                break;
            }
            default:
                out = TypedValue(
                    member_var.value,
                    InferredType(member_var.type,
                                 ExpressionHelpers::type_info_to_string(
                                     member_var.type)));
                return true;
            }
            return false;
        };

        auto resolve_from_struct = [&](const Variable &struct_var,
                                       TypedValue &out) -> bool {
            try {
                Variable member_var =
                    get_struct_member_from_variable(struct_var, node->name);
                return convert_member_to_typed(member_var, out);
            } catch (const std::exception &) {
                return false;
            }
        };

        std::function<std::string(const ASTNode *)> build_base_name =
            [&](const ASTNode *base) -> std::string {
            if (!base) {
                return "";
            }
            switch (base->node_type) {
            case ASTNodeType::AST_VARIABLE:
            case ASTNodeType::AST_IDENTIFIER:
                return base->name;
            case ASTNodeType::AST_ARRAY_REF:
                return interpreter_.extract_array_element_name(base);
            case ASTNodeType::AST_MEMBER_ACCESS: {
                std::string prefix = build_base_name(base->left.get());
                if (prefix.empty()) {
                    return "";
                }
                return prefix + "." + base->name;
            }
            default:
                return "";
            }
        };

        auto resolve_from_base_name = [&](const std::string &base_name,
                                          TypedValue &out) -> bool {
            if (base_name.empty()) {
                return false;
            }

            // Check if base is an enum variable
            if (Variable *base_var = interpreter_.find_variable(base_name)) {
                if (base_var->is_enum) {
                    std::string member_name = node->name;

                    if (member_name == "variant") {
                        // Return the variant name as a string
                        out = TypedValue(base_var->enum_variant,
                                         InferredType(TYPE_STRING, "string"));
                        out.is_numeric_result = false;
                        return true;
                    } else if (member_name == "value") {
                        if (base_var->has_associated_value) {
                            // 文字列型の関連値の場合
                            if (!base_var->associated_str_value.empty()) {
                                out = TypedValue(
                                    base_var->associated_str_value,
                                    InferredType(TYPE_STRING, "string"));
                                out.is_numeric_result = false;
                                return true;
                            }
                            // 数値型の関連値の場合
                            else {
                                out = TypedValue(base_var->associated_int_value,
                                                 InferredType(TYPE_INT, "int"));
                                return true;
                            }
                        } else {
                            throw std::runtime_error(
                                "Enum variant '" + base_var->enum_variant +
                                "' has no associated value");
                        }
                    } else {
                        throw std::runtime_error("Unknown enum member: " +
                                                 member_name);
                    }
                }
            }

            try {
                interpreter_.sync_struct_members_from_direct_access(base_name);
                interpreter_.ensure_struct_member_access_allowed(base_name,
                                                                 node->name);
            } catch (const std::exception &) {
                // best effort even if sync fails
            }

            std::string member_path = base_name + "." + node->name;
            if (Variable *direct_member =
                    interpreter_.find_variable(member_path)) {
                if (convert_member_to_typed(*direct_member, out)) {
                    return true;
                }
            }

            try {
                if (Variable *member_var =
                        interpreter_.get_struct_member(base_name, node->name)) {
                    if (convert_member_to_typed(*member_var, out)) {
                        return true;
                    }
                }
            } catch (const std::exception &) {
            }

            return false;
        };

        // (*ptr).member パターンをチェック（構造体ポインタのデリファレンス）
        if (node->left && node->left->node_type == ASTNodeType::AST_UNARY_OP &&
            node->left->op == "DEREFERENCE") {

            // デリファレンスの結果を取得
            TypedValue deref_value =
                evaluate_typed_expression(node->left.get());

            // 構造体の場合
            if (deref_value.is_struct() && deref_value.struct_data) {
                Variable struct_var = *deref_value.struct_data;
                TypedValue member_value(static_cast<int64_t>(0),
                                        InferredType());

                if (resolve_from_struct(struct_var, member_value)) {
                    last_typed_result_ = member_value;
                    return member_value;
                }
            }

            throw std::runtime_error(
                "Pointer dereference did not yield a struct");
        }

        // func()[index].member パターンをチェック
        if (node->left && node->left->node_type == ASTNodeType::AST_ARRAY_REF &&
            node->left->left &&
            node->left->left->node_type == ASTNodeType::AST_FUNC_CALL) {

            {
                char dbg_buf[512];
                snprintf(dbg_buf, sizeof(dbg_buf),
                         "Processing func()[index].member pattern: %s[].%s",
                         node->left->left->name.c_str(), node->name.c_str());
                debug_msg(DebugMsgId::GENERIC_DEBUG, dbg_buf);
            }

            try {
                (void)evaluate_typed_expression(node->left.get());
                throw std::runtime_error("Expected struct return exception");

            } catch (const ReturnException &struct_ret) {
                if (struct_ret.is_struct) {
                    TypedValue member_value(static_cast<int64_t>(0),
                                            InferredType());
                    if (resolve_from_struct(struct_ret.struct_value,
                                            member_value)) {
                        last_typed_result_ = member_value;
                        return member_value;
                    }
                }
                throw std::runtime_error(
                    "Expected struct element from function array access");
            }
        }

        // ptr[index].member パターンをチェック（ポインタの配列アクセス）
        if (node->left && node->left->node_type == ASTNodeType::AST_ARRAY_REF &&
            node->left->left &&
            node->left->left->node_type == ASTNodeType::AST_VARIABLE) {
            Variable *var = interpreter_.find_variable(node->left->left->name);
            if (var && var->is_pointer) {
                {
                    char dbg_buf[512];
                    snprintf(dbg_buf, sizeof(dbg_buf),
                             "Processing ptr[index].member pattern: %s[].%s",
                             node->left->left->name.c_str(),
                             node->name.c_str());
                    debug_msg(DebugMsgId::GENERIC_DEBUG, dbg_buf);
                }

                try {
                    // evaluate_expressionを使用してReturnExceptionを取得
                    (void)evaluate_expression(node->left.get());
                    debug_msg(DebugMsgId::GENERIC_DEBUG,
                              "ERROR: No ReturnException was thrown!");
                    throw std::runtime_error(
                        "Expected struct return exception");

                } catch (const ReturnException &struct_ret) {
                    {
                        char dbg_buf[512];
                        snprintf(dbg_buf, sizeof(dbg_buf),
                                 "Caught ReturnException, is_struct=%d",
                                 struct_ret.is_struct);
                        debug_msg(DebugMsgId::GENERIC_DEBUG, dbg_buf);
                    }
                    if (struct_ret.is_struct) {
                        TypedValue member_value(static_cast<int64_t>(0),
                                                InferredType());
                        if (resolve_from_struct(struct_ret.struct_value,
                                                member_value)) {
                            {
                                char dbg_buf[512];
                                snprintf(dbg_buf, sizeof(dbg_buf),
                                         "Successfully resolved member: %s",
                                         node->name.c_str());
                                debug_msg(DebugMsgId::GENERIC_DEBUG, dbg_buf);
                            }
                            last_typed_result_ = member_value;
                            return member_value;
                        }
                        {
                            char dbg_buf[512];
                            snprintf(dbg_buf, sizeof(dbg_buf),
                                     "Failed to resolve member: %s",
                                     node->name.c_str());
                            debug_msg(DebugMsgId::GENERIC_DEBUG, dbg_buf);
                        }
                    }
                    throw std::runtime_error(
                        "Expected struct element from pointer array access");
                }
            }
        }

        TypedValue resolved_value(static_cast<int64_t>(0), InferredType());
        bool resolved = false;

        std::string base_name = build_base_name(node->left.get());
        {
            char dbg_buf[512];
            snprintf(dbg_buf, sizeof(dbg_buf),
                     "[EVAL_TYPED] base_name='%s', member='%s'",
                     base_name.c_str(), node->name.c_str());
            debug_msg(DebugMsgId::GENERIC_DEBUG, dbg_buf);
        }
        debug_msg(DebugMsgId::NESTED_MEMBER_BASE_PATH, base_name.c_str(),
                  node->name.c_str());

        if (!base_name.empty()) {
            // まず個別変数を検索（優先）
            debug_msg(DebugMsgId::NESTED_MEMBER_RESOLVE_FROM_BASE);
            resolved = resolve_from_base_name(base_name, resolved_value);
            debug_msg(DebugMsgId::GENERIC_DEBUG,
                      "[EVAL_TYPED] resolve_from_base_name returned: %d, ");
            if (resolved) {
                debug_msg(DebugMsgId::NESTED_MEMBER_RESOLVE_SUCCESS,
                          resolved_value.is_numeric()
                              ? resolved_value.as_numeric()
                              : 0LL);
            } else {
                debug_msg(DebugMsgId::NESTED_MEMBER_RESOLVE_FAILED);

                // 個別変数が見つからない場合、struct_membersから検索
                if (Variable *base_var =
                        interpreter_.find_variable(base_name)) {
                    debug_msg(DebugMsgId::NESTED_MEMBER_BASE_VAR_FOUND,
                              base_var->type);
                    if (base_var->type == TYPE_STRUCT) {
                        resolved =
                            resolve_from_struct(*base_var, resolved_value);
                        if (resolved) {
                            debug_msg(DebugMsgId::NESTED_MEMBER_RESOLVE_SUCCESS,
                                      resolved_value.is_numeric()
                                          ? resolved_value.as_numeric()
                                          : 0LL);
                        }
                    }
                } else {
                    debug_msg(DebugMsgId::NESTED_MEMBER_BASE_VAR_NOT_FOUND);
                }
            }
        }

        if (!resolved) {
            try {
                evaluate_expression(node->left.get());
            } catch (const ReturnException &ret) {
                if (ret.is_struct) {
                    resolved =
                        resolve_from_struct(ret.struct_value, resolved_value);
                } else {
                    throw;
                }
            }
        }

        if (resolved) {
            last_typed_result_ = resolved_value;
            return resolved_value;
        }

        int64_t numeric_result = evaluate_expression(node);
        return consume_numeric_typed_value(node, numeric_result, inferred_type);
    }

    case ASTNodeType::AST_ARRAY_REF: {
        // 関数呼び出しの戻り値に対する配列アクセス: func()[index]
        if (node->left && node->left->node_type == ASTNodeType::AST_FUNC_CALL) {
            {
                char dbg_buf[512];
                snprintf(dbg_buf, sizeof(dbg_buf),
                         "Processing typed function call array access: %s",
                         node->left->name.c_str());
                debug_msg(DebugMsgId::GENERIC_DEBUG, dbg_buf);
            }

            // インデックスを評価
            int64_t index = evaluate_expression(node->array_index.get());

            try {
                // 関数を実行して戻り値を取得（副作用のため実行）
                (void)evaluate_expression(node->left.get());
                throw std::runtime_error(
                    "Function did not return an array via exception");
            } catch (const ReturnException &ret) {
                if (ret.is_array) {
                    // 構造体配列の戻り値の場合
                    if (ret.is_struct_array && !ret.struct_array_3d.empty() &&
                        !ret.struct_array_3d[0].empty() &&
                        !ret.struct_array_3d[0][0].empty()) {

                        if (index >= 0 &&
                            index < static_cast<int64_t>(
                                        ret.struct_array_3d[0][0].size())) {
                            // 構造体要素をReturnExceptionとして投げる
                            throw ReturnException(
                                ret.struct_array_3d[0][0][index]);
                        } else {
                            throw std::runtime_error(
                                "Array index out of bounds");
                        }
                    }
                    // 文字列配列の戻り値の場合
                    else if (!ret.str_array_3d.empty() &&
                             !ret.str_array_3d[0].empty() &&
                             !ret.str_array_3d[0][0].empty()) {

                        if (index >= 0 &&
                            index < static_cast<int64_t>(
                                        ret.str_array_3d[0][0].size())) {
                            return TypedValue(ret.str_array_3d[0][0][index],
                                              TYPE_STRING);
                        } else {
                            throw std::runtime_error(
                                "Array index out of bounds");
                        }
                    }
                    // 数値配列の戻り値の場合
                    else if (!ret.int_array_3d.empty() &&
                             !ret.int_array_3d[0].empty() &&
                             !ret.int_array_3d[0][0].empty()) {

                        if (index >= 0 &&
                            index < static_cast<int64_t>(
                                        ret.int_array_3d[0][0].size())) {
                            return TypedValue(ret.int_array_3d[0][0][index],
                                              TYPE_INT);
                        } else {
                            throw std::runtime_error(
                                "Array index out of bounds");
                        }
                    } else {
                        throw std::runtime_error(
                            "Empty array returned from function");
                    }
                } else {
                    throw std::runtime_error(
                        "Function does not return an array");
                }
            }
        }

        if (inferred_type.type_info == TYPE_STRING && node->left &&
            node->left->node_type == ASTNodeType::AST_MEMBER_ACCESS) {
            const ASTNode *member_node = node->left.get();
            std::string member_name = member_node->name;
            std::string object_name;

            if (member_node->left) {
                if (member_node->left->node_type == ASTNodeType::AST_VARIABLE) {
                    object_name = member_node->left->name;
                } else if (member_node->left->node_type ==
                           ASTNodeType::AST_ARRAY_REF) {
                    object_name = interpreter_.extract_array_element_name(
                        member_node->left.get());
                }
            }

            if (!object_name.empty() && node->array_index) {
                int64_t array_index =
                    evaluate_expression(node->array_index.get());
                try {
                    std::string value =
                        interpreter_.get_struct_member_array_string_element(
                            object_name, member_name,
                            static_cast<int>(array_index));
                    return TypedValue(value,
                                      InferredType(TYPE_STRING, "string"));
                } catch (const std::exception &) {
                    // フォールバックして通常処理
                }
            }
        }

        if (inferred_type.type_info == TYPE_STRING) {
            std::string array_name = interpreter_.extract_array_name(node);
            std::vector<int64_t> indices =
                interpreter_.extract_array_indices(node);

            if (!array_name.empty() && !indices.empty()) {
                bool resolved = false;
                std::string string_value;

                if (auto *array_service =
                        interpreter_.get_array_processing_service()) {
                    try {
                        string_value = array_service->getStringArrayElement(
                            array_name, indices,
                            ArrayProcessingService::ArrayContext::
                                LOCAL_VARIABLE);
                        resolved = true;
                    } catch (const std::exception &) {
                        resolved = false;
                    }
                }

                if (!resolved) {
                    if (Variable *var =
                            interpreter_.find_variable(array_name)) {
                        try {
                            if (var->is_multidimensional ||
                                !var->multidim_array_strings.empty()) {
                                string_value =
                                    interpreter_
                                        .getMultidimensionalStringArrayElement(
                                            *var, indices);
                                resolved = true;
                            } else if (!var->array_strings.empty() &&
                                       indices.size() == 1) {
                                int64_t idx = indices[0];
                                if (idx >= 0 &&
                                    idx < static_cast<int64_t>(
                                              var->array_strings.size())) {
                                    string_value =
                                        var->array_strings[static_cast<size_t>(
                                            idx)];
                                    resolved = true;
                                }
                            }
                        } catch (const std::exception &) {
                            resolved = false;
                        }
                    }
                }

                if (resolved) {
                    return TypedValue(string_value,
                                      InferredType(TYPE_STRING, "string"));
                }
            }
        }

        // 通常の配列要素アクセスの場合 - float/double配列対応
        std::string array_name = interpreter_.extract_array_name(node);
        std::vector<int64_t> indices = interpreter_.extract_array_indices(node);

        if (!array_name.empty() && !indices.empty()) {
            Variable *var = interpreter_.find_variable(array_name);
            if (var && var->is_array) {
                TypeInfo base_type =
                    (var->type >= TYPE_ARRAY_BASE)
                        ? static_cast<TypeInfo>(var->type - TYPE_ARRAY_BASE)
                        : var->type;

                // ポインタ型配列の場合（例: double*[5], int*[10]）
                if (base_type == TYPE_POINTER && indices.size() == 1) {
                    int64_t idx = indices[0];
                    if (idx >= 0 &&
                        idx < static_cast<int64_t>(var->array_values.size())) {
                        int64_t ptr_value = var->array_values[idx];

                        if (interpreter_.is_debug_mode()) {
                            std::cerr << "[POINTER_ARRAY_READ] Reading from "
                                      << array_name << "[" << idx << "]"
                                      << std::endl;
                            std::cerr << "  Value: " << ptr_value << " (0x"
                                      << std::hex << ptr_value << std::dec
                                      << ")" << std::endl;
                            std::cerr
                                << "  Has tag bit: "
                                << ((ptr_value & (1LL << 63)) ? "YES" : "NO")
                                << std::endl;
                        }

                        // ポインタ配列の要素はポインタ型として返す
                        InferredType ptr_type(TYPE_POINTER, "pointer");
                        TypedValue result(ptr_value, ptr_type);
                        result.numeric_type = TYPE_POINTER;
                        return result;
                    }
                }

                // float/double/quad配列の場合
                if (base_type == TYPE_FLOAT || base_type == TYPE_DOUBLE ||
                    base_type == TYPE_QUAD) {
                    if (var->is_multidimensional && indices.size() > 1) {
                        // 多次元配列のフラットインデックスを計算（row-major
                        // order）
                        int flat_index = 0;
                        int multiplier = 1;
                        for (int d = indices.size() - 1; d >= 0; d--) {
                            flat_index += indices[d] * multiplier;
                            if (d > 0) {
                                // 次の次元のサイズを掛ける
                                multiplier *= var->array_dimensions[d];
                            }
                        }

                        if (base_type == TYPE_FLOAT && flat_index >= 0 &&
                            flat_index <
                                static_cast<int>(
                                    var->multidim_array_float_values.size())) {
                            float f =
                                var->multidim_array_float_values[flat_index];
                            return TypedValue(
                                static_cast<double>(f),
                                InferredType(TYPE_FLOAT, "float"));
                        } else if (base_type == TYPE_DOUBLE &&
                                   flat_index >= 0 &&
                                   flat_index <
                                       static_cast<int>(
                                           var->multidim_array_double_values
                                               .size())) {
                            double d =
                                var->multidim_array_double_values[flat_index];
                            return TypedValue(
                                d, InferredType(TYPE_DOUBLE, "double"));
                        } else if (base_type == TYPE_QUAD && flat_index >= 0 &&
                                   flat_index <
                                       static_cast<int>(
                                           var->multidim_array_quad_values
                                               .size())) {
                            long double q =
                                var->multidim_array_quad_values[flat_index];
                            return TypedValue(q,
                                              InferredType(TYPE_QUAD, "quad"));
                        }
                    } else if (indices.size() == 1) {
                        // 1次元配列
                        int64_t idx = indices[0];
                        if (base_type == TYPE_FLOAT && idx >= 0 &&
                            idx < static_cast<int64_t>(
                                      var->array_float_values.size())) {
                            float f = var->array_float_values[idx];
                            return TypedValue(
                                static_cast<double>(f),
                                InferredType(TYPE_FLOAT, "float"));
                        } else if (base_type == TYPE_DOUBLE && idx >= 0 &&
                                   idx < static_cast<int64_t>(
                                             var->array_double_values.size())) {
                            double d = var->array_double_values[idx];
                            return TypedValue(
                                d, InferredType(TYPE_DOUBLE, "double"));
                        } else if (base_type == TYPE_QUAD && idx >= 0 &&
                                   idx < static_cast<int64_t>(
                                             var->array_quad_values.size())) {
                            long double q = var->array_quad_values[idx];
                            return TypedValue(q,
                                              InferredType(TYPE_QUAD, "quad"));
                        }
                    }
                }

                // 整数配列の処理（tiny, short, long, int, big）
                // tiny/short/longなど、符号付き整数型の配列要素取得
                if ((base_type == TYPE_TINY || base_type == TYPE_SHORT ||
                     base_type == TYPE_LONG || base_type == TYPE_INT ||
                     base_type == TYPE_BIG) &&
                    indices.size() == 1) {
                    int64_t idx = indices[0];

                    if (idx >= 0 &&
                        idx < static_cast<int64_t>(var->array_values.size())) {
                        int64_t raw_value = var->array_values[idx];

                        // 型に応じて符号拡張を行う
                        int64_t typed_value = raw_value;
                        switch (base_type) {
                        case TYPE_TINY: {
                            // 8ビット符号付き整数として解釈
                            int8_t tiny_val =
                                static_cast<int8_t>(raw_value & 0xFF);
                            typed_value = static_cast<int64_t>(tiny_val);
                            break;
                        }
                        case TYPE_SHORT: {
                            // 16ビット符号付き整数として解釈
                            int16_t short_val =
                                static_cast<int16_t>(raw_value & 0xFFFF);
                            typed_value = static_cast<int64_t>(short_val);
                            break;
                        }
                        case TYPE_LONG: {
                            // 64ビット符号付き整数（そのまま使用）
                            typed_value = raw_value;
                            break;
                        }
                        case TYPE_INT: {
                            // 32ビット符号付き整数として解釈
                            int32_t int_val =
                                static_cast<int32_t>(raw_value & 0xFFFFFFFF);
                            typed_value = static_cast<int64_t>(int_val);
                            break;
                        }
                        default:
                            // BIGなどはそのまま
                            typed_value = raw_value;
                            break;
                        }

                        return TypedValue(
                            typed_value,
                            InferredType(base_type,
                                         type_info_to_string_basic(base_type)));
                    }
                }
            }
        }

        // 構造体メンバー配列要素アクセスのケース: s.member[index]
        // このケースはextract_array_name()が"s.member"を返すため、
        // find_variable("s.member")で直接アクセス変数が見つかり、
        // 上記の整数配列処理で既に処理されている
        if (node->left &&
            node->left->node_type == ASTNodeType::AST_MEMBER_ACCESS) {

            // オブジェクト名とメンバー名を取得
            std::string obj_name;
            if (node->left->left) {
                if (node->left->left->node_type == ASTNodeType::AST_VARIABLE ||
                    node->left->left->node_type ==
                        ASTNodeType::AST_IDENTIFIER) {
                    obj_name = node->left->left->name;
                }
            }

            if (!obj_name.empty()) {
                std::string member_name = node->left->name;
                int64_t index = evaluate_expression(node->array_index.get());

                // 構造体メンバー配列要素を取得
                try {
                    int64_t value =
                        interpreter_.get_struct_member_array_element(
                            obj_name, member_name, static_cast<int>(index));

                    // 型情報を含めて返す
                    return consume_numeric_typed_value(node, value,
                                                       inferred_type);
                } catch (const std::exception &e) {
                    if (debug_mode) {
                        {
                            char dbg_buf[512];
                            snprintf(
                                dbg_buf, sizeof(dbg_buf),
                                "Failed to get struct member array element: %s",
                                e.what());
                            debug_msg(DebugMsgId::GENERIC_DEBUG, dbg_buf);
                        }
                    }
                    // フォールバックに進む
                }
            }
        }

        // フォールバック: 通常の整数評価
        int64_t numeric_result = evaluate_expression(node);
        return consume_numeric_typed_value(node, numeric_result, inferred_type);
    }

    case ASTNodeType::AST_IDENTIFIER: {
        // 識別子の場合、まず変数を探す
        Variable *var = interpreter_.find_variable(node->name);
        if (var) {
            // 関数ポインタの場合、関数ポインタ情報を含むTypedValueを返す
            if (var->is_function_pointer) {
                auto &fp_map = interpreter_.current_scope().function_pointers;
                auto it = fp_map.find(node->name);
                if (it != fp_map.end()) {
                    return TypedValue::function_pointer(
                        var->value, it->second.function_name,
                        it->second.function_node, inferred_type);
                }
            }

            // ポインタ変数の場合、TYPE_POINTERで返す
            if (var->is_pointer) {
                InferredType ptr_type;
                ptr_type.type_info = TYPE_POINTER;
                ptr_type.is_array = false;
                TypedValue result(var->value, ptr_type);
                return result;
            }

            // 構造体変数の場合、TypedValueとして返す（ReturnExceptionを避ける）
            if (var->type == TYPE_STRUCT) {
                return TypedValue(
                    *var, InferredType(TYPE_STRUCT, var->struct_type_name));
            }

            // 文字列変数の場合、ポインタ値から文字列を取得
            if (var->type == TYPE_STRING) {
                if (var->str_value.empty() && var->value != 0) {
                    // mallocで取得したポインタを文字列に変換
                    const char *ptr =
                        reinterpret_cast<const char *>(var->value);
                    return TypedValue(std::string(ptr),
                                      InferredType(TYPE_STRING, "string"));
                } else {
                    // 通常の文字列値を返す
                    return TypedValue(var->str_value,
                                      InferredType(TYPE_STRING, "string"));
                }
            }
        }
        // 通常の識別子の場合は通常評価
        int64_t numeric_result = evaluate_expression(node);
        return consume_numeric_typed_value(node, numeric_result, inferred_type);
    }

    default: {
        // デフォルトは従来の評価結果を数値として返す
        int64_t numeric_result = evaluate_expression(node);
        return consume_numeric_typed_value(node, numeric_result, inferred_type);
    }
    }
}

// 型推論対応の三項演算子評価
TypedValue ExpressionEvaluator::evaluate_ternary_typed(const ASTNode *node) {
    // 三項演算子の評価はTernaryHelpersに移動（130行）
    auto evaluate_expression_lambda = [this](const ASTNode *n) {
        return this->evaluate_expression(n);
    };
    auto evaluate_typed_expression_lambda = [this](const ASTNode *n) {
        return this->evaluate_typed_expression(n);
    };
    return TernaryHelpers::evaluate_ternary_typed(
        node, interpreter_, evaluate_expression_lambda,
        evaluate_typed_expression_lambda, type_engine_, last_typed_result_);
}

// 遅延評価されたTypedValueを実際に評価する
TypedValue ExpressionEvaluator::resolve_deferred_evaluation(
    const TypedValue &deferred_value) {
    if (!deferred_value.needs_deferred_evaluation() ||
        !deferred_value.deferred_node) {
        return deferred_value; // 遅延評価が不要または無効
    }

    const ASTNode *node = deferred_value.deferred_node;

    switch (node->node_type) {
    case ASTNodeType::AST_ARRAY_LITERAL:
        // 配列リテラルの場合、ノード参照を返す（代入処理で使用）
        return TypedValue::deferred(node, deferred_value.type);

    case ASTNodeType::AST_STRUCT_LITERAL:
        // 構造体リテラルの場合、ノード参照を返す（代入処理で使用）
        return TypedValue::deferred(node, deferred_value.type);

    case ASTNodeType::AST_FUNC_CALL:
        // 関数呼び出しの場合、実際に実行して結果を取得
        return evaluate_typed_expression(node);

    default:
        // その他の場合は通常の評価
        return evaluate_typed_expression(node);
    }
}

TypedValue ExpressionEvaluator::consume_numeric_typed_value(
    const ASTNode *node, int64_t numeric_result,
    const InferredType &inferred_type) {
    // メンバーアクセスヘルパーに移動（28行）
    return MemberAccessHelpers::consume_numeric_typed_value(
        node, numeric_result, inferred_type, last_captured_function_value_,
        &last_typed_result_);
}

// 構造体メンバー取得関数の実装
Variable ExpressionEvaluator::get_struct_member_from_variable(
    const Variable &struct_var, const std::string &member_name) {
    // メンバーアクセスヘルパーに移動（76行）
    return MemberAccessHelpers::get_struct_member_from_variable(
        struct_var, member_name, interpreter_);
}

// 関数戻り値からのメンバーアクセス処理
TypedValue ExpressionEvaluator::evaluate_function_member_access(
    const ASTNode *func_node, const std::string &member_name) {
    // メンバーアクセスヘルパーに移動（30行）
    return MemberAccessHelpers::evaluate_function_member_access(
        func_node, member_name, *this);
}

// 関数戻り値からの配列アクセス処理
TypedValue
ExpressionEvaluator::evaluate_function_array_access(const ASTNode *func_node,
                                                    const ASTNode *index_node) {
    // メンバーアクセスヘルパーに移動（42行）
    return MemberAccessHelpers::evaluate_function_array_access(
        func_node, index_node, *this);
}

// 関数戻り値からの複合アクセス処理（func()[index].member）
TypedValue ExpressionEvaluator::evaluate_function_compound_access(
    const ASTNode *func_node, const ASTNode *index_node,
    const std::string &member_name) {
    // メンバーアクセスヘルパーに移動（22行）
    return MemberAccessHelpers::evaluate_function_compound_access(
        func_node, index_node, member_name, *this);
}

// 再帰的メンバーアクセス処理（将来のネスト構造体対応）
TypedValue ExpressionEvaluator::evaluate_recursive_member_access(
    const Variable &base_var, const std::vector<std::string> &member_path) {
    // メンバーアクセスヘルパーに移動（55行）
    return MemberAccessHelpers::evaluate_recursive_member_access(
        base_var, member_path, interpreter_);
}

// ============================================================================
// NOTE: Tier 2リファクタリングで抽出されたヘルパーメソッドは
// expression_helpers.cpp に移動しました
//
// Phase 12リファクタリング: Method Receiver Resolution
// resolve_method_receiver, resolve_array_receiver, resolve_member_receiver,
// resolve_arrow_receiver, create_chain_receiver_from_expression は
// expression_receiver_resolution.cpp に移動しました
// ============================================================================

// ============================================================================
// v0.11.0 String Interpolation - 補間文字列評価
// ============================================================================
TypedValue
ExpressionEvaluator::evaluate_interpolated_string(const ASTNode *node) {
    std::string result;

    for (const auto &segment : node->interpolation_segments) {
        if (segment->is_interpolation_text) {
            // テキストセグメント
            result += segment->str_value;
        } else if (segment->is_interpolation_expr) {
            // 式セグメント
            TypedValue expr_value =
                evaluate_typed_expression(segment->left.get());
            std::string formatted = format_interpolated_value(
                expr_value, segment->interpolation_format);
            result += formatted;
        }
    }

    // 文字列型の TypedValue を構築
    InferredType string_type(TYPE_STRING, "string", false, 0);
    return TypedValue(result, string_type);
}

std::string
ExpressionEvaluator::format_interpolated_value(const TypedValue &value,
                                               const std::string &format_spec) {
    // フォーマット指定子がない場合はデフォルト変換
    if (format_spec.empty()) {
        if (value.type.type_info == TYPE_STRING) {
            return value.string_value;
        } else if (value.is_numeric_result) {
            if (value.is_float_result) {
                if (value.type.type_info == TYPE_QUAD) {
                    return std::to_string(value.quad_value);
                } else {
                    return std::to_string(value.double_value);
                }
            } else {
                return std::to_string(value.value);
            }
        } else if (value.type.type_info == TYPE_BOOL) {
            return value.value ? "true" : "false";
        }
        return "";
    }

    // フォーマット指定子を解析
    // 形式: [幅][.精度][型]
    // 例: "05" -> 幅5、0埋め
    //     ".2" -> 小数点以下2桁
    //     "x" -> 16進数
    //     "X" -> 16進数（大文字）
    //     "b" -> 2進数

    std::stringstream ss;
    size_t pos = 0;

    // 0埋めフラグ
    bool zero_pad = false;
    if (pos < format_spec.length() && format_spec[pos] == '0') {
        zero_pad = true;
        pos++;
    }

    // 幅の解析
    int width = 0;
    while (pos < format_spec.length() && std::isdigit(format_spec[pos])) {
        width = width * 10 + (format_spec[pos] - '0');
        pos++;
    }

    // 精度の解析
    int precision = -1;
    if (pos < format_spec.length() && format_spec[pos] == '.') {
        pos++;
        precision = 0;
        while (pos < format_spec.length() && std::isdigit(format_spec[pos])) {
            precision = precision * 10 + (format_spec[pos] - '0');
            pos++;
        }
    }

    // 型の解析
    char type_char = '\0';
    if (pos < format_spec.length()) {
        type_char = format_spec[pos];
    }

    // フォーマット適用
    if (type_char == 'x' || type_char == 'X') {
        // 16進数
        long long_val = value.value;

        ss << std::hex;
        if (type_char == 'X') {
            ss << std::uppercase;
        }
        if (zero_pad && width > 0) {
            ss << std::setfill('0') << std::setw(width);
        } else if (width > 0) {
            ss << std::setw(width);
        }
        ss << long_val;
    } else if (type_char == 'b') {
        // 2進数
        long long_val = value.value;

        std::string binary;
        if (long_val == 0) {
            binary = "0";
        } else {
            unsigned long uval = static_cast<unsigned long>(long_val);
            while (uval > 0) {
                binary = (uval % 2 == 0 ? "0" : "1") + binary;
                uval /= 2;
            }
        }
        if (zero_pad && width > 0) {
            while (binary.length() < static_cast<size_t>(width)) {
                binary = "0" + binary;
            }
        }
        ss << binary;
    } else {
        // デフォルトまたは小数点精度
        if (value.is_float_result) {
            double dval = value.type.type_info == TYPE_QUAD
                              ? value.quad_value
                              : value.double_value;
            if (precision >= 0) {
                ss << std::fixed << std::setprecision(precision);
            }
            if (width > 0) {
                if (zero_pad) {
                    ss << std::setfill('0');
                }
                ss << std::setw(width);
            }
            ss << dval;
        } else if (value.is_numeric_result) {
            long long_val = value.value;
            if (zero_pad && width > 0) {
                ss << std::setfill('0') << std::setw(width);
            } else if (width > 0) {
                ss << std::setw(width);
            }
            ss << long_val;
        } else if (value.type.type_info == TYPE_STRING) {
            ss << value.string_value;
        } else if (value.type.type_info == TYPE_BOOL) {
            ss << (value.value ? "true" : "false");
        }
    }

    return ss.str();
}

// ============================================================================
// evaluate_binary_op_impl - AST_BINARY_OPケースの実装
// ============================================================================
// Phase 13: 二項演算子の評価を分離 - ExpressionDispatcherへ移行
// +, -, *, /, %, <, >, ==, !=, &&, ||, &, |, ^, <<, >> 等
// ============================================================================
