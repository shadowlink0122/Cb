#pragma once

#include "../framework/integration_test_framework.hpp"
#include <vector>
#include <sstream>

inline void test_integration_performance() {
    std::cout << "[integration-test] Running performance tests..." << std::endl;
    
    // パーサー効率性テスト
    const std::string test_file_parser = "../../tests/cases/performance/parser_efficiency_test.cb";
    run_cb_test_with_output_and_time_auto(test_file_parser, 
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "Expected successful exit code for parser efficiency test");
            INTEGRATION_ASSERT_CONTAINS(output, "Complex arithmetic:", "Expected complex arithmetic result");
            INTEGRATION_ASSERT_CONTAINS(output, "Nested ternary:", "Expected nested ternary result");
            INTEGRATION_ASSERT_CONTAINS(output, "Complex logical:", "Expected complex logical result");
            INTEGRATION_ASSERT_CONTAINS(output, "Function array combo:", "Expected function array combo result");
            INTEGRATION_ASSERT_CONTAINS(output, "Mixed type ternary:", "Expected mixed type ternary result");
            INTEGRATION_ASSERT_CONTAINS(output, "Struct array result:", "Expected struct array result");
            INTEGRATION_ASSERT_CONTAINS(output, "Deep expression:", "Expected deep expression result");
            INTEGRATION_ASSERT_CONTAINS(output, "Array access chain:", "Expected array access chain result");
        });
    integration_test_passed_with_time_auto("parser efficiency test", test_file_parser);
    
    // 大規模パーサーテスト
    const std::string test_file_large = "../../tests/cases/performance/large_scale_parser_test.cb";
    run_cb_test_with_output_and_time_auto(test_file_large, 
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "Expected successful exit code for large scale parser test");
            INTEGRATION_ASSERT_CONTAINS(output, "=== Large Scale Parser Efficiency Test ===", "Expected test header");
            INTEGRATION_ASSERT_CONTAINS(output, "Huge calculation result:", "Expected huge calculation result");
            INTEGRATION_ASSERT_CONTAINS(output, "Array sum:", "Expected array sum result");
            INTEGRATION_ASSERT_CONTAINS(output, "Nested function result:", "Expected nested function result");
            INTEGRATION_ASSERT_CONTAINS(output, "Complex ternary:", "Expected complex ternary result");
            INTEGRATION_ASSERT_CONTAINS(output, "Dynamic indexing:", "Expected dynamic indexing result");
            INTEGRATION_ASSERT_CONTAINS(output, "Compound chain result:", "Expected compound chain result");
            INTEGRATION_ASSERT_CONTAINS(output, "Short circuit result:", "Expected short circuit result");
            INTEGRATION_ASSERT_CONTAINS(output, "Ultimate expression:", "Expected ultimate expression result");
            INTEGRATION_ASSERT_CONTAINS(output, "=== Test completed successfully ===", "Expected test completion");
        });
    integration_test_passed_with_time_auto("large scale parser test", test_file_large);
    
    // 統合パフォーマンステスト
    const std::string test_file_integrated = "../../tests/cases/performance/integrated_performance_test.cb";
    run_cb_test_with_output_and_time_auto(test_file_integrated, 
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "Expected successful exit code for integrated performance test");
            INTEGRATION_ASSERT_CONTAINS(output, "=== v0.8.1 Integrated Performance Test ===", "Expected test header");
            INTEGRATION_ASSERT_CONTAINS(output, "Initialized coordinates:", "Expected coordinate initialization");
            INTEGRATION_ASSERT_CONTAINS(output, "Valid point", "Expected valid point messages");
            INTEGRATION_ASSERT_CONTAINS(output, "Point A: distance=", "Expected distance calculation for point A");
            INTEGRATION_ASSERT_CONTAINS(output, "Point B: distance=", "Expected distance calculation for point B");
            INTEGRATION_ASSERT_CONTAINS(output, "Point C: distance=", "Expected distance calculation for point C");
            INTEGRATION_ASSERT_CONTAINS(output, "Average distance:", "Expected average distance");
            INTEGRATION_ASSERT_CONTAINS(output, "Complex nested access:", "Expected complex nested access result");
            INTEGRATION_ASSERT_CONTAINS(output, "Optimization result:", "Expected optimization result");
            INTEGRATION_ASSERT_CONTAINS(output, "Performance score:", "Expected performance score");
            INTEGRATION_ASSERT_CONTAINS(output, "Memory efficiency:", "Expected memory efficiency result");
            INTEGRATION_ASSERT_CONTAINS(output, "Recursive test: fib(6)=8", "Expected fibonacci result");
            INTEGRATION_ASSERT_CONTAINS(output, "=== FINAL RESULT: v0.8.1 PERFORMANCE IMPROVEMENTS VERIFIED ===", "Expected successful final result");
            INTEGRATION_ASSERT_CONTAINS(output, "=== Performance Statistics ===", "Expected statistics header");
            INTEGRATION_ASSERT_CONTAINS(output, "- Points processed: 3", "Expected 3 points processed");
            INTEGRATION_ASSERT_CONTAINS(output, "- Valid points: 3", "Expected 3 valid points");
            INTEGRATION_ASSERT_CONTAINS(output, "- Memory efficiency: OPTIMAL", "Expected optimal memory efficiency");
            INTEGRATION_ASSERT_CONTAINS(output, "- Type inference: WORKING", "Expected working type inference");
            INTEGRATION_ASSERT_CONTAINS(output, "- Chain processing: OPTIMIZED", "Expected optimized chain processing");
        });
    integration_test_passed_with_time_auto("integrated performance test", test_file_integrated);
    
    // 最終パーサー効率性テスト
    const std::string test_file_final = "../../tests/cases/performance/final_parser_efficiency.cb";
    run_cb_test_with_output_and_time_auto(test_file_final, 
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "Expected successful exit code for final parser efficiency test");
            INTEGRATION_ASSERT_CONTAINS(output, "=== FINAL PARSER EFFICIENCY VERIFICATION ===", "Expected test header");
            INTEGRATION_ASSERT_CONTAINS(output, "Complex expression:", "Expected complex expression result");
            INTEGRATION_ASSERT_CONTAINS(output, "Array chain result:", "Expected array chain result");
            INTEGRATION_ASSERT_CONTAINS(output, "Nested functions:", "Expected nested functions result");
            INTEGRATION_ASSERT_CONTAINS(output, "Ternary result:", "Expected ternary result");
            INTEGRATION_ASSERT_CONTAINS(output, "Logic result:", "Expected logic result");
            INTEGRATION_ASSERT_CONTAINS(output, "FINAL INTEGRATED RESULT:", "Expected final integrated result");
            INTEGRATION_ASSERT_CONTAINS(output, "=== PERFORMANCE STATS ===", "Expected performance stats");
            INTEGRATION_ASSERT_CONTAINS(output, "Variables: 20", "Expected variable count");
            INTEGRATION_ASSERT_CONTAINS(output, "=== EFFICIENCY VERIFIED ===", "Expected efficiency verification");
        });
    integration_test_passed_with_time_auto("final parser efficiency test", test_file_final);
    
    // 関数配列推論デモテスト
    const std::string test_file_func_array = "../../tests/cases/performance/func_array_inference_demo.cb";
    run_cb_test_with_output_and_time_auto(test_file_func_array, 
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "Expected successful exit code for func array inference demo");
            INTEGRATION_ASSERT_CONTAINS(output, "=== func()[index] Type Inference Verification ===", "Expected test header");
            INTEGRATION_ASSERT_CONTAINS(output, "get_numbers()[0] = 10", "Expected first array element");
            INTEGRATION_ASSERT_CONTAINS(output, "get_numbers()[2] = 30", "Expected third array element");
            INTEGRATION_ASSERT_CONTAINS(output, "get_numbers()[1] + get_numbers()[3] = 60", "Expected addition result");
            INTEGRATION_ASSERT_CONTAINS(output, "get_numbers()[get_single(1)] = 30", "Expected nested function call");
            INTEGRATION_ASSERT_CONTAINS(output, "Complex expression:", "Expected complex expression result");
            INTEGRATION_ASSERT_CONTAINS(output, "Ternary with func()[]: 30", "Expected ternary result");
            INTEGRATION_ASSERT_CONTAINS(output, "=== All func()[index] patterns working! ===", "Expected success message");
        });
    integration_test_passed_with_time_auto("func array inference demo test", test_file_func_array);
    
    // パーサー純粋効率性テスト
    const std::string test_file_pure = "../../tests/cases/performance/parser_pure_efficiency_test.cb";
    run_cb_test_with_output_and_time_auto(test_file_pure, 
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "Expected successful exit code for parser pure efficiency test");
            INTEGRATION_ASSERT_CONTAINS(output, "Complex arithmetic:", "Expected complex arithmetic result");
            INTEGRATION_ASSERT_CONTAINS(output, "Deep nesting:", "Expected deep nesting result");
            INTEGRATION_ASSERT_CONTAINS(output, "Multi-ternary:", "Expected multi-ternary result");
            INTEGRATION_ASSERT_CONTAINS(output, "Complex logical:", "Expected complex logical result");
            INTEGRATION_ASSERT_CONTAINS(output, "Array chains:", "Expected array chains result");
            INTEGRATION_ASSERT_CONTAINS(output, "Compound assignments:", "Expected compound assignments result");
            INTEGRATION_ASSERT_CONTAINS(output, "Nested functions:", "Expected nested functions result");
            INTEGRATION_ASSERT_CONTAINS(output, "Mixed expression:", "Expected mixed expression result");
            INTEGRATION_ASSERT_CONTAINS(output, "Bit operations:", "Expected bit operations result");
            INTEGRATION_ASSERT_CONTAINS(output, "Dynamic indexing:", "Expected dynamic indexing result");
            INTEGRATION_ASSERT_CONTAINS(output, "=== Parser efficiency test completed ===", "Expected test completion");
        });
    integration_test_passed_with_time_auto("parser pure efficiency test", test_file_pure);
    
    std::cout << "[integration-test] Performance tests completed" << std::endl;
}
