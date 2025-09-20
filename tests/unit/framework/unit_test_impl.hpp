#pragma once

#include "test_framework.hpp"

// グローバルテストフレームワークインスタンスの定義
UnitTestFramework* g_test_framework = nullptr;

// テストフレームワークの初期化
inline void initialize_test_framework() {
    if (!g_test_framework) {
        g_test_framework = new UnitTestFramework();
    }
}

// テストフレームワークのクリーンアップ
inline void cleanup_test_framework() {
    if (g_test_framework) {
        delete g_test_framework;
        g_test_framework = nullptr;
    }
}

// テスト結果の出力
inline int print_test_results() {
    if (g_test_framework) {
        g_test_framework->print_results();
        return g_test_framework->get_failed_count();
    }
    return 1;
}
