# Integration Test 作成ガイド

**作成日**: 2025年10月28日  
**目的**: Cb言語のIntegration Test（統合テスト）の作成方法を明確化

---

## 📖 概要

Integration Testは、Cb言語の機能が実際に動作することを検証するためのテストです。各機能ごとにCbテストファイル（`.cb`）とC++ Integration Testファイル（`.hpp`）を作成し、機能の動作を包括的に検証します。

---

## 🎯 Integration Testの目的

1. **機能の実動作検証**: 実際のCbコードが正しく実行されることを確認
2. **回帰テスト**: 新しい変更が既存機能を壊していないことを確認
3. **出力の検証**: プログラムの出力が期待通りであることを確認
4. **エラーハンドリング**: エラーケースが正しく処理されることを確認

---

## 📂 ディレクトリ構造

```
tests/
├── cases/                          # Cbテストファイル（.cb）
│   ├── <feature_name>/
│   │   ├── README.md              # テスト概要と実行方法
│   │   ├── test_basic.cb          # 基本機能テスト
│   │   ├── test_edge_cases.cb     # エッジケーステスト
│   │   ├── test_error_handling.cb # エラーハンドリングテスト
│   │   └── <other_tests>.cb       # その他のテスト
│   │
│   ├── string_interpolation/
│   │   ├── README.md
│   │   ├── test_basic.cb
│   │   ├── test_expressions.cb
│   │   └── ...
│   │
│   └── interface_bounds/
│       ├── README.md
│       ├── test_multiple_bounds_per_param.cb
│       └── ...
│
└── integration/                    # Integration Testファイル（.hpp）
    ├── main.cpp                   # テストランナー
    ├── framework/
    │   └── integration_test_framework.hpp
    │
    ├── <feature_name>/
    │   └── test_<feature_name>.hpp
    │
    ├── string_interpolation/
    │   └── test_string_interpolation.hpp
    │
    └── interface_bounds/
        └── test_interface_bounds.hpp
```

---

## 🔧 Integration Test作成の3ステップ

### ⚠️ 重要: 新機能を実装する際は、必ず以下の3つのステップを実行してください

---

## ステップ1: Cbテストケースの作成 (`tests/cases/`)

### 1.1 ディレクトリ構造の作成

機能ごとにディレクトリを作成し、テストケースを配置します。

```bash
tests/cases/<feature_name>/
├── README.md                    # テスト概要と実行方法
├── test_basic.cb               # 基本機能テスト
├── test_edge_cases.cb          # エッジケーステスト
├── test_error_handling.cb      # エラーハンドリングテスト
└── <other_tests>.cb            # その他のテスト
```

**例: 文字列補間機能の場合**
```bash
tests/cases/string_interpolation/
├── README.md
├── test_basic.cb
├── test_expressions.cb
├── test_array_access.cb
├── test_member_access.cb
├── test_types.cb
├── format_specifiers.cb
└── test_edge_cases.cb
```

**例: インターフェース境界機能の場合**
```bash
tests/cases/interface_bounds/
├── README.md
├── test_multiple_bounds_per_param.cb
├── test_function_multiple_bounds.cb
├── test_enum_multiple_bounds.cb
├── test_conflict_methods.cb
├── test_duplicate_impl_methods.cb
└── test_no_conflict_different_types.cb
```

### 1.2 Cbテストファイルの作成

**基本構造:**

```cb
void main() {
    println("=== <Feature Name> Test ===");
    
    // Test 1: Description
    // ... test code ...
    println("Test 1: <Description> - PASSED");
    
    // Test 2: Description
    // ... test code ...
    println("Test 2: <Description> - PASSED");
    
    println("=== All Tests Passed ===");
}
```

**例1: 正常系テスト**

