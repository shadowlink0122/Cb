#include "test_boundary_case.h"
#include <cassert>
#include <cstdlib>
#include <iostream>
#include <string>
#include "../run_and_capture_util.h"

// tiny
void test_integration_boundary_tiny_ok() {
    int exit_code = 0;
    char tmpfile[] = "/tmp/boundary_tiny_ok_XXXXXX";
    int fd = mkstemp(tmpfile);
    std::string cmd = std::string("./main ./tests/cases/boundary/tiny/ok.cb >") + tmpfile + " 2>&1";
    run_and_capture(cmd.c_str(), &exit_code);
    FILE* fp = fopen(tmpfile, "r");
    std::string output;
    if (fp) { char buf[4096]; while (fgets(buf, sizeof(buf), fp)) output += buf; fclose(fp); }
    remove(tmpfile);
    assert(output.find("127") != std::string::npos);
    assert(output.find("-128") != std::string::npos);
    std::cout << "[integration] boundary tiny ok test passed" << std::endl;
}
void test_integration_boundary_tiny_ng() {
    int exit_code = 0;
    char tmpfile[] = "/tmp/boundary_tiny_ng_XXXXXX";
    int fd = mkstemp(tmpfile);
    std::string cmd = std::string("./main ./tests/cases/boundary/tiny/ng.cb >") + tmpfile + " 2>&1";
    run_and_capture(cmd.c_str(), &exit_code);
    FILE* fp = fopen(tmpfile, "r");
    std::string output;
    if (fp) { char buf[4096]; while (fgets(buf, sizeof(buf), fp)) output += buf; fclose(fp); }
    remove(tmpfile);
    const char* expected = "型の範囲外の値を代入しようとしました";
    bool ok = (exit_code != 0) && (output.find(expected) != std::string::npos);
    if (ok) {
        std::cout << "[integration] boundary tiny ng test passed (error detected)" << std::endl;
    } else {
        assert(false);
    }
}
void test_integration_boundary_tiny_ng_neg() {
    int exit_code = 0;
    char tmpfile[] = "/tmp/boundary_tiny_ng_neg_XXXXXX";
    int fd = mkstemp(tmpfile);
    std::string cmd = std::string("./main ./tests/cases/boundary/tiny/ng_neg.cb >") + tmpfile + " 2>&1";
    run_and_capture(cmd.c_str(), &exit_code);
    FILE* fp = fopen(tmpfile, "r");
    std::string output;
    if (fp) { char buf[4096]; while (fgets(buf, sizeof(buf), fp)) output += buf; fclose(fp); }
    remove(tmpfile);
    const char* expected = "型の範囲外の値を代入しようとしました";
    bool ok = (exit_code != 0) && (output.find(expected) != std::string::npos);
    if (ok) {
        std::cout << "[integration] boundary tiny ng_neg test passed (error detected)" << std::endl;
    } else {
        assert(false);
    }
}
// short
void test_integration_boundary_short_ok() {
    int exit_code = 0;
    char tmpfile[] = "/tmp/boundary_short_ok_XXXXXX";
    int fd = mkstemp(tmpfile);
    std::string cmd = std::string("./main ./tests/cases/boundary/short/ok.cb >") + tmpfile + " 2>&1";
    run_and_capture(cmd.c_str(), &exit_code);
    FILE* fp = fopen(tmpfile, "r");
    std::string output;
    if (fp) { char buf[4096]; while (fgets(buf, sizeof(buf), fp)) output += buf; fclose(fp); }
    remove(tmpfile);
    assert(output.find("32767") != std::string::npos);
    assert(output.find("-32768") != std::string::npos);
    std::cout << "[integration] boundary short ok test passed" << std::endl;
}
void test_integration_boundary_short_ng() {
    int exit_code = 0;
    char tmpfile[] = "/tmp/boundary_short_ng_XXXXXX";
    int fd = mkstemp(tmpfile);
    std::string cmd = std::string("./main ./tests/cases/boundary/short/ng.cb >") + tmpfile + " 2>&1";
    run_and_capture(cmd.c_str(), &exit_code);
    FILE* fp = fopen(tmpfile, "r");
    std::string output;
    if (fp) { char buf[4096]; while (fgets(buf, sizeof(buf), fp)) output += buf; fclose(fp); }
    remove(tmpfile);
    const char* expected = "型の範囲外の値を代入しようとしました";
    bool ok = (exit_code != 0) && (output.find(expected) != std::string::npos);
    if (ok) {
        std::cout << "[integration] boundary short ng test passed (error detected)" << std::endl;
    } else {
        assert(false);
    }
}
void test_integration_boundary_short_ng_neg() {
    int exit_code = 0;
    char tmpfile[] = "/tmp/boundary_short_ng_neg_XXXXXX";
    int fd = mkstemp(tmpfile);
    std::string cmd = std::string("./main ./tests/cases/boundary/short/ng_neg.cb >") + tmpfile + " 2>&1";
    run_and_capture(cmd.c_str(), &exit_code);
    FILE* fp = fopen(tmpfile, "r");
    std::string output;
    if (fp) { char buf[4096]; while (fgets(buf, sizeof(buf), fp)) output += buf; fclose(fp); }
    remove(tmpfile);
    const char* expected = "型の範囲外の値を代入しようとしました";
    bool ok = (exit_code != 0) && (output.find(expected) != std::string::npos);
    if (ok) {
        std::cout << "[integration] boundary short ng_neg test passed (error detected)" << std::endl;
    } else {
        assert(false);
    }
}

