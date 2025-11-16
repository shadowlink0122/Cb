#ifndef DEBUG_AST_MESSAGES_H
#define DEBUG_AST_MESSAGES_H

#include "../debug.h"
#include "../debug_messages.h"

namespace DebugMessages {
namespace AST {

// AST関連のデバッグメッセージを初期化
void init_ast_messages(std::vector<DebugMessageTemplate> &messages);

} // namespace AST
} // namespace DebugMessages

#endif // DEBUG_AST_MESSAGES_H