```cb
// tests/cases/interface_bounds/test_multiple_bounds_per_param.cb
interface Allocator {
    void* allocate(int size);
    void deallocate(void* ptr);
}

interface Clone {
    void clone();
}

interface Debug {
    void debug();
}

struct Container<T, A: Allocator + Clone> {
    T* data;
    A allocator;
};

void main() {
    println("=== Multiple Interface Bounds Test ===");
    
    // Test 1: Single type parameter with multiple bounds
    println("Test 1: Container<T, A: Allocator + Clone> - PASSED");
    
    // Test 2: Parse success verification
    println("Test 2: Syntax parsing successful - PASSED");
    
    println("=== All Tests Passed ===");
}
```

**例2: エラーケーステスト**

```cb
// tests/cases/interface_bounds/test_conflict_methods.cb
interface Allocator {
    void reset();
}

interface Resettable {
    void reset();  // 衝突！
}

struct Container<T, A: Allocator + Resettable> {
    A allocator;
};

struct DummyAllocator {
    int dummy;
};

impl Allocator for DummyAllocator {
    void reset() {}
}

impl Resettable for DummyAllocator {
    void reset() {}  // エラーが期待される
}

void main() {
    println("=== Method Conflict Detection Test ===");
    println("Expected: Method name conflict error");
    
    Container<int, DummyAllocator> container;
    
    // ここには到達しないはず
    println("ERROR: Conflict was not detected!");
}
```

### 1.3 README.mdの作成

各機能のテストディレクトリには必ず`README.md`を作成してください。

**テンプレート:**

```markdown
# <機能名> テストスイート

## 概要

<機能の簡単な説明>

## 基本構文

\`\`\`cb
// 基本的な使用例
\`\`\`

## テストファイル

### 1. test_basic.cb
基本機能のテスト

**テスト内容**:
- 項目1
- 項目2

**実行方法**:
\`\`\`bash
./main tests/cases/<feature>/test_basic.cb
\`\`\`

**期待される出力**:
\`\`\`
=== Test Name ===
Test 1: ... - PASSED
=== All Tests Passed ===
\`\`\`

---

### 2. test_edge_cases.cb
エッジケースのテスト

...

## 全テスト実行

\`\`\`bash
for file in tests/cases/<feature>/*.cb; do
    ./main "$file"
done
\`\`\`

## 実装ステータス

- [x] テストケース作成
- [ ] Integration test作成
- [ ] ドキュメント更新
```

### 1.4 テストの実行確認

個別にテストファイルを実行して動作を確認します。

```bash
# 個別実行
./main tests/cases/<feature>/test_basic.cb

# 正常終了の確認（exit code == 0）
echo $?  # 0が表示されるはず

# エラーケースの確認（exit code != 0）
./main tests/cases/<feature>/test_error.cb
echo $?  # 0以外が表示されるはず
```

---

## ステップ2: Integration Testの作成 (`tests/integration/`)

### 2.1 ディレクトリとファイルの作成

各機能ごとにディレクトリと`.hpp`ファイルを作成します。

```bash
tests/integration/<feature_name>/
└── test_<feature_name>.hpp
```

**例:**
```bash
tests/integration/interface_bounds/
└── test_interface_bounds.hpp
```

### 2.2 Integration Testファイルの作成

**基本構造:**

```cpp
#pragma once

#include "../framework/integration_test_framework.hpp"

inline void test_integration_<feature_name>() {
    std::cout << "[integration-test] Running <feature name> tests..." << std::endl;
    
    // テスト1: 基本機能
    double execution_time_basic;
    run_cb_test_with_output_and_time(
        "../../tests/cases/<feature_name>/test_basic.cb", 
        [](const std::string& output, int exit_code) {
            // 終了コードの検証
            INTEGRATION_ASSERT_EQ(0, exit_code, 
                "test_basic.cb should execute successfully");
            
            // 出力内容の検証
            INTEGRATION_ASSERT_CONTAINS(output, "Expected output", 
                "Should contain expected output");
            INTEGRATION_ASSERT_CONTAINS(output, "Test passed", 
                "Should show success message");
        }, 
        execution_time_basic
    );
    integration_test_passed_with_time("<test name>", "test_basic.cb", 
                                      execution_time_basic);
    
    // 追加のテストケース...
    
    std::cout << "[integration-test] <Feature name> tests completed" << std::endl;
}
```

