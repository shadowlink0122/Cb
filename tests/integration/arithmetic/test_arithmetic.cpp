#include "test_arithmetic_case.h"
#include <cassert>
#include <cstdlib>
#include <iostream>
#include <string>
#include "../run_and_capture_util.h"

void test_integration_arithmetic() {
    int exit_code = 0;
    char tmpfile1[] = "/tmp/arithmetic_ok_XXXXXX";
    int fd1 = mkstemp(tmpfile1);
    std::string cmd1 = std::string("./main ./tests/cases/arithmetic/ok.cb >") + tmpfile1 + " 2>&1";
    run_and_capture(cmd1.c_str(), &exit_code);
    FILE* fp1 = fopen(tmpfile1, "r");
    std::string output;
    if (fp1) { char buf[4096]; while (fgets(buf, sizeof(buf), fp1)) output += buf; fclose(fp1); }
    remove(tmpfile1);
    assert(output.find("15") != std::string::npos);
    assert(output.find("5") != std::string::npos);
    assert(output.find("30000") != std::string::npos);
    std::cout << "[integration] arithmetic ok test passed" << std::endl;

    // 異常系 tiny型範囲外
    char tmpfile2[] = "/tmp/arithmetic_ng_XXXXXX";
    int fd2 = mkstemp(tmpfile2);
    std::string cmd2 = std::string("./main ./tests/cases/arithmetic/ng.cb >") + tmpfile2 + " 2>&1";
    run_and_capture(cmd2.c_str(), &exit_code);
    FILE* fp2 = fopen(tmpfile2, "r");
    std::string ng_output;
    if (fp2) { char buf[4096]; while (fgets(buf, sizeof(buf), fp2)) ng_output += buf; fclose(fp2); }
    remove(tmpfile2);
    const char* expected = "型の範囲外の値を代入しようとしました";
    bool ok = (exit_code != 0)
        && (ng_output.find(expected) != std::string::npos)
        && (ng_output.find("128") == std::string::npos);
    if (ok) {
        std::cout << "[integration] arithmetic ng test passed (error detected)" << std::endl;
    } else {
        assert(false);
    }

    // long型範囲外
    // 未定義動作のため実施しない
}
