#include "arithmetic/test_arithmetic_basic.h"
#include "run_and_capture_util.h"
#include <cassert>
#include <iostream>
#include <string>

void test_assign_basic() {
    int exit_code = 0;
    std::string cmd = "./main tests/cases/assign/int/ok.cb 2>&1";
    std::string output = run_and_capture(cmd, &exit_code);

    if (exit_code != 0) {
        std::cerr << "assign int ok.cb failed with exit code: " << exit_code
                  << std::endl;
        assert(false);
    }

    assert(output.find("2147483647") != std::string::npos);
    std::cout << "[integration] assign basic test passed" << std::endl;
}

void test_if_basic() {
    int exit_code = 0;
    std::string cmd = "./main tests/cases/if/basic.cb 2>&1";
    std::string output = run_and_capture(cmd, &exit_code);

    if (exit_code != 0) {
        std::cerr << "if basic.cb failed with exit code: " << exit_code
                  << std::endl;
        assert(false);
    }

    assert(output.find("ok") != std::string::npos);
    assert(output.find("ok2") != std::string::npos);
    assert(output.find("ok3") != std::string::npos);
    std::cout << "[integration] if basic test passed" << std::endl;
}

int main() {
    int fail = 0;
    try {
        test_integration_arithmetic_basic();
        test_assign_basic();
        test_if_basic();
        std::cout << "[integration] basic tests passed" << std::endl;
    } catch (const std::exception &e) {
        std::cerr << "[integration] test failed: " << e.what() << std::endl;
        fail = 1;
    } catch (...) {
        std::cerr << "[integration] test failed: unknown error" << std::endl;
        fail = 1;
    }
    return fail;
}
