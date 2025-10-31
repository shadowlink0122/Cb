// ============================================================================
// expression_dispatcher.cpp
// ============================================================================
// Phase 13: Expression Evaluation Dispatcher
//
// evaluate_expression()メソッドをExpressionDispatcherクラスに分離。
// ExpressionEvaluatorから委譲を受けて、全ASTノードタイプの評価をディスパッチ。
//
// 【目的】:
// - expression_evaluator.cppを700行程度に削減
// - 各ケースは既存のヘルパーモジュールに委譲
// - メンテナンス性の向上
// ============================================================================

#include "dispatcher.h"
#include "../../../../common/ast.h"
#include "../../../../common/debug.h"
#include "../../../../common/debug_messages.h"
#include "../../core/interpreter.h"
#include "../../core/pointer_metadata.h"
#include "../../managers/types/enums.h"
#include "../access/address_ops.h"
#include "../access/array.h"
#include "../access/member_helpers.h"
#include "../access/receiver_resolution.h"
#include "../access/special.h"
#include "../functions/call.h"
#include "../literals/eval.h"
#include "../operators/assignment.h"
#include "../operators/binary_unary.h"
#include "../operators/incdec.h"
#include "../operators/ternary.h"
#include "evaluator.h"
#include "helpers.h"
#include <functional>
#include <stdexcept>

ExpressionDispatcher::ExpressionDispatcher(
    ExpressionEvaluator &expression_evaluator)
    : expression_evaluator_(expression_evaluator),
      interpreter_(expression_evaluator.get_interpreter()) {}

