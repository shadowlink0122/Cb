#include "native_stdio_output.h"
#include <cstdio>

void NativeStdioOutput::write_char(char c) {
    putchar(c);
}

void NativeStdioOutput::write_string(const char *str) {
    fputs(str, stdout);
}

IOInterface *create_native_stdio_output() {
    return new NativeStdioOutput();
}
