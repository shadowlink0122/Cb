#include "native_stdio_output.h"
#include <cstdio>
#include <cstdarg>

void NativeStdioOutput::write_char(char c) {
    putchar(c);
    fflush(stdout);
}

void NativeStdioOutput::write_string(const char* str) {
    printf("%s", str);
    fflush(stdout);
}

void NativeStdioOutput::write_formatted(const char* format, ...) {
    va_list args;
    va_start(args, format);
    vprintf(format, args);
    va_end(args);
    fflush(stdout);
}

// ファクトリー関数の実装
IOInterface* create_native_stdio_output() {
    static NativeStdioOutput instance;
    return &instance;
}

// デフォルト実装（条件コンパイルで選択）
#ifndef CB_TARGET_BAREMETAL
#ifndef CB_TARGET_WASM
IOInterface* create_default_io() {
    return create_native_stdio_output();
}
#endif
#endif
