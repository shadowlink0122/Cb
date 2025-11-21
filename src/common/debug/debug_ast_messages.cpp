#include "debug_ast_messages.h"

namespace DebugMessages {
namespace AST {

void init_ast_messages(std::vector<DebugMessageTemplate> &messages) {
    // AST構築・処理関連のメッセージ
    // 現在はパーサとインタープリタに含まれているため
    // 将来的にAST変換やvalidationが追加された際に使用
    // TODO: AST-specific messages
    // - AST_VALIDATION_START
    // - AST_TRANSFORM_PASS
    // - AST_OPTIMIZATION
    // など
}

} // namespace AST
} // namespace DebugMessages
