# v0.14.0 テスト作成手順書

**作成日**: 2025-11-16
**対象バージョン**: v0.14.0以降

---

## 目次

1. [概要](#概要)
2. [テストの種類](#テストの種類)
3. [ディレクトリ構造](#ディレクトリ構造)
4. [実行モード](#実行モード)
5. [Integration Testの作成](#integration-testの作成)
6. [Unit Testの作成](#unit-testの作成)
7. [テストの実行](#テストの実行)
8. [ベストプラクティス](#ベストプラクティス)

---

## 概要

v0.14.0以降、Cbコンパイラは**インタプリタモード**と**コンパイラモード**の両方をサポートします。
テストは両方のモードで実行できるように設計する必要があります。

### 基本方針

1. **同じテストコードを両モードで実行**: インタプリタとコンパイラで同じ動作を保証
2. **機能ごとにunit testを分離**: HIR、MIR、LIRなど機能ごとにテストを整理
3. **自動テスト実行**: `make test`で全テストを実行

---

## テストの種類

### 1. Integration Test（統合テスト）

**目的**: Cb言語の実際のコードを実行して、期待される出力を検証

**対象**:
- 言語機能（構文、型システム、制御フロー）
- 標準ライブラリ
- エラーハンドリング
- パフォーマンス

**実行方法**:
```bash
cd tests/integration
make test                    # 全テスト実行
make test MODE=interpreter   # インタプリタモードのみ
make test MODE=compiler      # コンパイラモードのみ
```

### 2. Unit Test（ユニットテスト）

**目的**: 個別のコンポーネント（クラス、関数）の動作を検証

**対象**:
- HIR Generator
- MIR Builder
- LIR Generator
- 型システム
- パーサー

**実行方法**:
```bash
cd tests/unit
make test                    # 全unit test実行
make test TARGET=hir         # HIR関連のみ
```

---

## ディレクトリ構造

```
tests/
├── integration/                # 統合テスト
│   ├── framework/
│   │   └── integration_test_framework.hpp  # テストフレームワーク
│   ├── main.cpp               # テストランナー
│   └── Makefile
│
├── unit/                      # ユニットテスト
│   ├── hir/                   # HIR関連のテスト
│   │   ├── test_hir_generator.cpp
│   │   ├── test_hir_visitor.cpp
│   │   └── test_hir_dumper.cpp
│   ├── mir/                   # MIR関連のテスト
│   │   ├── test_mir_generator.cpp
│   │   ├── test_cfg_builder.cpp
│   │   └── test_ssa_builder.cpp
│   ├── lir/                   # LIR関連のテスト
│   │   └── test_lir_generator.cpp
│   ├── common/                # 共通機能のテスト
│   │   ├── test_error_reporter.cpp
│   │   └── test_type_system.cpp
│   ├── main.cpp               # ユニットテストランナー
│   └── Makefile
│
└── cases/                     # テストケース（Cbファイル）
    ├── basic/                 # 基本機能
    ├── async/                 # 非同期機能
    ├── generics/              # ジェネリクス
    ├── hir/                   # HIRテスト用
    └── errors/                # エラーケース
```

---

## 実行モード

### インタプリタモード（デフォルト）

```bash
./main test_file.cb
```

**特徴**:
- ASTを直接実行
- 高速起動
- デバッグしやすい

### コンパイラモード

```bash
./main -c test_file.cb
```

**特徴**:
- AST → HIR → MIR → LIR の変換を実行
- IR生成を検証
- 最適化の動作を確認

### テストフレームワークでの設定

```cpp
// v0.14.0: 実行モードを設定
IntegrationTestConfig::set_execution_mode(ExecutionMode::Interpreter);  // デフォルト
IntegrationTestConfig::set_execution_mode(ExecutionMode::Compiler);     // コンパイラモード
IntegrationTestConfig::set_execution_mode(ExecutionMode::Both);         // 両方実行
```

---

## Integration Testの作成

### ステップ1: テストケースファイルの作成

`tests/cases/`配下に`.cb`ファイルを作成します。

**例**: `tests/cases/basic/test_arithmetic.cb`

```cb
// 算術演算のテスト
int main() {
    int a = 10;
    int b = 20;
    int sum = a + b;
    println(sum);
    return 0;
}
```

### ステップ2: テストコードの作成

`tests/integration/main.cpp`にテストを追加します。

```cpp
#include "framework/integration_test_framework.hpp"

void test_arithmetic() {
    // v0.14.0: コンパイラモードとインタプリタモードの両方でテスト
    IntegrationTestConfig::set_execution_mode(ExecutionMode::Both);

    run_cb_test_with_output("../cases/basic/test_arithmetic.cb",
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "プログラムが正常終了すること");
            INTEGRATION_ASSERT_CONTAINS(output, "30", "30が出力されること");
        });

    integration_test_passed_with_time_auto("算術演算テスト", "test_arithmetic.cb");
}

int main() {
    IntegrationTestCounter::reset();
    TimingStats::reset();

    test_arithmetic();

    IntegrationTestCounter::print_summary();
    TimingStats::print_timing_summary();

    return IntegrationTestCounter::get_failed() > 0 ? 1 : 0;
}
```

### ステップ3: テストの実行

```bash
cd tests/integration
make test
```

---

## Unit Testの作成

### ステップ1: テストファイルの作成

機能ごとにディレクトリを分けて作成します。

**例**: `tests/unit/hir/test_hir_generator.cpp`

```cpp
#include "../../../src/backend/ir/hir/hir_generator.h"
#include "../../../src/common/ast.h"
#include <cassert>
#include <iostream>

// テストヘルパー: ASTノードを作成
std::unique_ptr<ASTNode> create_number_node(int value) {
    auto node = std::make_unique<ASTNode>();
    node->node_type = ASTNodeType::AST_NUMBER;
    node->int_value = value;
    node->type_info = TYPE_INT;
    return node;
}

// テスト: リテラルの変換
void test_literal_conversion() {
    cb::ir::HIRGenerator gen;

    // テスト用AST作成
    auto ast_node = create_number_node(42);

    // HIRに変換
    auto hir_expr = gen.convert_expr(ast_node.get());

    // 検証
    assert(hir_expr.kind == cb::ir::hir::HIRExpr::ExprKind::Literal);
    assert(hir_expr.literal_value == "42");

    std::cout << "[PASS] test_literal_conversion" << std::endl;
}

// テスト: 二項演算の変換
void test_binary_op_conversion() {
    cb::ir::HIRGenerator gen;

    // AST作成: 10 + 20
    auto left = create_number_node(10);
    auto right = create_number_node(20);

    auto binop = std::make_unique<ASTNode>();
    binop->node_type = ASTNodeType::AST_BINARY_OP;
    binop->op = "+";
    binop->left = std::move(left);
    binop->right = std::move(right);

    // HIRに変換
    auto hir_expr = gen.convert_expr(binop.get());

    // 検証
    assert(hir_expr.kind == cb::ir::hir::HIRExpr::ExprKind::BinaryOp);
    assert(hir_expr.op == "+");
    assert(hir_expr.left != nullptr);
    assert(hir_expr.right != nullptr);

    std::cout << "[PASS] test_binary_op_conversion" << std::endl;
}

int main() {
    std::cout << "=== HIR Generator Unit Tests ===" << std::endl;

    try {
        test_literal_conversion();
        test_binary_op_conversion();

        std::cout << "\n=== All tests passed ===" << std::endl;
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "\n[FAIL] Exception: " << e.what() << std::endl;
        return 1;
    }
}
```

### ステップ2: Makefileへの追加

`tests/unit/Makefile`に新しいテストターゲットを追加します。

```makefile
# HIR関連のテスト
HIR_TEST_SRCS = \
    hir/test_hir_generator.cpp \
    hir/test_hir_visitor.cpp \
    hir/test_hir_dumper.cpp

HIR_TEST_OBJS = $(HIR_TEST_SRCS:.cpp=.o)

test-hir: $(HIR_TEST_OBJS)
    @echo "Running HIR unit tests..."
    @for test in $(HIR_TEST_OBJS:.o=); do \
        g++ -o $$test.out $$test.o $(LDFLAGS); \
        ./$$test.out || exit 1; \
    done
```

### ステップ3: テストの実行

```bash
cd tests/unit
make test-hir
```

---

## テストの実行

### すべてのテストを実行

```bash
make test
```

これは以下を実行します：
1. Integration tests (インタプリタモード)
2. Integration tests (コンパイラモード)
3. Unit tests (all)

### 個別のテストカテゴリを実行

```bash
# Integration testのみ
cd tests/integration && make test

# Unit testのみ
cd tests/unit && make test

# HIR unit testのみ
cd tests/unit && make test-hir
```

### 特定のモードで実行

```bash
# インタプリタモードのみ
make test MODE=interpreter

# コンパイラモードのみ
make test MODE=compiler
```

---

## ベストプラクティス

### 1. テストケースの命名規則

```
test_<機能>_<詳細>.cb
```

**例**:
- `test_arithmetic_basic.cb` - 基本的な算術演算
- `test_if_statement_nested.cb` - ネストしたif文
- `test_generics_instantiation.cb` - ジェネリクスのインスタンス化

### 2. テストの独立性

各テストは独立して実行可能にする：
```cpp
void test_feature_a() {
    // セットアップ
    setup_test_environment();

    // テスト実行
    run_test();

    // クリーンアップ
    cleanup_test_environment();
}
```

### 3. エラーメッセージの明確化

```cpp
INTEGRATION_ASSERT_EQ(expected, actual,
    "関数fooは42を返すべきですが、実際には" + std::to_string(actual) + "を返しました");
```

### 4. パフォーマンステスト

実行時間を測定したい場合：
```cpp
run_cb_test_with_output_and_time(test_file, validator, execution_time);
TimingStats::add_time(execution_time);
```

### 5. カテゴリごとのテスト

関連するテストをグループ化：
```cpp
void run_arithmetic_tests() {
    CategoryTimingStats::set_current_category("Arithmetic");

    test_addition();
    test_subtraction();
    test_multiplication();

    CategoryTimingStats::print_category_summary("Arithmetic");
}
```

### 6. コンパイラモード専用のテスト

HIR/MIR/LIRの検証など、コンパイラモードでのみ有効なテスト：
```cpp
void test_hir_generation() {
    IntegrationTestConfig::set_execution_mode(ExecutionMode::Compiler);

    // HIR生成の検証
    // ...
}
```

---

## チェックリスト

新しいテストを追加する際のチェックリスト：

- [ ] テストケースファイル（`.cb`）を作成
- [ ] テストコードを作成
- [ ] 両方のモード（インタプリタ/コンパイラ）で動作確認
- [ ] エラーケースも含める
- [ ] テストが独立して実行可能
- [ ] 適切なエラーメッセージを設定
- [ ] ドキュメントを更新（必要に応じて）
- [ ] `make test`で全テストが通ることを確認

---

## トラブルシューティング

### テストが失敗する

1. **実行モードを確認**: インタプリタとコンパイラで動作が異なる可能性
2. **出力を確認**: 期待される出力と実際の出力を比較
3. **個別に実行**: 問題のあるテストだけを実行して調査

### パフォーマンス問題

1. **TimingStats**を使用して実行時間を測定
2. 遅いテストを特定して最適化
3. 必要に応じてテストを分割

### メモリリーク

```bash
valgrind ./test_main
```

---

## まとめ

v0.14.0以降のテストは：

1. **両モード対応**: インタプリタとコンパイラの両方で実行
2. **機能別整理**: HIR/MIR/LIRなど機能ごとにunit testを分離
3. **自動化**: `make test`で全テスト実行
4. **継続的改善**: 新機能追加時に対応するテストも追加

この手順書に従ってテストを作成することで、Cbコンパイラの品質を維持・向上できます。
