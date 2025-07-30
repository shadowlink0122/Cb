#include "test_assign_case.h"
#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <string>

static std::string run_and_capture(const std::string &cmd) {
    std::string result;
    FILE *pipe = popen(cmd.c_str(), "r");
    if (!pipe)
        return "";
    char buffer[256];
    while (fgets(buffer, sizeof(buffer), pipe)) {
        result += buffer;
    }
    pclose(pipe);
    return result;
}

// tiny
void test_integration_assign_tiny_ok() {
    std::string output = run_and_capture("./main ./tests/cases/assign/tiny/ok.cb");
    assert(output.find("127") != std::string::npos);
    std::cout << "[integration] assign tiny ok test passed\n";
}
void test_integration_assign_tiny_ng() {
    int ret = system("./main ./tests/cases/assign/tiny/ng.cb > /dev/null 2>&1");
    if (WIFEXITED(ret) && WEXITSTATUS(ret) != 0) {
        std::cout << "[integration] assign tiny ng test passed (error detected)\n";
    } else {
        std::cerr << "[integration] assign tiny ng test failed (no error)\n";
        assert(false);
    }
}
void test_integration_assign_tiny_ng_neg() {
    int ret = system("./main ./tests/cases/assign/tiny/ng_neg.cb > /dev/null 2>&1");
    if (WIFEXITED(ret) && WEXITSTATUS(ret) != 0) {
        std::cout << "[integration] assign tiny ng_neg test passed (error detected)\n";
    } else {
        std::cerr << "[integration] assign tiny ng_neg test failed (no error)\n";
        assert(false);
    }
}

// short
void test_integration_assign_short_ok() {
    std::string output = run_and_capture("./main ./tests/cases/assign/short/ok.cb");
    assert(output.find("32767") != std::string::npos);
    std::cout << "[integration] assign short ok test passed\n";
}
void test_integration_assign_short_ng() {
    int ret = system("./main ./tests/cases/assign/short/ng.cb > /dev/null 2>&1");
    if (WIFEXITED(ret) && WEXITSTATUS(ret) != 0) {
        std::cout << "[integration] assign short ng test passed (error detected)\n";
    } else {
        std::cerr << "[integration] assign short ng test failed (no error)\n";
        assert(false);
    }
}
void test_integration_assign_short_ng_neg() {
    int ret = system("./main ./tests/cases/assign/short/ng_neg.cb > /dev/null 2>&1");
    if (WIFEXITED(ret) && WEXITSTATUS(ret) != 0) {
        std::cout << "[integration] assign short ng_neg test passed (error detected)\n";
    } else {
        std::cerr << "[integration] assign short ng_neg test failed (no error)\n";
        assert(false);
    }
}

// int
void test_integration_assign_int_ok() {
    std::string output = run_and_capture("./main ./tests/cases/assign/int/ok.cb");
    assert(output.find("2147483647") != std::string::npos);
    std::cout << "[integration] assign int ok test passed\n";
}
void test_integration_assign_int_ng() {
    int ret = system("./main ./tests/cases/assign/int/ng.cb > /dev/null 2>&1");
    if (WIFEXITED(ret) && WEXITSTATUS(ret) != 0) {
        std::cout << "[integration] assign int ng test passed (error detected)\n";
    } else {
        std::cerr << "[integration] assign int ng test failed (no error)\n";
        assert(false);
    }
}
void test_integration_assign_int_ng_neg() {
    int ret = system("./main ./tests/cases/assign/int/ng_neg.cb > /dev/null 2>&1");
    if (WIFEXITED(ret) && WEXITSTATUS(ret) != 0) {
        std::cout << "[integration] assign int ng_neg test passed (error detected)\n";
    } else {
        std::cerr << "[integration] assign int ng_neg test failed (no error)\n";
        assert(false);
    }
}

// long
void test_integration_assign_long_ok() {
    std::string output = run_and_capture("./main ./tests/cases/assign/long/ok.cb");
    assert(output.find("9223372036854775807") != std::string::npos);
    std::cout << "[integration] assign long ok test passed\n";
}
void test_integration_assign_long_ng() {
}
void test_integration_assign_long_ng_neg() {
}

void test_integration_assign() {
    test_integration_assign_tiny_ok();
    test_integration_assign_tiny_ng();
    test_integration_assign_tiny_ng_neg();
    test_integration_assign_short_ok();
    test_integration_assign_short_ng();
    test_integration_assign_short_ng_neg();
    test_integration_assign_int_ok();
    test_integration_assign_int_ng();
    test_integration_assign_int_ng_neg();
    test_integration_assign_long_ok();
    test_integration_assign_long_ng();
    test_integration_assign_long_ng_neg();
}
