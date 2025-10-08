#include "ternary.h"
#include "../../../../common/debug.h"
#include "../../core/interpreter.h"
#include "../../core/type_inference.h"
#include <stdexcept>

namespace TernaryHelpers {

TypedValue evaluate_ternary_typed(
    const ASTNode *node, Interpreter &interpreter,
    std::function<int64_t(const ASTNode *)> evaluate_expression_callback,
    std::function<TypedValue(const ASTNode *)>
        evaluate_typed_expression_callback,
    TypeInferenceEngine &type_engine, TypedValue &last_typed_result) {
    debug_msg(DebugMsgId::TERNARY_EVAL_START);

    // 条件式を評価
    int64_t condition = evaluate_expression_callback(node->left.get());

    // 条件に基づいて選択されるノードを決定
    const ASTNode *selected_node =
        condition ? node->right.get() : node->third.get();

    // 選択されたノードの型を推論
    InferredType selected_type = type_engine.infer_type(selected_node);

    debug_msg(DebugMsgId::TERNARY_NODE_TYPE,
              static_cast<int>(selected_node->node_type),
              static_cast<int>(selected_type.type_info));
    debug_msg(DebugMsgId::TERNARY_TYPE_INFERENCE,
              static_cast<int>(selected_type.type_info),
              selected_type.type_name.c_str());

    // 単純な型（数値、文字列）の場合は直接評価
    if (selected_type.type_info == TYPE_INT ||
        selected_type.type_info == TYPE_BOOL) {
        TypedValue result = evaluate_typed_expression_callback(selected_node);
        last_typed_result = result;
        return result;
    } else if (selected_type.type_info == TYPE_STRING &&
               selected_node->node_type == ASTNodeType::AST_STRING_LITERAL) {
        TypedValue result = evaluate_typed_expression_callback(selected_node);
        last_typed_result = result;
        return result;
    } else if (selected_type.type_info == TYPE_STRING &&
               selected_node->node_type == ASTNodeType::AST_VARIABLE) {
        // 文字列変数参照の場合
        TypedValue result = evaluate_typed_expression_callback(selected_node);
        last_typed_result = result;
        return result;
    } else if (selected_type.type_info == TYPE_STRING &&
               selected_node->node_type == ASTNodeType::AST_FUNC_CALL) {
        // 文字列を返す関数呼び出しの場合
        try {
            // 関数を実行（副作用のため実行）
            (void)evaluate_expression_callback(selected_node);
            TypedValue result =
                TypedValue("", InferredType(TYPE_STRING, "string"));
            last_typed_result = result;
            return result;
        } catch (const ReturnException &ret) {
            if (ret.type == TYPE_STRING) {
                TypedValue result = TypedValue(
                    ret.str_value, InferredType(TYPE_STRING, "string"));
                last_typed_result = result;
                return result;
            } else {
                TypedValue result =
                    TypedValue("", InferredType(TYPE_STRING, "string"));
                last_typed_result = result;
                return result;
            }
        }
    } else if (selected_node->node_type == ASTNodeType::AST_ARRAY_REF) {
        // 配列アクセスの場合（関数呼び出し配列アクセスを含む）
        TypedValue result = evaluate_typed_expression_callback(selected_node);
        last_typed_result = result;
        return result;
    } else if (selected_node->node_type == ASTNodeType::AST_TERNARY_OP) {
        // ネストした三項演算子の場合は再帰的に評価
        TypedValue result = evaluate_ternary_typed(
            selected_node, interpreter, evaluate_expression_callback,
            evaluate_typed_expression_callback, type_engine, last_typed_result);
        last_typed_result = result;
        return result;
    } else if (selected_node->node_type == ASTNodeType::AST_ARRAY_REF) {
        // 配列要素アクセスの場合は直接評価
        int64_t numeric_result = evaluate_expression_callback(selected_node);
        TypedValue result = TypedValue(numeric_result, selected_type);
        last_typed_result = result;
        return result;
    } else if (selected_node->node_type == ASTNodeType::AST_MEMBER_ACCESS) {
        // 構造体メンバアクセスの場合 - 型に基づいて処理を分岐
        if (selected_type.type_info == TYPE_STRING) {
            debug_msg(DebugMsgId::TERNARY_STRING_MEMBER_ACCESS);
            // 文字列型のメンバアクセスの場合 - 直接構造体メンバにアクセス
            if (selected_node->left &&
                selected_node->left->node_type == ASTNodeType::AST_VARIABLE) {
                std::string struct_name = selected_node->left->name;
                std::string member_name = selected_node->name;

                // interpreter.get_struct_member
                // を使用する代わりに、直接変数名で検索
                std::string member_var_name = struct_name + "." + member_name;
                Variable *member_var =
                    interpreter.find_variable(member_var_name);

                if (member_var && member_var->type == TYPE_STRING) {
                    debug_msg(DebugMsgId::TERNARY_STRING_EVAL,
                              member_var->str_value.c_str());
                    TypedValue result =
                        TypedValue(member_var->str_value,
                                   InferredType(TYPE_STRING, "string"));
                    last_typed_result = result;
                    return result;
                }
            }
        }
        // 数値型の場合も型付き評価を利用
        TypedValue result = evaluate_typed_expression_callback(selected_node);
        last_typed_result = result;
        return result;
    } else if (selected_node->node_type == ASTNodeType::AST_FUNC_CALL) {
        // 関数呼び出し（メソッド呼び出し含む）の場合も型付き評価を利用
        TypedValue result = evaluate_typed_expression_callback(selected_node);
        last_typed_result = result;
        return result;
    }

    // 複雑な型（配列、構造体、関数呼び出しなど）の場合は遅延評価
    TypedValue result = TypedValue::deferred(selected_node, selected_type);
    last_typed_result = result;
    return result;
}

} // namespace TernaryHelpers
