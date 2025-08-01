
#include <cassert>
#include <string>
#include <vector>
#include <iostream>
#include "../run_and_capture_util.h"
#include "read_lines_util.h"

void test_bool_expr_basic() {
    // 実行結果を仮ファイルにリダイレクト
    int exit_code = 0;
    const char* out_file = "tests/cases/bool_expr/basic.out.tmp";
    std::string cmd = std::string("./main tests/cases/bool_expr/basic.cb > ") + out_file + " 2>&1";
    run_and_capture(cmd.c_str(), &exit_code);
    assert(exit_code == 0);

    // 期待値ファイル
    const char* expected_file = "tests/cases/bool_expr/basic.result";

    std::vector<std::string> output = read_lines(out_file);
    std::vector<std::string> expected = read_lines(expected_file);

    // 期待値ファイルが空なら自動生成（初回のみ）
    if (expected.empty() && !output.empty()) {
        FILE* fp = fopen(expected_file, "w");
        if (fp) {
            for (size_t i = 0; i < output.size(); ++i) {
                fprintf(fp, "%s\n", output[i].c_str());
            }
            fclose(fp);
        }
        // 再度読み直す
        expected = read_lines(expected_file);
    }

    if (output.size() != expected.size()) {
        std::cerr << "[bool_expr] output size mismatch: " << output.size() << " vs " << expected.size() << std::endl;
        assert(false);
    }
    for (size_t i = 0; i < expected.size(); ++i) {
        if (output[i] != expected[i]) {
            std::cerr << "[bool_expr] failed at line " << i+1 << ": got '" << output[i] << "', expected '" << expected[i] << "'\n";
            assert(false);
        }
    }
    // テンポラリファイル削除
    remove(out_file);
}
