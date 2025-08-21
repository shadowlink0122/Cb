#ifndef DEBUG_H
#define DEBUG_H

// デバッグモードフラグ
extern bool debug_mode;

// デバッグ出力関数
void debug_print(const char *fmt, ...);

#endif // DEBUG_H
