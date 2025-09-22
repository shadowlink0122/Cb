#include "framework/test_framework.hpp"

// 各モジュールのテスト群をインクルード
#include "backend/test_arithmetic.hpp"
#include "backend/test_boundary.hpp"
#include "backend/test_cross_type.hpp"
#include "backend/test_functions.hpp"
#include "backend/test_interpreter.hpp"

int main() {
    std::cout << "Running comprehensive unit tests..." << std::endl;

    try {
        // 各モジュールのテストを登録（すべて有効化）
        register_interpreter_tests();
        register_arithmetic_tests();
        register_boundary_tests();
        register_cross_type_tests();
        register_function_tests();

        // すべてのテストを実行
        test_runner.run_all();

        return test_runner.all_passed() ? 0 : 1;
    } catch (const std::exception &e) {
        std::cerr << "Test execution failed: " << e.what() << std::endl;
        return 1;
    }
}