int64_t ExpressionDispatcher::dispatch_expression(const ASTNode *node) {
    if (!node) {
        debug_msg(DebugMsgId::EXPR_EVAL_START,
                  "Null node in expression evaluation");
        if (debug_mode) {
            std::cerr << "[ERROR] Null node in expression evaluation"
                      << std::endl;
            std::cerr << "[ERROR] This usually means a parser error occurred"
                      << std::endl;
        }
        throw std::runtime_error("Null node in expression evaluation");
    }

    std::string node_type_str =
        std::to_string(static_cast<int>(node->node_type));
    debug_msg(DebugMsgId::EXPR_EVAL_START, node_type_str.c_str());

    if (node->node_type == ASTNodeType::AST_ARRAY_REF && node->name.empty()) {
        debug_msg(DebugMsgId::EXPR_EVAL_ARRAY_REF_START);
    }

    switch (node->node_type) {
    case ASTNodeType::AST_NUMBER:
        return ExpressionHelpers::evaluate_number_literal(node);

    case ASTNodeType::AST_NULLPTR:
    case ASTNodeType::AST_STRING_LITERAL:
        return ExpressionHelpers::evaluate_special_literal(node);

    case ASTNodeType::AST_IDENTIFIER:
        return LiteralEvalHelpers::evaluate_identifier(node, interpreter_);

    case ASTNodeType::AST_VARIABLE:
        return LiteralEvalHelpers::evaluate_variable(node, interpreter_);

    case ASTNodeType::AST_ARRAY_REF: {
        auto eval_func = [this](const ASTNode *n) {
            return this->dispatch_expression(n);
        };
        auto get_member_func = [this](const Variable &v,
                                      const std::string &name) {
            return expression_evaluator_.get_struct_member_from_variable(v,
                                                                         name);
        };
        return ArrayAccessHelpers::evaluate_array_ref(
            node, interpreter_, eval_func, get_member_func);
    }

    case ASTNodeType::AST_ARRAY_LITERAL:
        return ArrayAccessHelpers::evaluate_array_literal(node, interpreter_);

    case ASTNodeType::AST_BINARY_OP: {
        int64_t left = dispatch_expression(node->left.get());
        int64_t right = dispatch_expression(node->right.get());

        int64_t result = 0;
        if (node->op == "+" || node->op == "-" || node->op == "*" ||
            node->op == "/" || node->op == "%") {
            result = ExpressionHelpers::evaluate_arithmetic_binary(node->op,
                                                                   left, right);
        } else if (node->op == "<" || node->op == ">" || node->op == "<=" ||
                   node->op == ">=" || node->op == "==" || node->op == "!=") {
            result = ExpressionHelpers::evaluate_comparison_binary(node->op,
                                                                   left, right);
        } else if (node->op == "&&" || node->op == "||") {
            result = ExpressionHelpers::evaluate_logical_binary(node->op, left,
                                                                right);
        } else if (node->op == "&" || node->op == "|" || node->op == "^" ||
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

    case ASTNodeType::AST_TERNARY_OP: {
        TypedValue typed_result =
            expression_evaluator_.evaluate_ternary_typed(node);

        if (typed_result.is_string()) {
            return 0;
        } else {
            return typed_result.as_numeric();
        }
    }

    case ASTNodeType::AST_UNARY_OP: {
        debug_msg(DebugMsgId::UNARY_OP_DEBUG, node->op.c_str());

        if (node->op == "++_post" || node->op == "--_post") {
            return ExpressionHelpers::evaluate_postfix_incdec(node,
                                                              interpreter_);
        }

        if (node->op == "++" || node->op == "--") {
            return ExpressionHelpers::evaluate_prefix_incdec(node,
                                                             interpreter_);
        }

        if (node->op == "ADDRESS_OF") {
            auto eval_func = [this](const ASTNode *n) {
                return this->dispatch_expression(n);
            };
            return AddressOperationHelpers::evaluate_address_of(
                node, interpreter_, eval_func);
        }

        if (node->op == "DEREFERENCE") {
            auto eval_func = [this](const ASTNode *n) {
                return this->dispatch_expression(n);
            };
            return AddressOperationHelpers::evaluate_dereference(
                node, interpreter_, eval_func);
        }

        if (node->op == "!") {
            int64_t operand = dispatch_expression(node->left.get());
            return !operand;
        }

        if (node->op == "-") {
            int64_t operand = dispatch_expression(node->left.get());
            return -operand;
        }

        if (node->op == "~") {
            int64_t operand = dispatch_expression(node->left.get());
            return ~operand;
        }

        error_msg(DebugMsgId::UNKNOWN_UNARY_OP_ERROR, node->op.c_str());
        throw std::runtime_error("Unknown unary operator: " + node->op);
    }

    case ASTNodeType::AST_PRE_INCDEC:
    case ASTNodeType::AST_POST_INCDEC: {
        auto eval_func = [this](const ASTNode *n) {
            return this->dispatch_expression(n);
        };
        return IncDecHelpers::evaluate_incdec(node, interpreter_, eval_func);
    }

    case ASTNodeType::AST_FUNC_PTR_CALL:
        return FunctionCallHelpers::evaluate_function_pointer_call(
            node, interpreter_);

    case ASTNodeType::AST_FUNC_CALL: {
        // すべての関数呼び出し（関数ポインタ含む）を
        // evaluate_function_call_impl に委譲 evaluate_function_call_impl
        // 内で適切に処理される
        return expression_evaluator_.evaluate_function_call_impl(node);
    }

    case ASTNodeType::AST_ASSIGN: {
        auto eval_func = [this](const ASTNode *n) {
            return this->dispatch_expression(n);
        };
        auto eval_typed_func = [this](const ASTNode *n) {
            return expression_evaluator_.evaluate_typed_expression(n);
        };
        return AssignmentHelpers::evaluate_assignment(
            node, interpreter_, eval_func, eval_typed_func);
    }

    case ASTNodeType::AST_MEMBER_ACCESS:
        return expression_evaluator_.evaluate_member_access_impl(node);

    case ASTNodeType::AST_ARROW_ACCESS: {
        auto eval_func = [this](const ASTNode *n) {
            return this->dispatch_expression(n);
        };
        auto get_member_func = [this](const Variable &v,
                                      const std::string &name) {
            return expression_evaluator_.get_struct_member_from_variable(v,
                                                                         name);
        };
        return SpecialAccessHelpers::evaluate_arrow_access(
            node, interpreter_, expression_evaluator_, eval_func,
            get_member_func);
    }

    case ASTNodeType::AST_MEMBER_ARRAY_ACCESS: {
        auto eval_func = [this](const ASTNode *n) {
            return this->dispatch_expression(n);
        };
        auto get_member_func = [this](const Variable &v,
                                      const std::string &name) {
            return expression_evaluator_.get_struct_member_from_variable(v,
                                                                         name);
        };
        return SpecialAccessHelpers::evaluate_member_array_access(
            node, interpreter_, eval_func, get_member_func);
    }

    case ASTNodeType::AST_STRUCT_LITERAL:
        return SpecialAccessHelpers::evaluate_struct_literal(node);

    case ASTNodeType::AST_ENUM_ACCESS:
        return SpecialAccessHelpers::evaluate_enum_access(node, interpreter_);

    // v0.11.0: enum値の構築 (EnumName::member(value))
    case ASTNodeType::AST_ENUM_CONSTRUCT:
        return SpecialAccessHelpers::evaluate_enum_construct(node,
                                                             interpreter_);

    // v0.11.0 Week 2: 型キャスト (type)expr
    case ASTNodeType::AST_CAST_EXPR: {
        // キャスト対象の式を評価
        int64_t value =
            expression_evaluator_.evaluate_expression(node->cast_expr.get());

        // 構造体ポインタへのキャストの場合、メタデータを更新
        // (Point*)ptr のようなキャストを検出
        if (node->cast_type_info == TYPE_POINTER &&
            node->cast_target_type.find('*') != std::string::npos) {
            // 型名から構造体名を抽出（"Point*" -> "Point"）
            std::string struct_type_name = node->cast_target_type;
            size_t star_pos = struct_type_name.find('*');
            if (star_pos != std::string::npos) {
                struct_type_name = struct_type_name.substr(0, star_pos);
            }

            // 構造体定義が存在するか確認
            const StructDefinition *struct_def =
                interpreter_.find_struct_definition(struct_type_name);

            if (struct_def) {
                // 構造体ポインタへのキャスト
                // メタデータポインタかどうかをチェック（最上位ビットが1）
                bool has_metadata = (value & (1LL << 63)) != 0;

                if (has_metadata) {
                    // メタデータポインタの場合、最上位ビットをクリアして実際のポインタを取得
                    int64_t meta_ptr = value & ~(1LL << 63);
                    PointerSystem::PointerMetadata *metadata =
                        reinterpret_cast<PointerSystem::PointerMetadata *>(
                            meta_ptr);

                    if (metadata) {
                        metadata->struct_type_name = struct_type_name;
                        metadata->pointed_type = TYPE_STRUCT;

                        if (interpreter_.is_debug_mode()) {
                            std::cerr
                                << "[CAST_DEBUG] Updated pointer "
                                   "metadata: "
                                << "ptr=" << reinterpret_cast<void *>(value)
                                << " struct_type=" << struct_type_name
                                << std::endl;
                        }
                    }
                }
            }
        }

        // キャストノードには元の値をそのまま返す
        // （型情報は既にcast_type_infoに格納されている）
        return value;
    }

    // v0.11.0 Phase 1a: メモリ管理演算子
    case ASTNodeType::AST_NEW_EXPR:
        return interpreter_.evaluate_new_expression(node);

    case ASTNodeType::AST_DELETE_EXPR:
        return interpreter_.evaluate_delete_expression(node);

    case ASTNodeType::AST_SIZEOF_EXPR:
        return interpreter_.evaluate_sizeof_expression(node);

    // v0.10.0: 無名変数（Discard Variable）
    case ASTNodeType::AST_DISCARD_VARIABLE:
        throw std::runtime_error("Cannot reference discard variable '_'");

    // v0.10.0: 無名関数（Lambda Expression）
    case ASTNodeType::AST_LAMBDA_EXPR: {
        // ラムダ式はevaluate_typed_expressionで処理されるべき
        // ここではReturnExceptionがスローされるため到達しないはず
        // しかし、何らかの理由でここに到達した場合のフォールバック
        std::string lambda_name = node->internal_name;
        interpreter_.register_function_to_global(lambda_name, node);

        // ReturnExceptionを投げて適切に処理させる
        ReturnException ret(static_cast<int64_t>(0));
        ret.is_function_pointer = true;
        ret.function_pointer_name = lambda_name;
        ret.function_pointer_node = node;
        ret.type = node->lambda_return_type;
        throw ret;
    }

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
