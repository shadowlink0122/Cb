#include "framework/dummy_functions.hpp"
#include "framework/test_framework.hpp"
#include "framework/unit_test_impl.hpp"

// 各テストモジュールをインクルード
#include "backend/test_arithmetic.hpp"
#include "backend/test_boundary.hpp"
#include "backend/test_cross_type.hpp"
#include "backend/test_functions.hpp"
#include "backend/test_interpreter.hpp"
#include "common/test_ast.hpp"
#include "common/test_io_abstraction.hpp"
#include "common/test_type_utils.hpp"
#include "frontend/test_debug.hpp"
#include "frontend/test_parser_utils.hpp"

int main() {
    std::cout << "Running unit tests..." << std::endl;

    // テストフレームワークを初期化
    initialize_test_framework();

    int test_count = 0;

    // 各テストモジュールを実行
    try {
        register_type_utils_tests();
        register_ast_tests();
        register_io_abstraction_tests();
        register_parser_utils_tests();
        register_debug_tests();
        register_interpreter_tests();
        register_boundary_tests();
        register_arithmetic_tests();
        register_cross_type_tests();
        register_function_tests();

        std::cout << "Running " << test_count << " tests..." << std::endl;
    } catch (const std::exception &e) {
        std::cerr << "Error during test execution: " << e.what() << std::endl;
    }

    // テスト結果を出力
    int failed_count = print_test_results();

    // クリーンアップ
    cleanup_test_framework();

    return failed_count > 0 ? 1 : 0;
}
