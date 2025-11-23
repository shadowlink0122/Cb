# v0.14.0 テスト方法の見直し - 実装サマリー

**作成日**: 2025-11-16
**ステータス**: 実装中

---

## 概要

v0.14.0では、インタプリタモードとコンパイラモード（`-c`オプション）の両方をサポートするため、テストフレームワークを刷新しました。

---

## 完成した成果物

### 1. テスト作成手順書

**ファイル**: `docs/testing/test_creation_guide.md`

**内容**:
- テストの種類（Integration Test / Unit Test）の説明
- ディレクトリ構造の定義
- 実行モード（インタプリタ/コンパイラ）の説明
- Integration Testの作成手順
- Unit Testの作成手順
- ベストプラクティス

### 2. 新しいテストフレームワーク（v2）

**ファイル**: `tests/integration/framework/integration_test_framework_v2.hpp`

**主な機能**:
```cpp
// 実行モードの設定
enum class ExecutionMode {
    Interpreter,  // インタプリタモード（デフォルト）
    Compiler,     // コンパイラモード（-c オプション）
    Both          // 両方実行
};

// テスト設定
IntegrationTestConfig::set_execution_mode(ExecutionMode::Compiler);
IntegrationTestConfig::set_cb_executable_path("../../main");

// モード指定テスト実行
run_cb_test_with_output(test_file, validator, ExecutionMode::Compiler);

// 両モードでテスト
run_cb_test_with_output_both_modes(test_file, validator);
```

**改善点**:
- ✅ 実行コマンドの設定が可能
- ✅ インタプリタ/コンパイラモードを選択可能
- ✅ 両モードで同じテストを実行可能
- ✅ 既存のテストと互換性を維持

### 3. テストフレームワーク使用例

**ファイル**: `tests/integration/example_v2_test.cpp`

**例**:
```cpp
// インタプリタモードのみ
void test_interpreter_only() {
    run_cb_test_with_output("test.cb", validator, ExecutionMode::Interpreter);
}

// コンパイラモードのみ
void test_compiler_only() {
    run_cb_test_with_output("test.cb", validator, ExecutionMode::Compiler);
}

// 両モードで実行
void test_both_modes() {
    run_cb_test_with_output_both_modes("test.cb", validator);
}
```

### 4. Unit Testディレクトリ構造

```
tests/unit/
├── hir/                   # HIR関連のテスト ✅ 作成済み
│   ├── test_hir_generator.cpp
│   ├── test_hir_visitor.cpp (TODO)
│   └── test_hir_dumper.cpp (TODO)
├── mir/                   # MIR関連のテスト
│   ├── test_mir_generator.cpp (TODO)
│   ├── test_cfg_builder.cpp (TODO)
│   └── test_ssa_builder.cpp (TODO)
├── lir/                   # LIR関連のテスト
│   └── test_lir_generator.cpp (TODO)
└── common/                # 共通機能のテスト
    ├── test_error_reporter.cpp ✅ 既存
    └── test_type_system.cpp (TODO)
```

### 5. HIR Unit Testサンプル

**ファイル**: `tests/unit/hir/test_hir_generator.cpp`

**テスト内容**:
- リテラルの変換
- 変数参照の変換
- 二項演算の変換
- 関数定義の変換
- プログラム全体の変換

**現在の状態**: 実装済みだが、privateメソッドのアクセス問題で未解決

---

## 残タスク

### 優先度：高

1. **HIRGeneratorのテストアクセス問題の解決**
   - 方法1: テスト用のpublicメソッドを追加
   - 方法2: friend宣言を使用
   - 方法3: テスト用のラッパークラスを作成

2. **既存のintegration testをv2フレームワークに移行**
   - `tests/integration/main.cpp`を更新
   - 各テストで実行モードを設定

3. **Unit Test用のMakefileの整備**
   - `tests/unit/Makefile`を作成
   - HIR/MIR/LIR個別のターゲットを追加

### 優先度：中

4. **追加のunit testの作成**
   - HIR Visitor
   - HIR Dumper
   - MIR Generator（v0.14.0の次フェーズ）

5. **コンパイラモード専用のテストケース作成**
   - HIR生成の検証
   - 型情報の検証
   - ソース位置情報の検証

6. **パフォーマンステストの追加**
   - インタプリタ vs コンパイラの実行時間比較
   - メモリ使用量の測定

### 優先度：低

