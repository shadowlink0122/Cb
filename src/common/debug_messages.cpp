#include "debug_messages.h"
#include "debug/debug_ast_messages.h"
#include "debug/debug_codegen_cpp_messages.h"
#include "debug/debug_hir_messages.h"
#include "debug/debug_interpreter_messages.h"
#include "debug/debug_parser_messages.h"
#include <vector>

// デバッグメッセージテンプレート配列を動的に初期化する関数
static std::vector<DebugMessageTemplate> init_debug_messages() {
    // MAX_DEBUG_MSG_IDまで初期化
    std::vector<DebugMessageTemplate> messages(
        static_cast<int>(DebugMsgId::MAX_DEBUG_MSG_ID));

    // 各モジュールのメッセージを初期化
    DebugMessages::Parser::init_parser_messages(messages);
    DebugMessages::AST::init_ast_messages(messages);
    DebugMessages::Interpreter::init_interpreter_messages(messages);
    DebugMessages::HIR::init_hir_messages(messages);
    DebugMessages::CodegenCpp::init_codegen_cpp_messages(messages);

    return messages;
}

// グローバルアクセス関数
const DebugMessageTemplate &get_debug_message(DebugMsgId id) {
    static const auto messages = init_debug_messages();
    int index = static_cast<int>(id);
    if (index < 0 || index >= static_cast<int>(DebugMsgId::MAX_DEBUG_MSG_ID)) {
        static const DebugMessageTemplate fallback = {
            "[UNKNOWN] Unknown debug message ID",
            "[UNKNOWN] 不明なデバッグメッセージID"};
        return fallback;
    }
    return messages[index];
}
