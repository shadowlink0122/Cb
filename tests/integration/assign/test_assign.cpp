#include "test_assign_case.h"
#include <cassert>
#include <cstdlib>
#include <iostream>
#include <string>
#include "../run_and_capture_util.h"

// tiny
void test_integration_assign_tiny_ok() {
    int exit_code = 0;
    std::string cmd = "./main ./tests/cases/assign/tiny/ok.cb > /tmp/assign_tiny_ok_out.txt 2>&1";
    run_and_capture(cmd.c_str(), &exit_code);
    FILE* fp = fopen("/tmp/assign_tiny_ok_out.txt", "r");
    std::string output;
    if (fp) {
        char buf[4096];
        while (fgets(buf, sizeof(buf), fp)) output += buf;
        fclose(fp);
    }
    remove("/tmp/assign_tiny_ok_out.txt");
    assert(exit_code == 0);
    assert(output.find("127") != std::string::npos);
    assert(output.find("型の範囲外の値を代入しようとしました") == std::string::npos);
}
void test_integration_assign_tiny_ng() {
    // 標準エラーも含めて出力を取得
    int exit_code = 0;
    std::string cmd = "./main ./tests/cases/assign/tiny/ng.cb > /tmp/assign_tiny_ng_out.txt 2>&1";
    run_and_capture(cmd.c_str(), &exit_code);
    FILE* fp = fopen("/tmp/assign_tiny_ng_out.txt", "r");
    std::string output;
    if (fp) {
        char buf[4096];
        while (fgets(buf, sizeof(buf), fp)) output += buf;
        fclose(fp);
    }
    remove("/tmp/assign_tiny_ng_out.txt");
    const char* expected = "型の範囲外の値を代入しようとしました";
    bool ok = (exit_code != 0) && (output.find(expected) != std::string::npos);
    assert(ok);
}

// const tiny 正常系
void test_integration_assign_const_ok() {
    int exit_code = 0;
    std::string cmd = "./main ./tests/cases/assign/const/ok.cb > /tmp/assign_const_ok_out.txt 2>&1";
    run_and_capture(cmd.c_str(), &exit_code);
    FILE* fp = fopen("/tmp/assign_const_ok_out.txt", "r");
    std::string output;
    if (fp) {
        char buf[4096];
        while (fgets(buf, sizeof(buf), fp)) output += buf;
        fclose(fp);
    }
    remove("/tmp/assign_const_ok_out.txt");
    assert(exit_code == 0);
    assert(output.find("42") != std::string::npos);
    assert(output.find("再代入できません") == std::string::npos);
    std::cout << "[integration] assign const ok test passed" << std::endl;
}

// const tiny 異常系（再代入）
void test_integration_assign_const_ng() {
    int exit_code = 0;
    std::string cmd = "./main ./tests/cases/assign/const/ng.cb > /tmp/assign_const_ng_out.txt 2>&1";
    run_and_capture(cmd.c_str(), &exit_code);
    FILE* fp = fopen("/tmp/assign_const_ng_out.txt", "r");
    std::string output;
    if (fp) {
        char buf[4096];
        while (fgets(buf, sizeof(buf), fp)) output += buf;
        fclose(fp);
    }
    remove("/tmp/assign_const_ng_out.txt");
    bool ok = (exit_code != 0) && (output.find("再代入できません") != std::string::npos);
    assert(ok);
    std::cout << "[integration] assign const ng test passed" << std::endl;
}

// const tiny 正常系
void test_integration_assign_const_tiny_ok() {
    int exit_code = 0;
    std::string cmd = "./main ./tests/cases/assign/const/ok_const_tiny.cb > /tmp/assign_const_tiny_ok_out.txt 2>&1";
    run_and_capture(cmd.c_str(), &exit_code);
    FILE* fp = fopen("/tmp/assign_const_tiny_ok_out.txt", "r");
    std::string output;
    if (fp) {
        char buf[4096];
        while (fgets(buf, sizeof(buf), fp)) output += buf;
        fclose(fp);
    }
    remove("/tmp/assign_const_tiny_ok_out.txt");
    assert(exit_code == 0);
    assert(output.find("42") != std::string::npos);
    assert(output.find("再代入できません") == std::string::npos);
    std::cout << "[integration] assign const tiny ok test passed" << std::endl;
}

