#ifndef DEBUG_MESSAGES_H
#define DEBUG_MESSAGES_H

#include "debug.h"

// デバッグメッセージテンプレート構造体
struct DebugMessageTemplate {
    const char *en;
    const char *ja;
};

// デバッグメッセージテンプレート配列（外部宣言）
extern const DebugMessageTemplate debug_messages[];

// デバッグメッセージ配列のサイズ
extern const int debug_messages_size;

#endif // DEBUG_MESSAGES_H
