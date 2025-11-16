#include "framework/integration_test_framework.hpp"
#include <chrono>
#include <iostream>

int main() {
    auto start = std::chrono::high_resolution_clock::now();

    double execution_time;
    run_cb_test_with_output_and_time(
        "../../tests/cases/struct/performance_basic.cb",
        [](const std::string &output, int exit_code) {
            std::cout << "Exit code: " << exit_code << std::endl;
            std::cout << "Output length: " << output.length() << std::endl;
        },
        execution_time);

    auto end = std::chrono::high_resolution_clock::now();
    auto duration =
        std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

    std::cout << "Measured execution_time: " << execution_time << " seconds"
              << std::endl;
    std::cout << "Wall clock time: " << duration.count() / 1000.0 << " seconds"
              << std::endl;

    return 0;
}
