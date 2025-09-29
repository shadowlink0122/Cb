#include "framework/integration_test_framework.hpp"
#include "interface/test_interface_private.hpp"
#include <iostream>

int main() {
    std::cout << "[integration-test] Starting Interface Private Method Tests"
              << std::endl;

    try {
        test_integration_interface_private();
        std::cout
            << "\n[integration-test] All Interface Private Method Tests PASSED!"
            << std::endl;
        return 0;
    } catch (const std::exception &e) {
        std::cout
            << "\n[integration-test] Interface Private Method Tests FAILED: "
            << e.what() << std::endl;
        return 1;
    }
}
