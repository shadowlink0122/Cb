#include <iostream>
#ifdef __cplusplus
extern "C" {
#endif
void yyerror(const char *, const char *);
#ifdef __cplusplus
}
// C++用ラッパー
inline void yyerror(const char *s) { yyerror(s, ""); }
#endif
#include <exception>
#include <iostream>

void test_unit_type();
#ifdef __cplusplus
extern "C" {
#endif
void test_unit_assign();
#ifdef __cplusplus
}
#endif
// boundary
#ifdef __cplusplus
extern "C" {
#endif
void test_unit_boundary();
#ifdef __cplusplus
}
#endif
void test_unit_arithmetic();
// cross_type
#ifdef __cplusplus
extern "C" {
#endif
void test_unit_cross_type();
#ifdef __cplusplus
}
#endif

int main() {
    extern void set_debug_mode_from_env();
    set_debug_mode_from_env();
    int fail = 0;
    try {
        test_unit_type();
        test_unit_assign();
        test_unit_boundary();
        test_unit_arithmetic();
        test_unit_cross_type();
    } catch (const std::exception &e) {
        std::cerr << "[unit] test failed: " << e.what() << std::endl;
        fail = 1;
    } catch (...) {
        std::cerr << "[unit] test failed: unknown error" << std::endl;
        fail = 1;
    }
    if (fail == 0) {
        std::cout << "[unit] all tests passed" << std::endl;
    }
    return fail;
}