// int
void test_integration_boundary_int_ok() {
    int exit_code = 0;
    char tmpfile[] = "/tmp/boundary_int_ok_XXXXXX";
    int fd = mkstemp(tmpfile);
    std::string cmd = std::string("./main ./tests/cases/boundary/int/ok.cb >") + tmpfile + " 2>&1";
    run_and_capture(cmd.c_str(), &exit_code);
    FILE* fp = fopen(tmpfile, "r");
    std::string output;
    if (fp) { char buf[4096]; while (fgets(buf, sizeof(buf), fp)) output += buf; fclose(fp); }
    remove(tmpfile);
    assert(output.find("2147483647") != std::string::npos);
    assert(output.find("-2147483648") != std::string::npos);
    std::cout << "[integration] boundary int ok test passed" << std::endl;
}
void test_integration_boundary_int_ng() {
    int exit_code = 0;
    char tmpfile[] = "/tmp/boundary_int_ng_XXXXXX";
    int fd = mkstemp(tmpfile);
    std::string cmd = std::string("./main ./tests/cases/boundary/int/ng.cb >") + tmpfile + " 2>&1";
    run_and_capture(cmd.c_str(), &exit_code);
    FILE* fp = fopen(tmpfile, "r");
    std::string output;
    if (fp) { char buf[4096]; while (fgets(buf, sizeof(buf), fp)) output += buf; fclose(fp); }
    remove(tmpfile);
    const char* expected = "型の範囲外の値を代入しようとしました";
    bool ok = (exit_code != 0) && (output.find(expected) != std::string::npos);
    if (ok) {
        std::cout << "[integration] boundary int ng test passed (error detected)" << std::endl;
    } else {
        assert(false);
    }
}
void test_integration_boundary_int_ng_neg() {
    int exit_code = 0;
    char tmpfile[] = "/tmp/boundary_int_ng_neg_XXXXXX";
    int fd = mkstemp(tmpfile);
    std::string cmd = std::string("./main ./tests/cases/boundary/int/ng_neg.cb >") + tmpfile + " 2>&1";
    run_and_capture(cmd.c_str(), &exit_code);
    FILE* fp = fopen(tmpfile, "r");
    std::string output;
    if (fp) { char buf[4096]; while (fgets(buf, sizeof(buf), fp)) output += buf; fclose(fp); }
    remove(tmpfile);
    const char* expected = "型の範囲外の値を代入しようとしました";
    bool ok = (exit_code != 0) && (output.find(expected) != std::string::npos);
    if (ok) {
        std::cout << "[integration] boundary int ng_neg test passed (error detected)" << std::endl;
    } else {
        assert(false);
    }
}

// long
void test_integration_boundary_long_ok() {
    int exit_code = 0;
    char tmpfile[] = "/tmp/boundary_long_ok_XXXXXX";
    int fd = mkstemp(tmpfile);
    std::string cmd = std::string("./main ./tests/cases/boundary/long/ok.cb >") + tmpfile + " 2>&1";
    run_and_capture(cmd.c_str(), &exit_code);
    FILE* fp = fopen(tmpfile, "r");
    std::string output;
    if (fp) { char buf[4096]; while (fgets(buf, sizeof(buf), fp)) output += buf; fclose(fp); }
    remove(tmpfile);
    assert(output.find("9223372036854775807") != std::string::npos);
    assert(output.find("-9223372036854775808") != std::string::npos);
    std::cout << "[integration] boundary long ok test passed" << std::endl;
}
void test_integration_boundary_long_ng() {
    int exit_code = 0;
    char tmpfile[] = "/tmp/boundary_long_ng_XXXXXX";
    int fd = mkstemp(tmpfile);
    std::string cmd = std::string("./main ./tests/cases/boundary/long/ng.cb >") + tmpfile + " 2>&1";
    run_and_capture(cmd.c_str(), &exit_code);
    FILE* fp = fopen(tmpfile, "r");
    std::string output;
    if (fp) { char buf[4096]; while (fgets(buf, sizeof(buf), fp)) output += buf; fclose(fp); }
    remove(tmpfile);
    assert(output.find("-9223372036854775808") != std::string::npos);
    std::cout << "[integration] boundary long ng test passed (overflow to negative)" << std::endl;
}
void test_integration_boundary_long_ng_neg() {
    int exit_code = 0;
    char tmpfile[] = "/tmp/boundary_long_ng_neg_XXXXXX";
    int fd = mkstemp(tmpfile);
    std::string cmd = std::string("./main ./tests/cases/boundary/long/ng_neg.cb >") + tmpfile + " 2>&1";
    run_and_capture(cmd.c_str(), &exit_code);
    FILE* fp = fopen(tmpfile, "r");
    std::string output;
    if (fp) { char buf[4096]; while (fgets(buf, sizeof(buf), fp)) output += buf; fclose(fp); }
    remove(tmpfile);
    const char* expected = "型の範囲外の値を代入しようとしました";
    bool ok = (exit_code != 0) && (output.find(expected) != std::string::npos);
    if (ok) {
        std::cout << "[integration] boundary long ng_neg test passed (error detected)" << std::endl;
    } else {
        assert(false);
    }
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
    // test_integration_boundary_long_ng_neg(); // long型の範囲外（未定義動作）はテスト対象外とする
}
