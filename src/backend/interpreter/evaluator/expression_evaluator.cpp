#include "evaluator/expression_evaluator.h"
#include "../../../common/debug.h"
#include "../../../common/debug_messages.h"
#include "../../../common/utf8_utils.h"
#include "core/error_handler.h"
#include "core/interpreter.h"
#include "core/pointer_metadata.h" // ポインタメタデータシステム
#include "evaluator/expression_address_ops.h" // アドレス演算子と間接参照演算子のヘルパー
#include "evaluator/expression_array_access.h" // 配列アクセスのヘルパー
#include "evaluator/expression_assignment.h" // 代入演算子のヘルパー
#include "evaluator/expression_binary_unary_typed.h" // 二項/単項演算子（typed版）のヘルパー
#include "evaluator/expression_function_call.h" // 関数呼び出しのヘルパー
#include "evaluator/expression_helpers.h" // Tier 2リファクタリング: ヘルパー関数群
#include "evaluator/expression_incdec.h" // インクリメント/デクリメントのヘルパー
#include "evaluator/expression_literal_eval.h" // リテラル評価（数値、文字列、nullptr、変数）のヘルパー
#include "evaluator/expression_member_helpers.h" // メンバーアクセス関連のヘルパー
#include "evaluator/expression_receiver_resolution.h" // Phase 12: Method Receiver Resolution
#include "evaluator/expression_special_access.h" // 特殊アクセス（アロー、メンバー配列、Enum）のヘルパー
#include "evaluator/expression_ternary.h" // 三項演算子（?:）のヘルパー
#include "managers/array_manager.h"
#include "managers/enum_manager.h" // EnumManager定義が必要
#include "managers/type_manager.h" // TypeManager定義が必要
#include "services/array_processing_service.h"
#include "services/debug_service.h"
#include <cstdio>
#include <functional>
#include <iostream>
#include <stdexcept>

