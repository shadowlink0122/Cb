#include "debug.h"
#include "debug_messages.h"
#include <cstdarg>
#include <cstdio>

// デバッグモードフラグ（実装）
bool debug_mode = false;
DebugLanguage debug_language = DebugLanguage::ENGLISH;

// debug_print関数（既存）
void debug_print(const char *fmt, ...) {
    if (!debug_mode)
        return;

    va_list args;
    va_start(args, fmt);
    fprintf(stderr, "[DEBUG] ");
    vfprintf(stderr, fmt, args);
    va_end(args);
}

// debug_msg関数（新規）
void debug_msg(DebugMsgId msg_id, ...) {
    if (!debug_mode)
        return;

    int msg_index = static_cast<int>(msg_id);
    if (msg_index >= debug_messages_size) {
        return; // 範囲外アクセスを防止
    }

    const DebugMessageTemplate &msg = debug_messages[msg_index];
    const char *format =
        (debug_language == DebugLanguage::JAPANESE) ? msg.ja : msg.en;

    fprintf(stderr, "[DEBUG] ");
    va_list args;
    va_start(args, msg_id);
    vfprintf(stderr, format, args);
    va_end(args);
    fprintf(stderr, "\n");
}

// error_msg関数（新規 - stderrに直接出力、常時有効）
void error_msg(DebugMsgId msg_id, ...) {
    int msg_index = static_cast<int>(msg_id);
    if (msg_index >= debug_messages_size) {
        return; // 範囲外アクセスを防止
    }

    const DebugMessageTemplate &msg = debug_messages[msg_index];
    const char *format =
        (debug_language == DebugLanguage::JAPANESE) ? msg.ja : msg.en;

    va_list args;
    va_start(args, msg_id);
    vfprintf(stderr, format, args);
    va_end(args);
    fprintf(stderr, "\n");
}
