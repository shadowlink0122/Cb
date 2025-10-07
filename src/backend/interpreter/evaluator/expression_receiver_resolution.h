// ============================================================================
// expression_receiver_resolution.h
// ============================================================================
// Phase 12 Refactoring: Method Receiver Resolution
//
// メソッド呼び出しのレシーバ解決に特化したヘルパー関数群。
// レシーバは変数、配列要素、メンバーアクセス、アロー演算子など多様な形式で
// 指定されるため、それらを統一的に解決するロジックを提供。
//
// 【主な機能】:
// - resolve_method_receiver: レシーバノードを解決（メインエントリーポイント）
// - resolve_array_receiver: 配列要素のレシーバ解決
// - resolve_member_receiver: メンバーアクセスのレシーバ解決
// - resolve_arrow_receiver: アロー演算子のレシーバ解決
// - create_chain_receiver_from_expression: 式の評価結果からチェーンレシーバ作成
//
// 【レシーバ解決の種類】:
// - Direct: 変数への直接参照（Variable*とcanonical_nameを保持）
// - Chain: 式の評価結果（ReturnExceptionを保持）
//
// 【使用例】:
//   MethodReceiverResolution res = resolve_method_receiver(receiver_node);
//   if (res.kind == MethodReceiverResolution::Kind::Direct) {
//       // res.variable_ptrを使用
//   } else if (res.kind == MethodReceiverResolution::Kind::Chain) {
//       // res.chain_valueを使用
//   }
// ============================================================================

#ifndef EXPRESSION_RECEIVER_RESOLUTION_H
#define EXPRESSION_RECEIVER_RESOLUTION_H

#include <memory>
#include <string>

struct ASTNode;
class Interpreter;
class ReturnException;
struct Variable;
class ExpressionEvaluator;

namespace ReceiverResolutionHelpers {

// ============================================================================
// MethodReceiverResolution: レシーバ解決結果を表す構造体
// ============================================================================
struct MethodReceiverResolution {
    enum class Kind {
        None,    // 未解決
        Direct,  // 変数への直接参照
        Chain    // 式の評価結果（チェーン）
    };

    Kind kind;
    std::string canonical_name;                     // Direct時の変数の完全修飾名
    Variable* variable_ptr;                         // Direct時の変数ポインタ
    std::shared_ptr<ReturnException> chain_value;   // Chain時の評価結果

    MethodReceiverResolution();
};

// ============================================================================
// resolve_method_receiver: レシーバノードを解決（メインエントリーポイント）
// ============================================================================
// 【引数】:
//   - receiver_node: レシーバを表すASTノード
//   - evaluator: ExpressionEvaluatorへの参照
//
// 【戻り値】:
//   - MethodReceiverResolution: 解決結果
//
// 【動作】:
//   - VARIABLE/IDENTIFIERの場合: 変数を検索してDirect解決
//   - MEMBER_ACCESSの場合: resolve_member_receiverへ委譲
//   - ARROW_ACCESSの場合: resolve_arrow_receiverへ委譲
//   - ARRAY_REFの場合: resolve_array_receiverへ委譲
//   - FUNC_CALLの場合: create_chain_receiver_from_expressionへ委譲
//   - その他: create_chain_receiver_from_expressionへ委譲
// ============================================================================
MethodReceiverResolution resolve_method_receiver(const ASTNode* receiver_node, ExpressionEvaluator& evaluator);

// ============================================================================
// resolve_array_receiver: 配列要素のレシーバ解決
// ============================================================================
// 【引数】:
//   - array_node: 配列参照を表すASTノード（AST_ARRAY_REF）
//   - evaluator: ExpressionEvaluatorへの参照
//
// 【戻り値】:
//   - MethodReceiverResolution: 解決結果
//
// 【動作】:
//   - シンプルな変数配列の場合: base[index]形式の変数を検索してDirect解決
//   - それ以外の場合: create_chain_receiver_from_expressionへ委譲
// ============================================================================
MethodReceiverResolution resolve_array_receiver(const ASTNode* array_node, ExpressionEvaluator& evaluator);

// ============================================================================
// resolve_member_receiver: メンバーアクセスのレシーバ解決
// ============================================================================
// 【引数】:
//   - member_node: メンバーアクセスを表すASTノード（AST_MEMBER_ACCESS）
//   - evaluator: ExpressionEvaluatorへの参照
//
// 【戻り値】:
//   - MethodReceiverResolution: 解決結果
//
// 【動作】:
//   - ベースノードを再帰的に解決
//   - Direct解決の場合: base.member形式の変数を検索
//   - 構造体の場合: get_struct_member_from_variableでメンバー取得
//   - Chain解決の場合: チェーン内の構造体からメンバー取得
//   - 直接解決できない場合: create_chain_receiver_from_expressionへ委譲
// ============================================================================
MethodReceiverResolution resolve_member_receiver(const ASTNode* member_node, ExpressionEvaluator& evaluator);

// ============================================================================
// resolve_arrow_receiver: アロー演算子のレシーバ解決
// ============================================================================
// 【引数】:
//   - arrow_node: アロー演算子を表すASTノード（AST_ARROW_ACCESS）
//   - evaluator: ExpressionEvaluatorへの参照
//
// 【戻り値】:
//   - MethodReceiverResolution: 解決結果
//
// 【動作】:
//   - ベースノードを評価してポインタ値を取得
//   - nullポインタチェック
//   - ポインタが指す構造体を取得
//   - Interface型の場合: Interface全体をチェーン値として返す
//   - 通常の構造体の場合: get_struct_member_from_variableでメンバー取得
//   - 評価結果をチェーン値として返す
// ============================================================================
MethodReceiverResolution resolve_arrow_receiver(const ASTNode* arrow_node, ExpressionEvaluator& evaluator);

// ============================================================================
// create_chain_receiver_from_expression: 式の評価結果からチェーンレシーバ作成
// ============================================================================
// 【引数】:
//   - node: 評価する式のASTノード
//   - evaluator: ExpressionEvaluatorへの参照
//
// 【戻り値】:
//   - MethodReceiverResolution: 解決結果（常にChain）
//
// 【動作】:
//   - ノードを評価
//   - プリミティブ値の場合: 型推論してReturnExceptionを作成
//   - ReturnExceptionがスローされた場合: それをキャッチしてチェーン値とする
//   - 評価結果をチェーン値として返す
// ============================================================================
MethodReceiverResolution create_chain_receiver_from_expression(const ASTNode* node, ExpressionEvaluator& evaluator);

} // namespace ReceiverResolutionHelpers

#endif // EXPRESSION_RECEIVER_RESOLUTION_H
