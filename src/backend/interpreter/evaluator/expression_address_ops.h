#ifndef EXPRESSION_ADDRESS_OPS_H
#define EXPRESSION_ADDRESS_OPS_H

#include <cstdint>
#include <functional>

struct ASTNode;
class Interpreter;

// ========================================================================
// アドレス演算子と間接参照演算子のヘルパー関数
// ========================================================================

namespace AddressOperationHelpers {
    
    // アドレス演算子 (&) の評価
    // 変数、配列要素、構造体メンバーのアドレスを取得
    int64_t evaluate_address_of(
        const ASTNode* node,
        Interpreter& interpreter,
        std::function<int64_t(const ASTNode*)> evaluate_expression_func
    );
    
    // 間接参照演算子 (*) の評価
    // ポインタが指す値を取得（メタデータポインタと従来ポインタの両方に対応）
    int64_t evaluate_dereference(
        const ASTNode* node,
        Interpreter& interpreter,
        std::function<int64_t(const ASTNode*)> evaluate_expression_func
    );
    
}

#endif // EXPRESSION_ADDRESS_OPS_H