7. **テストカバレッジツールの導入**
   - gcovの設定
   - カバレッジレポートの自動生成

8. **CI/CDパイプラインの設定**
   - GitHub Actionsでの自動テスト実行

---

## 使い方

### Integration Testの実行（v2フレームワーク）

```bash
# サンプルテストのビルドと実行
cd tests/integration
g++ -std=c++17 -I../../src -o example_v2_test.out example_v2_test.cpp
./example_v2_test.out
```

**期待される出力**:
```
=== v0.14.0 Integration Test Framework Example ===

--- Testing INTERPRETER mode ---
[integration-test] [PASS] [INTERPRETER] 算術演算テスト（インタプリタのみ）

--- Testing COMPILER mode ---
[integration-test] [PASS] [COMPILER] HIR生成テスト（コンパイラのみ）

=== Test Summary ===
Total:  X
Passed: X
Failed: 0
```

### Unit Testの実行（HIR）

```bash
cd tests/unit
make -f Makefile.hir test-hir
```

**現在のステータス**: privateアクセス問題により未動作

---

## 移行計画

### フェーズ1: 基盤整備（完了）

- [x] テスト作成手順書の作成
- [x] 新しいテストフレームワーク（v2）の実装
- [x] サンプルテストの作成
- [x] unit testディレクトリ構造の作成
- [x] HIR unit testサンプルの作成

### フェーズ2: 問題解決（次のステップ）

- [ ] HIRGeneratorのテストアクセス問題を解決
- [ ] Unit testのビルドと実行を成功させる
- [ ] 既存テストの1つをv2フレームワークに移行（PoC）

### フェーズ3: 全面移行

- [ ] すべてのintegration testをv2に移行
- [ ] 新しいunit testの追加
- [ ] ドキュメントの更新

### フェーズ4: 拡張

- [ ] MIR/LIRのunit test追加
- [ ] パフォーマンステスト追加
- [ ] CI/CD統合

---

## 設計上の決定事項

### 1. 実行モードの分離

**決定**: インタプリタとコンパイラで同じテストケース（`.cb`ファイル）を使用するが、検証内容を変える

**理由**:
- テストケースの重複を避ける
- 両モードの互換性を保証
- メンテナンスコストを削減

### 2. ディレクトリ構造

**決定**: 機能ごとにunit testをフォルダ分け

```
tests/unit/
├── hir/      # HIR関連
├── mir/      # MIR関連
├── lir/      # LIR関連
└── common/   # 共通機能
```

**理由**:
- テストの整理と検索が容易
- 機能追加時に対応するテストフォルダに追加するだけ
- ビルドターゲットを個別に設定可能

### 3. テストフレームワークの後方互換性

**決定**: 既存のフレームワーク（v1）を残しつつ、v2を並行運用

**理由**:
- 既存テストを壊さない
- 段階的な移行が可能
- v1とv2の比較ができる

---

## 次のアクション

### 即座に実施すべきこと

1. **HIRGeneratorのアクセス制御を調整**
   ```cpp
   // hir_generator.h
   class HIRGenerator {
   public:
       // テスト用に公開
       #ifdef UNIT_TEST
       hir::HIRExpr convert_expr_for_test(const ASTNode* node) {
           return convert_expr(node);
       }
       #endif

   private:
       hir::HIRExpr convert_expr(const ASTNode* node);
   };
   ```

2. **ASTNodeのテストヘルパーを修正**
   ```cpp
   std::unique_ptr<ASTNode> create_number_node(int value) {
       auto node = std::make_unique<ASTNode>(ASTNodeType::AST_NUMBER);
       node->int_value = value;
       node->type_info = TYPE_INT;
       return node;
   }
   ```

3. **簡単なPoCテストを1つ動かす**
   - 最もシンプルなintegration testを選ぶ
   - v2フレームワークで実行
   - 両モード（インタプリタ/コンパイラ）で動作確認

---

## まとめ

v0.14.0のテスト方法見直しは、以下を実現します：

1. **両モード対応**: インタプリタとコンパイラで同じテストを実行
2. **整理された構造**: 機能ごとにunit testを分離
3. **柔軟な設定**: 実行モードをテストごとに設定可能
4. **段階的移行**: 既存テストを壊さずに新フレームワークへ移行

現在は**フェーズ1（基盤整備）**が完了し、**フェーズ2（問題解決）**に移行する段階です。
