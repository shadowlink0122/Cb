#pragma once

#include "../framework/integration_test_framework.hpp"

inline void test_interface_type_inference_chain() {
    std::cout << "[integration-test] Running interface type inference chain test..." << std::endl;

    run_cb_test_with_output_and_time_auto("../../tests/cases/interface/type_inference_chain.cb",
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "type_inference_chain.cb should exit with code 0");
            std::vector<std::string> lines = split_lines(output);
            INTEGRATION_ASSERT_EQ(14, lines.size(), "type_inference_chain.cb should emit 14 lines");
            INTEGRATION_ASSERT_EQ("33", lines[0], "First chained result should be 33");
            INTEGRATION_ASSERT_EQ("14", lines[1], "Second chained result should be 14");
            INTEGRATION_ASSERT_EQ("20", lines[2], "Third chained result should be 20");
            INTEGRATION_ASSERT_EQ("37", lines[3], "Fourth chained result should be 37");
            INTEGRATION_ASSERT_EQ("36", lines[4], "Fifth chained result should be 36");
            INTEGRATION_ASSERT_EQ("50", lines[5], "Sixth chained result should be 50");
            INTEGRATION_ASSERT_EQ("43", lines[6], "Seventh chained result should be 43");
            INTEGRATION_ASSERT_EQ("30", lines[7], "Eighth chained result should be 30");
            INTEGRATION_ASSERT_EQ("10", lines[8], "Ninth chained result should be 10");
            INTEGRATION_ASSERT_EQ("20", lines[9], "Tenth chained result should be 20");
            INTEGRATION_ASSERT_EQ("16", lines[10], "Eleventh chained result should be 16");
            INTEGRATION_ASSERT_EQ("28", lines[11], "Twelfth chained result should be 28");
            INTEGRATION_ASSERT_EQ("32", lines[12], "Thirteenth chained result should be 32");
            INTEGRATION_ASSERT_EQ("22", lines[13], "Fourteenth chained result should be 22");
        });

    integration_test_passed_with_time_auto("Interface type inference chain", "type_inference_chain.cb");

    std::cout << "[integration-test] Interface type inference chain test completed" << std::endl;
}
