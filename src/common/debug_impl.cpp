#include "debug.h"
#include "debug_messages.h"
#include <cstdarg>
#include <cstdio>

// デバッグモードフラグ（実装）
bool debug_mode = false;
DebugLanguage debug_language = DebugLanguage::ENGLISH;

// debug_msg関数
void debug_msg(DebugMsgId msg_id, ...) {
    if (!debug_mode)
        return;

    const DebugMessageTemplate &msg = get_debug_message(msg_id);
    const char *format =
        (debug_language == DebugLanguage::JAPANESE) ? msg.ja : msg.en;

    va_list args;
    va_start(args, msg_id);
    vfprintf(stderr, format, args);
    va_end(args);
    fprintf(stderr, "\n");
}

// error_msg関数（stderrに直接出力、常時有効）
void error_msg(DebugMsgId msg_id, ...) {
    const DebugMessageTemplate &msg = get_debug_message(msg_id);
    const char *format =
        (debug_language == DebugLanguage::JAPANESE) ? msg.ja : msg.en;

    va_list args;
    va_start(args, msg_id);
    vfprintf(stderr, format, args);
    va_end(args);
    fprintf(stderr, "\n");
}
