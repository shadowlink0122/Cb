# Cb 統合テスト (Integration Tests)

## 概要

このディレクトリには、**Cb言語の機能をエンドツーエンド**でテストする統合テストが含まれています。

## 統合テストの目的

統合テストは「Cb言語のユーザー視点」でのテストです。

- ✅ **テストすること**: Cbプログラムが正しく動作するか
- ❌ **テストしないこと**: HIR/MIR/LIRなどの内部実装の詳細

## テスト構造

### テストケース (各ディレクトリ)

統合テストは機能ごとにディレクトリで分類されています：

- `arithmetic/` - 算術演算
- `array/` - 配列
- `struct/` - 構造体
- `pointer/` - ポインタ
- `generics/` - ジェネリクス
- `async/` - 非同期処理
- `ffi/` - 外部関数インターフェース
- など...

各ディレクトリには以下のファイルが含まれます：

```
test_feature/
  ├── test_feature.hpp       # テストコード（C++）
  └── feature.cb              # テスト対象のCbプログラム
```

### 実行モード

統合テストは2つのモードで実行できます：

1. **インタプリタモード** (デフォルト)
   - Cbプログラムを実行し、出力を検証
   - コマンド: `./main test.cb`

2. **コンパイラモード**
   - Cbプログラムをコンパイルし、成功を検証
   - コマンド: `./main -c test.cb`

## テスト実行方法

### すべてのテストを実行
```bash
cd tests/integration
make test
```

### 特定のテストのみ実行
```bash
cd tests/integration
./main  # ビルドされた統合テストバイナリを実行
```

### テストのビルド
```bash
cd tests/integration
make
```

## テスト作成ガイドライン

### 新しい統合テストを追加する手順

1. **ディレクトリを作成**
   ```bash
   mkdir tests/integration/new_feature
   ```

2. **Cbテストファイルを作成**
   ```bash
   # tests/integration/new_feature/new_feature.cb
   fn main(): int {
       println("Test");
       return 0;
   }
   ```

3. **テストコード（C++）を作成**
   ```cpp
   // tests/integration/new_feature/test_new_feature.hpp
   #pragma once
   #include "../framework/integration_test_framework.hpp"
   
   void test_new_feature() {
       run_cb_test_with_output("new_feature/new_feature.cb",
           [](const std::string& output, int exit_code) {
               INTEGRATION_ASSERT_EQ(0, exit_code, "正常終了すること");
               INTEGRATION_ASSERT_CONTAINS(output, "Test", "Testが出力されること");
           });
   }
   ```

4. **main.cppにテストを追加**
   ```cpp
   #include "new_feature/test_new_feature.hpp"
   
   int main() {
       // ...
       test_new_feature();
       // ...
   }
   ```

5. **テストを実行**
   ```bash
   make clean && make && ./main
   ```

## 統合テストで検証すること

### ✅ 検証すべきこと
- プログラムの実行結果（標準出力）
- 終了コード（成功/失敗）
- エラーメッセージ
- 言語機能の動作（構文、セマンティクス）

### ❌ 検証すべきでないこと
- HIR/MIR/LIRの生成内容
  → `tests/unit/hir/`, `tests/unit/mir/`, `tests/unit/lir/` で検証
- 最適化パスの詳細
  → `tests/unit/backend/` で検証
- 内部データ構造
  → `tests/unit/common/` で検証

## ユニットテストとの違い

| 項目 | 統合テスト | ユニットテスト |
|------|----------|--------------|
| **目的** | Cb言語機能の検証 | 内部コンポーネントの検証 |
| **対象** | Cbプログラムの実行結果 | HIR/MIR/LIR、最適化パスなど |
| **視点** | ユーザー視点 | 開発者視点 |
| **粒度** | 粗い（機能単位） | 細かい（関数/クラス単位） |
| **実行方法** | Cbファイルを実行 | C++テストコードを直接実行 |

## テストフレームワーク

### 使用可能なアサーション

```cpp
// 基本アサーション
INTEGRATION_ASSERT(condition, "message");
INTEGRATION_ASSERT_EQ(expected, actual, "message");
INTEGRATION_ASSERT_CONTAINS(output, "expected_text", "message");

// テスト実行ヘルパー
run_cb_test_with_output(file, validation_func);
run_cb_test_with_output_and_time(file, validation_func, exec_time);
```

詳細は `framework/integration_test_framework_v2.hpp` を参照してください。

## 参考

- ユニットテストについては `tests/unit/README.md` を参照
- テスト全体の方針については `tests/README.md` を参照
