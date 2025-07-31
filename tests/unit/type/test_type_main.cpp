
#include <iostream>
#ifdef __cplusplus
extern "C" {
#endif
void yyerror(const char *, const char *);
#ifdef __cplusplus
}
inline void yyerror(const char *s) { yyerror(s, ""); }
#endif
#include <exception>

void test_unit_tiny();
void test_unit_short();
void test_unit_int();
void test_unit_long();

void test_unit_type() {
    test_unit_tiny();
    test_unit_short();
    test_unit_int();
    test_unit_long();
    std::cout << "[type] all tests passed" << std::endl;
}