**完全な例: interface_bounds**

```cpp
#pragma once

#include "../framework/integration_test_framework.hpp"

inline void test_integration_interface_bounds() {
    std::cout << "[integration-test] Running Interface Bounds tests..." << std::endl;
    
    // Test 1: Multiple bounds per parameter
    double execution_time_1;
    run_cb_test_with_output_and_time(
        "../../tests/cases/interface_bounds/test_multiple_bounds_per_param.cb",
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, 
                "Multiple bounds test should execute successfully");
            INTEGRATION_ASSERT_CONTAINS(output, 
                "=== Multiple Interface Bounds Test ===",
                "Should show test header");
            INTEGRATION_ASSERT_CONTAINS(output, 
                "Test 1: Container<T, A: Allocator + Clone> - PASSED",
                "Test 1 should pass");
            INTEGRATION_ASSERT_CONTAINS(output, 
                "=== All Tests Passed ===",
                "Should show completion message");
        },
        execution_time_1
    );
    integration_test_passed_with_time(
        "Interface Bounds", 
        "test_multiple_bounds_per_param.cb", 
        execution_time_1
    );
    
    // Test 2: Function multiple bounds
    double execution_time_2;
    run_cb_test_with_output_and_time(
        "../../tests/cases/interface_bounds/test_function_multiple_bounds.cb",
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, 
                "Function bounds test should succeed");
            INTEGRATION_ASSERT_CONTAINS(output, 
                "=== All Tests Passed ===",
                "Should complete successfully");
        },
        execution_time_2
    );
    integration_test_passed_with_time(
        "Interface Bounds", 
        "test_function_multiple_bounds.cb", 
        execution_time_2
    );
    
    // Test 3: Error case - conflict detection
    double execution_time_3;
    run_cb_test_with_output_and_time(
        "../../tests/cases/interface_bounds/test_conflict_methods.cb",
        [](const std::string& output, int exit_code) {
            // エラーケースなので exit_code != 0 が期待される
            INTEGRATION_ASSERT_NE(0, exit_code, 
                "Conflict test should fail with error");
            INTEGRATION_ASSERT_CONTAINS(output, 
                "Method name conflict",
                "Should show conflict error message");
            INTEGRATION_ASSERT_CONTAINS(output, 
                "reset",
                "Should mention conflicting method name");
        },
        execution_time_3
    );
    integration_test_passed_with_time(
        "Interface Bounds", 
        "test_conflict_methods.cb (error case)", 
        execution_time_3
    );
    
    std::cout << "[integration-test] Interface Bounds tests completed" << std::endl;
}
```

### 2.3 アサーションマクロ

Integration Testで使用可能なアサーションマクロ：

| マクロ | 説明 | 例 |
|--------|------|-----|
| `INTEGRATION_ASSERT_EQ(expected, actual, message)` | 値の等価性チェック | `INTEGRATION_ASSERT_EQ(0, exit_code, "Should succeed")` |
| `INTEGRATION_ASSERT_NE(expected, actual, message)` | 値の非等価性チェック | `INTEGRATION_ASSERT_NE(0, exit_code, "Should fail")` |
| `INTEGRATION_ASSERT_CONTAINS(output, substring, message)` | 出力に文字列が含まれるかチェック | `INTEGRATION_ASSERT_CONTAINS(output, "PASSED", "Should show success")` |
| `integration_test_passed_with_time(name, file, time)` | テスト成功の記録 | `integration_test_passed_with_time("Feature", "test.cb", time)` |

---

## ステップ3: main.cppへの登録

