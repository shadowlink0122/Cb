# Cb言語 コーディング規約

## 目次

1. [テストの作成](#テストの作成)
2. [コードスタイル](#コードスタイル)
3. [命名規則](#命名規則)
4. [ドキュメント](#ドキュメント)
5. [コミット規約](#コミット規約)

---

## テストの作成

**⚠️ 重要: 新機能を実装する際は、必ずテストを作成してください。**

### テスト作成の必須手順

新機能を実装する際は、以下の3つのステップを**必ず実行**してください：

#### 1. Cbテストケースの作成 (`tests/cases/`)

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

#### 2. Integration Testの作成 (`tests/integration/`)

各機能ごとにディレクトリと`.hpp`ファイルを作成します。

```bash
tests/integration/<feature_name>/
└── test_<feature_name>.hpp
```

**テストファイルの構造:**

```cpp
#pragma once

#include "../framework/integration_test_framework.hpp"

inline void test_integration_<feature_name>() {
    std::cout << "[integration-test] Running <feature name> tests..." << std::endl;
    
    // テスト1: 基本機能
    double execution_time_basic;
    run_cb_test_with_output_and_time("../../tests/cases/<feature_name>/test_basic.cb", 
        [](const std::string& output, int exit_code) {
            // 終了コードの検証
            INTEGRATION_ASSERT_EQ(0, exit_code, "test_basic.cb should execute successfully");
            
            // 出力内容の検証
            INTEGRATION_ASSERT_CONTAINS(output, "Expected output", "Should contain expected output");
            INTEGRATION_ASSERT_CONTAINS(output, "Test passed", "Should show success message");
        }, execution_time_basic);
    integration_test_passed_with_time("<test name>", "test_basic.cb", execution_time_basic);
    
    // 追加のテストケース...
    
    std::cout << "[integration-test] <Feature name> tests completed" << std::endl;
}
```

**アサーションマクロ:**

- `INTEGRATION_ASSERT_EQ(expected, actual, message)` - 値の等価性チェック
- `INTEGRATION_ASSERT_CONTAINS(output, substring, message)` - 出力に文字列が含まれるかチェック
- `integration_test_passed_with_time(name, file, time)` - テスト成功の記録

#### 3. main.cppへの登録 (`tests/integration/main.cpp`)

作成したテストを必ず`main.cpp`に登録してください。

**手順:**

1. **includeの追加:**
```cpp
#include "<feature_name>/test_<feature_name>.hpp"
```

2. **テスト関数の呼び出し:**
```cpp
// 適切なカテゴリセクションに追加
std::cout << "\n[integration-test] === <Category Name> ===" << std::endl;
CategoryTimingStats::set_current_category("<Category Name>");
run_test_with_continue(test_integration_<feature_name>, "<Feature Name> Tests",
                       failed_tests);
```

**例: 文字列補間の場合**
```cpp
// ファイル上部
#include "string_interpolation/test_string_interpolation.hpp"

// テスト実行部分
std::cout << "\n[integration-test] === String & I/O Tests ===" << std::endl;
CategoryTimingStats::set_current_category("String & I/O");
run_test_with_continue(test_integration_string, "String Tests", failed_tests);
run_test_with_continue(test_integration_string_interpolation, "String Interpolation Tests", failed_tests);
```

### テスト内でのバリデーション

Cbテストファイル内では、2つの方法でバリデーションを行います：

#### 方法1: `assert()`を使用（推奨）

テストケース内で直接検証を行います：

```cb
void main() {
    println("=== Feature Test ===");
    
    // Test 1
    int result = calculate(5);
    assert(result == 25);
    println("Test 1: Calculation - PASSED");
    
    // Test 2
    string output = format("Value: {}", 42);
    assert(output == "Value: 42");
    println("Test 2: Format - PASSED");
    
    println("=== All Tests Passed ===");
}
```

#### 方法2: 期待する出力をhppでハンドリング

Integration test側で出力を検証します：

```cpp
run_cb_test_with_output_and_time("../../tests/cases/feature/test.cb", 
    [](const std::string& output, int exit_code) {
        INTEGRATION_ASSERT_EQ(0, exit_code, "Should execute successfully");
        
        // 期待する出力の検証
        INTEGRATION_ASSERT_CONTAINS(output, "Test 1: Calculation - PASSED", "Test 1 should pass");
        INTEGRATION_ASSERT_CONTAINS(output, "Test 2: Format - PASSED", "Test 2 should pass");
        INTEGRATION_ASSERT_CONTAINS(output, "=== All Tests Passed ===", "All tests should complete");
        
        // 具体的な値の検証
        INTEGRATION_ASSERT_CONTAINS(output, "Result: 42", "Should output correct value");
    }, execution_time);
```

### テストの実行

```bash
# すべてのテストを実行
make test

# Integration testのみ実行
make integration-test

# Unit testのみ実行
make unit-test

# 個別のCbファイルを実行
./main tests/cases/feature/test_basic.cb
```

### テストのベストプラクティス

1. **テストファイルの命名規則:**
   - `test_<feature>.cb` - 機能別テスト
   - `test_basic.cb` - 基本的な機能テスト
   - `test_edge_cases.cb` - エッジケース
   - `test_error_handling.cb` - エラーケース

2. **テストの構造:**
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

3. **アサーションの使用:**
   - すべての検証に`assert()`を使用
   - エラーメッセージは明確に記述
   - テストが失敗した場合、どのテストが失敗したかすぐに分かるようにする

4. **エラーテストの記述:**
   - エラーが期待される場合も明示的にテストする
   - Integration testで`exit_code != 0`を検証

5. **テストの独立性:**
   - 各テストケースは独立して実行可能であること
   - テスト間で状態を共有しないこと

---

## コードスタイル

### C++コード

#### インデント
- スペース4つを使用
- タブは使用しない

#### 命名規則
- **関数名**: `snake_case`
  ```cpp
  void evaluate_expression();
  TypedValue parse_interpolated_string();
  ```

- **クラス名**: `PascalCase`
  ```cpp
  class ExpressionEvaluator;
  class RecursiveParser;
  ```

- **変数名**: `snake_case`
  ```cpp
  int node_count;
  std::string file_path;
  ```

- **定数**: `UPPER_SNAKE_CASE`
  ```cpp
  const int MAX_DEPTH = 100;
  ```

- **メンバー変数**: `snake_case_` (末尾にアンダースコア)
  ```cpp
  private:
      Interpreter &interpreter_;
      TypeInferenceEngine type_engine_;
  ```

#### コメント
```cpp
// 単行コメント: 簡潔な説明

/**
 * @brief 複数行コメント: 詳細な説明
 * 
 * 関数の目的、パラメータ、戻り値を説明
 * 
 * @param node ASTノード
 * @return 評価結果
 */
TypedValue evaluate_expression(const ASTNode *node);
```

#### ファイル構造
```cpp
// 1. インクルードガード
#pragma once

// 2. インクルード (標準ライブラリ → サードパーティ → プロジェクト内)
#include <string>
#include <vector>
#include "common/ast.h"

// 3. 前方宣言
class Interpreter;

// 4. クラス/関数定義
class MyClass {
public:
    // public メンバー
    
private:
    // private メンバー
};
```

### Cb言語コード

#### インデント
- スペース4つを使用

#### 命名規則
- **関数名**: `snake_case`
- **変数名**: `snake_case`
- **構造体名**: `PascalCase`
- **定数**: `UPPER_SNAKE_CASE`

```cb
struct Person {
    string name;
    int age;
};

const int MAX_SIZE = 100;

int calculate_sum(int a, int b) {
    int result = a + b;
    return result;
}

void main() {
    Person p;
    p.name = "Alice";
    p.age = 30;
}
```

---

## 命名規則

### ファイル名

- **C++ソースファイル**: `snake_case.cpp`
  - 例: `evaluator.cpp`, `type_inference.cpp`

- **C++ヘッダーファイル**: `snake_case.h` または `snake_case.hpp`
  - 例: `evaluator.h`, `integration_test_framework.hpp`

- **Cbソースファイル**: `snake_case.cb`
  - 例: `test_basic.cb`, `string_interpolation.cb`

### ディレクトリ名

- すべて小文字、アンダースコア区切り
- 例: `string_interpolation/`, `default_args/`, `test_cases/`

---

## ドキュメント

### 新機能の追加時

新機能を実装する際は、以下のドキュメントを作成・更新してください：

1. **機能仕様書** (`docs/features/<feature_name>.md`)
   - 機能の概要
   - 構文
   - 使用例
   - 実装詳細

2. **実装レポート** (`docs/<feature_name>_implementation_report.md`)
   - 実装の経緯
   - 設計判断
   - 課題と解決策

3. **README更新** (`README.md`)
   - 機能一覧への追加

4. **リリースノート** (`release_notes/`)
   - バージョン別の変更点

### ドキュメントのフォーマット

```markdown
# 機能名

## 概要
機能の簡潔な説明

## 構文
\`\`\`cb
// 構文例
\`\`\`

## 使用例
\`\`\`cb
// 具体的な使用例
\`\`\`

## 実装詳細
- Lexer: ...
- Parser: ...
- Evaluator: ...

## テスト
- テストファイル: `tests/cases/feature/`
- Integration test: `tests/integration/feature/`

## 制限事項
既知の制限や今後の拡張予定
```

---

## コミット規約

### コミットメッセージフォーマット

```
<type>(<scope>): <subject>

<body>

<footer>
```

### Type
- `feat`: 新機能
- `fix`: バグ修正
- `docs`: ドキュメントのみの変更
- `style`: コードの意味に影響しない変更（空白、フォーマット等）
- `refactor`: バグ修正や機能追加ではないコード変更
- `test`: テストの追加や修正
- `chore`: ビルドプロセスやツールの変更

### Scope
- 変更の影響範囲（例: `parser`, `evaluator`, `string_interpolation`）

### Subject
- 変更の簡潔な説明（50文字以内）
- 動詞で始める（例: "Add", "Fix", "Update"）

### 例

```
feat(string_interpolation): Add string interpolation feature

- Implement {expression} syntax in string literals
- Add format specifiers (:x, :.2, etc.)
- Support escape sequences ({{ and }})
- Add comprehensive test suite

Tests: 80 integration tests added
Files: tests/cases/string_interpolation/*, tests/integration/string_interpolation/
```

### コミット前のチェックリスト

- [ ] すべてのテストが通ること (`make test`)
- [ ] コードフォーマットが適用されていること (`make fmt`)
- [ ] 新機能にはテストが追加されていること
- [ ] ドキュメントが更新されていること
- [ ] コミットメッセージが規約に従っていること

---

## 開発ワークフロー

### 新機能の実装手順

1. **設計フェーズ**
   - 機能仕様を`docs/features/`に作成
   - 実装方針を決定

2. **実装フェーズ**
   - コードを実装
   - コードフォーマット適用 (`make fmt`)

3. **テストフェーズ** ⚠️ **最重要**
   - `tests/cases/<feature>/` にCbテストを作成
   - `tests/integration/<feature>/` にIntegration testを作成
   - `tests/integration/main.cpp` にテストを登録
   - すべてのテストが通ることを確認 (`make test`)

4. **ドキュメントフェーズ**
   - 機能ドキュメントの更新
   - READMEの更新
   - リリースノートの作成

5. **レビューフェーズ**
   - コミット前チェックリストを確認
   - コミットメッセージを作成
   - コミット実行

### コマンド一覧

```bash
# ビルド
make              # メインビルド
make clean        # クリーンビルド

# テスト
make test         # 全テスト実行
make integration-test  # Integration testのみ
make unit-test    # Unit testのみ

# フォーマット
make fmt          # コードフォーマット適用

# 個別実行
./main <file.cb>  # Cbファイルの実行
```

---

## トラブルシューティング

### テストが失敗する場合

1. **個別のCbファイルを直接実行して確認**
   ```bash
   ./main tests/cases/feature/test_basic.cb
   ```

2. **エラーメッセージを確認**
   - 構文エラー: Parserの問題
   - 実行時エラー: Evaluatorの問題
   - アサーション失敗: テストロジックの問題

3. **Integration testのログを確認**
   ```bash
   make integration-test 2>&1 | grep -A 20 "Feature Tests"
   ```

### コンパイルエラーの場合

1. **クリーンビルド**
   ```bash
   make clean && make
   ```

2. **依存関係の確認**
   - includeパスが正しいか
   - 必要なヘッダーがすべて含まれているか

---

## 参考資料

- [言語仕様](docs/spec.md)
- [アーキテクチャ](docs/architecture.md)
- [機能一覧](docs/features/)
- [リリースノート](release_notes/)

---

**最終更新: 2025年10月27日**
**バージョン: v0.11.0**
