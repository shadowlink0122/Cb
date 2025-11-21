#ifndef DEBUG_CODEGEN_CPP_MESSAGES_H
#define DEBUG_CODEGEN_CPP_MESSAGES_H

#include "../debug.h"
#include "../debug_messages.h"
#include <vector>

namespace DebugMessages {
namespace CodegenCpp {

// C++コード生成関連のデバッグメッセージを初期化
void init_codegen_cpp_messages(std::vector<DebugMessageTemplate> &messages);

} // namespace CodegenCpp
} // namespace DebugMessages

#endif // DEBUG_CODEGEN_CPP_MESSAGES_H
