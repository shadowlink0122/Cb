#include "../run_and_capture_util.h"
#include "../bool_expr/read_lines_util.h"
#include <string>
#include <vector>
#include <cassert>
#include <cstdio>
#include <iostream>

void test_integration_loop() {
    std::vector<std::string> files;
    files.push_back("loop/for_ok.cb");
    files.push_back("loop/while_ok.cb");
    files.push_back("loop/break_ok.cb");
    files.push_back("loop/break_expr_ok.cb");
    for (const auto &file : files) {
        std::string cb_path = "./tests/cases/" + file;
        std::string result_path = cb_path.substr(0, cb_path.size() - 3) + ".result";
        std::string out_path = cb_path.substr(0, cb_path.size() - 3) + ".out.tmp";
        std::string cmd = std::string("./main ") + cb_path + " >" + out_path + " 2>&1";
        int exit_code = 0;
        run_and_capture(cmd.c_str(), &exit_code);
        std::vector<std::string> output = read_lines(out_path.c_str());
        std::vector<std::string> expected = read_lines(result_path.c_str());
        remove(out_path.c_str());
        if (expected.empty() && !output.empty()) {
            FILE* fp = fopen(result_path.c_str(), "w");
            if (fp) {
                for (size_t i = 0; i < output.size(); ++i) {
                    fprintf(fp, "%s\n", output[i].c_str());
                }
                fclose(fp);
            }
            expected = read_lines(result_path.c_str());
        }
        if (output.size() != expected.size()) {
            std::cerr << "[loop] output size mismatch: " << output.size() << " vs " << expected.size() << " for " << file << std::endl;
            assert(false);
        }
        for (size_t i = 0; i < expected.size(); ++i) {
            if (output[i] != expected[i]) {
                std::cerr << "[loop] output mismatch at line " << i+1 << " for " << file << ": got '" << output[i] << "', expected '" << expected[i] << "'\n";
                assert(false);
            }
        }
    }
    std::cout << "[integration] loop test passed" << std::endl;
}
