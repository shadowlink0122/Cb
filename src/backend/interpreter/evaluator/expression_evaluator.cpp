#include "evaluator/expression_evaluator.h"
#include "evaluator/expression_helpers.h"  // Tier 2リファクタリング: ヘルパー関数群
#include "evaluator/expression_address_ops.h"  // アドレス演算子と間接参照演算子のヘルパー
#include "evaluator/expression_array_access.h"  // 配列アクセスのヘルパー
#include "evaluator/expression_function_call.h"  // 関数呼び出しのヘルパー
#include "evaluator/expression_incdec.h"  // インクリメント/デクリメントのヘルパー
#include "evaluator/expression_assignment.h"  // 代入演算子のヘルパー
#include "evaluator/expression_binary_unary_typed.h"  // 二項/単項演算子（typed版）のヘルパー
#include "evaluator/expression_special_access.h"  // 特殊アクセス（アロー、メンバー配列、Enum）のヘルパー
#include "evaluator/expression_literal_eval.h"  // リテラル評価（数値、文字列、nullptr、変数）のヘルパー
#include "evaluator/expression_ternary.h"  // 三項演算子（?:）のヘルパー
#include "core/interpreter.h"
#include "core/pointer_metadata.h"     // ポインタメタデータシステム
#include "managers/enum_manager.h"    // EnumManager定義が必要
#include "managers/type_manager.h"    // TypeManager定義が必要
#include "../../../common/debug_messages.h"
#include "../../../common/debug.h"
#include "../../../common/utf8_utils.h"
#include "core/error_handler.h"
#include "managers/array_manager.h"
#include "services/array_processing_service.h"
#include "services/debug_service.h"
#include <stdexcept>
#include <iostream>
#include <functional>
#include <cstdio>

// MethodReceiverResolutionのデフォルトコンストラクタ実装
ExpressionEvaluator::MethodReceiverResolution::MethodReceiverResolution()
    : kind(Kind::None), canonical_name(), variable_ptr(nullptr), chain_value(nullptr) {}

// レシーバ解決ヘルパー（メソッド呼び出し用）
ExpressionEvaluator::MethodReceiverResolution ExpressionEvaluator::resolve_method_receiver(const ASTNode* receiver_node) {
    MethodReceiverResolution result;
    if (!receiver_node) {
        return result;
    }

    switch (receiver_node->node_type) {
    case ASTNodeType::AST_VARIABLE:
    case ASTNodeType::AST_IDENTIFIER: {
        std::string name = receiver_node->name;
        if (name.empty()) {
            return result;
        }
        Variable* var = interpreter_.find_variable(name);
        if (var) {
            result.kind = MethodReceiverResolution::Kind::Direct;
            result.canonical_name = name;
            result.variable_ptr = var;
            return result;
        }
        break;
    }
    case ASTNodeType::AST_MEMBER_ACCESS:
        // メンバアクセスは別ヘルパーで解決
        return resolve_member_receiver(receiver_node);
    case ASTNodeType::AST_ARROW_ACCESS:
        // アロー演算子は (*ptr).member と等価
        return resolve_arrow_receiver(receiver_node);
    case ASTNodeType::AST_ARRAY_REF:
        return resolve_array_receiver(receiver_node);
    case ASTNodeType::AST_FUNC_CALL:
        return create_chain_receiver_from_expression(receiver_node);
    default:
        break;
    }

    return create_chain_receiver_from_expression(receiver_node);
}

ExpressionEvaluator::ExpressionEvaluator(Interpreter& interpreter) 
    : interpreter_(interpreter), type_engine_(interpreter), last_typed_result_(static_cast<int64_t>(0), InferredType()), last_captured_function_value_(std::nullopt) {}

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

