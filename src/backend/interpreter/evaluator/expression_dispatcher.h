// ============================================================================
// expression_dispatcher.h
// ============================================================================
// Phase 13 Step 1: Large-Scale Expression Evaluation Dispatcher
//
// evaluate_expression()メソッド全体（2,466行）を独立したクラスに分離。
// 巨大なswitch文を持つ式評価のディスパッチャー。
//
// 【戦略】:
// このリファクタリングは段階的アプローチの第1段階：
// 1. まず巨大なswitch文全体を別クラスに移動（このファイル）
// 2. その後、個別の大きなケースを更に分離（AST_FUNC_CALL, AST_MEMBER_ACCESS等）
// 3. 最終的に各ケースが管理可能なサイズになる
//
// 【主な機能】:
// - 全てのASTノードタイプの評価をディスパッチ
// - リテラル、変数参照、演算子、関数呼び出し、メンバーアクセス等
// - ExpressionEvaluatorからの移行を容易にするため、同じインターフェースを維持
//
// 【設計原則】:
// - ExpressionEvaluatorへの参照を保持
// - Interpreterへの直接アクセスも可能
// - 既存のヘルパー関数群を全て利用
// - 元のコードの動作を完全に保持
//
// 【使用例】:
//   ExpressionDispatcher dispatcher(expression_evaluator);
//   int64_t result = dispatcher.dispatch_expression(node);
// ============================================================================

#ifndef EXPRESSION_DISPATCHER_H
#define EXPRESSION_DISPATCHER_H

#include <cstdint>

// 前方宣言
struct ASTNode;
class Interpreter;
class ExpressionEvaluator;

// ============================================================================
// ExpressionDispatcher: 式評価のディスパッチャークラス
// ============================================================================
class ExpressionDispatcher {
  public:
    // コンストラクタ
    ExpressionDispatcher(ExpressionEvaluator &expression_evaluator);

    // ============================================================================
    // dispatch_expression: 式評価のメインディスパッチメソッド
    // ============================================================================
    // 【引数】:
    //   - node: 評価するASTノード
    //
    // 【戻り値】:
    //   - int64_t: 評価結果（プリミティブ型の場合）
    //   - 例外: ReturnException（構造体、配列、文字列等の場合）
    //
    // 【処理内容】:
    //   - ノードタイプに応じて適切な評価関数にディスパッチ
    //   - 巨大なswitch文で全てのASTノードタイプをカバー
    //   - 既存のヘルパー関数群を活用
    //
    // 【対応するノードタイプ】:
    //   - リテラル: NUMBER, STRING_LITERAL, NULLPTR
    //   - 変数: VARIABLE, IDENTIFIER
    //   - 配列: ARRAY_REF, ARRAY_LITERAL
    //   - 演算子: BINARY_OP, UNARY_OP, TERNARY_OP
    //   - インクリメント/デクリメント: PRE_INCDEC, POST_INCDEC
    //   - 関数呼び出し: FUNC_CALL, FUNC_PTR_CALL
    //   - 代入: ASSIGN
    //   - メンバーアクセス: MEMBER_ACCESS, ARROW_ACCESS, MEMBER_ARRAY_ACCESS
    //   - 構造体: STRUCT_LITERAL
    //   - Enum: ENUM_ACCESS
    // ============================================================================
    int64_t dispatch_expression(const ASTNode *node);

  private:
    ExpressionEvaluator &expression_evaluator_; // 式評価エンジンへの参照
    Interpreter &interpreter_; // インタープリターへの参照（高速アクセス用）
};

#endif // EXPRESSION_DISPATCHER_H
