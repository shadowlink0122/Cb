#ifndef TYPEDEF_POINTER_REFERENCE_TESTS_HPP
#define TYPEDEF_POINTER_REFERENCE_TESTS_HPP

#include "../framework/integration_test_framework.hpp"
#include <string>

namespace TypedefPointerReferenceTests {

inline void test_typedef_pointer() {
    double execution_time = 0.0;
    run_cb_test_with_output_and_time(
        "../../tests/cases/typedef/test_typedef_simple_reference.cb",
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "typedef参照テストがエラー終了");
            
            INTEGRATION_ASSERT(output.find("Test: Simple typedef reference") != std::string::npos, "テストヘッダーがない");
            INTEGRATION_ASSERT(output.find("50") != std::string::npos, "初期値が正しくない");
            INTEGRATION_ASSERT(output.find("100") != std::string::npos, "参照経由の変更が反映されていない");
        },
        execution_time
    );
}

inline void run_all_typedef_pointer_reference_tests() {
    std::cout << "\n============================================================" << std::endl;
    std::cout << "Running Typedef Pointer/Reference Tests..." << std::endl;
    std::cout << "============================================================" << std::endl;
    
    test_typedef_pointer();
    
    std::cout << "✅ PASS: Typedef Pointer/Reference Tests (1 test)" << std::endl;
}

} // namespace TypedefPointerReferenceTests

#endif // TYPEDEF_POINTER_REFERENCE_TESTS_HPP