int64_t ExpressionEvaluator::evaluate_expression(const ASTNode* node) {
    if (!node) {
        debug_msg(DebugMsgId::EXPR_EVAL_START, "Null node in expression evaluation");
        if (debug_mode) {
            std::cerr << "[ERROR] Null node in expression evaluation" << std::endl;
            // スタックトレース的な情報を出力
            std::cerr << "[ERROR] This usually means a parser error occurred" << std::endl;
        }
        throw std::runtime_error("Null node in expression evaluation");
    }

    std::string node_type_str = std::to_string(static_cast<int>(node->node_type));
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
                std::string error_message = (debug_language == DebugLanguage::JAPANESE) ? 
                    "selfはメソッドコンテキスト外では使用できません" : "self can only be used within method context";
                interpreter_.throw_runtime_error_with_location(error_message, node);
            }
            
            // selfが構造体またはインターフェース型の場合、ReturnExceptionで構造体を返す
            if (self_var->type == TYPE_STRUCT || self_var->type == TYPE_INTERFACE) {
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
            std::string error_message = (debug_language == DebugLanguage::JAPANESE) ? 
                "未定義の変数です: " + node->name : "Undefined variable: " + node->name;
            interpreter_.throw_runtime_error_with_location(error_message, node);
        }

        debug_msg(DebugMsgId::EXPR_EVAL_VAR_VALUE, node->name.c_str(), var->value);
        
        if (debug_mode && var->type == TYPE_POINTER) {
            std::cerr << "[EXPR_EVAL] Variable " << node->name << " value: " << var->value 
                      << " (0x" << std::hex << var->value << std::dec << ")" << std::endl;
        }
        
        return var->value;
    }

    case ASTNodeType::AST_VARIABLE: {
        debug_msg(DebugMsgId::EXPR_EVAL_VAR_REF, node->name.c_str());
        
        // selfキーワードの特別処理（構造体戻り値用）
        if (node->name == "self") {
            Variable *self_var = interpreter_.find_variable("self");
            if (!self_var) {
                std::string error_message = (debug_language == DebugLanguage::JAPANESE) ? 
                    "selfはメソッドコンテキスト外では使用できません" : "self can only be used within method context";
                interpreter_.throw_runtime_error_with_location(error_message, node);
            }
            
            // デバッグ出力: self変数の詳細情報
            debug_print("SELF_DEBUG: self found - type=%d, is_struct=%d, TYPE_STRUCT=%d, TYPE_INTERFACE=%d\n", 
                       (int)self_var->type, self_var->is_struct, (int)TYPE_STRUCT, (int)TYPE_INTERFACE);
            
            // selfが構造体またはインターフェース型の場合、ReturnExceptionで構造体を返す
            if (self_var->type == TYPE_STRUCT || self_var->type == TYPE_INTERFACE) {
                debug_print("SELF_DEBUG: Throwing ReturnException for struct self\n");
                interpreter_.sync_struct_members_from_direct_access("self");
                throw ReturnException(*self_var);
            } else {
                debug_print("SELF_DEBUG: self is not struct, returning primitive value\n");
                // primitive型の場合は適切な値を返す
                // 文字列の場合、特別な処理が必要な場合があるが、まずは値を返す
                return self_var->value;
            }
        }
        
        Variable *var = interpreter_.find_variable(node->name);
        if (!var) {
            debug_msg(DebugMsgId::EXPR_EVAL_VAR_NOT_FOUND, node->name.c_str());
            // エラー時にソースコード位置を表示
            std::string error_message = (debug_language == DebugLanguage::JAPANESE) ? 
                "未定義の変数です: " + node->name : "Undefined variable: " + node->name;
            interpreter_.throw_runtime_error_with_location(error_message, node);
        }

        // 参照型変数の場合、参照先変数の値を返す
        if (var->is_reference) {
            Variable* target_var = reinterpret_cast<Variable*>(var->value);
            if (!target_var) {
                throw std::runtime_error("Invalid reference variable: " + node->name);
            }
            
            if (debug_mode) {
                std::cerr << "[DEBUG] Reference access: " << node->name 
                          << " -> target value: " << target_var->value << std::endl;
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
                debug_msg(DebugMsgId::EXPR_EVAL_VAR_VALUE, node->name.c_str(), 0);
                return 0;
            } else {
                debug_msg(DebugMsgId::EXPR_EVAL_VAR_VALUE, node->name.c_str(), var->value);
                return var->value;
            }
        }

        // 構造体変数の場合、ReturnExceptionをスローして構造体データを返す
        if (var->type == TYPE_STRUCT) {
            throw ReturnException(*var);
        }

        debug_msg(DebugMsgId::EXPR_EVAL_VAR_VALUE, node->name.c_str(), var->value);
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
        auto eval_func = [this](const ASTNode* n) { return this->evaluate_expression(n); };
        auto get_member_func = [this](const Variable& v, const std::string& name) { 
            return this->get_struct_member_from_variable(v, name); 
        };
        return ArrayAccessHelpers::evaluate_array_ref(node, interpreter_, eval_func, get_member_func);
    }

    case ASTNodeType::AST_ARRAY_LITERAL: {
        return ArrayAccessHelpers::evaluate_array_literal(node, interpreter_);
    }

    // ========================================================================
    // 二項演算子の評価（+, -, *, /, %, <, >, ==, !=, &&, ||, &, |, ^, <<, >> 等）
    // Line 634-787: 算術演算、比較演算、論理演算、ビット演算
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
        bool left_is_pointer = (left & (1LL << 63)) != 0;  // メタデータポインタ
        bool right_is_pointer = (right & (1LL << 63)) != 0;
        
        // 変数がポインタ型かどうかもチェック
        if (node->left->node_type == ASTNodeType::AST_VARIABLE || 
            node->left->node_type == ASTNodeType::AST_IDENTIFIER) {
            Variable* left_var = interpreter_.find_variable(node->left->name);
            if (left_var && left_var->is_pointer) {
                left_is_pointer = true;
            }
        }
        if (node->right->node_type == ASTNodeType::AST_VARIABLE || 
            node->right->node_type == ASTNodeType::AST_IDENTIFIER) {
            Variable* right_var = interpreter_.find_variable(node->right->name);
            if (right_var && right_var->is_pointer) {
                right_is_pointer = true;
            }
        }
        
        // ポインタ同士の加算を禁止
        if (node->op == "+" && left_is_pointer && right_is_pointer) {
            throw std::runtime_error("Cannot add two pointers together. Pointer arithmetic only supports: pointer + integer, integer + pointer");
        }
        
        // ポインタ演算の特別処理
        if (node->op == "+" || node->op == "-") {
            // 左オペランドがメタデータポインタの場合
            if (left & (1LL << 63)) {
                int64_t clean_ptr = left & ~(1LL << 63);
                using namespace PointerSystem;
                PointerMetadata* meta = reinterpret_cast<PointerMetadata*>(clean_ptr);
                
                if (meta) {
                    // 真のポインタ演算：アドレス = アドレス + (オフセット × sizeof(要素型))
                    // 配列要素はint64_t（8バイト）として保存されているため、実際のメモリレイアウトに合わせる
                    ptrdiff_t offset = static_cast<ptrdiff_t>(right);
                    uintptr_t new_address;
                    size_t actual_element_size = sizeof(int64_t);  // 配列要素は常にint64_tで保存
                    
                    if (node->op == "+") {
                        new_address = meta->address + (offset * actual_element_size);
                    } else {  // "-"
                        new_address = meta->address - (offset * actual_element_size);
                    }
                    
                    // 範囲チェック（配列ポインタの場合）
                    if (meta->array_var) {
                        if (new_address < meta->array_start_addr || new_address >= meta->array_end_addr) {
                            throw std::runtime_error("Pointer arithmetic out of array bounds");
                        }
                    }
                    
                    // 新しいメタデータを作成
                    PointerMetadata* new_meta = new PointerMetadata();
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
                        new_meta->element_index = (new_address - meta->array_start_addr) / actual_element_size;
                    }
                    
                    // タグ付きポインタを返す
                    int64_t ptr_value = reinterpret_cast<int64_t>(new_meta);
                    ptr_value |= (1LL << 63);
                    return ptr_value;
                }
            }
            
            // 通常の整数演算（ポインタ演算後のフォールバック）
            result = ExpressionHelpers::evaluate_arithmetic_binary(node->op, left, right);
        }
        // 算術演算（+, -, *, /, %）
        else if (node->op == "+" || node->op == "-" || node->op == "*" || 
                 node->op == "/" || node->op == "%") {
            result = ExpressionHelpers::evaluate_arithmetic_binary(node->op, left, right);
        }
        // 比較演算（==, !=, <, >, <=, >=）
        else if (node->op == "==" || node->op == "!=" || node->op == "<" || 
                 node->op == ">" || node->op == "<=" || node->op == ">=") {
            result = ExpressionHelpers::evaluate_comparison_binary(node->op, left, right);
        }
        // 論理演算（&&, ||）
        else if (node->op == "&&" || node->op == "||") {
            result = ExpressionHelpers::evaluate_logical_binary(node->op, left, right);
        }
        // ビット演算（&, |, ^, <<, >>）
        else if (node->op == "&" || node->op == "|" || node->op == "^" || 
                 node->op == "<<" || node->op == ">>") {
            result = ExpressionHelpers::evaluate_bitwise_binary(node->op, left, right);
        }
        else {
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
            return ExpressionHelpers::evaluate_postfix_incdec(node, interpreter_);
        }
        
        // 前置インクリメント/デクリメント（++x, --x）
        if (node->op == "++" || node->op == "--") {
            return ExpressionHelpers::evaluate_prefix_incdec(node, interpreter_);
        }

        // アドレス演算子 (&)
        if (node->op == "ADDRESS_OF") {
            auto eval_func = [this](const ASTNode* n) { return this->evaluate_expression(n); };
            return AddressOperationHelpers::evaluate_address_of(node, interpreter_, eval_func);
        }
        
        // 間接参照演算子 (*)
        if (node->op == "DEREFERENCE") {
            auto eval_func = [this](const ASTNode* n) { return this->evaluate_expression(n); };
            return AddressOperationHelpers::evaluate_dereference(node, interpreter_, eval_func);
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
        return IncDecHelpers::evaluate_incdec(node, interpreter_, 
            [this](const ASTNode* n) { return this->evaluate_expression(n); });
    }

    // ========================================================================
    // 関数ポインタ呼び出し（(*funcPtr)(args)）
    // Line 1576-1719: 関数ポインタを介した間接呼び出し
    // ========================================================================
    // 関数ポインタ呼び出し（(*funcPtr)(args)）
    // Line 950-1110: 関数ポインタを介した関数呼び出しをFunctionCallHelpersに移動
    // ========================================================================
    case ASTNodeType::AST_FUNC_PTR_CALL: {
        return FunctionCallHelpers::evaluate_function_pointer_call(node, interpreter_);
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
        // 関数を探す
        const ASTNode *func = nullptr;
        
        // チェーン呼び出しのチェック: func()() (関数ポインタのチェーン)
        // leftが設定されている場合、それは関数ポインタチェーンまたはメソッドチェーンの可能性
        // 関数ポインタチェーンは非常に稀なケースなので、デフォルトでメソッドチェーンとして処理
        // ただし、leftがメソッド呼び出しでない場合（node->left->left == nullptr）で、
        // かつ戻り値が関数ポインタの場合のみ、関数ポインタチェーンとして処理する
        //
        // 判定ロジック：
        // - node->leftが存在 && node->left->node_type == AST_FUNC_CALL
        // - node->left->left == nullptr (メソッド呼び出しではない)
        // - 実行してみて、is_function_pointer == true
        // 
        // それ以外の場合は、後続のis_method_call処理に任せる
        // 
        // 後続の処理でメソッドチェーンとして適切に処理される
        // （resolve_method_receiverとcreate_chain_receiver_from_expressionが処理する）
        
        // Form 2: ptr(args) - 関数ポインタの可能性をチェック
        if (!node->left) {  // メソッド呼び出しでない場合
            std::string func_name = node->name;
            
            // 関数ポインタをfunction_pointersマップから検索
            auto& func_ptrs = interpreter_.current_scope().function_pointers;
            auto it = func_ptrs.find(func_name);
            bool found_in_local = (it != func_ptrs.end());
            
            // ローカルスコープで見つからない場合、グローバルスコープを検索
            if (!found_in_local) {
                auto& global_func_ptrs = interpreter_.get_global_scope().function_pointers;
                it = global_func_ptrs.find(func_name);
            }
            
            // 関数ポインタが見つかった場合
            if (found_in_local || it != interpreter_.get_global_scope().function_pointers.end()) {
                const FunctionPointer& func_ptr = it->second;
                const ASTNode* func_node = func_ptr.function_node;
                
                if (debug_mode) {
                    std::cerr << "[FUNC_PTR] Form 2 call: " << func_name 
                              << " -> " << func_ptr.function_name << std::endl;
                }
                
                // 引数を評価
                std::vector<int64_t> arg_values;
                std::vector<std::string> arg_strings;
                
                for (const auto& arg : node->arguments) {
                    TypedValue typed_val = interpreter_.evaluate_typed(arg.get());
                    arg_values.push_back(typed_val.value);
                    if (typed_val.type.type_info == TYPE_STRING) {
                        arg_strings.push_back(typed_val.string_value);
                    }
                }
                
                // 関数を呼び出し
                interpreter_.push_interpreter_scope();
                
                // 仮引数と実引数をバインド
                size_t param_idx = 0;
                for (const auto& param : func_node->parameters) {
                    if (param_idx >= arg_values.size()) {
                        throw std::runtime_error("Too few arguments for function pointer call");
                    }
                    
                    std::string param_name = param->name;
                    TypeInfo param_type = param->type_info;
                    bool is_unsigned = param->is_unsigned;
                    
                    if (param_type == TYPE_STRING) {
                        interpreter_.assign_variable(param_name, arg_strings[param_idx]);
                    } else {
                        interpreter_.assign_function_parameter(param_name, arg_values[param_idx], param_type, is_unsigned);
                    }
                    
                    param_idx++;
                }
                
                // 関数本体を実行
                int64_t result = 0;
                try {
                    if (func_node->body) {
                        interpreter_.exec_statement(func_node->body.get());
                    }
                } catch (const ReturnException& ret) {
                    interpreter_.pop_interpreter_scope();
                    // 戻り値を取得
                    if (ret.is_function_pointer || ret.type == TYPE_STRING || ret.is_struct || ret.is_array) {
                        throw ret;  // 複雑な型の場合はexceptionとして伝播
                    } else {
                        result = ret.value;
                    }
                    return result;
                }
                
                interpreter_.pop_interpreter_scope();
                return result;
            }
        }
        
        // 通常の関数呼び出し
        // 通常の関数呼び出し
        bool is_method_call = (node->left != nullptr); // レシーバーがある場合はメソッド呼び出し
        bool has_receiver = is_method_call;
        std::string receiver_name;
        MethodReceiverResolution receiver_resolution;
        bool impl_context_active = false;  // implコンテキストが有効かどうか
        struct MethodCallContext {
            bool uses_temp_receiver = false;
            std::string temp_variable_name;
            std::shared_ptr<ReturnException> chain_value;
            Variable concrete_receiver;
        } method_context;

        auto capture_numeric_return = [&](const TypedValue& typed_value) {
            if (node) {
                last_captured_function_value_ = std::make_pair(node, typed_value);
            }
        };

        if (is_method_call) {
            debug_msg(DebugMsgId::METHOD_CALL_START, node->name.c_str());
            receiver_resolution = resolve_method_receiver(node->left.get());

            if (receiver_resolution.kind == MethodReceiverResolution::Kind::Direct && receiver_resolution.variable_ptr) {
                receiver_name = receiver_resolution.canonical_name;
            } else if (receiver_resolution.kind == MethodReceiverResolution::Kind::Chain && receiver_resolution.chain_value) {
                method_context.chain_value = receiver_resolution.chain_value;

                const ReturnException& chain_ret = *receiver_resolution.chain_value;
                if (chain_ret.is_array) {
                    throw chain_ret;
                }
                
                // 関数ポインタチェーン: getOperation(3)(6, 7)のようなケース
                if (chain_ret.is_function_pointer) {
                    if (debug_mode) {
                        std::cerr << "[FUNC_PTR_CHAIN] Function pointer chain detected, value=" 
                                  << chain_ret.value << std::endl;
                    }
                    
                    // function_pointersマップから関数ポインタ情報を探す
                    const FunctionPointer* found_ptr = nullptr;
                    
                    // 現在のスコープを検索
                    for (const auto& pair : interpreter_.current_scope().function_pointers) {
                        Variable* var = interpreter_.find_variable(pair.first);
                        if (var && var->value == chain_ret.value) {
                            found_ptr = &pair.second;
                            break;
                        }
                    }
                    
                    // グローバルスコープも検索
                    if (!found_ptr) {
                        for (const auto& pair : interpreter_.get_global_scope().function_pointers) {
                            Variable* var = interpreter_.find_variable(pair.first);
                            if (var && var->value == chain_ret.value) {
                                found_ptr = &pair.second;
                                break;
                            }
                        }
                    }
                    
                    if (!found_ptr) {
                        throw std::runtime_error("Function pointer chain: pointer not found in function_pointers map");
                    }
                    
                    const ASTNode* func_node = found_ptr->function_node;
                    
                    if (debug_mode) {
                        std::cerr << "[FUNC_PTR_CHAIN] Calling function: " << found_ptr->function_name << std::endl;
                    }
                    
                    // 引数を評価
                    std::vector<int64_t> arg_values;
                    std::vector<std::string> arg_strings;
                    
                    for (const auto& arg : node->arguments) {
                        TypedValue typed_val = interpreter_.evaluate_typed(arg.get());
                        arg_values.push_back(typed_val.value);
                        if (typed_val.type.type_info == TYPE_STRING) {
                            arg_strings.push_back(typed_val.string_value);
                        }
                    }
                    
                    // 関数を呼び出し
                    interpreter_.push_interpreter_scope();
                    
                    // 仮引数と実引数をバインド
                    size_t param_idx = 0;
                    for (const auto& param : func_node->parameters) {
                        if (param_idx >= arg_values.size()) {
                            throw std::runtime_error("Too few arguments for function pointer chain call");
                        }
                        
                        std::string param_name = param->name;
                        TypeInfo param_type = param->type_info;
                        bool is_unsigned = param->is_unsigned;
                        
                        if (param_type == TYPE_STRING) {
                            interpreter_.assign_variable(param_name, arg_strings[param_idx]);
                        } else {
                            interpreter_.assign_function_parameter(param_name, arg_values[param_idx], param_type, is_unsigned);
                        }
                        
                        param_idx++;
                    }
                    
                    // 関数本体を実行
                    int64_t result = 0;
                    try {
                        if (func_node->body) {
                            interpreter_.exec_statement(func_node->body.get());
                        }
                    } catch (const ReturnException& ret) {
                        interpreter_.pop_interpreter_scope();
                        if (ret.is_function_pointer || ret.type == TYPE_STRING || ret.is_struct || ret.is_array) {
                            throw ret;  // 複雑な型の場合はexceptionとして伝播
                        } else {
                            result = ret.value;
                        }
                        return result;
                    }
                    
                    interpreter_.pop_interpreter_scope();
                    return result;
                }

                method_context.uses_temp_receiver = true;
                method_context.temp_variable_name = "__chain_receiver_" + std::to_string(rand() % 10000);

                Variable temp_receiver;
                temp_receiver.is_assigned = true;

                if (chain_ret.type == TYPE_STRUCT || chain_ret.type == TYPE_INTERFACE || chain_ret.is_struct) {
                    temp_receiver = chain_ret.struct_value;

                    if (temp_receiver.type == TYPE_INTERFACE) {
                        bool has_struct_members = temp_receiver.is_struct || !temp_receiver.struct_members.empty();
                        if (has_struct_members) {
                            temp_receiver.type = TYPE_STRUCT;
                            temp_receiver.is_struct = true;
                        } else {
                            TypeInfo resolved = TYPE_UNKNOWN;
                            if (!temp_receiver.struct_type_name.empty()) {
                                resolved = interpreter_.get_type_manager()->string_to_type_info(temp_receiver.struct_type_name);
                            }
                            if (resolved == TYPE_UNKNOWN && temp_receiver.current_type != TYPE_UNKNOWN) {
                                resolved = temp_receiver.current_type;
                            }
                            if (resolved == TYPE_UNKNOWN) {
                                resolved = TYPE_INT;
                            }
                            // Interface型の場合、現在保持している値を保持する必要がある
                            // (temp_receiverはchain_ret.struct_valueからコピーされており、既に正しい値を持っている)
                            temp_receiver.type = resolved;
                            temp_receiver.is_struct = false;
                        }
                    } else if (temp_receiver.type != TYPE_STRUCT && temp_receiver.is_struct) {
                        temp_receiver.type = TYPE_STRUCT;
                    }

                    if (temp_receiver.type == TYPE_STRUCT) {
                        temp_receiver.is_struct = true;
                    }
                } else if (chain_ret.type == TYPE_STRING) {
                    temp_receiver.type = TYPE_STRING;
                    temp_receiver.str_value = chain_ret.str_value;
                } else {
                    temp_receiver.type = chain_ret.type;
                    temp_receiver.value = chain_ret.value;
                }

                method_context.concrete_receiver = temp_receiver;
                interpreter_.add_temp_variable(method_context.temp_variable_name, temp_receiver);
                receiver_name = method_context.temp_variable_name;
                receiver_resolution.kind = MethodReceiverResolution::Kind::Direct;
                receiver_resolution.variable_ptr = interpreter_.find_variable(receiver_name);
            } else {
                throw std::runtime_error("Invalid method receiver");
            }

            Variable* receiver_var = receiver_resolution.variable_ptr;
            if (!receiver_var) {
                receiver_var = interpreter_.find_variable(receiver_name);
            }
            if (!receiver_var) {
                throw std::runtime_error("Undefined receiver: " + receiver_name);
            }
            debug_msg(DebugMsgId::METHOD_CALL_RECEIVER_FOUND, receiver_name.c_str());
            debug_print("RECEIVER_DEBUG: Looking for receiver '%s'\n", receiver_name.c_str());

            std::string type_name;

            auto resolve_struct_like_type = [&](const Variable &var) -> std::string {
                if (!var.struct_type_name.empty()) {
                    return var.struct_type_name;
                }
                if (!var.implementing_struct.empty()) {
                    return var.implementing_struct;
                }
                if (var.type == TYPE_UNION && var.current_type != TYPE_UNKNOWN) {
                    return type_info_to_string(var.current_type);
                }
                return std::string();
            };

            // Check if receiver is a pointer type
            if (receiver_var->type == TYPE_POINTER) {
                // Dereference the pointer to get the actual struct
                int64_t ptr_value = receiver_var->value;
                if (ptr_value == 0) {
                    throw std::runtime_error("Null pointer dereference in method call");
                }
                Variable* pointed_struct = reinterpret_cast<Variable*>(ptr_value);
                if (pointed_struct) {
                    if (debug_mode) {
                        debug_print("POINTER_DEREF_BEFORE: type=%d, is_struct=%d, struct_type_name='%s'\n",
                                   static_cast<int>(pointed_struct->type), pointed_struct->is_struct ? 1 : 0,
                                   pointed_struct->struct_type_name.c_str());
                    }
                    
                    type_name = resolve_struct_like_type(*pointed_struct);
                    if (type_name.empty() && (pointed_struct->type == TYPE_STRUCT || pointed_struct->is_struct)) {
                        type_name = pointed_struct->struct_type_name;
                    }
                    
                    // Ensure pointed_struct is recognized as a struct (but don't overwrite TYPE_INTERFACE)
                    // Interface型の場合は型情報を保持する
                    if (pointed_struct->type != TYPE_INTERFACE && pointed_struct->interface_name.empty() &&
                        (!pointed_struct->struct_type_name.empty() || !pointed_struct->struct_members.empty())) {
                        pointed_struct->type = TYPE_STRUCT;
                        pointed_struct->is_struct = true;
                    }
                    
                    // Update receiver_var to point to the dereferenced struct
                    receiver_var = pointed_struct;
                    receiver_resolution.variable_ptr = pointed_struct;
                    debug_print("POINTER_METHOD: Dereferenced pointer, type='%s', is_struct=%d\n", 
                               type_name.c_str(), pointed_struct->is_struct ? 1 : 0);
                }
            }
            
            if (type_name.empty() && (receiver_var->type >= TYPE_ARRAY_BASE || receiver_var->is_array)) {
                type_name = resolve_struct_like_type(*receiver_var);
                if (type_name.empty()) {
                    TypeInfo base_type = TYPE_UNKNOWN;
                    if (receiver_var->type >= TYPE_ARRAY_BASE) {
                        base_type = static_cast<TypeInfo>(receiver_var->type - TYPE_ARRAY_BASE);
                    } else if (receiver_var->array_type_info.base_type != TYPE_UNKNOWN) {
                        base_type = receiver_var->array_type_info.base_type;
                    }
                    if (base_type == TYPE_UNKNOWN) {
                        base_type = TYPE_INT;
                    }
                    type_name = type_info_to_string(base_type) + "[]";
                }
            } else if (type_name.empty() && (receiver_var->type == TYPE_STRUCT || receiver_var->is_struct)) {
                type_name = resolve_struct_like_type(*receiver_var);
            } else if (type_name.empty() && (receiver_var->type == TYPE_INTERFACE || !receiver_var->interface_name.empty())) {
                type_name = resolve_struct_like_type(*receiver_var);
                if (type_name.empty()) {
                    type_name = receiver_var->interface_name;
                }
                debug_msg(DebugMsgId::METHOD_CALL_INTERFACE, node->name.c_str(), type_name.c_str());
            } else {
                type_name = resolve_struct_like_type(*receiver_var);
                if (type_name.empty()) {
                    type_name = type_info_to_string(receiver_var->type);
                }
            }

            if (type_name.empty()) {
                type_name = type_info_to_string(receiver_var->type);
            }

            std::string method_key = type_name + "::" + node->name;
            auto &global_scope = interpreter_.get_global_scope();
            auto it = global_scope.functions.find(method_key);
            if (it != global_scope.functions.end()) {
                func = it->second;
            } else {
                for (const auto& impl_def : interpreter_.get_impl_definitions()) {
                    if (impl_def.struct_name == type_name) {
                        std::string method_full_name = impl_def.interface_name + "_" + impl_def.struct_name + "_" + node->name;
                        auto it2 = global_scope.functions.find(method_full_name);
                        if (it2 != global_scope.functions.end()) {
                            func = it2->second;
                            break;
                        }
                    }
                }
            }
        } else {
            auto &global_scope = interpreter_.get_global_scope();
            auto it = global_scope.functions.find(node->name);
            if (it != global_scope.functions.end()) {
                func = it->second;
            }
        }

        if (!func) {
            if (is_method_call) {
                std::string debug_type_name;
                if (is_method_call) {
                    if (!receiver_name.empty()) {
                        Variable* debug_receiver = interpreter_.find_variable(receiver_name);
                        if (!debug_receiver && receiver_resolution.variable_ptr) {
                            debug_receiver = receiver_resolution.variable_ptr;
                        }
                        if (debug_receiver) {
                            if (!debug_receiver->struct_type_name.empty()) {
                                debug_type_name = debug_receiver->struct_type_name;
                            } else {
                                debug_type_name = type_info_to_string(debug_receiver->type);
                            }
                        }
                    }
                }
                std::cerr << "[METHOD_LOOKUP_FAIL] receiver='" << receiver_name
                          << "' type='" << debug_type_name
                          << "' method='" << node->name << "'" << std::endl;
            }
            throw std::runtime_error("Undefined function: " + node->name);
        }

        if (is_method_call && !receiver_name.empty()) {
            std::string private_check_name = receiver_name;

            if (private_check_name != "self") {
                Variable* receiver_var = interpreter_.find_variable(private_check_name);
                if (!receiver_var && receiver_resolution.variable_ptr) {
                    receiver_var = receiver_resolution.variable_ptr;
                }
                if (receiver_var) {
                    std::string type_name;
                    if (receiver_var->type == TYPE_STRUCT) {
                        type_name = receiver_var->struct_type_name;
                    } else if (!receiver_var->interface_name.empty()) {
                        type_name = receiver_var->struct_type_name;
                    } else {
                        type_name = type_info_to_string(receiver_var->type);
                    }
                    for (const auto& impl_def : interpreter_.get_impl_definitions()) {
                        if (impl_def.struct_name == type_name) {
                            for (const auto& method : impl_def.methods) {
                                if (method->name == node->name && method->is_private_method) {
                                    throw std::runtime_error("Cannot access private method '" + node->name + "' from outside the impl block");
                                }
                            }
                            break;
                        }
                    }
                }
            }
        }

        // 新しいスコープを作成
        auto cleanup_method_context = [&]() {
            if (method_context.uses_temp_receiver && !method_context.temp_variable_name.empty()) {
                Variable* temp_var = interpreter_.find_variable(method_context.temp_variable_name);
                if (temp_var && method_context.chain_value) {
                    if (temp_var->type == TYPE_STRUCT || temp_var->is_struct) {
                        method_context.chain_value->struct_value = *temp_var;
                        method_context.chain_value->struct_value.type = TYPE_STRUCT;
                        method_context.chain_value->struct_value.is_struct = true;
                        method_context.chain_value->is_struct = true;
                        method_context.chain_value->type = TYPE_STRUCT;
                    } else if (temp_var->type == TYPE_STRING) {
                        method_context.chain_value->str_value = temp_var->str_value;
                        method_context.chain_value->type = TYPE_STRING;
                        method_context.chain_value->is_struct = false;
                        method_context.chain_value->is_array = false;
                    } else {
                        method_context.chain_value->value = temp_var->value;
                        method_context.chain_value->type = temp_var->type;
                        method_context.chain_value->is_struct = false;
                        method_context.chain_value->is_array = false;
                    }
                }
                interpreter_.remove_temp_variable(method_context.temp_variable_name);
                method_context.uses_temp_receiver = false;
            }
        };
    interpreter_.push_scope();
    bool method_scope_active = true;
        
        // メソッド呼び出しの場合、selfコンテキストを設定
        bool used_resolution_ptr = false;  // Track if we used pointer dereference
        Variable* dereferenced_struct_ptr = nullptr;  // Store the dereferenced struct pointer
        
        if (is_method_call) {
            Variable* receiver_var = nullptr;
            
            // Prioritize receiver_resolution.variable_ptr (e.g., after pointer dereference)
            used_resolution_ptr = false;
            if (debug_mode) {
                debug_print("SELF_SETUP_RESOLUTION: receiver_resolution.variable_ptr=%p, receiver_name='%s'\n",
                           static_cast<void*>(receiver_resolution.variable_ptr), receiver_name.c_str());
            }
            if (receiver_resolution.variable_ptr) {
                receiver_var = receiver_resolution.variable_ptr;
                used_resolution_ptr = true;
                dereferenced_struct_ptr = receiver_resolution.variable_ptr;  // Save pointer for writeback
                if (debug_mode) {
                    debug_print("SELF_SETUP_USING_RESOLUTION: type=%d, is_struct=%d, struct_type_name='%s'\n",
                               static_cast<int>(receiver_var->type), receiver_var->is_struct ? 1 : 0,
                               receiver_var->struct_type_name.c_str());
                }
            } else if (!receiver_name.empty()) {
                receiver_var = interpreter_.find_variable(receiver_name);
            } else if (node->left &&
                (node->left->node_type == ASTNodeType::AST_VARIABLE || node->left->node_type == ASTNodeType::AST_IDENTIFIER)) {
                receiver_var = interpreter_.find_variable(node->left->name);
                if (receiver_name.empty()) {
                    receiver_name = node->left->name;
                }
            }

            if (!receiver_var) {
                std::string error_name = receiver_name;
                if (error_name.empty() && node->left) {
                    error_name = node->left->name;
                }
                throw std::runtime_error("Receiver variable not found: " + error_name);
            }

            // Only sync if receiver_var was not from pointer dereference
            // (i.e., not from receiver_resolution.variable_ptr)
            if (!used_resolution_ptr && !receiver_name.empty() &&
                (receiver_var->type == TYPE_STRUCT || receiver_var->type == TYPE_INTERFACE || receiver_var->is_struct)) {
                interpreter_.sync_struct_members_from_direct_access(receiver_name);
                Variable* synced_var = interpreter_.find_variable(receiver_name);
                if (synced_var) {
                    receiver_var = synced_var;
                }
            }

            auto& current_scope = interpreter_.get_current_scope();
            
            // Copy receiver to self
            current_scope.variables["self"] = *receiver_var;
            
            // Ensure self has correct type info after copy
            Variable& self_var = current_scope.variables["self"];
            if (debug_mode) {
                debug_print("SELF_SETUP_BEFORE: self.type=%d, self.is_struct=%d, struct_type_name='%s', struct_members=%zu\n",
                           static_cast<int>(self_var.type), self_var.is_struct ? 1 : 0,
                           self_var.struct_type_name.c_str(), self_var.struct_members.size());
            }
            // Only mark as struct if it actually has struct members or is already TYPE_STRUCT
            // Don't mark primitive types as struct even if they have a type_name
            if (self_var.type == TYPE_STRUCT || !self_var.struct_members.empty()) {
                self_var.type = TYPE_STRUCT;
                self_var.is_struct = true;
            }
            if (debug_mode) {
                debug_print("SELF_SETUP_AFTER: self.type=%d, self.is_struct=%d\n",
                           static_cast<int>(self_var.type), self_var.is_struct ? 1 : 0);
            }

            if (!receiver_name.empty()) {
                Variable receiver_info;
                receiver_info.type = TYPE_STRING;
                receiver_info.str_value = receiver_name;
                receiver_info.is_assigned = true;
                current_scope.variables["__self_receiver__"] = receiver_info;
                debug_msg(DebugMsgId::METHOD_CALL_SELF_CONTEXT_SET, receiver_name.c_str());
            }

            if (receiver_var->type == TYPE_STRUCT || receiver_var->type == TYPE_INTERFACE || receiver_var->is_struct) {
                for (const auto& member_pair : receiver_var->struct_members) {
                    const std::string& member_name = member_pair.first;
                    std::string self_member_path = "self." + member_name;
                    Variable member_value = member_pair.second;

                    if (!receiver_name.empty()) {
                        if (Variable* direct_member_var = interpreter_.find_variable(receiver_name + "." + member_name)) {
                            member_value = *direct_member_var;
                        } else {
                            try {
                                if (Variable* struct_member = interpreter_.get_struct_member(receiver_name, member_name)) {
                                    member_value = *struct_member;
                                }
                            } catch (...) {
                                // ignore fallback failures
                            }
                        }
                    }

                    if (member_pair.second.is_multidimensional) {
                        member_value.is_multidimensional = true;
                        member_value.array_dimensions = member_pair.second.array_dimensions;
                        member_value.multidim_array_values = member_pair.second.multidim_array_values;
                        debug_print("SELF_SETUP: Preserved multidimensional info for %s (dimensions: %zu, values: %zu)\n",
                                    self_member_path.c_str(),
                                    member_pair.second.array_dimensions.size(),
                                    member_pair.second.multidim_array_values.size());
                    }

                    if (member_value.is_array) {
                        const bool is_string_array = (member_value.type == TYPE_STRING);

                        int total_elements = member_value.array_size;
                        if (total_elements <= 0) {
                            if (member_value.is_multidimensional && !member_value.multidim_array_values.empty()) {
                                total_elements = static_cast<int>(member_value.multidim_array_values.size());
                            } else if (!member_value.array_values.empty()) {
                                total_elements = static_cast<int>(member_value.array_values.size());
                            } else if (!member_value.array_dimensions.empty()) {
                                total_elements = 1;
                                for (int dim_size : member_value.array_dimensions) {
                                    if (dim_size == 0) {
                                        total_elements = 0;
                                        break;
                                    }
                                    total_elements *= dim_size;
                                }
                            }
                        }

                        if (total_elements < 0) {
                            total_elements = 0;
                        }

                        member_value.array_size = total_elements;

                        if (!is_string_array) {
                            if (member_value.is_multidimensional) {
                                if (member_value.array_values.size() < member_value.multidim_array_values.size()) {
                                    member_value.array_values = member_value.multidim_array_values;
                                } else if (member_value.array_values.empty() && !member_value.multidim_array_values.empty()) {
                                    member_value.array_values = member_value.multidim_array_values;
                                }
                            }
                            if (member_value.array_values.size() < static_cast<size_t>(total_elements)) {
                                member_value.array_values.resize(total_elements, 0);
                            }
                            if (member_value.is_multidimensional && member_value.multidim_array_values.size() < static_cast<size_t>(total_elements)) {
                                member_value.multidim_array_values.resize(total_elements, 0);
                            }
                        } else {
                            if (member_value.array_strings.size() < static_cast<size_t>(total_elements)) {
                                member_value.array_strings.resize(total_elements);
                            }
                        }

                        for (int idx = 0; idx < total_elements; ++idx) {
                            std::string element_path = self_member_path + "[" + std::to_string(idx) + "]";
                            Variable element_var;
                            bool element_assigned = false;

                            if (!receiver_name.empty()) {
                                std::string receiver_element_path = receiver_name + "." + member_name + "[" + std::to_string(idx) + "]";
                                if (Variable* receiver_element = interpreter_.find_variable(receiver_element_path)) {
                                    element_var = *receiver_element;
                                    element_assigned = true;
                                }
                            }

                            if (!element_assigned) {
                                element_var.type = is_string_array ? TYPE_STRING : member_value.type;
                                element_var.is_assigned = true;
                                if (is_string_array) {
                                    std::string value = (idx < static_cast<int>(member_value.array_strings.size()))
                                                        ? member_value.array_strings[idx]
                                                        : std::string();
                                    element_var.str_value = value;
                                } else {
                                    int64_t value = 0;
                                    if (member_value.is_multidimensional && idx < static_cast<int>(member_value.multidim_array_values.size())) {
                                        value = member_value.multidim_array_values[idx];
                                    } else if (idx < static_cast<int>(member_value.array_values.size())) {
                                        value = member_value.array_values[idx];
                                    }
                                    element_var.value = value;
                                }
                            }

                            current_scope.variables[element_path] = element_var;

                            if (is_string_array) {
                                if (idx >= static_cast<int>(member_value.array_strings.size())) {
                                    member_value.array_strings.resize(idx + 1);
                                }
                                member_value.array_strings[idx] = element_var.str_value;
                            } else {
                                if (idx >= static_cast<int>(member_value.array_values.size())) {
                                    member_value.array_values.resize(idx + 1);
                                }
                                member_value.array_values[idx] = element_var.value;
                                if (member_value.is_multidimensional) {
                                    if (idx >= static_cast<int>(member_value.multidim_array_values.size())) {
                                        member_value.multidim_array_values.resize(idx + 1);
                                    }
                                    member_value.multidim_array_values[idx] = element_var.value;
                                }
                            }
                        }
                    }

                    current_scope.variables[self_member_path] = member_value;
                    debug_print("SELF_SETUP: Created %s\n", self_member_path.c_str());
                    
                    // メンバーが構造体の場合、そのネストメンバーも再帰的に作成
                    if (member_value.type == TYPE_STRUCT || member_value.is_struct) {
                        std::string nested_base_name = receiver_name + "." + member_name;
                        
                        // ネストした構造体の個別変数を作成
                        for (const auto& nested_member_pair : member_value.struct_members) {
                            const std::string& nested_member_name = nested_member_pair.first;
                            std::string nested_self_path = self_member_path + "." + nested_member_name;
                            std::string nested_receiver_path = nested_base_name + "." + nested_member_name;
                            
                            Variable nested_member_value = nested_member_pair.second;
                            
                            // receiver側の個別変数から値を取得
                            if (Variable* nested_direct_var = interpreter_.find_variable(nested_receiver_path)) {
                                nested_member_value = *nested_direct_var;
                            }
                            
                            current_scope.variables[nested_self_path] = nested_member_value;
                            debug_print("SELF_SETUP: Created nested member %s = %lld\n", 
                                       nested_self_path.c_str(), nested_member_value.value);
                        }
                    }
                }
                debug_msg(DebugMsgId::METHOD_CALL_SELF_MEMBER_SETUP);
            }
        }
        
        // 現在の関数名を設定
        std::string prev_function_name = interpreter_.current_function_name;
        interpreter_.current_function_name = node->name;
        
        debug_msg(DebugMsgId::METHOD_CALL_EXECUTE, node->name.c_str());
        
        try {
            // パラメータの評価と設定
            if (func->parameters.size() != node->arguments.size()) {
                throw std::runtime_error("Argument count mismatch for function: " + node->name);
            }
            
            for (size_t i = 0; i < func->parameters.size(); i++) {
                const auto &param = func->parameters[i];
                const auto &arg = node->arguments[i];
                
                // 関数ポインタパラメータのサポート
                if (param->type_info == TYPE_POINTER && 
                    arg->node_type == ASTNodeType::AST_UNARY_OP && 
                    arg->op == "ADDRESS_OF" && 
                    arg->is_function_address) {
                    // 引数が関数アドレス（&func形式）の場合
                    // まず関数が実際に存在するかを確認
                    std::string func_name = arg->function_address_name;
                    const ASTNode* target_func = interpreter_.find_function(func_name);
                    
                    // 関数が見つかった場合のみ関数ポインタとして処理
                    if (target_func) {
                        // 関数ポインタとして登録
                        FunctionPointer func_ptr(target_func, func_name, target_func->type_info);
                        interpreter_.current_scope().function_pointers[param->name] = func_ptr;
                        
                        // 変数としても登録（値は関数ノードの実際のメモリアドレス）
                        int64_t func_address = reinterpret_cast<int64_t>(target_func);
                        interpreter_.assign_function_parameter(param->name, func_address, TYPE_POINTER, false);
                        
                        // 変数に関数ポインタフラグを設定
                        Variable* param_var = interpreter_.find_variable(param->name);
                        if (param_var) {
                            param_var->is_function_pointer = true;
                            param_var->function_pointer_name = func_name;
                        }
                        
                        if (debug_mode) {
                            std::cerr << "[FUNC_CALL] Registered function pointer argument: " 
                                      << param->name << " = &" << func_name << std::endl;
                        }
                        
                        continue;  // 次のパラメータへ
                    }
                    // 関数が見つからない場合は通常のポインタパラメータとして処理を継続
                }
                
                // 参照パラメータのサポート
                if (param->is_reference) {
                    // 参照パラメータは変数のみを受け取れる
                    if (arg->node_type != ASTNodeType::AST_VARIABLE && arg->node_type != ASTNodeType::AST_IDENTIFIER) {
                        throw std::runtime_error("Reference parameter '" + param->name + "' requires a variable, not an expression");
                    }
                    
                    // 引数の変数を取得
                    Variable* source_var = interpreter_.find_variable(arg->name);
                    if (!source_var) {
                        throw std::runtime_error("Undefined variable for reference parameter: " + arg->name);
                    }
                    
                    // 参照変数を作成（参照先のポインタを保存）
                    Variable ref_var;
                    ref_var.is_reference = true;
                    ref_var.is_assigned = true;
                    ref_var.type = source_var->type;
                    ref_var.value = reinterpret_cast<int64_t>(source_var);
                    
                    // 構造体型情報をコピー
                    ref_var.struct_type_name = source_var->struct_type_name;
                    ref_var.is_struct = source_var->is_struct;
                    ref_var.type_name = source_var->type_name;
                    ref_var.interface_name = source_var->interface_name;
                    ref_var.implementing_struct = source_var->implementing_struct;
                    
                    // ポインタ型情報をコピー
                    ref_var.is_pointer = source_var->is_pointer;
                    ref_var.pointer_depth = source_var->pointer_depth;
                    ref_var.pointer_base_type = source_var->pointer_base_type;
                    ref_var.pointer_base_type_name = source_var->pointer_base_type_name;
                    
                    // 参照の連鎖対応（source_varも参照なら実体を取得）
                    if (source_var->is_reference) {
                        Variable* target_var = reinterpret_cast<Variable*>(source_var->value);
                        ref_var.value = reinterpret_cast<int64_t>(target_var);
                        // 参照先の型情報も更新
                        ref_var.type = target_var->type;
                        ref_var.struct_type_name = target_var->struct_type_name;
                        ref_var.is_struct = target_var->is_struct;
                        ref_var.type_name = target_var->type_name;
                    }
                    
                    // パラメータスコープに参照変数を登録
                    interpreter_.current_scope().variables[param->name] = ref_var;
                    continue;  // 次のパラメータへ
                }
                
                // 配列パラメータのサポート
                if (param->is_array) {
                    if (arg->node_type == ASTNodeType::AST_VARIABLE) {
                        // 変数として渡された場合
                        Variable* source_var = interpreter_.find_variable(arg->name);
                        if (!source_var || !source_var->is_array) {
                            throw std::runtime_error("Array argument expected for parameter: " + param->name);
                        }
                        
                        // 配列をコピーしてパラメータに設定
                        interpreter_.assign_array_parameter(param->name, *source_var, param->type_info);
                        
                        // const修飾を設定
                        if (param->is_const) {
                            Variable* param_var = interpreter_.find_variable(param->name);
                            if (param_var) {
                                param_var->is_const = true;
                            }
                        }
                    } else if (arg->node_type == ASTNodeType::AST_ARRAY_LITERAL) {
                        // 配列リテラルとして直接渡された場合
                        debug_msg(DebugMsgId::ARRAY_LITERAL_INIT_PROCESSING,
                                ("Processing array literal argument for parameter: " + param->name).c_str());
                        
                        // 一時的な配列変数を作成
                        std::string temp_var_name = "__temp_array_" + std::to_string(i);
                        Variable temp_var;
                        temp_var.is_array = true;
                        temp_var.type = param->type_info;
                        temp_var.is_assigned = false;
                        
                        // 配列リテラルから値を取得
                        std::vector<int64_t> values;
                        std::vector<std::string> str_values;
                        
                        for (const auto &element : arg->arguments) {
                            if (element->node_type == ASTNodeType::AST_STRING_LITERAL) {
                                str_values.push_back(element->str_value);
                            } else {
                                int64_t val = evaluate_expression(element.get());
                                values.push_back(val);
                            }
                        }
                        
                        // 一時変数に値を設定
                        if (!str_values.empty()) {
                            temp_var.array_strings = str_values;
                            temp_var.array_size = str_values.size();
                            temp_var.type = static_cast<TypeInfo>(TYPE_ARRAY_BASE + TYPE_STRING);
                        } else {
                            temp_var.array_values = values;
                            temp_var.array_size = values.size();
                            temp_var.type = static_cast<TypeInfo>(TYPE_ARRAY_BASE + TYPE_INT);
                        }
                        temp_var.is_assigned = true;
                        
                        // パラメータに設定
                        interpreter_.assign_array_parameter(param->name, temp_var, param->type_info);
                        
                        // const修飾を設定
                        if (param->is_const) {
                            Variable* param_var = interpreter_.find_variable(param->name);
                            if (param_var) {
                                param_var->is_const = true;
                            }
                        }
                    } else {
                        throw std::runtime_error("Only array variables can be passed as array parameters");
                    }
                } else {
                    // 通常の値パラメータの型チェック
                    // 引数の型を事前にチェック
                    if (arg->node_type == ASTNodeType::AST_STRING_LITERAL && param->type_info != TYPE_STRING) {
                        throw std::runtime_error("Type mismatch: cannot pass string literal to non-string parameter '" + param->name + "'");
                    }
                    
                    // 文字列パラメータの場合
                    if (param->type_info == TYPE_STRING) {
                        if (arg->node_type == ASTNodeType::AST_STRING_LITERAL) {
                            // 文字列リテラルを直接代入
                            Variable param_var;
                            param_var.type = TYPE_STRING;
                            param_var.str_value = arg->str_value;
                            param_var.is_assigned = true;
                            param_var.is_const = param->is_const;  // パラメータのconst修飾を保持
                            interpreter_.current_scope().variables[param->name] = param_var;
                        } else if (arg->node_type == ASTNodeType::AST_VARIABLE) {
                            // 文字列変数を代入
                            Variable* source_var = interpreter_.find_variable(arg->name);
                            if (!source_var || source_var->type != TYPE_STRING) {
                                throw std::runtime_error("Type mismatch: expected string variable for parameter '" + param->name + "'");
                            }
                            Variable param_var;
                            param_var.type = TYPE_STRING;
                            param_var.str_value = source_var->str_value;
                            param_var.is_assigned = true;
                            param_var.is_const = param->is_const;  // パラメータのconst修飾を保持
                            interpreter_.current_scope().variables[param->name] = param_var;
                        } else {
                            throw std::runtime_error("Type mismatch: cannot pass non-string expression to string parameter '" + param->name + "'");
                        }
                    } else {
                        auto is_interface_compatible = [](const Variable* var) {
                            if (!var) {
                                return false;
                            }
                            if (var->is_struct || var->type == TYPE_INTERFACE) {
                                return true;
                            }
                            if (var->type >= TYPE_ARRAY_BASE) {
                                return true;
                            }
                            switch (var->type) {
                                case TYPE_INT:
                                case TYPE_LONG:
                                case TYPE_SHORT:
                                case TYPE_TINY:
                                case TYPE_BOOL:
                                case TYPE_STRING:
                                case TYPE_CHAR:
                                    return true;
                                default:
                                    return false;
                            }
                        };

                        auto assign_interface_argument = [&](const Variable& source, const std::string& source_name) {
                            Variable interface_placeholder(param->type_name, true);
                            interpreter_.assign_interface_view(param->name, interface_placeholder, source, source_name);
                        };

                        bool param_is_interface = false;
                        if (param->type_info == TYPE_INTERFACE) {
                            param_is_interface = true;
                        } else if (!param->type_name.empty()) {
                            if (interpreter_.find_interface_definition(param->type_name) != nullptr) {
                                param_is_interface = true;
                            }
                        }

                        if (param_is_interface) {
                            if (arg->node_type == ASTNodeType::AST_VARIABLE || arg->node_type == ASTNodeType::AST_IDENTIFIER) {
                                std::string source_name = arg->name;
                                Variable* source_var = interpreter_.find_variable(source_name);
                                if (!source_var) {
                                    throw std::runtime_error("Source variable not found: " + source_name);
                                }
                                if (!is_interface_compatible(source_var)) {
                                    throw std::runtime_error("Cannot pass non-struct/non-primitive to interface parameter '" + param->name + "'");
                                }
                                assign_interface_argument(*source_var, source_name);
                            } else if (arg->node_type == ASTNodeType::AST_STRING_LITERAL) {
                                Variable temp;
                                temp.type = TYPE_STRING;
                                temp.str_value = arg->str_value;
                                temp.is_assigned = true;
                                temp.struct_type_name = "string";
                                assign_interface_argument(temp, "");
                            } else {
                                auto build_temp_from_primitive = [&](TypeInfo value_type, int64_t numeric_value, const std::string& string_value) {
                                    Variable temp;
                                    temp.type = value_type;
                                    temp.is_assigned = true;
                                    if (!arg->type_name.empty()) {
                                        temp.struct_type_name = arg->type_name;
                                    } else {
                                        temp.struct_type_name = type_info_to_string(value_type);
                                    }
                                    if (value_type == TYPE_STRING) {
                                        temp.str_value = string_value;
                                    } else {
                                        temp.value = numeric_value;
                                    }
                                    return temp;
                                };

                                try {
                                    int64_t numeric_value = evaluate_expression(arg.get());
                                    TypeInfo resolved_type = arg->type_info != TYPE_UNKNOWN ? arg->type_info : TYPE_INT;
                                    if (resolved_type == TYPE_STRING) {
                                        Variable temp = build_temp_from_primitive(TYPE_STRING, 0, arg->str_value);
                                        assign_interface_argument(temp, "");
                                    } else {
                                        Variable temp = build_temp_from_primitive(resolved_type, numeric_value, "");
                                        assign_interface_argument(temp, "");
                                    }
                                } catch (const ReturnException& ret) {
                                    if (ret.is_array) {
                                        throw std::runtime_error("Cannot pass array return value to interface parameter '" + param->name + "'");
                                    }
                                    if (!ret.is_struct && ret.type == TYPE_STRING) {
                                        Variable temp = build_temp_from_primitive(TYPE_STRING, 0, ret.str_value);
                                        assign_interface_argument(temp, "");
                                    } else if (!ret.is_struct) {
                                        Variable temp = build_temp_from_primitive(ret.type, ret.value, ret.str_value);
                                        assign_interface_argument(temp, "");
                                    } else {
                                        assign_interface_argument(ret.struct_value, "");
                                    }
                                }
                            }
                            continue;
                        }

                        // struct型パラメータかチェック
                        if (param->type_info == TYPE_STRUCT) {
                            Variable* source_var = nullptr;
                            std::string source_var_name;
                            
                            if (arg->node_type == ASTNodeType::AST_VARIABLE) {
                                // struct変数を引数として渡す場合
                                source_var_name = arg->name;
                                source_var = interpreter_.find_variable(arg->name);
                            } else if (arg->node_type == ASTNodeType::AST_ARRAY_REF) {
                                // 構造体配列要素を引数として渡す場合 (struct_array[0])
                                std::string array_name = arg->left->name;
                                int64_t index = evaluate_expression(arg->array_index.get());
                                source_var_name = array_name + "[" + std::to_string(index) + "]";
                                
                                // 配列要素の最新状態を同期
                                interpreter_.sync_struct_members_from_direct_access(source_var_name);
                                // 同期後に再度取得
                                source_var = interpreter_.find_variable(source_var_name);
                            }
                            
                            if (source_var && source_var->is_struct) {
                                
                                // typedef名を実際のstruct名に解決
                                std::string resolved_struct_type = interpreter_.resolve_typedef(param->type_name);
                                std::string source_resolved_type = interpreter_.resolve_typedef(source_var->struct_type_name);
                                
                                // struct型の互換性チェック
                                // "struct Point"と"Point"は同じ型として扱う
                                std::string normalized_resolved = resolved_struct_type;
                                std::string normalized_source = source_resolved_type;
                                
                                // "struct StructName"を"StructName"に正規化
                                if (normalized_resolved.substr(0, 7) == "struct " && normalized_resolved.length() > 7) {
                                    normalized_resolved = normalized_resolved.substr(7);
                                }
                                if (normalized_source.substr(0, 7) == "struct " && normalized_source.length() > 7) {
                                    normalized_source = normalized_source.substr(7);
                                }
                                
                                if (normalized_resolved != normalized_source) {
                                    throw std::runtime_error("Type mismatch: cannot pass struct type '" + source_var->struct_type_name + 
                                                            "' to parameter '" + param->name + "' of type '" + param->type_name + "'");
                                }
                                
                                // ソース構造体の最新状態を同期
                                Variable* sync_source_var = nullptr;
                                if (!source_var_name.empty()) {
                                    interpreter_.sync_struct_members_from_direct_access(source_var_name);
                                    sync_source_var = interpreter_.find_variable(source_var_name);
                                } else {
                                    debug_print("WARNING: Empty source_var_name, skipping sync\n");
                                }
                                
                                if (!sync_source_var) {
                                    throw std::runtime_error("Source struct variable not found: " + source_var_name);
                                }
                                
                                // 文字列配列メンバの場合、追加で確実にarray_stringsを同期
                                for (auto& source_member_pair : sync_source_var->struct_members) {
                                    if (source_member_pair.second.is_array && source_member_pair.second.type == TYPE_STRING) {
                                        // 個別要素変数から文字列配列を再構築
                                        std::string base_name = source_var_name.empty() ? "unknown" : source_var_name;
                                        std::string source_member_name = base_name + "." + source_member_pair.first;
                                        for (int i = 0; i < source_member_pair.second.array_size; i++) {
                                            std::string element_name = source_member_name + "[" + std::to_string(i) + "]";
                                            Variable* element_var = interpreter_.find_variable(element_name);
                                            if (element_var && element_var->type == TYPE_STRING) {
                                                if (source_member_pair.second.array_strings.size() <= static_cast<size_t>(i)) {
                                                    source_member_pair.second.array_strings.resize(i + 1);
                                                }
                                                source_member_pair.second.array_strings[i] = element_var->str_value;
                                            }
                                        }
                                    }
                                }
                                
                                // struct変数をコピーしてパラメータに設定
                                Variable param_var = *sync_source_var;
                                param_var.is_const = param->is_const;  // パラメータのconst修飾を保持
                                param_var.is_struct = true; // 明示的にstructフラグを設定
                                param_var.type = TYPE_STRUCT; // 型情報も設定
                                // 解決されたstruct型名を設定
                                param_var.struct_type_name = resolved_struct_type;
                                
                                // struct_membersの配列要素も確実にコピー
                                for (auto& member_pair : param_var.struct_members) {
                                    if (member_pair.second.is_array && member_pair.second.type == TYPE_STRING) {
                                        // 文字列配列の場合、array_stringsを確実にコピー
                                        const auto& source_member = sync_source_var->struct_members.find(member_pair.first);
                                        if (source_member != sync_source_var->struct_members.end()) {
                                            debug_print("DEBUG: Copying string array %s: size=%d\n", 
                                                       member_pair.first.c_str(), 
                                                       static_cast<int>(source_member->second.array_strings.size()));
                                            member_pair.second.array_strings = source_member->second.array_strings;
                                            if (!source_member->second.array_strings.empty()) {
                                                debug_print("DEBUG: First element: '%s'\n", 
                                                           source_member->second.array_strings[0].c_str());
                                            }
                                        }
                                    }
                                }
                                
                                interpreter_.current_scope().variables[param->name] = param_var;
                                
                                // 個別メンバー変数も作成（値を正しく設定）
                                // 元の構造体定義から type_name 情報を取得
                                const StructDefinition* struct_def = interpreter_.find_struct_definition(resolved_struct_type);
                                for (const auto& member_pair : sync_source_var->struct_members) {
                                    // 配列要素のキー (例: "dimensions[0]") をスキップ
                                    if (member_pair.first.find('[') != std::string::npos) {
                                        continue;
                                    }
                                    
                                    std::string full_member_name = param->name + "." + member_pair.first;
                                    Variable member_var = member_pair.second;
                                    // 値を確実に設定
                                    member_var.is_assigned = true;
                                    
                                    // 元の構造体定義から type_name を取得して設定
                                    if (struct_def) {
                                        for (const auto& member : struct_def->members) {
                                            if (member.name == member_pair.first) {
                                                member_var.type_name = member.type_alias;
                                                member_var.is_pointer = member.is_pointer;
                                                member_var.pointer_depth = member.pointer_depth;
                                                member_var.pointer_base_type_name = member.pointer_base_type_name;
                                                member_var.pointer_base_type = member.pointer_base_type;
                                                member_var.is_reference = member.is_reference;
                                                member_var.is_unsigned = member.is_unsigned;
                                                break;
                                            }
                                        }
                                    }
                                    
                                    interpreter_.current_scope().variables[full_member_name] = member_var;
                                    
                                    // 配列メンバの場合、個別要素変数も作成
                                    if (member_var.is_array) {
                                        // ソース側の配列要素変数をコピー
                                        std::string source_member_name = source_var_name + "." + member_pair.first;
                                        for (int i = 0; i < member_var.array_size; i++) {
                                            std::string source_element_name = source_member_name + "[" + std::to_string(i) + "]";
                                            std::string param_element_name = full_member_name + "[" + std::to_string(i) + "]";
                                            
                                            Variable* source_element = interpreter_.find_variable(source_element_name);
                                            if (source_element) {
                                                Variable element_var = *source_element;
                                                element_var.is_assigned = true;
                                                interpreter_.current_scope().variables[param_element_name] = element_var;
                                            } else {
                                                // 個別要素変数が存在しない場合、struct_membersの配列から作成
                                                Variable element_var;
                                                if (member_var.type == TYPE_STRING && i < static_cast<int>(sync_source_var->struct_members[member_pair.first].array_strings.size())) {
                                                    element_var.type = TYPE_STRING;
                                                    element_var.str_value = sync_source_var->struct_members[member_pair.first].array_strings[i];
                                                } else if (member_var.type != TYPE_STRING && i < static_cast<int>(sync_source_var->struct_members[member_pair.first].array_values.size())) {
                                                    element_var.type = member_var.type;
                                                    element_var.value = sync_source_var->struct_members[member_pair.first].array_values[i];
                                                } else {
                                                    // デフォルト値を設定
                                                    element_var.type = member_var.type;
                                                    if (member_var.type == TYPE_STRING) {
                                                        element_var.str_value = "";
                                                    } else {
                                                        element_var.value = 0;
                                                    }
                                                }
                                                element_var.is_assigned = true;
                                                interpreter_.current_scope().variables[param_element_name] = element_var;
                                            }
                                        }
                                    }
                                }
                            } else {
                                throw std::runtime_error("Type mismatch: cannot pass non-struct expression to struct parameter '" + param->name + "'");
                            }
                        } else {
                            // 数値パラメータの場合
                            if (arg->node_type == ASTNodeType::AST_STRING_LITERAL) {
                                throw std::runtime_error("Type mismatch: cannot pass string literal to numeric parameter '" + param->name + "'");
                            }
                            
                            TypedValue arg_value = evaluate_typed_expression(arg.get());
                            interpreter_.assign_function_parameter(
                                param->name, arg_value, param->type_info,
                                param->is_unsigned);
                            
                            // const修飾を設定
                            if (param->is_const) {
                                Variable* param_var = interpreter_.find_variable(param->name);
                                if (param_var) {
                                    param_var->is_const = true;
                                }
                            }
                        }
                    }
                }
            }
            
            // implメソッド呼び出しの場合、implコンテキストを設定
            if (is_method_call && !receiver_name.empty()) {
                Variable* receiver_var = nullptr;
                if (used_resolution_ptr && dereferenced_struct_ptr) {
                    receiver_var = dereferenced_struct_ptr;
                } else {
                    receiver_var = interpreter_.find_variable(receiver_name);
                }
                
                // Interface型のレシーバーの場合、implコンテキストを設定
                if (receiver_var && receiver_var->type == TYPE_INTERFACE) {
                    std::string interface_name = receiver_var->interface_name;
                    std::string struct_type = receiver_var->struct_type_name;
                    
                    if (!interface_name.empty() && !struct_type.empty()) {
                        interpreter_.enter_impl_context(interface_name, struct_type);
                        impl_context_active = true;
                        if (debug_mode) {
                            debug_print("IMPL_CONTEXT: Entered %s::%s for method %s\n",
                                       interface_name.c_str(), struct_type.c_str(), node->name.c_str());
                        }
                    }
                }
            }
            
            // 関数本体を実行
            try {
                if (func->body) {
                    interpreter_.execute_statement(func->body.get());
                }
                
                // implコンテキストをクリア
                if (impl_context_active) {
                    interpreter_.exit_impl_context();
                    impl_context_active = false;
                }
                
                // void関数は0を返す
                
                // メソッド実行後、selfの変更をレシーバーに同期
                if (has_receiver && !receiver_name.empty()) {
                    Variable* receiver_var = nullptr;
                    
                    // If we used pointer dereference, write back to the dereferenced struct
                    if (used_resolution_ptr && dereferenced_struct_ptr) {
                        receiver_var = dereferenced_struct_ptr;
                        if (debug_mode) {
                            debug_print("SELF_WRITEBACK_PTR: Using dereferenced struct at %p\n",
                                       static_cast<void*>(dereferenced_struct_ptr));
                        }
                    } else {
                        receiver_var = interpreter_.find_variable(receiver_name);
                    }
                    
                    if (receiver_var && (receiver_var->type == TYPE_STRUCT || receiver_var->type == TYPE_INTERFACE)) {
                        // すべての self.* 変数を検索して書き戻し
                        auto& current_scope = interpreter_.get_current_scope();
                        for (const auto& var_pair : current_scope.variables) {
                            const std::string& var_name = var_pair.first;
                            
                            // self. で始まる変数を検索
                            if (var_name.find("self.") == 0) {
                                // self.member または self.member.nested の形式
                                std::string member_path = var_name.substr(5); // "self." を除去
                                
                                const Variable& self_member_var = var_pair.second;
                                
                                // If using dereferenced pointer, write directly to struct_members
                                if (used_resolution_ptr && dereferenced_struct_ptr) {
                                    // Extract member name (first component of member_path)
                                    std::string member_name = member_path;
                                    size_t dot_pos = member_path.find('.');
                                    if (dot_pos != std::string::npos) {
                                        member_name = member_path.substr(0, dot_pos);
                                    }
                                    
                                    // Write directly to struct_members
                                    if (receiver_var->struct_members.find(member_name) != receiver_var->struct_members.end()) {
                                        receiver_var->struct_members[member_name].value = self_member_var.value;
                                        receiver_var->struct_members[member_name].str_value = self_member_var.str_value;
                                        receiver_var->struct_members[member_name].is_assigned = self_member_var.is_assigned;
                                        receiver_var->struct_members[member_name].float_value = self_member_var.float_value;
                                        receiver_var->struct_members[member_name].double_value = self_member_var.double_value;
                                        receiver_var->struct_members[member_name].quad_value = self_member_var.quad_value;
                                        
                                        // Also sync to individual variable if it exists
                                        interpreter_.sync_individual_member_from_struct(receiver_var, member_name);
                                        
                                        if (debug_mode) {
                                            debug_print("SELF_WRITEBACK_PTR: %s -> struct_members[%s] (value=%lld)\n",
                                                       var_name.c_str(), member_name.c_str(),
                                                       self_member_var.value);
                                        }
                                    }
                                } else {
                                    // Normal writeback to named variables
                                    std::string receiver_path = receiver_name + "." + member_path;
                                    
                                    // receiver側の対応する変数に値を書き戻し
                                    Variable* receiver_member_var = interpreter_.find_variable(receiver_path);
                                    if (receiver_member_var) {
                                        receiver_member_var->value = self_member_var.value;
                                        receiver_member_var->str_value = self_member_var.str_value;
                                        receiver_member_var->is_assigned = self_member_var.is_assigned;
                                        receiver_member_var->float_value = self_member_var.float_value;
                                        receiver_member_var->double_value = self_member_var.double_value;
                                        receiver_member_var->quad_value = self_member_var.quad_value;
                                        
                                        debug_print("SELF_WRITEBACK: %s -> %s (value=%lld)\n",
                                                   var_name.c_str(), receiver_path.c_str(),
                                                   self_member_var.value);
                                    }
                                }
                            }
                        }
                    }
                }
                
                cleanup_method_context();
                interpreter_.pop_scope();
                method_scope_active = false;
                interpreter_.current_function_name = prev_function_name;
                return 0;
            } catch (const ReturnException &ret) {
                // implコンテキストをクリア
                if (impl_context_active) {
                    interpreter_.exit_impl_context();
                    impl_context_active = false;
                }
                
                // return文で戻り値がある場合
                
                // メソッド実行後、selfの変更をレシーバーに同期
                if (has_receiver && !receiver_name.empty()) {
                    Variable* receiver_var = nullptr;
                    
                    // If we used pointer dereference, write back to the dereferenced struct
                    if (used_resolution_ptr && dereferenced_struct_ptr) {
                        receiver_var = dereferenced_struct_ptr;
                        if (debug_mode) {
                            debug_print("SELF_WRITEBACK_PTR: Using dereferenced struct at %p\n",
                                       static_cast<void*>(dereferenced_struct_ptr));
                        }
                    } else {
                        receiver_var = interpreter_.find_variable(receiver_name);
                    }
                    
                    if (receiver_var && (receiver_var->type == TYPE_STRUCT || receiver_var->type == TYPE_INTERFACE)) {
                        // すべての self.* 変数を検索して書き戻し
                        auto& current_scope = interpreter_.get_current_scope();
                        for (const auto& var_pair : current_scope.variables) {
                            const std::string& var_name = var_pair.first;
                            
                            // self. で始まる変数を検索
                            if (var_name.find("self.") == 0) {
                                // self.member または self.member.nested の形式
                                std::string member_path = var_name.substr(5); // "self." を除去
                                
                                const Variable& self_member_var = var_pair.second;
                                
                                // If using dereferenced pointer, write directly to struct_members
                                if (used_resolution_ptr && dereferenced_struct_ptr) {
                                    // Extract member name (first component of member_path)
                                    std::string member_name = member_path;
                                    size_t dot_pos = member_path.find('.');
                                    if (dot_pos != std::string::npos) {
                                        member_name = member_path.substr(0, dot_pos);
                                    }
                                    
                                    // Write directly to struct_members
                                    if (receiver_var->struct_members.find(member_name) != receiver_var->struct_members.end()) {
                                        receiver_var->struct_members[member_name].value = self_member_var.value;
                                        receiver_var->struct_members[member_name].str_value = self_member_var.str_value;
                                        receiver_var->struct_members[member_name].is_assigned = self_member_var.is_assigned;
                                        receiver_var->struct_members[member_name].float_value = self_member_var.float_value;
                                        receiver_var->struct_members[member_name].double_value = self_member_var.double_value;
                                        receiver_var->struct_members[member_name].quad_value = self_member_var.quad_value;
                                        
                                        // Also sync to individual variable if it exists
                                        interpreter_.sync_individual_member_from_struct(receiver_var, member_name);
                                        
                                        if (debug_mode) {
                                            debug_print("SELF_WRITEBACK_PTR: %s -> struct_members[%s] (value=%lld)\n",
                                                       var_name.c_str(), member_name.c_str(),
                                                       self_member_var.value);
                                        }
                                    }
                                } else {
                                    // Normal writeback to named variables
                                    std::string receiver_path = receiver_name + "." + member_path;
                                    
                                    // receiver側の対応する変数に値を書き戻し
                                    Variable* receiver_member_var = interpreter_.find_variable(receiver_path);
                                    if (receiver_member_var) {
                                        receiver_member_var->value = self_member_var.value;
                                        receiver_member_var->str_value = self_member_var.str_value;
                                        receiver_member_var->is_assigned = self_member_var.is_assigned;
                                        receiver_member_var->float_value = self_member_var.float_value;
                                        receiver_member_var->double_value = self_member_var.double_value;
                                        receiver_member_var->quad_value = self_member_var.quad_value;
                                        
                                        debug_print("SELF_WRITEBACK: %s -> %s (value=%lld)\n",
                                                   var_name.c_str(), receiver_path.c_str(),
                                                   self_member_var.value);
                                    }
                                }
                            }
                        }
                    }
                }
                
                cleanup_method_context();
                interpreter_.pop_scope();
                method_scope_active = false;
                interpreter_.current_function_name = prev_function_name;
                
                // 関数ポインタ戻り値の場合は例外を再度投げる
                if (ret.is_function_pointer) {
                    throw ret;
                }
                
                if (ret.is_struct) {
                    // struct戻り値の場合、構造体を一時的に処理して戻り値として使用
                    debug_msg(DebugMsgId::INTERPRETER_GET_STRUCT_MEMBER, "Processing struct return value");
                    // 構造体戻り値は0を返す（実際の構造体はReturnExceptionで管理）
                    throw ret; // 上位レベルでstruct処理が必要な場合は例外を伝播
                } else if (ret.is_array) {
                    // 配列戻り値の場合は例外を再度投げる
                    throw ret;
                }
                // 文字列戻り値の場合は例外を再度投げる
                if (ret.type == TYPE_STRING) {
                    throw ret;
                }
                // float/double/quad戻り値の場合は例外を再度投げる
                // (evaluate_expressionはint64_tしか返せないため、上位でTypedValueとして処理する必要がある)
                if (ret.type == TYPE_FLOAT || ret.type == TYPE_DOUBLE || ret.type == TYPE_QUAD) {
                    throw ret;
                }
                // 参照戻り値の場合は例外を再度投げる
                if (ret.is_reference) {
                    throw ret;
                }
                // 通常の戻り値の場合
                auto make_typed_from_return = [&](int64_t coerced_numeric) -> TypedValue {
                    if (ret.type == TYPE_FLOAT) {
                        return TypedValue(ret.double_value, InferredType(TYPE_FLOAT, "float"));
                    }
                    if (ret.type == TYPE_DOUBLE) {
                        return TypedValue(ret.double_value, InferredType(TYPE_DOUBLE, "double"));
                    }
                    if (ret.type == TYPE_QUAD) {
                        return TypedValue(ret.quad_value, InferredType(TYPE_QUAD, "quad"));
                    }
                    TypeInfo resolved = ret.type != TYPE_UNKNOWN ? ret.type : TYPE_INT;
                    std::string resolved_name = type_info_to_string(resolved);
                    if (resolved_name.empty()) {
                        resolved = TYPE_INT;
                        resolved_name = type_info_to_string(resolved);
                    }
                    return TypedValue(coerced_numeric, InferredType(resolved, resolved_name));
                };

                int64_t return_value = ret.value;
                if (func && func->is_unsigned && return_value < 0) {
                    const char* call_kind = is_method_call ? "method" : "function";
                    DEBUG_WARN(FUNCTION,
                               "Unsigned %s '%s' returned negative value (%lld); clamping to 0",
                               call_kind, func->name.c_str(),
                               static_cast<long long>(return_value));
                    return_value = 0;
                }
                TypedValue typed_return = make_typed_from_return(return_value);
                capture_numeric_return(typed_return);
                return return_value;
            }
        } catch (const ReturnException &ret) {
            // implコンテキストをクリア
            if (impl_context_active) {
                interpreter_.exit_impl_context();
                impl_context_active = false;
            }
            
            // 再投げされたReturnExceptionを処理
            cleanup_method_context();
            if (method_scope_active) {
                interpreter_.pop_scope();
                method_scope_active = false;
            }
            interpreter_.current_function_name = prev_function_name;
            throw ret;
        } catch (...) {
            // implコンテキストをクリア
            if (impl_context_active) {
                interpreter_.exit_impl_context();
                impl_context_active = false;
            }
            
            cleanup_method_context();
            if (method_scope_active) {
                interpreter_.pop_scope();
                method_scope_active = false;
            }
            interpreter_.current_function_name = prev_function_name;
            throw;
        }
    }

    // ========================================================================
    // 代入式（=, +=, -=, *=, /=, %= 等）
    // Line 3292-3453: 代入演算子の評価
    // 変数、配列要素、構造体メンバーへの代入に対応
    // ========================================================================
    case ASTNodeType::AST_ASSIGN: {
        return AssignmentHelpers::evaluate_assignment(node, interpreter_,
            [this](const ASTNode* n) { return this->evaluate_expression(n); },
            [this](const ASTNode* n) { return this->evaluate_typed_expression(n); });
    }
    
    // ========================================================================
    // 構造体メンバーアクセス（obj.member）
    // Line 3459-3900: ドット演算子によるメンバーアクセス
    // ネストした構造体、配列要素の構造体メンバーに対応
    // TODO: このセクションを evaluate_struct_member_access() に分離すべき
    // ========================================================================
    case ASTNodeType::AST_MEMBER_ACCESS: {
        // メンバアクセス: obj.member または array[index].member または self.member
        std::string var_name;
        std::string member_name = node->name;

        // ネストしたメンバーアクセスの場合（再帰的に処理）
        if (!node->member_chain.empty() && node->member_chain.size() > 1) {
            // ベース変数を取得
            Variable base_var;
            if (node->left->node_type == ASTNodeType::AST_VARIABLE) {
                Variable* var = interpreter_.find_variable(node->left->name);
                if (!var || var->type != TYPE_STRUCT) {
                    throw std::runtime_error("Base variable for nested access is not a struct: " + node->left->name);
                }
                base_var = *var;
            } else if (node->left->node_type == ASTNodeType::AST_IDENTIFIER && node->left->name == "self") {
                // selfの場合
                Variable* var = interpreter_.find_variable("self");
                if (!var || (var->type != TYPE_STRUCT && var->type != TYPE_INTERFACE)) {
                    throw std::runtime_error("self is not a struct or interface");
                }
                base_var = *var;
            } else if (node->left->node_type == ASTNodeType::AST_MEMBER_ACCESS || 
                       node->left->node_type == ASTNodeType::AST_ARRAY_REF) {
                // ネストしたメンバーアクセスまたは配列アクセスの場合: scene.triangle.vertices や array[index] のような
                // 完全なパスを構築
                std::function<std::string(const ASTNode*)> build_path;
                build_path = [&](const ASTNode* n) -> std::string {
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
                        throw std::runtime_error("Unsupported node type in nested member access path building");
                    }
                };
                std::string full_path = build_path(node->left.get());
                Variable* var = interpreter_.find_variable(full_path);
                if (!var || var->type != TYPE_STRUCT) {
                    throw std::runtime_error("Base variable for nested access is not a struct: " + full_path);
                }
                base_var = *var;
            } else {
                throw std::runtime_error("Complex base types for nested access not yet supported");
            }
            
            // 再帰的にメンバーチェーンをたどる
            try {
                Variable current_var = base_var;
                
                for (size_t i = 0; i < node->member_chain.size(); ++i) {
                    const std::string& member_name_in_chain = node->member_chain[i];
                    
                    // 現在の変数から次のメンバーを取得
                    current_var = get_struct_member_from_variable(current_var, member_name_in_chain);
                    
                    // 最後のメンバーでない場合、次のメンバーにアクセスするために構造体である必要がある
                    if (i < node->member_chain.size() - 1) {
                        if (current_var.type != TYPE_STRUCT && current_var.type != TYPE_INTERFACE) {
                            throw std::runtime_error("Intermediate member is not a struct: " + member_name_in_chain);
                        }
                    }
                }
                
                // 最終的な値を返す
                if (current_var.type == TYPE_STRING) {
                    last_typed_result_ = TypedValue(current_var.str_value, InferredType(TYPE_STRING, "string"));
                    return 0;
                } else if (current_var.type == TYPE_POINTER) {
                    return current_var.value;
                } else if (current_var.type == TYPE_FLOAT || current_var.type == TYPE_DOUBLE || current_var.type == TYPE_QUAD) {
                    // float/double/quadの場合はTypedValueに設定して返す
                    InferredType float_type(current_var.type, "");
                    if (current_var.type == TYPE_QUAD) {
                        last_typed_result_ = TypedValue(current_var.quad_value, float_type);
                    } else {
                        last_typed_result_ = TypedValue(current_var.float_value, float_type);
                    }
                    return static_cast<int64_t>(current_var.float_value);
                } else {
                    return current_var.value;
                }
            } catch (const std::exception& e) {
                throw std::runtime_error("Nested member access failed: " + std::string(e.what()));
            }
        }
        
        // leftがAST_MEMBER_ACCESSの場合、まずleftを評価して構造体を取得
        if (node->left->node_type == ASTNodeType::AST_MEMBER_ACCESS) {
            debug_msg(DebugMsgId::NESTED_MEMBER_EVAL_START, "left is AST_MEMBER_ACCESS");
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
                const ASTNode* current = node->left.get();
                
                // 再帰的にパスを構築
                std::function<std::string(const ASTNode*)> build_path;
                build_path = [&](const ASTNode* n) -> std::string {
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
                        throw std::runtime_error("Unsupported node type in nested member access");
                    }
                };
                
                struct_path = build_path(current);
                
                // 完全パスを構築（最終メンバーまで含む）
                std::string full_member_path = struct_path + "." + member_name;
                
                debug_msg(DebugMsgId::NESTED_MEMBER_FULL_PATH, full_member_path.c_str());
                
                // 個別変数を直接検索
                Variable* member_var_ptr = interpreter_.find_variable(full_member_path);
                if (member_var_ptr) {
                    debug_msg(DebugMsgId::NESTED_MEMBER_INDIVIDUAL_VAR_FOUND,
                             full_member_path.c_str(), member_var_ptr->value);
                    // 個別変数が見つかった場合、それを使用
                    if (member_var_ptr->type == TYPE_STRING) {
                        last_typed_result_ = TypedValue(member_var_ptr->str_value, InferredType(TYPE_STRING, "string"));
                        return 0;
                    } else if (member_var_ptr->type == TYPE_STRUCT) {
                        last_typed_result_ = TypedValue(member_var_ptr->value, InferredType(TYPE_STRUCT, member_var_ptr->type_name));
                        return member_var_ptr->value;
                    } else if (member_var_ptr->type == TYPE_FLOAT || member_var_ptr->type == TYPE_DOUBLE || member_var_ptr->type == TYPE_QUAD) {
                        // float/double/quadの場合
                        InferredType float_type(member_var_ptr->type, "");
                        if (member_var_ptr->type == TYPE_QUAD) {
                            last_typed_result_ = TypedValue(member_var_ptr->quad_value, float_type);
                        } else {
                            last_typed_result_ = TypedValue(member_var_ptr->float_value, float_type);
                        }
                        return static_cast<int64_t>(member_var_ptr->float_value);
                    } else {
                        last_typed_result_ = TypedValue(member_var_ptr->value, InferredType(member_var_ptr->type, ""));
                        return member_var_ptr->value;
                    }
                }
                
                // 個別変数が見つからない場合は従来の方法（struct_membersから取得）
                Variable* intermediate_var = interpreter_.find_variable(struct_path);
                if (!intermediate_var) {
                    throw std::runtime_error("Intermediate struct not found: " + struct_path);
                }
                
                if (intermediate_var->type != TYPE_STRUCT) {
                    throw std::runtime_error("Intermediate value is not a struct: " + struct_path);
                }
                
                intermediate_struct = *intermediate_var;
                Variable member_var = get_struct_member_from_variable(intermediate_struct, member_name);
                
                // 型情報を設定
                if (member_var.type == TYPE_STRING) {
                    last_typed_result_ = TypedValue(member_var.str_value, InferredType(TYPE_STRING, "string"));
                    return 0;
                } else if (member_var.type == TYPE_STRUCT) {
                    last_typed_result_ = TypedValue(member_var.value, InferredType(TYPE_STRUCT, member_var.type_name));
                    return member_var.value;
                } else if (member_var.type == TYPE_FLOAT || member_var.type == TYPE_DOUBLE || member_var.type == TYPE_QUAD) {
                    // float/double/quadの場合
                    InferredType float_type(member_var.type, "");
                    if (member_var.type == TYPE_QUAD) {
                        last_typed_result_ = TypedValue(member_var.quad_value, float_type);
                    } else {
                        last_typed_result_ = TypedValue(member_var.float_value, float_type);
                    }
                    return static_cast<int64_t>(member_var.float_value);
                } else {
                    last_typed_result_ = TypedValue(member_var.value, InferredType(member_var.type, ""));
                    return member_var.value;
                }
            } else {
                throw std::runtime_error("Left side of nested member access did not evaluate to a struct");
            }
        } else if (node->left->node_type == ASTNodeType::AST_VARIABLE) {
            // 通常のstruct変数: obj.member
            var_name = node->left->name;
        } else if (node->left->node_type == ASTNodeType::AST_IDENTIFIER && node->left->name == "self") {
            // selfメンバアクセス: self.member
            var_name = "self";
            debug_msg(DebugMsgId::SELF_MEMBER_ACCESS_START, member_name.c_str());
            
            // selfメンバーアクセスの特別処理
            std::string self_member_path = "self." + member_name;
            Variable* self_member = interpreter_.find_variable(self_member_path);
            if (self_member) {
                debug_msg(DebugMsgId::SELF_MEMBER_ACCESS_FOUND, self_member_path.c_str());
                if (self_member->type == TYPE_STRING) {
                    return 0; // 文字列の場合は別途処理
                }
                debug_msg(DebugMsgId::SELF_MEMBER_ACCESS_VALUE, self_member->value);
                return self_member->value;
            }
        } else if (node->left->node_type == ASTNodeType::AST_ARRAY_REF) {
            // struct配列要素: array[index].member または obj.array[index].member
            std::string array_name;
            
            // 配列のベース名を取得（メンバーアクセスや配列アクセスの場合を考慮）
            if (node->left->left->node_type == ASTNodeType::AST_MEMBER_ACCESS || 
                node->left->left->node_type == ASTNodeType::AST_ARRAY_REF) {
                // obj.array[index].member や obj.array[i][j].member の場合
                // 完全なパスを構築: obj.array
                std::function<std::string(const ASTNode*)> build_path;
                build_path = [&](const ASTNode* n) -> std::string {
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
                        throw std::runtime_error("Unsupported node type in array member access");
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
            debug_msg(DebugMsgId::EXPR_EVAL_START, "Function call member access");
            
            try {
                // 関数を実行してReturnExceptionを捕捉
                evaluate_typed_expression(node->left.get());
                // 通常の戻り値の場合はエラー
                throw std::runtime_error("Function did not return a struct for member access");
            } catch (const ReturnException& ret_ex) {
                // 構造体戻り値からメンバーを取得
                if (ret_ex.is_struct_array && ret_ex.struct_array_3d.size() > 0) {
                    // 構造体配列の場合（将来拡張）
                    throw std::runtime_error("Struct array function return member access not yet fully supported");
                } else {
                    // 単一構造体の場合
                    Variable struct_var = ret_ex.struct_value;
                    Variable member_var = get_struct_member_from_variable(struct_var, member_name);
                    
                    if (member_var.type == TYPE_STRING) {
                        // 文字列の場合は別途処理が必要（呼び出し元で処理される）
                        TypedValue typed_result(static_cast<int64_t>(0), InferredType(TYPE_STRING, "string"));
                        typed_result.string_value = member_var.str_value;
                        typed_result.is_numeric_result = false;
                        last_typed_result_ = typed_result;
                        return 0;
                    } else if (member_var.type == TYPE_FLOAT || member_var.type == TYPE_DOUBLE || member_var.type == TYPE_QUAD) {
                        InferredType float_type(member_var.type, "");
                        if (member_var.type == TYPE_QUAD) {
                            last_typed_result_ = TypedValue(member_var.quad_value, float_type);
                        } else {
                            last_typed_result_ = TypedValue(member_var.float_value, float_type);
                        }
                        return static_cast<int64_t>(member_var.float_value);
                    } else {
                        return member_var.value;
                    }
                }
            }
        } else if (node->left->node_type == ASTNodeType::AST_ARRAY_REF && 
                   node->left->left && node->left->left->node_type == ASTNodeType::AST_FUNC_CALL) {
            // 関数配列戻り値でのメンバアクセス: func()[index].member
            debug_msg(DebugMsgId::EXPR_EVAL_START, "Function array member access");
            
            try {
                // 関数を実行してReturnExceptionを捕捉
                evaluate_expression(node->left->left.get());
                throw std::runtime_error("Function did not return an array for indexed member access");
            } catch (const ReturnException& ret_ex) {
                if (ret_ex.is_struct_array && ret_ex.struct_array_3d.size() > 0) {
                    // インデックスを評価
                    int64_t index = evaluate_expression(node->left->array_index.get());
                    
                    // 配列境界チェック
                    if (index < 0 || index >= (int64_t)ret_ex.struct_array_3d.size()) {
                        throw std::runtime_error("Array index out of bounds in function struct array member access");
                    }
                    
                    // 指定インデックスの構造体からメンバーを取得
                    if (ret_ex.struct_array_3d.size() > 0 && 
                        ret_ex.struct_array_3d[0].size() > 0 &&
                        ret_ex.struct_array_3d[0][0].size() > index) {
                        Variable struct_var = ret_ex.struct_array_3d[0][0][index];
                        Variable member_var = get_struct_member_from_variable(struct_var, member_name);
                        
                        if (member_var.type == TYPE_STRING) {
                            TypedValue typed_result(static_cast<int64_t>(0), InferredType(TYPE_STRING, "string"));
                            typed_result.string_value = member_var.str_value;
                            typed_result.is_numeric_result = false;
                            last_typed_result_ = typed_result;
                            return 0;
                        } else if (member_var.type == TYPE_FLOAT || member_var.type == TYPE_DOUBLE || member_var.type == TYPE_QUAD) {
                            InferredType float_type(member_var.type, "");
                            if (member_var.type == TYPE_QUAD) {
                                last_typed_result_ = TypedValue(member_var.quad_value, float_type);
                            } else {
                                last_typed_result_ = TypedValue(member_var.float_value, float_type);
                            }
                            return static_cast<int64_t>(member_var.float_value);
                        } else {
                            return member_var.value;
                        }
                    } else {
                        throw std::runtime_error("Invalid struct array structure");
                    }
                } else {
                    throw std::runtime_error("Function did not return a struct array for indexed member access");
                }
            }
        } else if (node->left->node_type == ASTNodeType::AST_UNARY_OP && node->left->op == "DEREFERENCE") {
            // デリファレンスされたポインタからのメンバーアクセス: (*pp).member
            debug_msg(DebugMsgId::EXPR_EVAL_START, "Pointer dereference member access");
            
            // デリファレンスを評価して構造体のポインタ値を取得
            int64_t ptr_value = evaluate_expression(node->left.get());
            
            // ポインタ値から構造体変数を取得
            Variable* struct_var = reinterpret_cast<Variable*>(ptr_value);
            if (!struct_var) {
                throw std::runtime_error("Null pointer dereference in member access");
            }
            
            // 構造体メンバーを取得
            Variable member_var = get_struct_member_from_variable(*struct_var, member_name);
            
            if (member_var.type == TYPE_STRING) {
                TypedValue typed_result(static_cast<int64_t>(0), InferredType(TYPE_STRING, "string"));
                typed_result.string_value = member_var.str_value;
                typed_result.is_numeric_result = false;
                last_typed_result_ = typed_result;
                return 0;
            } else if (member_var.type == TYPE_FLOAT || member_var.type == TYPE_DOUBLE || member_var.type == TYPE_QUAD) {
                InferredType float_type(member_var.type, "");
                if (member_var.type == TYPE_QUAD) {
                    last_typed_result_ = TypedValue(member_var.quad_value, float_type);
                } else {
                    last_typed_result_ = TypedValue(member_var.float_value, float_type);
                }
                return static_cast<int64_t>(member_var.float_value);
            } else {
                return member_var.value;
            }
        } else {
            throw std::runtime_error("Invalid member access");
        }
        
        // 参照型変数の場合、参照先を取得
        Variable* base_var = interpreter_.find_variable(var_name);
        std::string actual_var_name = var_name;
        
        if (base_var && base_var->is_reference) {
            // 参照の場合、参照先の変数から直接メンバを取得
            debug_print("[DEBUG] Member access on reference variable: %s\n", var_name.c_str());
        }
        
        // 個別変数として直接アクセスを試す（構造体配列の場合）
        std::string full_member_path = actual_var_name + "." + member_name;

        interpreter_.sync_struct_members_from_direct_access(actual_var_name);
        interpreter_.ensure_struct_member_access_allowed(actual_var_name, member_name);
        Variable* member_var = interpreter_.find_variable(full_member_path);
        
        if (!member_var) {
            // struct_membersから探す（通常の構造体の場合）
            // 参照の場合、直接参照先から取得
            if (base_var && base_var->is_reference) {
                Variable result_member = get_struct_member_from_variable(*base_var, member_name);
                // 一時変数として返す必要があるため、last_typed_result_を使用
                if (result_member.type == TYPE_STRING) {
                    TypedValue typed_result(static_cast<int64_t>(0), InferredType(TYPE_STRING, "string"));
                    typed_result.string_value = result_member.str_value;
                    typed_result.is_numeric_result = false;
                    last_typed_result_ = typed_result;
                    return 0;
                } else if (result_member.type == TYPE_FLOAT || result_member.type == TYPE_DOUBLE || result_member.type == TYPE_QUAD) {
                    InferredType float_type(result_member.type, "");
                    if (result_member.type == TYPE_QUAD) {
                        last_typed_result_ = TypedValue(result_member.quad_value, float_type);
                    } else {
                        last_typed_result_ = TypedValue(result_member.float_value, float_type);
                    }
                    return static_cast<int64_t>(result_member.float_value);
                } else if (result_member.type == TYPE_STRUCT) {
                    // 構造体メンバの場合、ReturnExceptionでラップして返す
                    throw ReturnException(result_member);
                } else {
                    return result_member.value;
                }
            }
            
            member_var = interpreter_.get_struct_member(actual_var_name, member_name);
        }
        
        if (!member_var) {
            throw std::runtime_error("Member not found: " + actual_var_name + "." + member_name);
        }
        
        if (member_var->type == TYPE_STRING) {
            // 文字列メンバは別途処理が必要（呼び出し元で処理される）
            return 0; // 文字列の場合は0を返すが、実際の文字列は別途取得される
        } else if (member_var->type == TYPE_FLOAT || member_var->type == TYPE_DOUBLE || member_var->type == TYPE_QUAD) {
            // float/double/quadの場合は型情報を保持
            InferredType float_type(member_var->type, "");
            if (member_var->type == TYPE_QUAD) {
                last_typed_result_ = TypedValue(member_var->quad_value, float_type);
            } else {
                last_typed_result_ = TypedValue(member_var->float_value, float_type);
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
        auto evaluate_expression_lambda = [this](const ASTNode* n) {
            return this->evaluate_expression(n);
        };
        auto get_struct_member_lambda = [this](const Variable& v, const std::string& name) {
            return this->get_struct_member_from_variable(v, name);
        };
        return SpecialAccessHelpers::evaluate_arrow_access(node, interpreter_, evaluate_expression_lambda, get_struct_member_lambda);
    }
    
    // ========================================================================
    // メンバー配列アクセス（obj.member[index]）
    // ========================================================================
    case ASTNodeType::AST_MEMBER_ARRAY_ACCESS: {
        // メンバー配列アクセスはSpecialAccessHelpersに移動（100行）
        auto evaluate_expression_lambda = [this](const ASTNode* n) {
            return this->evaluate_expression(n);
        };
        auto get_struct_member_lambda = [this](const Variable& v, const std::string& name) {
            return this->get_struct_member_from_variable(v, name);
        };
        return SpecialAccessHelpers::evaluate_member_array_access(node, interpreter_, evaluate_expression_lambda, get_struct_member_lambda);
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
            std::cerr << "[ERROR] Unsupported expression node type: " << static_cast<int>(node->node_type) << std::endl;
        }
        throw std::runtime_error("Unknown expression node type");
    }

    return 0;
}
// ============================================================================
// evaluate_expression メソッド終了
// ============================================================================

// 型をインターフェース用の文字列に変換するヘルパー関数
std::string ExpressionEvaluator::type_info_to_string(TypeInfo type) {
    const char* name = ::type_info_to_string(type);
    if (name && *name) {
        return std::string(name);
    }
    return "unknown";
}

void ExpressionEvaluator::sync_self_changes_to_receiver(const std::string& receiver_name, Variable* receiver_var) {
    debug_print("SELF_SYNC: Syncing self changes back to %s\n", receiver_name.c_str());
    
    // 構造体の各メンバーについて、selfから元の変数に同期
    for (const auto& member_pair : receiver_var->struct_members) {
        const std::string& member_name = member_pair.first;
        std::string self_member_path = "self." + member_name;
        std::string receiver_member_path = receiver_name + "." + member_name;
        
        // selfメンバーの変数を取得
        Variable* self_member = interpreter_.find_variable(self_member_path);
        Variable* receiver_member = interpreter_.find_variable(receiver_member_path);
        
        if (self_member && receiver_member) {
            // selfメンバーの値を元の変数に同期
            receiver_member->value = self_member->value;
            receiver_member->str_value = self_member->str_value;
            receiver_member->type = self_member->type;
            receiver_member->is_assigned = self_member->is_assigned;
            
            debug_print("SELF_SYNC: %s.%s = %lld (\"%s\")\n", 
                       receiver_name.c_str(), member_name.c_str(), 
                       (long long)receiver_member->value, 
                       receiver_member->str_value.c_str());
        }
    }
}

// 型推論対応の式評価
TypedValue ExpressionEvaluator::evaluate_typed_expression(const ASTNode* node) {
    if (!node) {
    return TypedValue(static_cast<int64_t>(0), InferredType());
    }
    
    debug_msg(DebugMsgId::TYPED_EVAL_ENTRY, static_cast<int>(node->node_type));
    
    // ReturnExceptionをキャッチして構造体を処理
    try {
        return evaluate_typed_expression_internal(node);
    } catch (const ReturnException& ret_ex) {
        if (debug_mode) {
            debug_print("TYPED_EVAL_RETURN: is_struct=%d type=%d is_array=%d is_function_pointer=%d\n",
                        ret_ex.is_struct ? 1 : 0,
                        static_cast<int>(ret_ex.type),
                        ret_ex.is_array ? 1 : 0,
                        ret_ex.is_function_pointer ? 1 : 0);
        }
        if (ret_ex.is_function_pointer) {
            // 関数ポインタの場合、ReturnExceptionを再スロー
            if (debug_mode) {
                std::cerr << "[TYPED_EVAL] Re-throwing function pointer ReturnException" << std::endl;
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
            return TypedValue(ret_ex.str_value, InferredType(TYPE_STRING, "string"));
        }

        if (ret_ex.type == TYPE_FLOAT) {
            return TypedValue(ret_ex.double_value, InferredType(TYPE_FLOAT, "float"));
        }
        if (ret_ex.type == TYPE_DOUBLE) {
            return TypedValue(ret_ex.double_value, InferredType(TYPE_DOUBLE, "double"));
        }
        if (ret_ex.type == TYPE_QUAD) {
            return TypedValue(ret_ex.quad_value, InferredType(TYPE_QUAD, "quad"));
        }

        // 通常の数値の場合
        return TypedValue(ret_ex.value, InferredType(ret_ex.type, type_info_to_string(ret_ex.type)));
    }
}

// 実際の型推論対応の式評価（内部実装）
TypedValue ExpressionEvaluator::evaluate_typed_expression_internal(const ASTNode* node) {
    if (!node) {
    return TypedValue(static_cast<int64_t>(0), InferredType());
    }
    
    debug_msg(DebugMsgId::TYPED_EVAL_INTERNAL_ENTRY, static_cast<int>(node->node_type));
    
    // まず型を推論
    InferredType inferred_type = type_engine_.infer_type(node);

    switch (node->node_type) {
        case ASTNodeType::AST_TERNARY_OP:
            return evaluate_ternary_typed(node);
            
        case ASTNodeType::AST_STRING_LITERAL: {
            // 文字列リテラルの評価はLiteralEvalHelpersに移動（6行）
            return LiteralEvalHelpers::evaluate_string_literal_typed(node, inferred_type);
        }
            
        case ASTNodeType::AST_NUMBER: {
            // 数値リテラルの評価はLiteralEvalHelpersに移動（18行）
            return LiteralEvalHelpers::evaluate_number_literal_typed(node, inferred_type);
        }

        case ASTNodeType::AST_NULLPTR: {
            // nullptrの評価はLiteralEvalHelpersに移動（4行）
            return LiteralEvalHelpers::evaluate_nullptr_literal_typed();
        }
            
        case ASTNodeType::AST_BINARY_OP: {
            // 二項演算子の評価（typed版）はBinaryUnaryTypedHelpersに移動（318行）
            auto evaluate_typed_lambda = [this](const ASTNode* n) {
                return this->evaluate_typed_expression(n);
            };
            return BinaryUnaryTypedHelpers::evaluate_binary_op_typed(node, interpreter_, inferred_type, evaluate_typed_lambda);
        }

        case ASTNodeType::AST_UNARY_OP: {
            // 単項演算子の評価（typed版）はBinaryUnaryTypedHelpersに移動（177行）
            auto evaluate_typed_lambda = [this](const ASTNode* n) {
                return this->evaluate_typed_expression(n);
            };
            auto evaluate_expression_lambda = [this](const ASTNode* n) {
                return this->evaluate_expression(n);
            };
            return BinaryUnaryTypedHelpers::evaluate_unary_op_typed(node, interpreter_, inferred_type, 
                                                                     evaluate_typed_lambda, evaluate_expression_lambda);
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
                InferredType function_return_type = type_engine_.infer_function_return_type(node->name, {});
                
                // 関数を実行して結果を取得
                int64_t numeric_result = evaluate_expression(node);
                
                // 推論された型に基づいて適切なTypedValueを返す
                if (function_return_type.type_info == TYPE_STRING) {
                    // 文字列戻り値の場合（実際の文字列は evaluate_expression では取得困難）
                    return TypedValue("", InferredType(TYPE_STRING, "string"));
                } else if (function_return_type.type_info == TYPE_STRUCT) {
                    // 構造体戻り値の場合は例外をキャッチして処理
                    throw std::runtime_error("Struct return should be caught as exception");
                } else {
                    // 数値戻り値の場合
                    return consume_numeric_typed_value(node, numeric_result, function_return_type);
                }
            } catch (const ReturnException& ret) {
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
                    InferredType struct_type(TYPE_STRUCT, struct_var.struct_type_name);
                    return TypedValue(struct_var, struct_type);
                } else if (ret.type == TYPE_STRING) {
                    return TypedValue(ret.str_value, InferredType(TYPE_STRING, "string"));
                } else if (ret.type == TYPE_FLOAT) {
                    return TypedValue(ret.double_value, InferredType(TYPE_FLOAT, "float"));
                } else if (ret.type == TYPE_DOUBLE) {
                    return TypedValue(ret.double_value, InferredType(TYPE_DOUBLE, "double"));
                } else if (ret.type == TYPE_QUAD) {
                    return TypedValue(ret.quad_value, InferredType(TYPE_QUAD, "quad"));
                } else {
                    return TypedValue(ret.value, InferredType(ret.type, type_info_to_string(ret.type)));
                }
            }
        }
        
        case ASTNodeType::AST_VARIABLE: {
            // 変数参照の評価はLiteralEvalHelpersに移動（66行）
            return LiteralEvalHelpers::evaluate_variable_typed(node, interpreter_, inferred_type);
        }
        
        case ASTNodeType::AST_MEMBER_ACCESS: {
            debug_msg(DebugMsgId::TYPED_MEMBER_ACCESS_CASE, node->name.c_str(), node->member_chain.size());
            
            // member_chainが2つ以上ある場合（ネストメンバアクセス）
            if (!node->member_chain.empty() && node->member_chain.size() > 1) {
                // ベース変数を取得
                Variable base_var;
                if (node->left->node_type == ASTNodeType::AST_VARIABLE) {
                    Variable* var = interpreter_.find_variable(node->left->name);
                    if (!var || var->type != TYPE_STRUCT) {
                        throw std::runtime_error("Base variable for nested access is not a struct: " + node->left->name);
                    }
                    base_var = *var;
                } else {
                    throw std::runtime_error("Complex base types for nested access not yet supported in typed evaluation");
                }
                
                // 再帰的にメンバーチェーンをたどる
                Variable current_var = base_var;
                for (size_t i = 0; i < node->member_chain.size(); ++i) {
                    const std::string& member_name_in_chain = node->member_chain[i];
                    
                    // 現在の変数から次のメンバーを取得
                    current_var = get_struct_member_from_variable(current_var, member_name_in_chain);
                    
                    // 最後のメンバーでない場合、次のメンバーにアクセスするために構造体である必要がある
                    if (i < node->member_chain.size() - 1) {
                        if (current_var.type != TYPE_STRUCT && current_var.type != TYPE_INTERFACE) {
                            throw std::runtime_error("Intermediate member is not a struct: " + member_name_in_chain);
                        }
                    }
                }
                
                // 最終的な値をTypedValueとして返す
                if (current_var.type == TYPE_STRING) {
                    return TypedValue(current_var.str_value, InferredType(TYPE_STRING, "string"));
                } else if (current_var.type == TYPE_STRUCT) {
                    return TypedValue(current_var, InferredType(TYPE_STRUCT, current_var.struct_type_name));
                } else if (current_var.type == TYPE_FLOAT) {
                    return TypedValue(static_cast<double>(current_var.float_value), InferredType(TYPE_FLOAT, "float"));
                } else if (current_var.type == TYPE_DOUBLE) {
                    return TypedValue(current_var.double_value, InferredType(TYPE_DOUBLE, "double"));
                } else if (current_var.type == TYPE_QUAD) {
                    return TypedValue(current_var.quad_value, InferredType(TYPE_QUAD, "quad"));
                } else {
                    return TypedValue(current_var.value, InferredType(current_var.type, type_info_to_string(current_var.type)));
                }
            }
            
            auto convert_member_to_typed = [&](const Variable& member_var,
                                               TypedValue& out) -> bool {
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
                    out = TypedValue(member_var,
                                      InferredType(TYPE_STRUCT,
                                                   member_var.struct_type_name));
                    return true;
                case TYPE_UNION: {
                    TypeInfo active = member_var.current_type;
                    if (active == TYPE_STRING) {
                        out = TypedValue(member_var.str_value,
                                         InferredType(TYPE_STRING, "string"));
                        return true;
                    }
                    if (active == TYPE_FLOAT) {
                        out = TypedValue(static_cast<double>(member_var.float_value),
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
                        out = TypedValue(member_var.value,
                                         InferredType(active,
                                                      type_info_to_string(active)));
                        return true;
                    }
                    break;
                }
                default:
                    out = TypedValue(member_var.value,
                                      InferredType(member_var.type,
                                                   type_info_to_string(member_var.type)));
                    return true;
                }
                return false;
            };

            auto resolve_from_struct = [&](const Variable& struct_var,
                                           TypedValue& out) -> bool {
                try {
                    Variable member_var =
                        get_struct_member_from_variable(struct_var, node->name);
                    return convert_member_to_typed(member_var, out);
                } catch (const std::exception&) {
                    return false;
                }
            };

            std::function<std::string(const ASTNode*)> build_base_name =
                [&](const ASTNode* base) -> std::string {
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

            auto resolve_from_base_name = [&](const std::string& base_name,
                                              TypedValue& out) -> bool {
                if (base_name.empty()) {
                    return false;
                }

                try {
                    interpreter_.sync_struct_members_from_direct_access(base_name);
                    interpreter_.ensure_struct_member_access_allowed(base_name,
                                                                     node->name);
                } catch (const std::exception&) {
                    // best effort even if sync fails
                }

                std::string member_path = base_name + "." + node->name;
                if (Variable* direct_member =
                        interpreter_.find_variable(member_path)) {
                    if (convert_member_to_typed(*direct_member, out)) {
                        return true;
                    }
                }

                try {
                    if (Variable* member_var =
                            interpreter_.get_struct_member(base_name,
                                                            node->name)) {
                        if (convert_member_to_typed(*member_var, out)) {
                            return true;
                        }
                    }
                } catch (const std::exception&) {
                }

                return false;
            };

            // (*ptr).member パターンをチェック（構造体ポインタのデリファレンス）
            if (node->left && node->left->node_type == ASTNodeType::AST_UNARY_OP && 
                node->left->op == "DEREFERENCE") {
                
                // デリファレンスの結果を取得
                TypedValue deref_value = evaluate_typed_expression(node->left.get());
                
                // 構造体の場合
                if (deref_value.is_struct() && deref_value.struct_data) {
                    Variable struct_var = *deref_value.struct_data;
                    TypedValue member_value(static_cast<int64_t>(0), InferredType());
                    
                    if (resolve_from_struct(struct_var, member_value)) {
                        last_typed_result_ = member_value;
                        return member_value;
                    }
                }
                
                throw std::runtime_error("Pointer dereference did not yield a struct");
            }

            // func()[index].member パターンをチェック
            if (node->left && node->left->node_type == ASTNodeType::AST_ARRAY_REF &&
                node->left->left &&
                node->left->left->node_type == ASTNodeType::AST_FUNC_CALL) {

                debug_print("Processing func()[index].member pattern: %s[].%s\n",
                            node->left->left->name.c_str(),
                            node->name.c_str());

                try {
                    (void)evaluate_typed_expression(node->left.get());
                    throw std::runtime_error(
                        "Expected struct return exception");

                } catch (const ReturnException& struct_ret) {
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
            debug_msg(DebugMsgId::NESTED_MEMBER_BASE_PATH, base_name.c_str(), node->name.c_str());
            
            if (!base_name.empty()) {
                // まず個別変数を検索（優先）
                debug_msg(DebugMsgId::NESTED_MEMBER_RESOLVE_FROM_BASE);
                resolved = resolve_from_base_name(base_name, resolved_value);
                if (resolved) {
                    debug_msg(DebugMsgId::NESTED_MEMBER_RESOLVE_SUCCESS,
                             resolved_value.is_numeric() ? resolved_value.as_numeric() : 0LL);
                } else {
                    debug_msg(DebugMsgId::NESTED_MEMBER_RESOLVE_FAILED);
                    
                    // 個別変数が見つからない場合、struct_membersから検索
                    if (Variable* base_var = interpreter_.find_variable(base_name)) {
                        debug_msg(DebugMsgId::NESTED_MEMBER_BASE_VAR_FOUND, base_var->type);
                        if (base_var->type == TYPE_STRUCT) {
                            resolved = resolve_from_struct(*base_var, resolved_value);
                            if (resolved) {
                                debug_msg(DebugMsgId::NESTED_MEMBER_RESOLVE_SUCCESS,
                                         resolved_value.is_numeric() ? resolved_value.as_numeric() : 0LL);
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
                } catch (const ReturnException& ret) {
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
                debug_print("Processing typed function call array access: %s\n", node->left->name.c_str());
                
                // インデックスを評価
                int64_t index = evaluate_expression(node->array_index.get());
                
                try {
                    // 関数を実行して戻り値を取得（副作用のため実行）
                    (void)evaluate_expression(node->left.get());
                    throw std::runtime_error("Function did not return an array via exception");
                } catch (const ReturnException& ret) {
                    if (ret.is_array) {
                        // 構造体配列の戻り値の場合
                        if (ret.is_struct_array && !ret.struct_array_3d.empty() && 
                            !ret.struct_array_3d[0].empty() && !ret.struct_array_3d[0][0].empty()) {
                            
                            if (index >= 0 && index < static_cast<int64_t>(ret.struct_array_3d[0][0].size())) {
                                // 構造体要素をReturnExceptionとして投げる
                                throw ReturnException(ret.struct_array_3d[0][0][index]);
                            } else {
                                throw std::runtime_error("Array index out of bounds");
                            }
                        }
                        // 文字列配列の戻り値の場合
                        else if (!ret.str_array_3d.empty() && 
                            !ret.str_array_3d[0].empty() && !ret.str_array_3d[0][0].empty()) {
                            
                            if (index >= 0 && index < static_cast<int64_t>(ret.str_array_3d[0][0].size())) {
                                return TypedValue(ret.str_array_3d[0][0][index], TYPE_STRING);
                            } else {
                                throw std::runtime_error("Array index out of bounds");
                            }
                        }
                        // 数値配列の戻り値の場合
                        else if (!ret.int_array_3d.empty() && 
                                 !ret.int_array_3d[0].empty() && !ret.int_array_3d[0][0].empty()) {
                            
                            if (index >= 0 && index < static_cast<int64_t>(ret.int_array_3d[0][0].size())) {
                                return TypedValue(ret.int_array_3d[0][0][index], TYPE_INT);
                            } else {
                                throw std::runtime_error("Array index out of bounds");
                            }
                        } else {
                            throw std::runtime_error("Empty array returned from function");
                        }
                    } else {
                        throw std::runtime_error("Function does not return an array");
                    }
                }
            }
            
            if (inferred_type.type_info == TYPE_STRING &&
                node->left && node->left->node_type == ASTNodeType::AST_MEMBER_ACCESS) {
                const ASTNode* member_node = node->left.get();
                std::string member_name = member_node->name;
                std::string object_name;

                if (member_node->left) {
                    if (member_node->left->node_type == ASTNodeType::AST_VARIABLE) {
                        object_name = member_node->left->name;
                    } else if (member_node->left->node_type == ASTNodeType::AST_ARRAY_REF) {
                        object_name = interpreter_.extract_array_element_name(member_node->left.get());
                    }
                }

                if (!object_name.empty() && node->array_index) {
                    int64_t array_index = evaluate_expression(node->array_index.get());
                    try {
                        std::string value = interpreter_.get_struct_member_array_string_element(
                            object_name, member_name, static_cast<int>(array_index));
                        return TypedValue(value, InferredType(TYPE_STRING, "string"));
                    } catch (const std::exception &) {
                        // フォールバックして通常処理
                    }
                }
            }

            if (inferred_type.type_info == TYPE_STRING) {
                std::string array_name = interpreter_.extract_array_name(node);
                std::vector<int64_t> indices = interpreter_.extract_array_indices(node);

                if (!array_name.empty() && !indices.empty()) {
                    bool resolved = false;
                    std::string string_value;

                    if (auto *array_service = interpreter_.get_array_processing_service()) {
                        try {
                            string_value = array_service->getStringArrayElement(
                                array_name, indices,
                                ArrayProcessingService::ArrayContext::LOCAL_VARIABLE);
                            resolved = true;
                        } catch (const std::exception &) {
                            resolved = false;
                        }
                    }

                    if (!resolved) {
                        if (Variable *var = interpreter_.find_variable(array_name)) {
                            try {
                                if (var->is_multidimensional ||
                                    !var->multidim_array_strings.empty()) {
                                    string_value = interpreter_.getMultidimensionalStringArrayElement(*var, indices);
                                    resolved = true;
                                } else if (!var->array_strings.empty() && indices.size() == 1) {
                                    int64_t idx = indices[0];
                                    if (idx >= 0 &&
                                        idx < static_cast<int64_t>(var->array_strings.size())) {
                                        string_value = var->array_strings[static_cast<size_t>(idx)];
                                        resolved = true;
                                    }
                                }
                            } catch (const std::exception &) {
                                resolved = false;
                            }
                        }
                    }

                    if (resolved) {
                        return TypedValue(string_value, InferredType(TYPE_STRING, "string"));
                    }
                }
            }

            // 通常の配列要素アクセスの場合 - float/double配列対応
            std::string array_name = interpreter_.extract_array_name(node);
            std::vector<int64_t> indices = interpreter_.extract_array_indices(node);
            
            if (!array_name.empty() && !indices.empty()) {
                Variable* var = interpreter_.find_variable(array_name);
                if (var && var->is_array) {
                    TypeInfo base_type = (var->type >= TYPE_ARRAY_BASE) 
                                        ? static_cast<TypeInfo>(var->type - TYPE_ARRAY_BASE)
                                        : var->type;
                    
                    // float/double/quad配列の場合
                    if (base_type == TYPE_FLOAT || base_type == TYPE_DOUBLE || base_type == TYPE_QUAD) {
                        if (var->is_multidimensional && indices.size() > 1) {
                            // 多次元配列のフラットインデックスを計算（row-major order）
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
                                flat_index < static_cast<int>(var->multidim_array_float_values.size())) {
                                float f = var->multidim_array_float_values[flat_index];
                                return TypedValue(static_cast<double>(f), InferredType(TYPE_FLOAT, "float"));
                            } else if (base_type == TYPE_DOUBLE && flat_index >= 0 && 
                                      flat_index < static_cast<int>(var->multidim_array_double_values.size())) {
                                double d = var->multidim_array_double_values[flat_index];
                                return TypedValue(d, InferredType(TYPE_DOUBLE, "double"));
                            } else if (base_type == TYPE_QUAD && flat_index >= 0 && 
                                      flat_index < static_cast<int>(var->multidim_array_quad_values.size())) {
                                long double q = var->multidim_array_quad_values[flat_index];
                                return TypedValue(q, InferredType(TYPE_QUAD, "quad"));
                            }
                        } else if (indices.size() == 1) {
                            // 1次元配列
                            int64_t idx = indices[0];
                            if (base_type == TYPE_FLOAT && idx >= 0 && 
                                idx < static_cast<int64_t>(var->array_float_values.size())) {
                                float f = var->array_float_values[idx];
                                return TypedValue(static_cast<double>(f), InferredType(TYPE_FLOAT, "float"));
                            } else if (base_type == TYPE_DOUBLE && idx >= 0 && 
                                      idx < static_cast<int64_t>(var->array_double_values.size())) {
                                double d = var->array_double_values[idx];
                                return TypedValue(d, InferredType(TYPE_DOUBLE, "double"));
                            } else if (base_type == TYPE_QUAD && idx >= 0 && 
                                      idx < static_cast<int64_t>(var->array_quad_values.size())) {
                                long double q = var->array_quad_values[idx];
                                return TypedValue(q, InferredType(TYPE_QUAD, "quad"));
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
            Variable* var = interpreter_.find_variable(node->name);
            if (var) {
                // 関数ポインタの場合、関数ポインタ情報を含むTypedValueを返す
                if (var->is_function_pointer) {
                    auto& fp_map = interpreter_.current_scope().function_pointers;
                    auto it = fp_map.find(node->name);
                    if (it != fp_map.end()) {
                        return TypedValue::function_pointer(
                            var->value,
                            it->second.function_name,
                            it->second.function_node,
                            inferred_type
                        );
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
TypedValue ExpressionEvaluator::evaluate_ternary_typed(const ASTNode* node) {
    // 三項演算子の評価はTernaryHelpersに移動（130行）
    auto evaluate_expression_lambda = [this](const ASTNode* n) {
        return this->evaluate_expression(n);
    };
    auto evaluate_typed_expression_lambda = [this](const ASTNode* n) {
        return this->evaluate_typed_expression(n);
    };
    return TernaryHelpers::evaluate_ternary_typed(
        node, interpreter_, 
        evaluate_expression_lambda, 
        evaluate_typed_expression_lambda,
        type_engine_,
        last_typed_result_
    );
}

// 遅延評価されたTypedValueを実際に評価する
TypedValue ExpressionEvaluator::resolve_deferred_evaluation(const TypedValue& deferred_value) {
    if (!deferred_value.needs_deferred_evaluation() || !deferred_value.deferred_node) {
        return deferred_value; // 遅延評価が不要または無効
    }
    
    const ASTNode* node = deferred_value.deferred_node;
    
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

TypedValue ExpressionEvaluator::consume_numeric_typed_value(const ASTNode* node, int64_t numeric_result, const InferredType& inferred_type) {
    if (last_captured_function_value_.has_value()) {
        if (last_captured_function_value_->first == node) {
            TypedValue captured = std::move(last_captured_function_value_->second);
            last_captured_function_value_ = std::nullopt;
            return captured;
        }
        last_captured_function_value_ = std::nullopt;
    }

    InferredType resolved_type = inferred_type;
    if (resolved_type.type_info == TYPE_UNKNOWN) {
        resolved_type.type_info = TYPE_INT;
    }
    if (resolved_type.type_name.empty()) {
        resolved_type.type_name = type_info_to_string(resolved_type.type_info);
    }

    switch (resolved_type.type_info) {
    case TYPE_FLOAT:
    case TYPE_DOUBLE:
        return TypedValue(static_cast<double>(numeric_result), resolved_type);
    case TYPE_QUAD:
        return TypedValue(static_cast<long double>(numeric_result), resolved_type);
    default:
        return TypedValue(numeric_result, resolved_type);
    }
}

// 構造体メンバー取得関数の実装
Variable ExpressionEvaluator::get_struct_member_from_variable(const Variable& struct_var, const std::string& member_name) {
    // 参照型の場合、参照先の変数を取得
    const Variable* actual_var = &struct_var;
    if (struct_var.is_reference) {
        // 参照先はvalueフィールドにポインタとして格納されている
        actual_var = reinterpret_cast<Variable*>(struct_var.value);
        if (!actual_var) {
            throw std::runtime_error("Invalid reference in member access");
        }
        debug_print("[DEBUG] get_struct_member_from_variable: resolving reference to target (type=%d)\n", 
                   actual_var->type);
    }
    
    if (actual_var->type != TYPE_STRUCT) {
        throw std::runtime_error("Variable is not a struct");
    }
    
    debug_print("[DEBUG] get_struct_member_from_variable: looking for '%s' in struct (type='%s', members=%zu)\n",
               member_name.c_str(), actual_var->struct_type_name.c_str(), actual_var->struct_members.size());
    for (const auto& pair : actual_var->struct_members) {
        debug_print("[DEBUG]   - member: '%s' (type=%d)\n", pair.first.c_str(), pair.second.type);
    }
    
    auto enforce_privacy = [&](const Variable& member_var) -> Variable {
        if (!member_var.is_private_member) {
            return member_var;
        }

        std::string struct_type = actual_var->struct_type_name;
        if (struct_type.empty() && !actual_var->implementing_struct.empty()) {
            struct_type = actual_var->implementing_struct;
        }

        if (!interpreter_.is_current_impl_context_for(struct_type)) {
            std::string type_label = struct_type.empty() ? std::string("<anonymous>") : struct_type;
            throw std::runtime_error("Cannot access private struct member: " + type_label + "." + member_name);
        }

        return member_var;
    };

    // まず struct_members から直接検索
    auto member_it = actual_var->struct_members.find(member_name);
    if (member_it != actual_var->struct_members.end()) {
        // ネストされた構造体メンバーの場合、そのstruct_membersを確認
        if (member_it->second.type == TYPE_STRUCT) {
            debug_print("[DEBUG] Found struct member '%s' (type=%d, struct_type='%s', struct_members.size()=%zu)\n",
                       member_name.c_str(), member_it->second.type,
                       member_it->second.struct_type_name.c_str(),
                       member_it->second.struct_members.size());
        }
        return enforce_privacy(member_it->second);
    }
    
    // 構造体の識別子（struct_type_name）を使用してメンバーを検索
    std::string member_var_name = actual_var->struct_type_name + "." + member_name;
    Variable* member_var = interpreter_.find_variable(member_var_name);
    
    if (member_var) {
        return enforce_privacy(*member_var);
    }
    
    // インタープリターの get_struct_member を使用
    try {
        std::string temp_struct_name = "temp_struct_" + struct_var.struct_type_name;
        member_var = interpreter_.get_struct_member(temp_struct_name, member_name);
        if (member_var) {
            return enforce_privacy(*member_var);
        }
    } catch (...) {
        // 失敗した場合は続行
    }
    
    throw std::runtime_error("Struct member not found: " + member_name);
}

// 関数戻り値からのメンバーアクセス処理
TypedValue ExpressionEvaluator::evaluate_function_member_access(const ASTNode* func_node, const std::string& member_name) {
    debug_msg(DebugMsgId::EXPR_EVAL_START, "evaluate_function_member_access");
    
    try {
        // 関数を実行してReturnExceptionを捕捉
        evaluate_expression(func_node);
        throw std::runtime_error("Function did not return a struct for member access");
    } catch (const ReturnException& ret_ex) {
        debug_print("FUNC_MEMBER_ACCESS: ReturnException caught - type=%d, is_struct=%d\n", 
                   ret_ex.type, ret_ex.is_struct);
        debug_print("FUNC_MEMBER_ACCESS: struct_value type=%d, is_struct=%d, members=%zu\n",
                   ret_ex.struct_value.type, ret_ex.struct_value.is_struct, 
                   ret_ex.struct_value.struct_members.size());
        
        if (ret_ex.is_struct_array && ret_ex.struct_array_3d.size() > 0) {
            throw std::runtime_error("Struct array function return member access requires index");
        } else {
            // 単一構造体の場合
            Variable struct_var = ret_ex.struct_value;
            debug_print("FUNC_MEMBER_ACCESS: Looking for member %s in struct\n", member_name.c_str());
            Variable member_var = get_struct_member_from_variable(struct_var, member_name);
            
            if (member_var.type == TYPE_STRING) {
                TypedValue result(member_var.str_value, InferredType(TYPE_STRING, "string"));
                last_typed_result_ = result;
                return result;
            } else {
                TypedValue result(member_var.value, InferredType(TYPE_INT, "int"));
                last_typed_result_ = result;
                return result;
            }
        }
    }
}

// 関数戻り値からの配列アクセス処理
TypedValue ExpressionEvaluator::evaluate_function_array_access(const ASTNode* func_node, const ASTNode* index_node) {
    debug_msg(DebugMsgId::EXPR_EVAL_START, "evaluate_function_array_access");
    
    // インデックスを評価
    int64_t index = evaluate_expression(index_node);
    
    try {
        // 関数を実行して戻り値を取得
        evaluate_expression(func_node);
        throw std::runtime_error("Function did not return an array via exception");
    } catch (const ReturnException& ret) {
        if (!ret.is_array) {
            throw std::runtime_error("Function does not return an array");
        }
        
        if (ret.is_struct_array && !ret.struct_array_3d.empty() && 
            !ret.struct_array_3d[0].empty() && !ret.struct_array_3d[0][0].empty()) {
            // 構造体配列の場合
            if (index >= 0 && index < static_cast<int64_t>(ret.struct_array_3d[0][0].size())) {
                Variable struct_element = ret.struct_array_3d[0][0][index];
                // 構造体として返す（後でメンバーアクセス可能）
                TypedValue result(static_cast<int64_t>(0), InferredType(TYPE_STRUCT, struct_element.struct_type_name));
                result.is_struct_result = true;
                result.struct_data = std::make_shared<Variable>(struct_element);  // 構造体データを保持
                last_typed_result_ = result;
                return result;
            } else {
                throw std::runtime_error("Array index out of bounds");
            }
        } else if (!ret.int_array_3d.empty() && 
                   !ret.int_array_3d[0].empty() && !ret.int_array_3d[0][0].empty()) {
            // 数値配列の場合
            if (index >= 0 && index < static_cast<int64_t>(ret.int_array_3d[0][0].size())) {
                return TypedValue(ret.int_array_3d[0][0][index], InferredType(TYPE_INT, "int"));
            } else {
                throw std::runtime_error("Array index out of bounds");
            }
        } else {
            throw std::runtime_error("Unsupported array type in function return");
        }
    }
}

// 関数戻り値からの複合アクセス処理（func()[index].member）
TypedValue ExpressionEvaluator::evaluate_function_compound_access(const ASTNode* func_node, const ASTNode* index_node, const std::string& member_name) {
    debug_msg(DebugMsgId::EXPR_EVAL_START, "evaluate_function_compound_access");
    
    // まず配列アクセスを実行
    TypedValue array_result = evaluate_function_array_access(func_node, index_node);
    
    if (!array_result.is_struct_result || !array_result.struct_data) {
        throw std::runtime_error("Array element is not a struct for member access");
    }
    
    // 構造体データからメンバーを取得
    Variable member_var = get_struct_member_from_variable(*array_result.struct_data, member_name);
    
    if (member_var.type == TYPE_STRING) {
        TypedValue result(member_var.str_value, InferredType(TYPE_STRING, "string"));
        last_typed_result_ = result;
        return result;
    } else {
        TypedValue result(member_var.value, InferredType(TYPE_INT, "int"));
        last_typed_result_ = result;
        return result;
    }
}

// 再帰的メンバーアクセス処理（将来のネスト構造体対応）
TypedValue ExpressionEvaluator::evaluate_recursive_member_access(const Variable& base_var, const std::vector<std::string>& member_path) {
    debug_msg(DebugMsgId::EXPR_EVAL_START, "evaluate_recursive_member_access");
    
    if (member_path.empty()) {
        throw std::runtime_error("Empty member path for recursive access");
    }
    
    std::cerr << "DEBUG_RECURSIVE: Starting recursive access with " << member_path.size() << " levels" << std::endl;
    for (size_t i = 0; i < member_path.size(); ++i) {
        std::cerr << "DEBUG_RECURSIVE: Path[" << i << "] = " << member_path[i] << std::endl;
    }
    
    Variable current_var = base_var;
    
    // 各レベルでのメンバーアクセスを再帰的に処理
    for (size_t i = 0; i < member_path.size(); ++i) {
        const std::string& member_name = member_path[i];
        std::cerr << "DEBUG_RECURSIVE: Accessing member[" << i << "] = " << member_name << std::endl;
        std::cerr << "DEBUG_RECURSIVE: Current var type = " << static_cast<int>(current_var.type) << std::endl;
        
        // 現在の変数が構造体でない場合はエラー
        if (current_var.type != TYPE_STRUCT) {
            throw std::runtime_error("Cannot access member '" + member_name + "' on non-struct type");
        }
        
        // メンバーを取得
        try {
            current_var = get_struct_member_from_variable(current_var, member_name);
            std::cerr << "DEBUG_RECURSIVE: Successfully accessed member, new type = " << static_cast<int>(current_var.type) << std::endl;
        } catch (const std::exception& e) {
            std::cerr << "DEBUG_RECURSIVE: Failed to access member '" << member_name << "': " << e.what() << std::endl;
            throw;
        }
        
        // 最後のレベルでない場合、構造体である必要がある（将来の拡張用）
        if (i < member_path.size() - 1 && current_var.type != TYPE_STRUCT) {
            throw std::runtime_error("Intermediate member '" + member_name + "' is not a struct for further nesting");
        }
    }
    
    // 最終結果を TypedValue に変換
    std::cerr << "DEBUG_RECURSIVE: Final result type = " << static_cast<int>(current_var.type) << std::endl;
    if (current_var.type == TYPE_STRING) {
    TypedValue result(static_cast<int64_t>(0), InferredType(TYPE_STRING, "string"));
        result.string_value = current_var.str_value;
        result.is_numeric_result = false;
        return result;
    } else if (current_var.type == TYPE_STRUCT) {
        // 構造体の場合、完全なデータを保持
        TypedValue result(current_var, InferredType(TYPE_STRUCT, current_var.struct_type_name));
        std::cerr << "DEBUG_RECURSIVE: Returning struct TypedValue" << std::endl;
        return result;
    } else {
        return TypedValue(current_var.value, InferredType(TYPE_INT, "int"));
    }
}

ExpressionEvaluator::MethodReceiverResolution ExpressionEvaluator::resolve_array_receiver(const ASTNode* array_node) {
    MethodReceiverResolution result;
    if (!array_node || array_node->node_type != ASTNodeType::AST_ARRAY_REF) {
        return result;
    }

    // シンプルな変数配列の場合は直接参照を試みる
    if (array_node->left && array_node->left->node_type == ASTNodeType::AST_VARIABLE && array_node->array_index) {
        std::string base_name = array_node->left->name;
        try {
            int64_t index_value = evaluate_expression(array_node->array_index.get());
            std::string element_name = base_name + "[" + std::to_string(index_value) + "]";
            Variable* element_var = interpreter_.find_variable(element_name);
            if (element_var) {
                result.kind = MethodReceiverResolution::Kind::Direct;
                result.canonical_name = element_name;
                result.variable_ptr = element_var;
                return result;
            }
        } catch (const ReturnException&) {
            // インデックス評価で構造体等が返った場合はチェーン扱い
        }
    }

    return create_chain_receiver_from_expression(array_node);
}

ExpressionEvaluator::MethodReceiverResolution ExpressionEvaluator::resolve_member_receiver(const ASTNode* member_node) {
    MethodReceiverResolution result;
    if (!member_node || member_node->node_type != ASTNodeType::AST_MEMBER_ACCESS) {
        return result;
    }

    const ASTNode* base_node = member_node->left.get();
    if (!base_node) {
        return result;
    }

    const std::string member_name = member_node->name;

    std::function<std::string(const ASTNode*)> build_canonical_name = [&](const ASTNode* node) -> std::string {
        if (!node) {
            return "";
        }
        switch (node->node_type) {
        case ASTNodeType::AST_VARIABLE:
        case ASTNodeType::AST_IDENTIFIER:
            return node->name;
        case ASTNodeType::AST_MEMBER_ACCESS: {
            std::string base = build_canonical_name(node->left.get());
            if (base.empty()) {
                return "";
            }
            return base + "." + node->name;
        }
        default:
            return "";
        }
    };

    MethodReceiverResolution base_resolution = resolve_method_receiver(base_node);

    auto create_chain_from_struct = [&](const Variable& struct_var) {
        try {
            Variable member_var = get_struct_member_from_variable(struct_var, member_name);
            auto chain_ret = std::make_shared<ReturnException>(member_var);
            result.kind = MethodReceiverResolution::Kind::Chain;
            result.chain_value = chain_ret;
            return true;
        } catch (const std::exception&) {
            return false;
        }
    };

    if (base_resolution.kind == MethodReceiverResolution::Kind::Direct && base_resolution.variable_ptr) {
        Variable* base_var = base_resolution.variable_ptr;
        std::string base_name = base_resolution.canonical_name;
        if (base_name.empty()) {
            base_name = build_canonical_name(base_node);
        }

        if (!base_name.empty()) {
            std::string member_path = base_name + "." + member_name;
            Variable* member_var = interpreter_.find_variable(member_path);
            if (!member_var) {
                try {
                    member_var = interpreter_.get_struct_member(base_name, member_name);
                } catch (...) {
                    member_var = nullptr;
                }
            }

            if (member_var) {
                result.kind = MethodReceiverResolution::Kind::Direct;
                result.canonical_name = member_path;
                result.variable_ptr = member_var;
                return result;
            }
        }

        if ((base_var->type == TYPE_STRUCT || base_var->is_struct || base_var->type == TYPE_INTERFACE) &&
            create_chain_from_struct(*base_var)) {
            return result;
        }
    }

    if (base_resolution.kind == MethodReceiverResolution::Kind::Chain && base_resolution.chain_value) {
        const ReturnException& chain_ret = *base_resolution.chain_value;
        if (chain_ret.is_struct || chain_ret.type == TYPE_STRUCT) {
            if (create_chain_from_struct(chain_ret.struct_value)) {
                return result;
            }
        }
    }

    // 直接解決できない場合は式全体をチェーンとして扱う
    return create_chain_receiver_from_expression(member_node);
}

ExpressionEvaluator::MethodReceiverResolution ExpressionEvaluator::resolve_arrow_receiver(const ASTNode* arrow_node) {
    MethodReceiverResolution result;
    if (!arrow_node || arrow_node->node_type != ASTNodeType::AST_ARROW_ACCESS) {
        return result;
    }

    const ASTNode* base_node = arrow_node->left.get();
    if (!base_node) {
        return result;
    }

    const std::string member_name = arrow_node->name;

    // ポインタを評価
    try {
        int64_t ptr_value = evaluate_expression(base_node);
        
        if (ptr_value == 0) {
            // nullポインタの場合はエラー
            return result;
        }
        
        // ポインタから構造体を取得
        Variable* struct_var = reinterpret_cast<Variable*>(ptr_value);
        
        if (!struct_var) {
            return result;
        }
        
        // resolve_arrow_receiverは常にメソッド呼び出しコンテキストから呼ばれる(resolve_method_receiverから)
        // Interface型のポインタの場合、Interface Variable全体を返す必要がある
        // 注意: member_nameはメソッド名の場合もあるが、ここではレシーバを返すだけで、メンバーは取得しない
        if (struct_var->type == TYPE_INTERFACE || !struct_var->interface_name.empty()) {
            // Interface型全体をチェーン値として返す(member_nameは無視)
            auto chain_ret = std::make_shared<ReturnException>(*struct_var);
            result.kind = MethodReceiverResolution::Kind::Chain;
            result.chain_value = chain_ret;
            return result;
        }
        
        // 通常の構造体のメンバーアクセスの場合は、構造体のメンバーを取得
        Variable member_var = get_struct_member_from_variable(*struct_var, member_name);
        
        // チェーン値として返す
        auto chain_ret = std::make_shared<ReturnException>(member_var);
        result.kind = MethodReceiverResolution::Kind::Chain;
        result.chain_value = chain_ret;
        
        return result;
    } catch (const std::exception&) {
        // エラーの場合は空の結果を返す
        return result;
    }
}

ExpressionEvaluator::MethodReceiverResolution ExpressionEvaluator::create_chain_receiver_from_expression(const ASTNode* node) {
    MethodReceiverResolution result;
    if (!node) {
        return result;
    }

    try {
        int64_t primitive_value = evaluate_expression(node);
        InferredType inferred_type = type_engine_.infer_type(node);
        TypeInfo chain_type = inferred_type.type_info;
        if (chain_type == TYPE_UNKNOWN) {
            chain_type = TYPE_INT;
        }
        ReturnException chain_ret(primitive_value, chain_type);
        result.kind = MethodReceiverResolution::Kind::Chain;
        result.chain_value = std::make_shared<ReturnException>(chain_ret);
        return result;
    } catch (const ReturnException& ret) {
        result.kind = MethodReceiverResolution::Kind::Chain;
        result.chain_value = std::make_shared<ReturnException>(ret);
        return result;
    }
}

// ============================================================================
// NOTE: Tier 2リファクタリングで抽出されたヘルパーメソッドは
// expression_helpers.cpp に移動しました
// ============================================================================
