#ifndef DEBUG_MESSAGES_H
#define DEBUG_MESSAGES_H

#include "debug.h"

// デバッグメッセージテンプレート構造体
struct DebugMessageTemplate {
    const char *en;
    const char *ja;
};

// デバッグメッセージ取得関数
const DebugMessageTemplate &get_debug_message(DebugMsgId msg_id);

// デバッグメッセージ配列のサイズ（後方互換性のため）
extern const int debug_messages_size;

#endif // DEBUG_MESSAGES_H
