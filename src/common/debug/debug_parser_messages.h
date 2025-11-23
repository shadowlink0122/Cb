#ifndef DEBUG_PARSER_MESSAGES_H
#define DEBUG_PARSER_MESSAGES_H

#include "../debug.h"
#include "../debug_messages.h"
#include <vector>

namespace DebugMessages {
namespace Parser {

// パーサ関連のデバッグメッセージを初期化
void init_parser_messages(std::vector<DebugMessageTemplate> &messages);

} // namespace Parser
} // namespace DebugMessages

#endif // DEBUG_PARSER_MESSAGES_H
