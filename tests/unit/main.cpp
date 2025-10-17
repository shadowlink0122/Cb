#include "framework/test_framework.hpp"

// 各モジュールのテスト群をインクルード
#include "backend/test_arithmetic.hpp"
#include "backend/test_boundary.hpp"
#include "backend/test_cross_type.hpp"
#include "backend/test_functions.hpp"
#include "backend/test_interpreter.hpp"
#include "backend/test_pointer.hpp"

// フロントエンドのテスト群
#include "frontend/preprocessor/test_directive_parser.hpp"
#include "frontend/preprocessor/test_macro_definition.hpp"
#include "frontend/preprocessor/test_macro_expander.hpp"
#include "frontend/preprocessor/test_preprocessor.hpp"
#include "frontend/preprocessor/test_token_preprocessor.hpp"
#include "frontend/recursive_parser/test_lexer_preprocessor.hpp"

int main() {
    std::cout << "Running comprehensive unit tests..." << std::endl;

    try {
        // 各モジュールのテストを登録（すべて有効化）
        register_interpreter_tests();
        register_arithmetic_tests();
        register_boundary_tests();
        register_cross_type_tests();
        register_function_tests();
        register_pointer_tests();

        // プリプロセッサのテストを登録
        register_macro_definition_tests();
        register_directive_parser_tests();
        register_macro_expander_tests();
        register_preprocessor_tests();

        // レキサーのプリプロセッサテスト
        run_lexer_preprocessor_tests();

        // トークンベースプリプロセッサのテスト
        run_token_preprocessor_tests();

        // すべてのテストを実行
        test_runner.run_all();

        return test_runner.all_passed() ? 0 : 1;
    } catch (const std::exception &e) {
        std::cerr << "Test execution failed: " << e.what() << std::endl;
        return 1;
    }
}