### 3.1 includeの追加

`tests/integration/main.cpp`のファイル上部に、作成したテストファイルのincludeを追加します。

```cpp
// tests/integration/main.cpp

// 既存のinclude
#include "string_interpolation/test_string_interpolation.hpp"
#include "switch/test_switch.hpp"

// 新規追加
#include "interface_bounds/test_interface_bounds.hpp"
```

### 3.2 テスト関数の呼び出し

適切なカテゴリセクションに、テスト関数の呼び出しを追加します。

```cpp
// カテゴリセクションの例

std::cout << "\n[integration-test] === Type System Tests ===" << std::endl;
CategoryTimingStats::set_current_category("Type System");

run_test_with_continue(test_integration_generics, 
                       "Generics Tests", 
                       failed_tests);
run_test_with_continue(test_integration_interface_bounds,  // 新規追加
                       "Interface Bounds Tests", 
                       failed_tests);
```

**カテゴリ一覧:**

- **Basic Language Features** - 基本言語機能
- **String & I/O Tests** - 文字列・入出力
- **Control Flow Tests** - 制御フロー
- **Type System Tests** - 型システム
- **Memory Management Tests** - メモリ管理
- **Advanced Features** - 高度な機能

### 3.3 完全な例

```cpp
// tests/integration/main.cpp

#include <iostream>
#include <vector>
#include <string>
#include "framework/integration_test_framework.hpp"

// Feature includes
#include "string_interpolation/test_string_interpolation.hpp"
#include "switch/test_switch.hpp"
#include "interface_bounds/test_interface_bounds.hpp"  // 新規追加
// ... other includes ...

int main() {
    std::vector<std::string> failed_tests;
    
    std::cout << "\n╔════════════════════════════════════════════════════════════╗" << std::endl;
    std::cout << "║         Cb Language Integration Test Suite                ║" << std::endl;
    std::cout << "╚════════════════════════════════════════════════════════════╝\n" << std::endl;
    
    // === Type System Tests ===
    std::cout << "\n[integration-test] === Type System Tests ===" << std::endl;
    CategoryTimingStats::set_current_category("Type System");
    
    run_test_with_continue(test_integration_interface_bounds,  // 新規追加
                           "Interface Bounds Tests", 
                           failed_tests);
    
    // ... other tests ...
    
    // === Summary ===
    std::cout << "\n[integration-test] ========================================" << std::endl;
    std::cout << "[integration-test] Integration Test Summary" << std::endl;
    std::cout << "[integration-test] ========================================" << std::endl;
    
    if (failed_tests.empty()) {
        std::cout << "[integration-test] ✅ All tests passed!" << std::endl;
        CategoryTimingStats::print_summary();
        return 0;
    } else {
        std::cout << "[integration-test] ❌ " << failed_tests.size() 
                  << " test(s) failed:" << std::endl;
        for (const auto& test : failed_tests) {
            std::cout << "[integration-test]   - " << test << std::endl;
        }
        return 1;
    }
}
```

---

## 🧪 テストの実行

### 全テストの実行

```bash
# すべてのテストを実行（Integration test含む）
make test

# Integration testのみ実行
make integration-test

# Unit testのみ実行
make unit-test
```

### 個別実行

```bash
# 個別のCbファイルを実行
./main tests/cases/<feature>/test_basic.cb

# 終了コードの確認
echo $?
```

---

## 📝 テストのベストプラクティス

### 1. テストファイルの命名規則

- `test_<feature>.cb` - 機能別テスト
- `test_basic.cb` - 基本的な機能テスト
- `test_edge_cases.cb` - エッジケース
- `test_error_handling.cb` - エラーケース

### 2. テストの構造

```cb
void main() {
    println("=== <Feature Name> Test ===");
    
    // Test 1: Description
    // ... test code ...
    println("Test 1: <Description> - PASSED");
    
    // Test 2: Description
    // ... test code ...
    println("Test 2: <Description> - PASSED");
    
    println("=== All Tests Passed ===");
}
```

