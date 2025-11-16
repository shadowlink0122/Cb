#ifndef DEBUG_INTERPRETER_MESSAGES_H
#define DEBUG_INTERPRETER_MESSAGES_H

#include "../debug.h"
#include "../debug_messages.h"

namespace DebugMessages {
namespace Interpreter {

// インタープリタ関連のデバッグメッセージを初期化
void init_interpreter_messages(std::vector<DebugMessageTemplate> &messages);

} // namespace Interpreter
} // namespace DebugMessages

#endif // DEBUG_INTERPRETER_MESSAGES_H
