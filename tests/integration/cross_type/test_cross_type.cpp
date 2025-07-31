
#include "../run_and_capture_util.h"
#include <cassert>
#include <iostream>
#include <string>

void test_integration_cross_type() {
    // 正常系
    int exit_code = 0;
    char tmpfile1[] = "/tmp/cross_type_ok_XXXXXX";
    int fd1 = mkstemp(tmpfile1);
    std::string cmd1 = std::string("./main ./tests/cases/cross_type/ok.cb >") + tmpfile1 + " 2>&1";
    run_and_capture(cmd1.c_str(), &exit_code);
    FILE* fp1 = fopen(tmpfile1, "r");
    std::string output;
    if (fp1) { char buf[4096]; while (fgets(buf, sizeof(buf), fp1)) output += buf; fclose(fp1); }
    remove(tmpfile1);
    assert(exit_code == 0);
    assert(output.find("42") != std::string::npos);
    std::cout << "[integration] cross_type ok test passed" << std::endl;

    // 異常系（型不一致や範囲外）
    char tmpfile2[] = "/tmp/cross_type_ng_XXXXXX";
    int fd2 = mkstemp(tmpfile2);
    std::string cmd2 = std::string("./main ./tests/cases/cross_type/ng.cb >") + tmpfile2 + " 2>&1";
    run_and_capture(cmd2.c_str(), &exit_code);
    FILE* fp2 = fopen(tmpfile2, "r");
    std::string ng_output;
    if (fp2) { char buf[4096]; while (fgets(buf, sizeof(buf), fp2)) ng_output += buf; fclose(fp2); }
    remove(tmpfile2);
    const char* expected = "型の範囲外の値を代入しようとしました";
    bool ok = (exit_code != 0) && (ng_output.find(expected) != std::string::npos);
    if (ok) {
        std::cout << "[integration] cross_type ng test passed (error detected)" << std::endl;
    } else {
        std::cerr << "[integration] cross_type ng test failed: exit_code=" << exit_code << ", output=" << ng_output << std::endl;
        assert(false);
    }
}