### 3. アサーションの使用

```cb
void main() {
    int result = calculate(5);
    assert(result == 25);  // 検証
    println("Test: Calculation - PASSED");
}
```

### 4. エラーテストの記述

エラーが期待される場合も明示的にテストする：

```cb
void main() {
    println("=== Error Test ===");
    println("Expected: Error message");
    
    // エラーが発生するコード
    invalid_operation();
    
    // ここには到達しないはず
    println("ERROR: No error was detected!");
}
```

Integration test側で`exit_code != 0`を検証：

```cpp
run_cb_test_with_output_and_time(
    "../../tests/cases/feature/test_error.cb",
    [](const std::string& output, int exit_code) {
        INTEGRATION_ASSERT_NE(0, exit_code, "Should fail with error");
        INTEGRATION_ASSERT_CONTAINS(output, "Error:", "Should show error");
    },
    execution_time
);
```

### 5. テストの独立性

- 各テストケースは独立して実行可能であること
- テスト間で状態を共有しないこと
- グローバル変数に依存しないこと

---

## 📊 テストカバレッジの目標

- **新機能**: 80%以上のカバレッジ
- **重要機能**: 90%以上のカバレッジ
- **コア機能**: 95%以上のカバレッジ

---

## ✅ チェックリスト

新機能のIntegration Test追加時：

- [ ] `tests/cases/<feature>/` ディレクトリ作成
- [ ] `tests/cases/<feature>/README.md` 作成
- [ ] 基本機能テスト (`test_basic.cb`) 作成
- [ ] エッジケーステスト (`test_edge_cases.cb`) 作成
- [ ] エラーケーステスト (`test_error_handling.cb`) 作成
- [ ] 各テストファイルを個別実行して動作確認
- [ ] `tests/integration/<feature>/test_<feature>.hpp` 作成
- [ ] Integration testで各Cbテストの出力を検証
- [ ] `tests/integration/main.cpp` にinclude追加
- [ ] `tests/integration/main.cpp` にテスト関数呼び出し追加
- [ ] `make integration-test` で実行可能を確認
- [ ] `make test` で全テスト成功を確認
- [ ] カバレッジ80%以上を達成

---

## 🐛 トラブルシューティング

### テストが見つからない

**エラー例:**
```
Error: Could not open file: tests/cases/feature/test_basic.cb
```

**解決:**
- ファイルパスが正しいか確認
- ファイルが実際に存在するか確認
- `.cb`拡張子が付いているか確認

### テストは通るがIntegration testで失敗

**原因:** 出力文字列が期待と異なる

**解決:**
```bash
# 実際の出力を確認
./main tests/cases/feature/test_basic.cb

# Integration testの期待値を実際の出力に合わせる
INTEGRATION_ASSERT_CONTAINS(output, "実際の出力文字列", "...");
```

### exit codeが期待と異なる

**確認:**
```bash
./main tests/cases/feature/test.cb
echo $?  # exit codeを表示
```

**正常終了:** exit code == 0  
**エラー終了:** exit code != 0

---

## 📚 関連ドキュメント

- **コーディング規約**: `docs/CODING_GUIDELINES.md`
- **言語仕様**: `docs/spec.md`
- **アーキテクチャ**: `docs/architecture.md`
- **標準ライブラリテスト**: `docs/testing/stdlib_test_structure.md`

---

## 📞 サポート

### 質問・問題報告

テストに関する質問や問題がある場合：

1. 既存のテストケースを参照（`tests/cases/`内のサンプル）
2. Integration testの既存実装を参照（`tests/integration/`内のサンプル）
3. このドキュメントのトラブルシューティングセクションを確認

---

**最終更新**: 2025年10月28日  
**バージョン**: v1.0  
**ステータス**: ✅ 完成
