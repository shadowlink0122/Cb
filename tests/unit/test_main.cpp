#include <iostream>
extern "C" void yyerror(const char *) {}
void yyerror(const char *, const char *) {}
#include <exception>
#include <iostream>

void test_unit_type();
void test_unit_assign();
void test_unit_boundary();
void test_unit_arithmetic();
void test_unit_cross_type();

int main() {
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