ExpressionEvaluator::ExpressionEvaluator(Interpreter &interpreter)
    : interpreter_(interpreter), type_engine_(interpreter),
      last_typed_result_(static_cast<int64_t>(0), InferredType()),
      last_captured_function_value_(std::nullopt) {}

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
    if (!node) {
        debug_msg(DebugMsgId::EXPR_EVAL_START,
                  "Null node in expression evaluation");
        if (debug_mode) {
            std::cerr << "[ERROR] Null node in expression evaluation"
                      << std::endl;
            // スタックトレース的な情報を出力
            std::cerr << "[ERROR] This usually means a parser error occurred"
                      << std::endl;
        }
        throw std::runtime_error("Null node in expression evaluation");
    }

    std::string node_type_str =
        std::to_string(static_cast<int>(node->node_type));
    debug_msg(DebugMsgId::EXPR_EVAL_START, node_type_str.c_str());

    // 多次元配列アクセスの場合のみ詳細ログ
    if (node->node_type == ASTNodeType::AST_ARRAY_REF && node->name.empty()) {
        debug_msg(DebugMsgId::EXPR_EVAL_ARRAY_REF_START);
    }

    switch (node->node_type) {
    // ========================================================================
    // リテラル値の評価（NUMBER, NULLPTR, STRING_LITERAL）
    // ========================================================================
    case ASTNodeType::AST_NUMBER:
        return ExpressionHelpers::evaluate_number_literal(node);

    case ASTNodeType::AST_NULLPTR:
    case ASTNodeType::AST_STRING_LITERAL:
        return ExpressionHelpers::evaluate_special_literal(node);

    // ========================================================================
    // 変数参照の評価（IDENTIFIER, VARIABLE）
    // ========================================================================
    case ASTNodeType::AST_IDENTIFIER: {
        debug_msg(DebugMsgId::EXPR_EVAL_VAR_REF, node->name.c_str());

        // selfキーワードの処理
        if (node->name == "self") {
            // メソッドコンテキスト内で self を処理
            // 現在のメソッド実行コンテキストからself変数を取得
            Variable *self_var = interpreter_.find_variable("self");
            if (!self_var) {
                std::string error_message =
                    (debug_language == DebugLanguage::JAPANESE)
                        ? "selfはメソッドコンテキスト外では使用できません"
                        : "self can only be used within method context";
                interpreter_.throw_runtime_error_with_location(error_message,
                                                               node);
            }

            // selfが構造体またはインターフェース型の場合、ReturnExceptionで構造体を返す
            if (self_var->type == TYPE_STRUCT ||
                self_var->type == TYPE_INTERFACE) {
                interpreter_.sync_struct_members_from_direct_access("self");
                throw ReturnException(*self_var);
            } else {
                // プリミティブ型の場合は値を返す
                return self_var->value;
            }
        }

        // 通常の識別子として処理
        Variable *var = interpreter_.find_variable(node->name);
        if (!var) {
            debug_msg(DebugMsgId::EXPR_EVAL_VAR_NOT_FOUND, node->name.c_str());
            std::string error_message =
                (debug_language == DebugLanguage::JAPANESE)
                    ? "未定義の変数です: " + node->name
                    : "Undefined variable: " + node->name;
            interpreter_.throw_runtime_error_with_location(error_message, node);
        }

        debug_msg(DebugMsgId::EXPR_EVAL_VAR_VALUE, node->name.c_str(),
                  var->value);

        if (debug_mode && var->type == TYPE_POINTER) {
            std::cerr << "[EXPR_EVAL] Variable " << node->name
                      << " value: " << var->value << " (0x" << std::hex
                      << var->value << std::dec << ")" << std::endl;
        }

        return var->value;
    }

    case ASTNodeType::AST_VARIABLE: {
        debug_msg(DebugMsgId::EXPR_EVAL_VAR_REF, node->name.c_str());

        // selfキーワードの特別処理（構造体戻り値用）
        if (node->name == "self") {
            Variable *self_var = interpreter_.find_variable("self");
            if (!self_var) {
                std::string error_message =
                    (debug_language == DebugLanguage::JAPANESE)
                        ? "selfはメソッドコンテキスト外では使用できません"
                        : "self can only be used within method context";
                interpreter_.throw_runtime_error_with_location(error_message,
                                                               node);
            }

            // デバッグ出力: self変数の詳細情報
            debug_print("SELF_DEBUG: self found - type=%d, is_struct=%d, "
                        "TYPE_STRUCT=%d, TYPE_INTERFACE=%d\n",
                        (int)self_var->type, self_var->is_struct,
                        (int)TYPE_STRUCT, (int)TYPE_INTERFACE);

            // selfが構造体またはインターフェース型の場合、ReturnExceptionで構造体を返す
            if (self_var->type == TYPE_STRUCT ||
                self_var->type == TYPE_INTERFACE) {
                debug_print(
                    "SELF_DEBUG: Throwing ReturnException for struct self\n");
                interpreter_.sync_struct_members_from_direct_access("self");
                throw ReturnException(*self_var);
            } else {
                debug_print("SELF_DEBUG: self is not struct, returning "
                            "primitive value\n");
                // primitive型の場合は適切な値を返す
                // 文字列の場合、特別な処理が必要な場合があるが、まずは値を返す
                return self_var->value;
            }
        }

        Variable *var = interpreter_.find_variable(node->name);
        if (!var) {
            debug_msg(DebugMsgId::EXPR_EVAL_VAR_NOT_FOUND, node->name.c_str());
            // エラー時にソースコード位置を表示
            std::string error_message =
                (debug_language == DebugLanguage::JAPANESE)
                    ? "未定義の変数です: " + node->name
                    : "Undefined variable: " + node->name;
            interpreter_.throw_runtime_error_with_location(error_message, node);
        }

        // 参照型変数の場合、参照先変数の値を返す
        if (var->is_reference) {
            Variable *target_var = reinterpret_cast<Variable *>(var->value);
            if (!target_var) {
                throw std::runtime_error("Invalid reference variable: " +
                                         node->name);
            }

            if (debug_mode) {
                std::cerr << "[DEBUG] Reference access: " << node->name
                          << " -> target value: " << target_var->value
                          << std::endl;
            }

            // 参照先が構造体の場合
            if (target_var->type == TYPE_STRUCT) {
                throw ReturnException(*target_var);
            }

            // 参照先の値を返す
            return target_var->value;
        }

        // ユニオン型変数の場合、current_typeに応じて適切な値を返す
        if (var->type == TYPE_UNION) {
            if (var->current_type == TYPE_STRING) {
                // 文字列の場合は、数値評価コンテキストでは0を返す
                // 実際の文字列値はstr_valueに格納されている
                debug_msg(DebugMsgId::EXPR_EVAL_VAR_VALUE, node->name.c_str(),
                          0);
                return 0;
            } else {
                debug_msg(DebugMsgId::EXPR_EVAL_VAR_VALUE, node->name.c_str(),
                          var->value);
                return var->value;
            }
        }

        // 構造体変数の場合、ReturnExceptionをスローして構造体データを返す
        if (var->type == TYPE_STRUCT) {
            throw ReturnException(*var);
        }

        debug_msg(DebugMsgId::EXPR_EVAL_VAR_VALUE, node->name.c_str(),
                  var->value);
        return var->value;
    }

    // ========================================================================
    // 配列アクセスと配列リテラルの評価
    // Line 280-559: 配列の要素アクセス、多次元配列、配列リテラル
    // ========================================================================
    // 配列アクセス（ARRAY_REF）
    // Line 265-605: 配列アクセス評価をArrayAccessHelpersに移動
    // ========================================================================
    case ASTNodeType::AST_ARRAY_REF: {
        auto eval_func = [this](const ASTNode *n) {
            return this->evaluate_expression(n);
        };
        auto get_member_func = [this](const Variable &v,
                                      const std::string &name) {
            return this->get_struct_member_from_variable(v, name);
        };
        return ArrayAccessHelpers::evaluate_array_ref(
            node, interpreter_, eval_func, get_member_func);
    }

    case ASTNodeType::AST_ARRAY_LITERAL: {
        return ArrayAccessHelpers::evaluate_array_literal(node, interpreter_);
    }

    // ========================================================================
    // 二項演算子の評価（+, -, *, /, %, <, >, ==, !=, &&, ||, &, |, ^, <<, >>
    // 等） Line 634-787: 算術演算、比較演算、論理演算、ビット演算
    // TODO: この巨大なswitch文を以下に分割すべき:
    //   - evaluate_arithmetic_binary() : +, -, *, /, %
    //   - evaluate_comparison_binary() : <, >, <=, >=, ==, !=
    //   - evaluate_logical_binary() : &&, ||
    //   - evaluate_bitwise_binary() : &, |, ^, <<, >>
    // ========================================================================
    case ASTNodeType::AST_BINARY_OP: {
        debug_msg(DebugMsgId::EXPR_EVAL_BINARY_OP, node->op.c_str());

        int64_t left = evaluate_expression(node->left.get());
        int64_t right = evaluate_expression(node->right.get());

        debug_msg(DebugMsgId::BINARY_OP_VALUES, left, right);

        // デバッグ: 減算操作の詳細を出力
        int64_t result = 0;

        // ポインタ演算のチェック
        bool left_is_pointer = (left & (1LL << 63)) != 0; // メタデータポインタ
        bool right_is_pointer = (right & (1LL << 63)) != 0;

        // 変数がポインタ型かどうかもチェック
        if (node->left->node_type == ASTNodeType::AST_VARIABLE ||
            node->left->node_type == ASTNodeType::AST_IDENTIFIER) {
            Variable *left_var = interpreter_.find_variable(node->left->name);
            if (left_var && left_var->is_pointer) {
                left_is_pointer = true;
            }
        }
        if (node->right->node_type == ASTNodeType::AST_VARIABLE ||
            node->right->node_type == ASTNodeType::AST_IDENTIFIER) {
            Variable *right_var = interpreter_.find_variable(node->right->name);
            if (right_var && right_var->is_pointer) {
                right_is_pointer = true;
            }
        }

        // ポインタ同士の加算を禁止
        if (node->op == "+" && left_is_pointer && right_is_pointer) {
            throw std::runtime_error(
                "Cannot add two pointers together. Pointer arithmetic only "
                "supports: pointer + integer, integer + pointer");
        }

        // ポインタ演算の特別処理
        if (node->op == "+" || node->op == "-") {
            // 左オペランドがメタデータポインタの場合
            if (left & (1LL << 63)) {
                int64_t clean_ptr = left & ~(1LL << 63);
                using namespace PointerSystem;
                PointerMetadata *meta =
                    reinterpret_cast<PointerMetadata *>(clean_ptr);

                if (meta) {
                    // 真のポインタ演算：アドレス = アドレス + (オフセット ×
                    // sizeof(要素型))
                    // 配列要素はint64_t（8バイト）として保存されているため、実際のメモリレイアウトに合わせる
                    ptrdiff_t offset = static_cast<ptrdiff_t>(right);
                    uintptr_t new_address;
                    size_t actual_element_size =
                        sizeof(int64_t); // 配列要素は常にint64_tで保存

                    if (node->op == "+") {
                        new_address =
                            meta->address + (offset * actual_element_size);
                    } else { // "-"
                        new_address =
                            meta->address - (offset * actual_element_size);
                    }

                    // 範囲チェック（配列ポインタの場合）
                    if (meta->array_var) {
                        if (new_address < meta->array_start_addr ||
                            new_address >= meta->array_end_addr) {
                            throw std::runtime_error(
                                "Pointer arithmetic out of array bounds");
                        }
                    }

                    // 新しいメタデータを作成
                    PointerMetadata *new_meta = new PointerMetadata();
                    new_meta->target_type = meta->target_type;
                    new_meta->address = new_address;
                    new_meta->pointed_type = meta->pointed_type;
                    new_meta->type_size = meta->type_size;
                    new_meta->element_type = meta->element_type;

                    // 範囲チェック情報をコピー
                    new_meta->array_var = meta->array_var;
                    new_meta->array_start_addr = meta->array_start_addr;
                    new_meta->array_end_addr = meta->array_end_addr;

                    // インデックスを更新（レガシー互換性のため）
                    if (meta->array_var && actual_element_size > 0) {
                        new_meta->element_index =
                            (new_address - meta->array_start_addr) /
                            actual_element_size;
                    }

                    // タグ付きポインタを返す
                    int64_t ptr_value = reinterpret_cast<int64_t>(new_meta);
                    ptr_value |= (1LL << 63);
                    return ptr_value;
                }
            }

            // 通常の整数演算（ポインタ演算後のフォールバック）
            result = ExpressionHelpers::evaluate_arithmetic_binary(node->op,
                                                                   left, right);
        }
        // 算術演算（+, -, *, /, %）
        else if (node->op == "+" || node->op == "-" || node->op == "*" ||
                 node->op == "/" || node->op == "%") {
            result = ExpressionHelpers::evaluate_arithmetic_binary(node->op,
                                                                   left, right);
        }
        // 比較演算（==, !=, <, >, <=, >=）
        else if (node->op == "==" || node->op == "!=" || node->op == "<" ||
                 node->op == ">" || node->op == "<=" || node->op == ">=") {
            result = ExpressionHelpers::evaluate_comparison_binary(node->op,
                                                                   left, right);
        }
        // 論理演算（&&, ||）
        else if (node->op == "&&" || node->op == "||") {
            result = ExpressionHelpers::evaluate_logical_binary(node->op, left,
                                                                right);
        }
        // ビット演算（&, |, ^, <<, >>）
        else if (node->op == "&" || node->op == "|" || node->op == "^" ||
                 node->op == "<<" || node->op == ">>") {
            result = ExpressionHelpers::evaluate_bitwise_binary(node->op, left,
                                                                right);
        } else {
            error_msg(DebugMsgId::UNKNOWN_BINARY_OP_ERROR, node->op.c_str());
            throw std::runtime_error("Unknown binary operator: " + node->op);
        }

        debug_msg(DebugMsgId::BINARY_OP_RESULT_DEBUG, result);
        return result;
    }

    // ========================================================================
    // 三項演算子の評価（condition ? true_expr : false_expr）
    // Line 792-804: 三項演算子の評価と型推論
    // ========================================================================
    case ASTNodeType::AST_TERNARY_OP: {
        // 三項演算子: condition ? true_expr : false_expr (型推論対応)
        TypedValue typed_result = evaluate_ternary_typed(node);

        if (typed_result.is_string()) {
            // 文字列の場合は、プロトコルとして0を返し、実際の文字列値は
            // 必要に応じて別途取得する（OutputManagerなど）
            return 0;
        } else {
            return typed_result.as_numeric();
        }
    }

    // ========================================================================
    // 単項演算子の評価（!, -, ~, &, * 等）
    // Line 813-1086: 論理否定、符号反転、ビット否定、アドレス取得、参照外し
    // TODO: ポインタ演算（ADDRESS_OF, DEREFERENCE）は別メソッドに分離すべき
    // ========================================================================
    case ASTNodeType::AST_UNARY_OP: {
        debug_msg(DebugMsgId::UNARY_OP_DEBUG, node->op.c_str());

        // 後置インクリメント/デクリメント（x++, x--）
        if (node->op == "++_post" || node->op == "--_post") {
            return ExpressionHelpers::evaluate_postfix_incdec(node,
                                                              interpreter_);
        }

        // 前置インクリメント/デクリメント（++x, --x）
        if (node->op == "++" || node->op == "--") {
            return ExpressionHelpers::evaluate_prefix_incdec(node,
                                                             interpreter_);
        }

        // アドレス演算子 (&)
        if (node->op == "ADDRESS_OF") {
            auto eval_func = [this](const ASTNode *n) {
                return this->evaluate_expression(n);
            };
            return AddressOperationHelpers::evaluate_address_of(
                node, interpreter_, eval_func);
        }

        // 間接参照演算子 (*)
        if (node->op == "DEREFERENCE") {
            auto eval_func = [this](const ASTNode *n) {
                return this->evaluate_expression(n);
            };
            return AddressOperationHelpers::evaluate_dereference(
                node, interpreter_, eval_func);
        }

        int64_t operand = evaluate_expression(node->left.get());
        return ExpressionHelpers::evaluate_simple_unary(node->op, operand);
    }

    // ========================================================================
    // インクリメント/デクリメント演算子（++, --）
    // Line 1091-1570: 前置・後置インクリメント/デクリメント
    // 変数、配列要素、ポインタ、構造体メンバーに対応
    // ========================================================================
    case ASTNodeType::AST_PRE_INCDEC:
    case ASTNodeType::AST_POST_INCDEC: {
        return IncDecHelpers::evaluate_incdec(
            node, interpreter_,
            [this](const ASTNode *n) { return this->evaluate_expression(n); });
    }

    // ========================================================================
    // 関数ポインタ呼び出し（(*funcPtr)(args)）
    // Line 1576-1719: 関数ポインタを介した間接呼び出し
    // ========================================================================
    // 関数ポインタ呼び出し（(*funcPtr)(args)）
    // Line 950-1110:
    // 関数ポインタを介した関数呼び出しをFunctionCallHelpersに移動
    // ========================================================================
    case ASTNodeType::AST_FUNC_PTR_CALL: {
        return FunctionCallHelpers::evaluate_function_pointer_call(
            node, interpreter_);
    }

    // ========================================================================
    // 関数呼び出し（func(args)）
    // Line 964-2666: 通常の関数呼び出し、メソッド呼び出し、引数評価
    // これは最も大きなセクション（約1,700行）で、以下を含む:
    //   - 関数検索とバインディング
    //   - 引数評価と型変換
    //   - スコープ管理
    //   - 戻り値処理
    //   - メソッド呼び出し（self/receiver処理）
    //   - Interface経由の呼び出し
    // TODO: このセクションを以下に分割すべき:
    //   - evaluate_direct_function_call()
    //   - evaluate_method_call()
    //   - evaluate_interface_method_call()
    // ========================================================================
    case ASTNodeType::AST_FUNC_CALL: {
        // Delegated to expression_function_call_impl.cpp (1,546 lines
        // extracted)
        return evaluate_function_call_impl(node);
    }

    // ========================================================================
    // 代入式（=, +=, -=, *=, /=, %= 等）
    // Line 3292-3453: 代入演算子の評価
    // 変数、配列要素、構造体メンバーへの代入に対応
    // ========================================================================
    case ASTNodeType::AST_ASSIGN: {
        return AssignmentHelpers::evaluate_assignment(
            node, interpreter_,
            [this](const ASTNode *n) { return this->evaluate_expression(n); },
            [this](const ASTNode *n) {
                return this->evaluate_typed_expression(n);
            });
    }

    // ========================================================================
    // 構造体メンバーアクセス（obj.member）
    // Line 3459-3900: ドット演算子によるメンバーアクセス
    // ネストした構造体、配列要素の構造体メンバーに対応
    // TODO: このセクションを evaluate_struct_member_access() に分離すべき
    // ========================================================================
    case ASTNodeType::AST_MEMBER_ACCESS: {
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
                    throw std::runtime_error(
                        "self is not a struct or interface");
                }
                base_var = *var;
            } else if (node->left->node_type ==
                           ASTNodeType::AST_MEMBER_ACCESS ||
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
                        int64_t index =
                            evaluate_expression(n->array_index.get());
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
                    const std::string &member_name_in_chain =
                        node->member_chain[i];

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
                    last_typed_result_ =
                        TypedValue(current_var.str_value,
                                   InferredType(TYPE_STRING, "string"));
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
                        int64_t index =
                            evaluate_expression(n->array_index.get());
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
                        last_typed_result_ =
                            TypedValue(member_var_ptr->value,
                                       InferredType(TYPE_STRUCT,
                                                    member_var_ptr->type_name));
                        return member_var_ptr->value;
                    } else if (member_var_ptr->type == TYPE_FLOAT ||
                               member_var_ptr->type == TYPE_DOUBLE ||
                               member_var_ptr->type == TYPE_QUAD) {
                        // float/double/quadの場合
                        InferredType float_type(member_var_ptr->type, "");
                        if (member_var_ptr->type == TYPE_QUAD) {
                            last_typed_result_ = TypedValue(
                                member_var_ptr->quad_value, float_type);
                        } else {
                            last_typed_result_ = TypedValue(
                                member_var_ptr->float_value, float_type);
                        }
                        return static_cast<int64_t>(
                            member_var_ptr->float_value);
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
                    last_typed_result_ =
                        TypedValue(member_var.str_value,
                                   InferredType(TYPE_STRING, "string"));
                    return 0;
                } else if (member_var.type == TYPE_STRUCT) {
                    last_typed_result_ = TypedValue(
                        member_var.value,
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
            debug_msg(DebugMsgId::SELF_MEMBER_ACCESS_START,
                      member_name.c_str());

            // selfメンバーアクセスの特別処理
            std::string self_member_path = "self." + member_name;
            Variable *self_member =
                interpreter_.find_variable(self_member_path);
            if (self_member) {
                debug_msg(DebugMsgId::SELF_MEMBER_ACCESS_FOUND,
                          self_member_path.c_str());
                if (self_member->type == TYPE_STRING) {
                    return 0; // 文字列の場合は別途処理
                }
                debug_msg(DebugMsgId::SELF_MEMBER_ACCESS_VALUE,
                          self_member->value);
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
                        int64_t index =
                            evaluate_expression(n->array_index.get());
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
            // 関数呼び出し結果でのメンバアクセス: func().member
            debug_msg(DebugMsgId::EXPR_EVAL_START,
                      "Function call member access");

            try {
                // 関数を実行してReturnExceptionを捕捉
                evaluate_typed_expression(node->left.get());
                // 通常の戻り値の場合はエラー
                throw std::runtime_error(
                    "Function did not return a struct for member access");
            } catch (const ReturnException &ret_ex) {
                // 構造体戻り値からメンバーを取得
                if (ret_ex.is_struct_array &&
                    ret_ex.struct_array_3d.size() > 0) {
                    // 構造体配列の場合（将来拡張）
                    throw std::runtime_error(
                        "Struct array function return member access not yet "
                        "fully supported");
                } else {
                    // 単一構造体の場合
                    Variable struct_var = ret_ex.struct_value;
                    Variable member_var = get_struct_member_from_variable(
                        struct_var, member_name);

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
            // 関数配列戻り値でのメンバアクセス: func()[index].member
            debug_msg(DebugMsgId::EXPR_EVAL_START,
                      "Function array member access");

            try {
                // 関数を実行してReturnExceptionを捕捉
                evaluate_expression(node->left->left.get());
                throw std::runtime_error("Function did not return an array for "
                                         "indexed member access");
            } catch (const ReturnException &ret_ex) {
                if (ret_ex.is_struct_array &&
                    ret_ex.struct_array_3d.size() > 0) {
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
                        Variable struct_var =
                            ret_ex.struct_array_3d[0][0][index];
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
                                last_typed_result_ = TypedValue(
                                    member_var.quad_value, float_type);
                            } else {
                                last_typed_result_ = TypedValue(
                                    member_var.float_value, float_type);
                            }
                            return static_cast<int64_t>(member_var.float_value);
                        } else {
                            return member_var.value;
                        }
                    } else {
                        throw std::runtime_error(
                            "Invalid struct array structure");
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
                    TypedValue typed_result(
                        static_cast<int64_t>(0),
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
            throw std::runtime_error("Member not found: " + actual_var_name +
                                     "." + member_name);
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
                last_typed_result_ =
                    TypedValue(member_var->quad_value, float_type);
            } else {
                last_typed_result_ =
                    TypedValue(member_var->float_value, float_type);
            }
            return static_cast<int64_t>(member_var->float_value);
        }
        return member_var->value;
    }

    // ========================================================================
    // アロー演算子（ptr->member）
    // Line 3912-3957: ポインタを介した構造体メンバーアクセス
    // ptr->member は (*ptr).member と等価
    // ========================================================================
    case ASTNodeType::AST_ARROW_ACCESS: {
        // アロー演算子アクセス（ptr->member）はSpecialAccessHelpersに移動（55行）
        auto evaluate_expression_lambda = [this](const ASTNode *n) {
            return this->evaluate_expression(n);
        };
        auto get_struct_member_lambda = [this](const Variable &v,
                                               const std::string &name) {
            return this->get_struct_member_from_variable(v, name);
        };
        return SpecialAccessHelpers::evaluate_arrow_access(
            node, interpreter_, evaluate_expression_lambda,
            get_struct_member_lambda);
    }

    // ========================================================================
    // メンバー配列アクセス（obj.member[index]）
    // ========================================================================
    case ASTNodeType::AST_MEMBER_ARRAY_ACCESS: {
        // メンバー配列アクセスはSpecialAccessHelpersに移動（100行）
        auto evaluate_expression_lambda = [this](const ASTNode *n) {
            return this->evaluate_expression(n);
        };
        auto get_struct_member_lambda = [this](const Variable &v,
                                               const std::string &name) {
            return this->get_struct_member_from_variable(v, name);
        };
        return SpecialAccessHelpers::evaluate_member_array_access(
            node, interpreter_, evaluate_expression_lambda,
            get_struct_member_lambda);
    }

    case ASTNodeType::AST_STRUCT_LITERAL: {
        // 構造体リテラル評価はSpecialAccessHelpersに移動（5行）
        return SpecialAccessHelpers::evaluate_struct_literal(node);
    }

    // ========================================================================
    // Enum値アクセス
    // ========================================================================
    case ASTNodeType::AST_ENUM_ACCESS: {
        // Enum値アクセスはSpecialAccessHelpersに移動（15行）
        return SpecialAccessHelpers::evaluate_enum_access(node, interpreter_);
    }

    // ========================================================================
    // 未対応のノード型
    // ========================================================================
    default:
        error_msg(DebugMsgId::UNSUPPORTED_EXPR_NODE_ERROR);
        if (debug_mode) {
            std::cerr << "[ERROR] Unsupported expression node type: "
                      << static_cast<int>(node->node_type) << std::endl;
        }
        throw std::runtime_error("Unknown expression node type");
    }

    return 0;
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
            debug_print("TYPED_EVAL_RETURN: is_struct=%d type=%d is_array=%d "
                        "is_function_pointer=%d\n",
                        ret_ex.is_struct ? 1 : 0, static_cast<int>(ret_ex.type),
                        ret_ex.is_array ? 1 : 0,
                        ret_ex.is_function_pointer ? 1 : 0);
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

        if (ret_ex.is_struct || ret_ex.type == TYPE_STRUCT) {
            // 構造体の場合、ReturnExceptionを再スロー（メンバアクセスで処理される）
            throw;
        }

        if (ret_ex.is_array) {
            // 配列戻り値は呼び出し元で特別処理する
            throw;
        }

        if (ret_ex.type == TYPE_STRING) {
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

        // 通常の数値の場合
        return TypedValue(
            ret_ex.value,
            InferredType(ret_ex.type,
                         ExpressionHelpers::type_info_to_string(ret_ex.type)));
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

    case ASTNodeType::AST_NUMBER: {
        // 数値リテラルの評価はLiteralEvalHelpersに移動（18行）
        return LiteralEvalHelpers::evaluate_number_literal_typed(node,
                                                                 inferred_type);
    }

    case ASTNodeType::AST_NULLPTR: {
        // nullptrの評価はLiteralEvalHelpersに移動（4行）
        return LiteralEvalHelpers::evaluate_nullptr_literal_typed();
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
            if (ret.is_struct || ret.type == TYPE_STRUCT) {
                // 構造体の場合
                Variable struct_var = ret.struct_value;
                InferredType struct_type(TYPE_STRUCT,
                                         struct_var.struct_type_name);
                return TypedValue(struct_var, struct_type);
            } else if (ret.type == TYPE_STRING) {
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

    case ASTNodeType::AST_MEMBER_ACCESS: {
        debug_msg(DebugMsgId::TYPED_MEMBER_ACCESS_CASE, node->name.c_str(),
                  node->member_chain.size());

        // member_chainが2つ以上ある場合（ネストメンバアクセス）
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
            if (current_var.type == TYPE_STRING) {
                return TypedValue(current_var.str_value,
                                  InferredType(TYPE_STRING, "string"));
            } else if (current_var.type == TYPE_STRUCT) {
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

            debug_print("Processing func()[index].member pattern: %s[].%s\n",
                        node->left->left->name.c_str(), node->name.c_str());

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

        TypedValue resolved_value(static_cast<int64_t>(0), InferredType());
        bool resolved = false;

        std::string base_name = build_base_name(node->left.get());
        debug_msg(DebugMsgId::NESTED_MEMBER_BASE_PATH, base_name.c_str(),
                  node->name.c_str());

        if (!base_name.empty()) {
            // まず個別変数を検索（優先）
            debug_msg(DebugMsgId::NESTED_MEMBER_RESOLVE_FROM_BASE);
            resolved = resolve_from_base_name(base_name, resolved_value);
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
            debug_print("Processing typed function call array access: %s\n",
                        node->left->name.c_str());

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
        node, numeric_result, inferred_type, last_captured_function_value_);
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
