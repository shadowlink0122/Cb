#include "framework/test_framework.h"

// 各テストモジュールをインクルード
#include "backend/test_arithmetic.hpp"
#include "backend/test_boundary.hpp"
#include "backend/test_cross_type.hpp"
#include "backend/test_functions.hpp"
#include "backend/test_interpreter.hpp"
#include "common/test_ast.hpp"
#include "common/test_type_utils.hpp"
#include "frontend/test_debug.hpp"
#include "frontend/test_parser_utils.hpp"

// テストランナーのインスタンス
TestRunner test_runner;

int main() {
    std::cout << "Running unit tests..." << std::endl;

    // 各テストモジュールを登録
    register_type_utils_tests();
    register_ast_tests();
    register_parser_utils_tests();
    register_debug_tests();
    register_interpreter_tests();
    register_boundary_tests();
    register_arithmetic_tests();
    register_cross_type_tests();
    register_function_tests();

    // すべてのテストを実行
    test_runner.run_all();

    return 0;
}
