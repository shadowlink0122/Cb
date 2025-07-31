#ifdef __cplusplus
extern "C" {
#endif
void yyerror(const char *, const char *);
#ifdef __cplusplus
}
inline void yyerror(const char *s) { yyerror(s, ""); }
#endif
#include "arithmetic/test_arithmetic_case.h"
#include "assign/test_assign_case.h"
#include "boundary/test_boundary_case.h"
#include "cross_type/test_cross_type_case.h"
#include "func/test_func_case.h"
#include <iostream>

int main() {
    int fail = 0;
    try {
        test_integration_arithmetic();
        test_integration_assign();
        test_integration_boundary();
        test_integration_cross_type();
        test_integration_func();
    } catch (const std::exception &e) {
        std::cerr << "[integration] test failed: " << e.what() << std::endl;
        fail = 1;
    } catch (...) {
        std::cerr << "[integration] test failed: unknown error" << std::endl;
        fail = 1;
    }
    if (fail == 0) {
        std::cout << "[integration] all tests passed" << std::endl;
    }
    return fail;
}
