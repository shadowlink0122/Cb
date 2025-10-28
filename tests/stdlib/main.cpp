#include "allocators/test_bump_allocator.hpp"
#include "allocators/test_system_allocator.hpp"
#include "collections/test_vector.hpp"
#include "framework/stdlib_test_framework.hpp"

#include <cstdlib>
#include <iostream>

int main() {
    StdlibTestRunner runner;

    // Register all stdlib tests
    register_system_allocator_tests(runner);
    register_bump_allocator_tests(runner);
    register_vector_tests(runner);

    // Run all tests
    runner.run_all();

    // Return exit code based on results
    return runner.all_passed() ? 0 : 1;
}
