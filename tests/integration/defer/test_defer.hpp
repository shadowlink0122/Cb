#pragma once
#include "../framework/integration_test_framework.hpp"

void test_integration_defer() {
    std::cout << "[integration-test] Running defer tests..." << std::endl;
    
    double execution_time;
    
    // Test 1: Basic defer functionality
    run_cb_test_with_output_and_time("../cases/defer/test_defer_basic.cb", 
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "test_defer_basic.cb should execute successfully");
            INTEGRATION_ASSERT_CONTAINS(output, "Start", "Should print 'Start'");
            INTEGRATION_ASSERT_CONTAINS(output, "Middle", "Should print 'Middle'");
            INTEGRATION_ASSERT_CONTAINS(output, "1", "Should print deferred '1'");
            INTEGRATION_ASSERT_CONTAINS(output, "2", "Should print deferred '2'");
            
            // Check LIFO order: "1" should appear before "2"
            size_t pos_1 = output.find("1");
            size_t pos_2 = output.find("2");
            INTEGRATION_ASSERT(pos_1 < pos_2, "defer should execute in LIFO order");
        }, execution_time);
    integration_test_passed_with_time("Basic defer with LIFO order", "test_defer_basic.cb", execution_time);
    
    // Test 2: Simple defer with println
    run_cb_test_with_output_and_time("../cases/defer/test_defer_println.cb", 
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "test_defer_println.cb should execute successfully");
            INTEGRATION_ASSERT_CONTAINS(output, "1", "Should print '1'");
            INTEGRATION_ASSERT_CONTAINS(output, "2", "Should print '2'");
            INTEGRATION_ASSERT_CONTAINS(output, "3", "Should print deferred '3'");
        }, execution_time);
    integration_test_passed_with_time("Simple defer with println", "test_defer_println.cb", execution_time);
    
    // Test 3: Two defer statements (LIFO order)
    run_cb_test_with_output_and_time("../cases/defer/test_defer_two.cb", 
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "test_defer_two.cb should execute successfully");
            
            auto lines = split_lines(output);
            INTEGRATION_ASSERT_EQ(2, (int)lines.size(), "Should have exactly 2 output lines");
            INTEGRATION_ASSERT_EQ("1", lines[0], "First defer should print '1' (LIFO)");
            INTEGRATION_ASSERT_EQ("2", lines[1], "Second defer should print '2'");
        }, execution_time);
    integration_test_passed_with_time("Two defer statements in LIFO order", "test_defer_two.cb", execution_time);
    
    // Test 4: Mixed defer and regular statements
    run_cb_test_with_output_and_time("../cases/defer/test_defer_mixed.cb", 
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "test_defer_mixed.cb should execute successfully");
            INTEGRATION_ASSERT_CONTAINS(output, "Start", "Should print 'Start'");
            INTEGRATION_ASSERT_CONTAINS(output, "1", "Should print deferred '1'");
            INTEGRATION_ASSERT_CONTAINS(output, "2", "Should print deferred '2'");
            
            // Check order: "Start" should appear before deferred outputs
            size_t pos_start = output.find("Start");
            size_t pos_1 = output.find("1");
            INTEGRATION_ASSERT(pos_start < pos_1, "Regular statements should execute before defer");
        }, execution_time);
    integration_test_passed_with_time("Mixed defer and regular statements", "test_defer_mixed.cb", execution_time);
    
    // Test 5: Defer after regular statements
    run_cb_test_with_output_and_time("../cases/defer/test_defer_after.cb", 
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "test_defer_after.cb should execute successfully");
            INTEGRATION_ASSERT_CONTAINS(output, "Start", "Should print 'Start'");
            INTEGRATION_ASSERT_CONTAINS(output, "Middle", "Should print 'Middle'");
            INTEGRATION_ASSERT_CONTAINS(output, "1", "Should print deferred '1'");
            INTEGRATION_ASSERT_CONTAINS(output, "2", "Should print deferred '2'");
            
            // Check execution order
            size_t pos_start = output.find("Start");
            size_t pos_middle = output.find("Middle");
            size_t pos_1 = output.find("1");
            size_t pos_2 = output.find("2");
            
            INTEGRATION_ASSERT(pos_start < pos_middle, "'Start' before 'Middle'");
            INTEGRATION_ASSERT(pos_middle < pos_1, "'Middle' before deferred statements");
            INTEGRATION_ASSERT(pos_1 < pos_2, "defer in LIFO order");
        }, execution_time);
    integration_test_passed_with_time("Defer after regular statements", "test_defer_after.cb", execution_time);
    
    // Test 6: Nested scope with defer
    run_cb_test_with_output_and_time("../cases/defer/test_defer_scope.cb", 
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "test_defer_scope.cb should execute successfully");
            
            // Check all expected outputs
            INTEGRATION_ASSERT_CONTAINS(output, "main: start", "Should print 'main: start'");
            INTEGRATION_ASSERT_CONTAINS(output, "block1: start", "Should print 'block1: start'");
            INTEGRATION_ASSERT_CONTAINS(output, "block1: end", "Should print 'block1: end'");
            INTEGRATION_ASSERT_CONTAINS(output, "main: middle", "Should print 'main: middle'");
            INTEGRATION_ASSERT_CONTAINS(output, "block2: start", "Should print 'block2: start'");
            INTEGRATION_ASSERT_CONTAINS(output, "block2: end", "Should print 'block2: end'");
            INTEGRATION_ASSERT_CONTAINS(output, "main: end", "Should print 'main: end'");
            INTEGRATION_ASSERT_CONTAINS(output, "block1: defer", "Should print 'block1: defer'");
            INTEGRATION_ASSERT_CONTAINS(output, "block2: defer", "Should print 'block2: defer'");
            INTEGRATION_ASSERT_CONTAINS(output, "main: defer", "Should print 'main: defer'");
            
            // Check scope-based defer execution order
            size_t pos_block1_end = output.find("block1: end");
            size_t pos_block1_defer = output.find("block1: defer");
            size_t pos_block2_end = output.find("block2: end");
            size_t pos_block2_defer = output.find("block2: defer");
            size_t pos_main_end = output.find("main: end");
            size_t pos_main_defer = output.find("main: defer");
            
            INTEGRATION_ASSERT(pos_block1_end < pos_block1_defer, 
                              "block1 defer should execute after block1 ends");
            INTEGRATION_ASSERT(pos_block2_end < pos_block2_defer, 
                              "block2 defer should execute after block2 ends");
            INTEGRATION_ASSERT(pos_main_end < pos_block2_defer, 
                              "nested block defer should execute before outer scope ends");
            INTEGRATION_ASSERT(pos_block2_defer < pos_main_defer, 
                              "block2 defer should execute before main defer");
            INTEGRATION_ASSERT(pos_block1_defer < pos_main_defer, 
                              "block1 defer should execute before main defer");
        }, execution_time);
    integration_test_passed_with_time("Nested scope with defer", "test_defer_scope.cb", execution_time);
    
    // Test 7: Defer with for loop
    run_cb_test_with_output_and_time("../cases/defer/test_defer_loop.cb", 
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "test_defer_loop.cb should execute successfully");
            INTEGRATION_ASSERT_CONTAINS(output, "Loop test:", "Should print 'Loop test:'");
            INTEGRATION_ASSERT_CONTAINS(output, "0", "Should print loop counter 0");
            INTEGRATION_ASSERT_CONTAINS(output, "1", "Should print loop counter 1");
            INTEGRATION_ASSERT_CONTAINS(output, "2", "Should print loop counter 2");
            INTEGRATION_ASSERT_CONTAINS(output, "Done", "Should print 'Done'");
            INTEGRATION_ASSERT_CONTAINS(output, "defer", "Should execute defer after loop");
            
            // Check order: defer should execute after loop completes
            size_t pos_done = output.find("Done");
            size_t pos_defer = output.find("defer");
            INTEGRATION_ASSERT(pos_done < pos_defer, 
                              "defer should execute after loop completes");
        }, execution_time);
    integration_test_passed_with_time("Defer with for loop", "test_defer_loop.cb", execution_time);
    
    // Test 8: Defer with break statement
    run_cb_test_with_output_and_time("../cases/defer/test_defer_break.cb", 
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "test_defer_break.cb should execute successfully");
            INTEGRATION_ASSERT_CONTAINS(output, "Break test:", "Should print 'Break test:'");
            INTEGRATION_ASSERT_CONTAINS(output, "0", "Should print loop counter 0");
            INTEGRATION_ASSERT_CONTAINS(output, "1", "Should print loop counter 1");
            INTEGRATION_ASSERT_CONTAINS(output, "2", "Should print loop counter 2");
            INTEGRATION_ASSERT_CONTAINS(output, "Done", "Should print 'Done'");
            INTEGRATION_ASSERT_CONTAINS(output, "defer", "Should execute defer after break");
            
            // Should NOT print 3 or 4 (loop breaks at i==2)
            auto lines = split_lines(output);
            for (const auto& line : lines) {
                INTEGRATION_ASSERT(line != "3", "Should not print 3 (loop breaks)");
                INTEGRATION_ASSERT(line != "4", "Should not print 4 (loop breaks)");
            }
            
            // Check order: defer should execute after break
            size_t pos_done = output.find("Done");
            size_t pos_defer = output.find("defer");
            INTEGRATION_ASSERT(pos_done < pos_defer, 
                              "defer should execute after break");
        }, execution_time);
    integration_test_passed_with_time("Defer with break statement", "test_defer_break.cb", execution_time);
    
    std::cout << "[integration-test] Defer tests completed successfully" << std::endl;
}