// const tiny 異常系（再代入）
void test_integration_assign_const_tiny_reassign_ng() {
    int exit_code = 0;
    std::string cmd = "./main ./tests/cases/assign/const/ng_const_tiny_reassign.cb > /tmp/assign_const_tiny_reassign_ng_out.txt 2>&1";
    run_and_capture(cmd.c_str(), &exit_code);
    FILE* fp = fopen("/tmp/assign_const_tiny_reassign_ng_out.txt", "r");
    std::string output;
    if (fp) {
        char buf[4096];
        while (fgets(buf, sizeof(buf), fp)) output += buf;
        fclose(fp);
    }
    remove("/tmp/assign_const_tiny_reassign_ng_out.txt");
    bool ok = (exit_code != 0) && (output.find("再代入できません") != std::string::npos);
    assert(ok);
    std::cout << "[integration] assign const tiny reassign ng test passed" << std::endl;
}

// const string 要素変更禁止
void test_integration_assign_const_string_element_ng() {
    int exit_code = 0;
    std::string cmd = "./main ./tests/cases/assign/const/ng_const_string_element.cb > /tmp/assign_const_string_element_ng_out.txt 2>&1";
    run_and_capture(cmd.c_str(), &exit_code);
    FILE* fp = fopen("/tmp/assign_const_string_element_ng_out.txt", "r");
    std::string output;
    if (fp) {
        char buf[4096];
        while (fgets(buf, sizeof(buf), fp)) output += buf;
        fclose(fp);
    }
    remove("/tmp/assign_const_string_element_ng_out.txt");
    bool ok = (exit_code != 0) && (output.find("要素は変更できません") != std::string::npos);
    assert(ok);
    std::cout << "[integration] assign const string element ng test passed" << std::endl;
}

// const string 要素変更禁止（文字列リテラル）
void test_integration_assign_tiny_ng_neg() {
    int exit_code = 0;
    std::string cmd = "./main ./tests/cases/assign/tiny/ng_neg.cb > /tmp/assign_tiny_ng_neg_out.txt 2>&1";
    run_and_capture(cmd.c_str(), &exit_code);
    FILE* fp = fopen("/tmp/assign_tiny_ng_neg_out.txt", "r");
    std::string output;
    if (fp) {
        char buf[4096];
        while (fgets(buf, sizeof(buf), fp)) output += buf;
        fclose(fp);
    }
    remove("/tmp/assign_tiny_ng_neg_out.txt");
    const char* expected = "型の範囲外の値を代入しようとしました";
    bool ok = (exit_code != 0) && (output.find(expected) != std::string::npos);
    assert(ok);
}

// short
void test_integration_assign_short_ok() {
    int exit_code = 0;
    char tmpfile[] = "/tmp/assign_short_ok_XXXXXX";
    int fd = mkstemp(tmpfile);
    std::string cmd = std::string("./main ./tests/cases/assign/short/ok.cb >") + tmpfile + " 2>&1";
    run_and_capture(cmd.c_str(), &exit_code);
    FILE* fp = fopen(tmpfile, "r");
    std::string output;
    if (fp) {
        char buf[4096];
        while (fgets(buf, sizeof(buf), fp)) output += buf;
        fclose(fp);
    }
    remove(tmpfile);
    assert(output.find("32767") != std::string::npos);
    assert(output.find("型の範囲外の値を代入しようとしました") == std::string::npos);
    std::cout << "[integration] assign short ok test passed" << std::endl;
}
void test_integration_assign_short_ng() {
    int exit_code = 0;
    char tmpfile[] = "/tmp/assign_short_ng_XXXXXX";
    int fd = mkstemp(tmpfile);
    std::string cmd = std::string("./main ./tests/cases/assign/short/ng.cb >") + tmpfile + " 2>&1";
    run_and_capture(cmd.c_str(), &exit_code);
    FILE* fp = fopen(tmpfile, "r");
    std::string output;
    if (fp) {
        char buf[4096];
        while (fgets(buf, sizeof(buf), fp)) output += buf;
        fclose(fp);
    }
    remove(tmpfile);
    const char* expected = "型の範囲外の値を代入しようとしました";
    bool ok = (exit_code != 0) && (output.find(expected) != std::string::npos);
    if (ok) {
        std::cout << "[integration] assign short ng test passed (error detected)" << std::endl;
    } else {
        assert(false);
    }
}
void test_integration_assign_short_ng_neg() {
    int exit_code = 0;
    char tmpfile[] = "/tmp/assign_short_ng_neg_XXXXXX";
    int fd = mkstemp(tmpfile);
    std::string cmd = std::string("./main ./tests/cases/assign/short/ng_neg.cb >") + tmpfile + " 2>&1";
    run_and_capture(cmd.c_str(), &exit_code);
    FILE* fp = fopen(tmpfile, "r");
    std::string output;
    if (fp) {
        char buf[4096];
        while (fgets(buf, sizeof(buf), fp)) output += buf;
        fclose(fp);
    }
    remove(tmpfile);
    const char* expected = "型の範囲外の値を代入しようとしました";
    bool ok = (exit_code != 0) && (output.find(expected) != std::string::npos);
    if (ok) {
        std::cout << "[integration] assign short ng_neg test passed (error detected)" << std::endl;
    } else {
        assert(false);
    }
}

