#include <cstdio>
#include <iostream>

// debug_printラッパーのextern宣言
extern void debug_print(const char *fmt, ...);

int main() {
    std::cerr << "[debug_build] debug_print test: should see DEBUG line below "
                 "if -DDEBUG"
              << std::endl;
    debug_print("DEBUG: test_debug_print called!\n");
    return 0;
}
