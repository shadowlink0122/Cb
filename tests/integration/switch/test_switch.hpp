#pragma once
#include "../framework/integration_test_framework.hpp"

void test_integration_switch() {
    std::cout << "[integration-test] Running Switch Statement Tests..." << std::endl;
    
    double execution_time;
    
    // Test 1: Basic switch functionality
    run_cb_test_with_output_and_time("../cases/switch/test_switch_basic.cb", 
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "test_switch_basic.cb should execute successfully");
            INTEGRATION_ASSERT_CONTAINS(output, "=== Basic Switch Test ===", "Should print test header");
            INTEGRATION_ASSERT_CONTAINS(output, "Two", "Should match case (2)");
            INTEGRATION_ASSERT_CONTAINS(output, "Not one or two", "Should execute else clause");
            INTEGRATION_ASSERT_CONTAINS(output, "=== Test completed ===", "Should complete test");
        }, execution_time);
    integration_test_passed_with_time("Basic switch with single values and else", "test_switch_basic.cb", execution_time);
    
    // Test 2: OR operator in switch cases
    run_cb_test_with_output_and_time("../cases/switch/test_switch_or.cb", 
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "test_switch_or.cb should execute successfully");
            INTEGRATION_ASSERT_CONTAINS(output, "One, Two or Three", "Should match OR condition (1 || 2 || 3)");
            INTEGRATION_ASSERT_CONTAINS(output, "Four or Five", "Should match OR condition (4 || 5)");
            INTEGRATION_ASSERT_CONTAINS(output, "Other", "Should execute else for non-matching value");
        }, execution_time);
    integration_test_passed_with_time("Switch with OR operator (||)", "test_switch_or.cb", execution_time);
    
    // Test 3: Range operator in switch cases
    run_cb_test_with_output_and_time("../cases/switch/test_switch_range.cb", 
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "test_switch_range.cb should execute successfully");
            INTEGRATION_ASSERT_CONTAINS(output, "Grade A", "Should match range 90...100");
            INTEGRATION_ASSERT_CONTAINS(output, "Grade B", "Should match range 80...89");
            INTEGRATION_ASSERT_CONTAINS(output, "Grade F", "Should execute else for out of range");
            
            auto lines = split_lines(output);
            int grade_b_count = 0;
            for (const auto& line : lines) {
                if (line == "Grade B") grade_b_count++;
            }
            INTEGRATION_ASSERT_EQ(2, grade_b_count, "Should print 'Grade B' twice (85 and 89)");
        }, execution_time);
    integration_test_passed_with_time("Switch with range operator (...)", "test_switch_range.cb", execution_time);
    
    // Test 4: Mixed OR and range operators
    run_cb_test_with_output_and_time("../cases/switch/test_switch_mixed.cb", 
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "test_switch_mixed.cb should execute successfully");
            INTEGRATION_ASSERT_CONTAINS(output, "One", "Should match single value");
            INTEGRATION_ASSERT_CONTAINS(output, "Two or Three", "Should match OR condition");
            INTEGRATION_ASSERT_CONTAINS(output, "Ten to Twenty", "Should match range condition");
            INTEGRATION_ASSERT_CONTAINS(output, "Other", "Should execute else");
        }, execution_time);
    integration_test_passed_with_time("Switch with mixed conditions (single, OR, range)", "test_switch_mixed.cb", execution_time);
    
    // Test 5: Switch with return statements
    run_cb_test_with_output_and_time("../cases/switch/test_switch_return.cb", 
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "test_switch_return.cb should execute successfully");
            
            auto lines = split_lines(output);
            bool found_a = false, found_b = false, found_c = false, found_d = false, found_f = false;
            
            for (const auto& line : lines) {
                if (line == "A") found_a = true;
                if (line == "B") found_b = true;
                if (line == "C") found_c = true;
                if (line == "D") found_d = true;
                if (line == "F") found_f = true;
            }
            
            INTEGRATION_ASSERT(found_a, "Should return 'A' for score 95");
            INTEGRATION_ASSERT(found_b, "Should return 'B' for score 85");
            INTEGRATION_ASSERT(found_c, "Should return 'C' for score 75");
            INTEGRATION_ASSERT(found_d, "Should return 'D' for score 65");
            INTEGRATION_ASSERT(found_f, "Should return 'F' for score 55");
        }, execution_time);
    integration_test_passed_with_time("Switch with return statements in function", "test_switch_return.cb", execution_time);
    
    // Test 6: Complex conditions (|| and ... combined)
    run_cb_test_with_output_and_time("../cases/switch/test_switch_complex.cb", 
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "test_switch_complex.cb should execute successfully");
            INTEGRATION_ASSERT_CONTAINS(output, "1, 2, or 3 to 5", "Should match complex condition");
            INTEGRATION_ASSERT_CONTAINS(output, "10 to 15, or 20", "Should match range || single value");
            INTEGRATION_ASSERT_CONTAINS(output, "Other", "Should execute else");
            
            auto lines = split_lines(output);
            int match_10_15_count = 0;
            for (const auto& line : lines) {
                if (line == "10 to 15, or 20") match_10_15_count++;
            }
            INTEGRATION_ASSERT_EQ(2, match_10_15_count, 
                "Should match '10 to 15, or 20' twice (for 12 and 20)");
        }, execution_time);
    integration_test_passed_with_time("Switch with complex conditions (|| and ... combined)", "test_switch_complex.cb", execution_time);
    
    // Test 7: Switch with typedef types
    run_cb_test_with_output_and_time("../cases/switch/test_switch_typedef.cb", 
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "test_switch_typedef.cb should execute successfully");
            INTEGRATION_ASSERT_CONTAINS(output, "Child", "Should categorize age 10 as Child");
            INTEGRATION_ASSERT_CONTAINS(output, "Teenager", "Should categorize age 15 as Teenager");
            INTEGRATION_ASSERT_CONTAINS(output, "Adult", "Should categorize age 30 as Adult");
            INTEGRATION_ASSERT_CONTAINS(output, "Senior", "Should categorize age 70 as Senior");
            INTEGRATION_ASSERT_CONTAINS(output, "A", "Should return grade A");
            INTEGRATION_ASSERT_CONTAINS(output, "B", "Should return grade B");
            INTEGRATION_ASSERT_CONTAINS(output, "C", "Should return grade C");
            INTEGRATION_ASSERT_CONTAINS(output, "D", "Should return grade D");
            INTEGRATION_ASSERT_CONTAINS(output, "F", "Should return grade F");
        }, execution_time);
    integration_test_passed_with_time("Switch with typedef types (Age, Score)", "test_switch_typedef.cb", execution_time);
    
    // Test 8: Switch with struct members
    run_cb_test_with_output_and_time("../cases/switch/test_switch_struct.cb", 
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "test_switch_struct.cb should execute successfully");
            INTEGRATION_ASSERT_CONTAINS(output, "Excellent", "Should evaluate Alice's score as Excellent");
            INTEGRATION_ASSERT_CONTAINS(output, "Average", "Should evaluate Bob's score as Average");
            INTEGRATION_ASSERT_CONTAINS(output, "Fail", "Should evaluate Charlie's score as Fail");
            INTEGRATION_ASSERT_CONTAINS(output, "University", "Should check university age");
            INTEGRATION_ASSERT_CONTAINS(output, "Graduate", "Should check graduate age");
            INTEGRATION_ASSERT_CONTAINS(output, "Alice is university age", "Should match direct member access");
        }, execution_time);
    integration_test_passed_with_time("Switch with struct members (Student.score, Student.age)", "test_switch_struct.cb", execution_time);
    
    // Test 9: Switch with enum types
    run_cb_test_with_output_and_time("../cases/switch/test_switch_enum.cb", 
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "test_switch_enum.cb should execute successfully");
            INTEGRATION_ASSERT_CONTAINS(output, "Red", "Should match Color enum RED");
            INTEGRATION_ASSERT_CONTAINS(output, "Green", "Should match Color enum GREEN");
            INTEGRATION_ASSERT_CONTAINS(output, "Yellow or Black", "Should match OR condition with enums");
            INTEGRATION_ASSERT_CONTAINS(output, "Waiting", "Should match Status PENDING");
            INTEGRATION_ASSERT_CONTAINS(output, "In progress", "Should match Status PROCESSING");
            INTEGRATION_ASSERT_CONTAINS(output, "Done", "Should match Status COMPLETED");
            INTEGRATION_ASSERT_CONTAINS(output, "Error", "Should match Status FAILED");
            INTEGRATION_ASSERT_CONTAINS(output, "Primary color", "Should match range check on enum");
        }, execution_time);
    integration_test_passed_with_time("Switch with enum types (Color, Status)", "test_switch_enum.cb", execution_time);
    
    // Test 10: Switch with array elements
    run_cb_test_with_output_and_time("../cases/switch/test_switch_array.cb", 
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "test_switch_array.cb should execute successfully");
            INTEGRATION_ASSERT_CONTAINS(output, "Grade A", "Should match array[0] = 95");
            INTEGRATION_ASSERT_CONTAINS(output, "Grade B", "Should match array[1] = 85");
            INTEGRATION_ASSERT_CONTAINS(output, "Grade C", "Should match array[2] = 75");
            INTEGRATION_ASSERT_CONTAINS(output, "Grade D", "Should match array[3] = 65");
            INTEGRATION_ASSERT_CONTAINS(output, "Grade F", "Should match array[4] = 55");
            INTEGRATION_ASSERT_CONTAINS(output, "Small", "Should match multidim array small values");
            INTEGRATION_ASSERT_CONTAINS(output, "Medium", "Should match multidim array medium values");
            INTEGRATION_ASSERT_CONTAINS(output, "Large", "Should match multidim array large values");
            
            auto lines = split_lines(output);
            int small_count = 0, medium_count = 0, large_count = 0;
            for (const auto& line : lines) {
                if (line == "Small") small_count++;
                if (line == "Medium") medium_count++;
                if (line == "Large") large_count++;
            }
            INTEGRATION_ASSERT_EQ(3, small_count, "Should print 'Small' 3 times");
            INTEGRATION_ASSERT_EQ(2, medium_count, "Should print 'Medium' 2 times");
            INTEGRATION_ASSERT_EQ(1, large_count, "Should print 'Large' 1 time");
        }, execution_time);
    integration_test_passed_with_time("Switch with array elements (1D and 2D arrays)", "test_switch_array.cb", execution_time);
    
    // Test 11: Nested switch statements
    run_cb_test_with_output_and_time("../cases/switch/test_switch_nested.cb", 
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "test_switch_nested.cb should execute successfully");
            INTEGRATION_ASSERT_CONTAINS(output, "Category: Academic", "Should match category 1");
            INTEGRATION_ASSERT_CONTAINS(output, "Level: Good", "Should match nested level 85 in Academic");
            INTEGRATION_ASSERT_CONTAINS(output, "Category: Sports", "Should match category 2");
            INTEGRATION_ASSERT_CONTAINS(output, "Level: Professional", "Should match nested level 95 in Sports");
            
            // Check order: Category should appear before Level
            size_t pos_academic = output.find("Category: Academic");
            size_t pos_good = output.find("Level: Good");
            size_t pos_sports = output.find("Category: Sports");
            size_t pos_professional = output.find("Level: Professional");
            
            INTEGRATION_ASSERT(pos_academic < pos_good, 
                "Category should appear before Level in first test");
            INTEGRATION_ASSERT(pos_sports < pos_professional, 
                "Category should appear before Level in second test");
        }, execution_time);
    integration_test_passed_with_time("Nested switch statements", "test_switch_nested.cb", execution_time);
    
    std::cout << "[integration-test] ✅ PASS: Switch Statement Tests (11 tests)" << std::endl;
}
