// ============================================================================
// expression_receiver_resolution.cpp
// ============================================================================
// Phase 12 Refactoring: Method Receiver Resolution Implementation
//
// メソッド呼び出しのレシーバ解決に特化したヘルパー関数群の実装。
// ============================================================================

#include "expression_receiver_resolution.h"
#include "evaluator/expression_evaluator.h"
#include "evaluator/expression_member_helpers.h"
#include "core/interpreter.h"
#include "../../../common/ast.h"
#include "../../../common/debug.h"
#include <functional>

namespace ReceiverResolutionHelpers {

// ============================================================================
// MethodReceiverResolution: デフォルトコンストラクタ
// ============================================================================
MethodReceiverResolution::MethodReceiverResolution()
    : kind(Kind::None), canonical_name(), variable_ptr(nullptr), chain_value(nullptr) {}

// ============================================================================
// resolve_method_receiver: レシーバノードを解決（メインエントリーポイント）
// ============================================================================
MethodReceiverResolution resolve_method_receiver(const ASTNode* receiver_node, ExpressionEvaluator& evaluator) {
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
        Variable* var = evaluator.get_interpreter().find_variable(name);
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
        return resolve_member_receiver(receiver_node, evaluator);
    case ASTNodeType::AST_ARROW_ACCESS:
        // アロー演算子は (*ptr).member と等価
        return resolve_arrow_receiver(receiver_node, evaluator);
    case ASTNodeType::AST_ARRAY_REF:
        return resolve_array_receiver(receiver_node, evaluator);
    case ASTNodeType::AST_FUNC_CALL:
        return create_chain_receiver_from_expression(receiver_node, evaluator);
    default:
        break;
    }

    return create_chain_receiver_from_expression(receiver_node, evaluator);
}

// ============================================================================
// resolve_array_receiver: 配列要素のレシーバ解決
// ============================================================================
MethodReceiverResolution resolve_array_receiver(const ASTNode* array_node, ExpressionEvaluator& evaluator) {
    MethodReceiverResolution result;
    if (!array_node || array_node->node_type != ASTNodeType::AST_ARRAY_REF) {
        return result;
    }

    // シンプルな変数配列の場合は直接参照を試みる
    if (array_node->left && array_node->left->node_type == ASTNodeType::AST_VARIABLE && array_node->array_index) {
        std::string base_name = array_node->left->name;
        try {
            int64_t index_value = evaluator.evaluate_expression(array_node->array_index.get());
            std::string element_name = base_name + "[" + std::to_string(index_value) + "]";
            Variable* element_var = evaluator.get_interpreter().find_variable(element_name);
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

    return create_chain_receiver_from_expression(array_node, evaluator);
}

// ============================================================================
// resolve_member_receiver: メンバーアクセスのレシーバ解決
// ============================================================================
MethodReceiverResolution resolve_member_receiver(const ASTNode* member_node, ExpressionEvaluator& evaluator) {
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

    MethodReceiverResolution base_resolution = resolve_method_receiver(base_node, evaluator);

    auto create_chain_from_struct = [&](const Variable& struct_var) {
        try {
            Variable member_var = MemberAccessHelpers::get_struct_member_from_variable(struct_var, member_name, evaluator.get_interpreter());
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
            Variable* member_var = evaluator.get_interpreter().find_variable(member_path);
            if (!member_var) {
                try {
                    member_var = evaluator.get_interpreter().get_struct_member(base_name, member_name);
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
    return create_chain_receiver_from_expression(member_node, evaluator);
}

// ============================================================================
// resolve_arrow_receiver: アロー演算子のレシーバ解決
// ============================================================================
MethodReceiverResolution resolve_arrow_receiver(const ASTNode* arrow_node, ExpressionEvaluator& evaluator) {
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
        int64_t ptr_value = evaluator.evaluate_expression(base_node);
        
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
        Variable member_var = MemberAccessHelpers::get_struct_member_from_variable(*struct_var, member_name, evaluator.get_interpreter());
        
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

// ============================================================================
// create_chain_receiver_from_expression: 式の評価結果からチェーンレシーバ作成
// ============================================================================
MethodReceiverResolution create_chain_receiver_from_expression(const ASTNode* node, ExpressionEvaluator& evaluator) {
    MethodReceiverResolution result;
    if (!node) {
        return result;
    }

    try {
        int64_t primitive_value = evaluator.evaluate_expression(node);
        InferredType inferred_type = evaluator.get_type_engine().infer_type(node);
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

} // namespace ReceiverResolutionHelpers
