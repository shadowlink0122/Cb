#ifndef DEBUG_HIR_MESSAGES_H
#define DEBUG_HIR_MESSAGES_H

#include "../debug.h"
#include "../debug_messages.h"
#include <vector>

namespace DebugMessages {
namespace HIR {

// HIR関連のデバッグメッセージを初期化
void init_hir_messages(std::vector<DebugMessageTemplate> &messages);

} // namespace HIR
} // namespace DebugMessages

#endif // DEBUG_HIR_MESSAGES_H
