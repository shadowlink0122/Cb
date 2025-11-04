#include "allocators/test_bump_allocator.hpp"
#include "allocators/test_system_allocator.hpp"
// #include "collections/test_nested_generics.hpp"  // TODO: Fix segfault
#include "collections/test_map.hpp"
#include "collections/test_queue.hpp"
#include "collections/test_vector.hpp"
#include "framework/stdlib_test_framework.hpp"
#include "std/option_test.hpp"
#include "std/result_test.hpp"

#include <cstdlib>
#include <iostream>

int main() {
    StdlibTestRunner runner;

    // Register all stdlib tests
    register_system_allocator_tests(runner);
    register_bump_allocator_tests(runner);
    register_vector_tests(runner);
    register_queue_tests(runner);
    register_map_tests(runner);
    // register_nested_generics_tests(runner);  // TODO: Fix segfault
    register_result_tests(runner);
    register_option_tests(runner);

    // Run all tests
    runner.run_all();

    // Return exit code based on results
    return runner.all_passed() ? 0 : 1;
}
