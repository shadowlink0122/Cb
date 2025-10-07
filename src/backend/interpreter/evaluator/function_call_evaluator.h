// ============================================================================
// function_call_evaluator.h
// ============================================================================
// Phase 13 Refactoring: Large-Scale Function Call Evaluation
//
// AST_FUNC_CALLケース全体（1,553行）を独立したクラスに分離。
// 関数呼び出し、メソッド呼び出し、関数ポインタ呼び出しの全てを処理。
//
// 【主な機能】:
// - 通常の関数呼び出し
// - メソッド呼び出し（レシーバ付き）
// - 関数ポインタ呼び出し（Form 1, Form 2）
// - 関数ポインタチェーン呼び出し
// - 引数の評価と型変換
// - 戻り値の処理（プリミティブ、構造体、配列、関数ポインタ）
//
// 【設計原則】:
// - 元のevaluate_expression()内のAST_FUNC_CALLケースをそのまま移植
// - Interpreterへの参照を保持し、全ての機能にアクセス
// - ExpressionEvaluatorへの参照も保持し、再帰的評価を可能にする
//
// 【使用例】:
//   FunctionCallEvaluator evaluator(interpreter, expression_evaluator);
//   int64_t result = evaluator.evaluate_function_call(node);
// ============================================================================

#ifndef FUNCTION_CALL_EVALUATOR_H
#define FUNCTION_CALL_EVALUATOR_H

#include <cstdint>
#include <string>

// 前方宣言
struct ASTNode;
class Interpreter;
class ExpressionEvaluator;

// ============================================================================
// FunctionCallEvaluator: 関数呼び出し専用評価クラス
// ============================================================================
class FunctionCallEvaluator {
  public:
    // コンストラクタ
    FunctionCallEvaluator(Interpreter &interpreter,
                          ExpressionEvaluator &expression_evaluator);

    // ============================================================================
    // evaluate_function_call: 関数呼び出しのメイン評価メソッド
    // ============================================================================
    // 【引数】:
    //   - node: AST_FUNC_CALLノード
    //
    // 【戻り値】:
    //   - int64_t: 関数の戻り値（プリミティブ型の場合）
    //   - 例外: ReturnException（構造体、配列、関数ポインタの場合）
    //
    // 【処理内容】:
    //   1. 関数ポインタ呼び出しのチェック（Form 1, Form 2）
    //   2. メソッド呼び出しのチェック
    //   3. 通常の関数呼び出し
    //   4. 引数の評価と型変換
    //   5. 関数本体の実行
    //   6. 戻り値の処理
    //
    // 【対応する呼び出し形式】:
    //   - func(args)                    // 通常の関数呼び出し
    //   - receiver.method(args)         // メソッド呼び出し
    //   - ptr(args)                     // 関数ポインタ呼び出し（Form 2）
    //   - (*ptr)(args)                  // 関数ポインタ呼び出し（Form 1）
    //   - func()()                      // 関数ポインタチェーン
    // ============================================================================
    int64_t evaluate_function_call(const ASTNode *node);

  private:
    Interpreter &interpreter_; // インタープリターへの参照
    ExpressionEvaluator &expression_evaluator_; // 式評価エンジンへの参照
};

#endif // FUNCTION_CALL_EVALUATOR_H
