#include "test_boundary_case.h"
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
void test_integration_boundary_tiny_ok() {
    std::string output = run_and_capture("./main ./tests/cases/boundary/tiny/ok.cb");
    assert(output.find("127") != std::string::npos);
    assert(output.find("-128") != std::string::npos);
    std::cout << "[integration] boundary tiny ok test passed\n";
}
void test_integration_boundary_tiny_ng() {
    int ret = system("./main ./tests/cases/boundary/tiny/ng.cb > /dev/null 2>&1");
    if (WIFEXITED(ret) && WEXITSTATUS(ret) != 0) {
        std::cout << "[integration] boundary tiny ng test passed (error detected)\n";
    } else {
        std::cerr << "[integration] boundary tiny ng test failed (no error)\n";
        assert(false);
    }
}
void test_integration_boundary_tiny_ng_neg() {
    int ret = system("./main ./tests/cases/boundary/tiny/ng_neg.cb > /dev/null 2>&1");
    if (WIFEXITED(ret) && WEXITSTATUS(ret) != 0) {
        std::cout << "[integration] boundary tiny ng_neg test passed (error detected)\n";
    } else {
        std::cerr << "[integration] boundary tiny ng_neg test failed (no error)\n";
        assert(false);
    }
}
// short
void test_integration_boundary_short_ok() {
    std::string output = run_and_capture("./main ./tests/cases/boundary/short/ok.cb");
    assert(output.find("32767") != std::string::npos);
    assert(output.find("-32768") != std::string::npos);
    std::cout << "[integration] boundary short ok test passed\n";
}
void test_integration_boundary_short_ng() {
    int ret = system("./main ./tests/cases/boundary/short/ng.cb > /dev/null 2>&1");
    if (WIFEXITED(ret) && WEXITSTATUS(ret) != 0) {
        std::cout << "[integration] boundary short ng test passed (error detected)\n";
    } else {
        std::cerr << "[integration] boundary short ng test failed (no error)\n";
        assert(false);
    }
}
void test_integration_boundary_short_ng_neg() {
    int ret = system("./main ./tests/cases/boundary/short/ng_neg.cb > /dev/null 2>&1");
    if (WIFEXITED(ret) && WEXITSTATUS(ret) != 0) {
        std::cout << "[integration] boundary short ng_neg test passed (error detected)\n";
    } else {
        std::cerr << "[integration] boundary short ng_neg test failed (no error)\n";
        assert(false);
    }
}

// int
void test_integration_boundary_int_ok() {
    std::string output = run_and_capture("./main ./tests/cases/boundary/int/ok.cb");
    assert(output.find("2147483647") != std::string::npos);
    assert(output.find("-2147483648") != std::string::npos);
    std::cout << "[integration] boundary int ok test passed\n";
}
void test_integration_boundary_int_ng() {
    int ret = system("./main ./tests/cases/boundary/int/ng.cb > /dev/null 2>&1");
    if (WIFEXITED(ret) && WEXITSTATUS(ret) != 0) {
        std::cout << "[integration] boundary int ng test passed (error detected)\n";
    } else {
        std::cerr << "[integration] boundary int ng test failed (no error)\n";
        assert(false);
    }
}
void test_integration_boundary_int_ng_neg() {
    int ret = system("./main ./tests/cases/boundary/int/ng_neg.cb > /dev/null 2>&1");
    if (WIFEXITED(ret) && WEXITSTATUS(ret) != 0) {
        std::cout << "[integration] boundary int ng_neg test passed (error detected)\n";
    } else {
        std::cerr << "[integration] boundary int ng_neg test failed (no error)\n";
        assert(false);
    }
}

// long
void test_integration_boundary_long_ok() {
    std::string output = run_and_capture("./main ./tests/cases/boundary/long/ok.cb");
    assert(output.find("9223372036854775807") != std::string::npos);
    assert(output.find("-9223372036854775808") != std::string::npos);
    std::cout << "[integration] boundary long ok test passed\n";
}
void test_integration_boundary_long_ng() {
    std::string output = run_and_capture("./main ./tests/cases/boundary/long/ng.cb");
    // 期待する動作や出力内容に応じて比較（例：オーバーフローで-9223372036854775808になるなど）
    if (output.find("-9223372036854775808") != std::string::npos) {
        std::cout << "[integration] boundary long ng test passed (overflow to negative)\n";
    } else {
        std::cerr << "[integration] boundary long ng test failed (unexpected output)\n";
        assert(false);
    }
}
void test_integration_boundary_long_ng_neg() {
    // 
}

void test_integration_boundary() {
    test_integration_boundary_tiny_ok();
    test_integration_boundary_tiny_ng();
    test_integration_boundary_tiny_ng_neg();
    test_integration_boundary_short_ok();
    test_integration_boundary_short_ng();
    test_integration_boundary_short_ng_neg();
    test_integration_boundary_int_ok();
    test_integration_boundary_int_ng();
    test_integration_boundary_int_ng_neg();
    test_integration_boundary_long_ok();
    test_integration_boundary_long_ng();
    test_integration_boundary_long_ng_neg();
}