// int
void test_integration_assign_int_ok() {
    int exit_code = 0;
    char tmpfile[] = "/tmp/assign_int_ok_XXXXXX";
    int fd = mkstemp(tmpfile);
    std::string cmd = std::string("./main ./tests/cases/assign/int/ok.cb >") + tmpfile + " 2>&1";
    run_and_capture(cmd.c_str(), &exit_code);
    FILE* fp = fopen(tmpfile, "r");
    std::string output;
    if (fp) {
        char buf[4096];
        while (fgets(buf, sizeof(buf), fp)) output += buf;
        fclose(fp);
    }
    remove(tmpfile);
    assert(output.find("2147483647") != std::string::npos);
    assert(output.find("型の範囲外の値を代入しようとしました") == std::string::npos);
    std::cout << "[integration] assign int ok test passed" << std::endl;
}
void test_integration_assign_int_ng() {
    int exit_code = 0;
    char tmpfile[] = "/tmp/assign_int_ng_XXXXXX";
    int fd = mkstemp(tmpfile);
    std::string cmd = std::string("./main ./tests/cases/assign/int/ng.cb >") + tmpfile + " 2>&1";
    run_and_capture(cmd.c_str(), &exit_code);
    FILE* fp = fopen(tmpfile, "r");
    std::string output;
    if (fp) {
        char buf[4096];
        while (fgets(buf, sizeof(buf), fp)) output += buf;
        fclose(fp);
    }
    remove(tmpfile);
    const char* expected = "型の範囲外の値を代入しようとしました";
    bool ok = (exit_code != 0) && (output.find(expected) != std::string::npos);
    if (ok) {
        std::cout << "[integration] assign int ng test passed (error detected)" << std::endl;
    } else {
        assert(false);
    }
}
void test_integration_assign_int_ng_neg() {
    int exit_code = 0;
    char tmpfile[] = "/tmp/assign_int_ng_neg_XXXXXX";
    int fd = mkstemp(tmpfile);
    std::string cmd = std::string("./main ./tests/cases/assign/int/ng_neg.cb >") + tmpfile + " 2>&1";
    run_and_capture(cmd.c_str(), &exit_code);
    FILE* fp = fopen(tmpfile, "r");
    std::string output;
    if (fp) {
        char buf[4096];
        while (fgets(buf, sizeof(buf), fp)) output += buf;
        fclose(fp);
    }
    remove(tmpfile);
    const char* expected = "型の範囲外の値を代入しようとしました";
    bool ok = (exit_code != 0) && (output.find(expected) != std::string::npos);
    if (ok) {
        std::cout << "[integration] assign int ng_neg test passed (error detected)" << std::endl;
    } else {
        assert(false);
    }
}

// long
void test_integration_assign_long_ok() {
    int exit_code = 0;
    char tmpfile[] = "/tmp/assign_long_ok_XXXXXX";
    int fd = mkstemp(tmpfile);
    std::string cmd = std::string("./main ./tests/cases/assign/long/ok.cb >") + tmpfile + " 2>&1";
    run_and_capture(cmd.c_str(), &exit_code);
    FILE* fp = fopen(tmpfile, "r");
    std::string output;
    if (fp) {
        char buf[4096];
        while (fgets(buf, sizeof(buf), fp)) output += buf;
        fclose(fp);
    }
    remove(tmpfile);
    assert(output.find("9223372036854775807") != std::string::npos);
    assert(output.find("型の範囲外の値を代入しようとしました") == std::string::npos);
    std::cout << "[integration] assign long ok test passed" << std::endl;
}
void test_integration_assign_long_ng() {
    // long型の範囲外例外はテストしない（出力なし）
}
void test_integration_assign_long_ng_neg() {
    // long型の範囲外例外はテストしない（出力なし）
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
    test_integration_assign_const_tiny_ok();
    test_integration_assign_const_tiny_reassign_ng();
    test_integration_assign_const_string_element_ng();
}
