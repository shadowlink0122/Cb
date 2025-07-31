#include <cstdarg>
extern "C" void vdebug_printf(const char *fmt, va_list args);
#include <cstdio>
#include <iostream>

// vdebug_printfのみ参照
// debug_printラッパー実装
void debug_print(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    vdebug_printf(fmt, args);
    va_end(args);
}

int main() {
    std::cerr << "[debug_build] debug_print test: should see DEBUG line below "
                 "if -DDEBUG"
              << std::endl;
    debug_print("DEBUG: test_debug_print called!\n");
    return 0;
}
