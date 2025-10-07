#pragma once
#include "../../../common/ast.h"
#include <cstdint>
#include <functional>

// 前方宣言
class Interpreter;

// ============================================================================
// 関数呼び出し評価のヘルパー関数群
// ============================================================================

namespace FunctionCallHelpers {

// 関数ポインタ呼び出しの評価（AST_FUNC_PTR_CALL）
// (*funcPtr)(args) 形式の呼び出しを処理
int64_t evaluate_function_pointer_call(
    const ASTNode* node,
    Interpreter& interpreter
);

} // namespace FunctionCallHelpers
