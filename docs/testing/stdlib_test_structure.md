# 標準ライブラリテスト構造

**作成日**: 2025年10月28日  
**目的**: Cb標準ライブラリ（stdlib/）のテスト体制整備

---

## 📖 概要

Cb標準ライブラリのテストは以下の2層構造で実施されます：

### 1. Cb言語レベルのテスト (`tests/cases/stdlib/`)
- **目的**: ユーザー視点での動作確認
- **方法**: 
  - **必ず`import`文でstdlibを読み込む**
  - stdlibのAPIを使用してテストを実行
  - 期待される出力を`println`で表示
- **実行**: `./main tests/cases/stdlib/<category>/<test>.cb`

### 2. C++統合テスト (`tests/stdlib/`)
- **目的**: Cbテストファイルの自動検証
- **方法**:
  - **Cbテストファイルを実行**
  - **出力内容を検証**（`STDLIB_ASSERT_CONTAINS`）
  - **exit codeを検証**（`STDLIB_ASSERT_EQ`）
- **実行**: `cd tests/stdlib && ./test_main`

### テストフロー図

```
1. stdlib/<category>/<module>.cb
   ↓ export
2. tests/cases/stdlib/<category>/<module>_test.cb
   ↓ import "stdlib/<category>/<module>.cb"
   ↓ 実行 (./main tests/cases/...)
3. tests/stdlib/<category>/<module>_test.hpp
   ↓ run_cb_test("../../tests/cases/stdlib/<category>/<module>_test.cb")
   ↓ 出力検証 (STDLIB_ASSERT_CONTAINS)
   ↓ exit code検証 (STDLIB_ASSERT_EQ)
4. tests/stdlib/main.cpp
   ↓ register_*_tests(runner)
   ↓ runner.run_all()
   → 成功/失敗
```

---

## 📂 ディレクトリ構造

```
tests/
├── cases/stdlib/              # Cb言語テスト（ユーザー視点）
│   ├── allocators/
│   │   ├── test_system_allocator.cb
│   │   └── test_bump_allocator.cb
│   ├── collections/
│   │   ├── test_vector.cb
│   │   ├── test_queue.cb      # 今後追加予定
│   │   ├── test_stack.cb      # 今後追加予定
│   │   └── test_map.cb        # 今後追加予定
│   └── async/                 # 今後追加予定
│       ├── test_task.cb
│       └── test_task_queue.cb
│
└── stdlib/                    # C++統合テスト（内部実装）
    ├── main.cpp               # テストランナー
    ├── framework/
    │   └── stdlib_test_framework.hpp
    ├── allocators/
    │   ├── test_system_allocator.hpp
    │   └── test_bump_allocator.hpp
    ├── collections/
    │   └── test_vector.hpp
    └── async/                 # 今後追加予定
        ├── test_task.hpp
        └── test_task_queue.hpp
```

---

## 🧪 テストの実行方法

### 全てのstdlibテストを実行

```bash
make stdlib-test
```

**実行内容**:
- C++統合テスト (10件)
- Cbテスト (3ファイル × 複数テスト)

### 個別実行

#### C++テストのみ

```bash
make stdlib-test-cpp
```

#### Cbテストのみ

```bash
make stdlib-test-cb
```

#### 特定の.cbファイルを実行

```bash
./main tests/cases/stdlib/test_vector.cb
```

---

## 📝 tests/cases/stdlib/ （Cb言語テスト）

### 目的

- **ユーザー視点での動作確認**
- **import/exportの検証**
- **stdlib APIの実際の使用例**

### 重要: テストの作成方法

**必須手順:**
1. `tests/cases/stdlib/<category>/*.cb` にCbテストファイルを作成
2. **stdlibファイルを`import`文で読み込む**（インライン定義は禁止）
3. importしたAPIを使用してテストを実行
4. `tests/stdlib/<category>/*.hpp` にC++統合テストを作成
5. C++テストで`.cb`ファイルを実行し、出力を検証
6. `tests/stdlib/main.cpp` にテスト関数を登録

### ディレクトリ構造

tests/cases/stdlib/ は以下のようにライブラリの種別ごとにフォルダ分けされています：

- **std/** - 基本型（Result, Option など）
- **allocators/** - メモリアロケータ関連
  - SystemAllocator, BumpAllocator など
- **collections/** - コレクション（データ構造）関連
  - Vector, Queue, Stack, Map など
- **async/** - 非同期処理関連（今後追加予定）
  - TaskQueue, EventLoop など
- **io/** - 入出力関連（今後追加予定）
  - File, Stream など

### ファイル構成

#### std/result_test.cb（例: Result型のテスト）

```cb
// ⚠️ 重要: stdlibファイルを必ずimportする
import "stdlib/std/result.cb";

void main() {
    println("=== Result<T, E> stdlib test ===");
    
    // Test 1: Result<int, string> - Ok variant
    Result<int, string> ok_result = Result<int, string>::Ok(42);
    assert(ok_result.variant == "Ok");
    assert(ok_result.value == 42);
    println("Test 1: Result<int, string>::Ok(42) - PASSED");
    
    // Test 2: Result<int, string> - Err variant
    Result<int, string> err_result = Result<int, string>::Err("error");
    assert(err_result.variant == "Err");
    assert(err_result.value == "error");
    println("Test 2: Result<int, string>::Err - PASSED");
    
    // Test 3: Pattern matching with Result
    match (ok_result) {
        Ok(value) => println("Match Ok: ", value),
        Err(error) => println("Match Err: ", error)
    }
    println("Test 3: Pattern matching - PASSED");
    
    println("=== All Result tests passed ===");
}
```

**テスト項目**:
- Result<T, E>構造体のimport確認
- Ok/Errバリアントの作成
- `.variant`と`.value`アクセス
- match文によるパターンマッチング

**⚠️ 禁止事項**:
```cb
// ❌ インライン定義は禁止
enum Result<T, E> {
    Ok(T),
    Err(E)
};

// ✅ 必ずimportを使用
import "stdlib/std/result.cb";
```

#### allocators/system_allocator.cb（例: SystemAllocatorのテスト）

```cb
// ⚠️ 重要: stdlibファイルを必ずimportする
import "stdlib/allocators/system_allocator.cb";

void test_system_allocator_basic() {
    SystemAllocator alloc;
    void* ptr = alloc.allocate(100);
    alloc.deallocate(ptr);
    println("✅ Test passed");
}

void main() {
    test_system_allocator_basic();
}
```

**テスト項目**:
- SystemAllocator構造体のimport
- allocate/deallocateの呼び出し
- Allocatorインターフェースの実装確認

#### allocators/bump_allocator.cb

```cb
import "stdlib/allocators/bump_allocator.cb";

void test_bump_allocator_init() {
    BumpAllocator alloc;
    bump_allocator_init(alloc, 1024);
    assert(alloc.buffer_size == 1024);
    println("✅ Test passed");
}

void main() {
    test_bump_allocator_init();
    // 他のテスト...
}
```

**テスト項目**:
- BumpAllocator構造体のimport
- 初期化関数の動作
- リセット機能
- deallocate無視の動作

#### collections/vector.cb

```cb
import "stdlib/collections/vector.cb";

void test_vector_init() {
    Vector<int, SystemAllocator> vec;
    vector_init_int_system(vec, 10);
    assert(vec.capacity == 10);
    println("✅ Test passed");
}

void main() {
    test_vector_init();
    // 他のテスト...
}
```

**テスト項目**:
- Vector構造体のimport
- ジェネリック型パラメータ
- 各種操作（push/pop/resize）
- 複数のAllocatorとの組み合わせ

---

## 🧪 tests/stdlib/ （C++統合テスト）

### 目的

- **Cbテストファイルの実行と検証**
- **出力内容の確認**
- **exit codeの検証**

### 重要: C++テストの作成方法

**必須手順:**
1. `tests/stdlib/<category>/*.hpp` にC++テストファイルを作成
2. **`tests/cases/stdlib/<category>/*.cb` を実行**
3. **出力内容を`STDLIB_ASSERT_*`マクロで検証**
4. **exit codeを検証**
5. `tests/stdlib/main.cpp` にテスト関数を登録

### ファイル構成

#### framework/stdlib_test_framework.hpp

テストフレームワークの提供:

```cpp
class StdlibTestRunner {
    void add_test(const std::string& name, std::function<void()> test_func);
    void run_all();
    bool all_passed() const;
};

// アサーションマクロ
#define STDLIB_ASSERT_TRUE(expr)
#define STDLIB_ASSERT_EQ(a, b)
#define STDLIB_ASSERT_CONTAINS(output, substring)
```

#### std/result_test.hpp（例: Result型のC++テスト）

```cpp
#pragma once

#include "../framework/stdlib_test_framework.hpp"
#include <cstdlib>
#include <string>
#include <sstream>

// ヘルパー関数: Cbテストファイルを実行して出力を取得
inline std::pair<std::string, int> run_cb_test(const std::string& test_file) {
    std::string command = "../../main " + test_file + " 2>&1";
    FILE* pipe = popen(command.c_str(), "r");
    if (!pipe) return {"", -1};
    
    std::stringstream output;
    char buffer[128];
    while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
        output << buffer;
    }
    
    int exit_code = pclose(pipe);
    return {output.str(), WEXITSTATUS(exit_code)};
}

inline void test_result_basic() {
    auto [output, exit_code] = run_cb_test("../../tests/cases/stdlib/std/result_test.cb");
    
    // exit codeの検証
    STDLIB_ASSERT_EQ(0, exit_code);
    
    // 出力内容の検証
    STDLIB_ASSERT_CONTAINS(output, "=== Result<T, E> stdlib test ===");
    STDLIB_ASSERT_CONTAINS(output, "Test 1: Result<int, string>::Ok(42) - PASSED");
    STDLIB_ASSERT_CONTAINS(output, "Test 2: Result<int, string>::Err - PASSED");
    STDLIB_ASSERT_CONTAINS(output, "Test 3: Pattern matching - PASSED");
    STDLIB_ASSERT_CONTAINS(output, "=== All Result tests passed ===");
}

inline void register_result_tests(StdlibTestRunner& runner) {
    runner.add_test("result_basic", test_result_basic);
}
```

**重要なポイント:**
1. **Cbテストファイルを実行**: `run_cb_test()` で `.cb` ファイルを実行
2. **出力を取得**: stdout/stderrを文字列として取得
3. **exit codeを検証**: 0 = 成功、0以外 = 失敗
4. **出力内容を検証**: 期待される文字列が含まれているか確認

#### allocators/system_allocator_test.hpp

```cpp
#pragma once

#include "../framework/stdlib_test_framework.hpp"

inline void test_system_allocator_execution() {
    // tests/cases/stdlib/allocators/system_allocator.cb を実行
    auto [output, exit_code] = run_cb_test(
        "../../tests/cases/stdlib/allocators/system_allocator.cb");
    
    // exit codeの検証（正常終了を期待）
    STDLIB_ASSERT_EQ(0, exit_code);
    
    // 出力内容の検証
    STDLIB_ASSERT_CONTAINS(output, "✅ Test passed");
}

inline void register_system_allocator_tests(StdlibTestRunner& runner) {
    runner.add_test("system_allocator_execution", 
                    test_system_allocator_execution);
}
```

#### main.cpp

```cpp
#include <iostream>
#include "framework/stdlib_test_framework.hpp"

// テストファイルのinclude
#include "std/result_test.hpp"
#include "std/option_test.hpp"
#include "allocators/system_allocator_test.hpp"
#include "allocators/bump_allocator_test.hpp"
#include "collections/vector_test.hpp"

int main() {
    StdlibTestRunner runner;
    
    std::cout << "=== Stdlib Tests ===" << std::endl;
    
    // 各カテゴリのテストを登録
    std::cout << "\n[std]" << std::endl;
    register_result_tests(runner);
    register_option_tests(runner);
    
    std::cout << "\n[allocators]" << std::endl;
    register_system_allocator_tests(runner);
    register_bump_allocator_tests(runner);
    
    std::cout << "\n[collections]" << std::endl;
    register_vector_tests(runner);
    
    // 全テストを実行
    runner.run_all();
    
    return runner.all_passed() ? 0 : 1;
}
```

**登録の流れ:**
1. `#include` で `.hpp` ファイルを読み込む
2. `register_*_tests(runner)` でテスト関数を登録
3. `runner.run_all()` で全テストを実行
4. 結果に応じて exit code を返す (0 = 成功, 1 = 失敗)

---

## 🔧 Makefileターゲット

### make stdlib-test

全てのstdlibテストを実行:

```makefile
stdlib-test: stdlib-test-cpp stdlib-test-cb
	@echo "✅ All stdlib tests completed!"
```

### make stdlib-test-cpp

C++統合テストのみ実行:

```makefile
stdlib-test-cpp: $(TESTS_DIR)/stdlib/test_main
	@cd tests/stdlib && ./test_main
```

### make stdlib-test-cb

Cb言語テストのみ実行:

```makefile
stdlib-test-cb: $(MAIN_TARGET)
	@./$(MAIN_TARGET) tests/cases/stdlib/test_system_allocator.cb
	@./$(MAIN_TARGET) tests/cases/stdlib/test_bump_allocator.cb
	@./$(MAIN_TARGET) tests/cases/stdlib/test_vector.cb
```

### make test

全てのテストを実行（stdlib含む）:

```makefile
test: integration-test unit-test stdlib-test
	@echo "=== Test Summary ==="
	@echo "Integration tests: completed"
	@echo "Unit tests: completed"
	@echo "Stdlib tests: completed"
```

---

## 🔄 CI統合

### .github/workflows/ci.yml

stdlib-testジョブを追加:

```yaml
stdlib-test:
  runs-on: ubuntu-latest
  needs: build
  steps:
    - uses: actions/checkout@v3
      with:
        clean: true
    - name: Install build tools
      run: sudo apt-get update && sudo apt-get install -y bison flex g++
    - name: Run stdlib tests (make stdlib-test)
      run: make stdlib-test
```

**実行順序**:
1. lint (コードフォーマット)
2. build (ビルド)
3. unit-test (単体テスト)
4. integration-test (統合テスト)
5. **stdlib-test** (標準ライブラリテスト)

---

## 📊 テストカバレッジ

### 現在のカバレッジ

| モジュール | Cbテスト | C++テスト | カバレッジ |
|-----------|---------|----------|----------|
| SystemAllocator | ✅ 5件 | ✅ 2件 | 80% |
| BumpAllocator | ✅ 4件 | ✅ 3件 | 85% |
| Vector | ✅ 5件 | ✅ 5件 | 70% |
| **合計** | **14件** | **10件** | **75%** |

### 目標カバレッジ

- **Week 3終了時**: 90%
- **v0.12.0リリース時**: 95%

---

## 🎯 テスト追加手順（完全版）

### 新しいstdlibモジュールを追加する場合

#### ステップ1: stdlibファイルにexportを追加

```cb
// stdlib/std/new_type.cb
export enum NewType<T> {
    Variant1(T),
    Variant2
};
```

#### ステップ2: Cbテストファイル作成（重要）

**⚠️ 必ずimportを使用:**

```cb
// tests/cases/stdlib/std/new_type_test.cb
import "stdlib/std/new_type.cb";  // ← 必須！

void main() {
    println("=== NewType test ===");
    
    // Test 1: Variant1の作成
    NewType<int> v1 = NewType<int>::Variant1(42);
    assert(v1.variant == "Variant1");
    assert(v1.value == 42);
    println("Test 1: Variant1 - PASSED");
    
    // Test 2: Variant2の作成
    NewType<int> v2 = NewType<int>::Variant2;
    assert(v2.variant == "Variant2");
    println("Test 2: Variant2 - PASSED");
    
    // Test 3: Pattern matching
    match (v1) {
        Variant1(value) => println("Matched Variant1: ", value),
        Variant2 => println("Matched Variant2")
    }
    println("Test 3: Pattern matching - PASSED");
    
    println("=== All NewType tests passed ===");
}
```

**配置場所:**
- `tests/cases/stdlib/<category>/` 配下
- 例: `tests/cases/stdlib/std/new_type_test.cb`

#### ステップ3: C++テストファイル作成

```cpp
// tests/stdlib/std/new_type_test.hpp
#pragma once

#include "../framework/stdlib_test_framework.hpp"

inline void test_new_type_basic() {
    // Cbテストファイルを実行
    auto [output, exit_code] = run_cb_test(
        "../../tests/cases/stdlib/std/new_type_test.cb");
    
    // exit codeの検証
    STDLIB_ASSERT_EQ(0, exit_code);
    
    // 出力内容の検証
    STDLIB_ASSERT_CONTAINS(output, "=== NewType test ===");
    STDLIB_ASSERT_CONTAINS(output, "Test 1: Variant1 - PASSED");
    STDLIB_ASSERT_CONTAINS(output, "Test 2: Variant2 - PASSED");
    STDLIB_ASSERT_CONTAINS(output, "Test 3: Pattern matching - PASSED");
    STDLIB_ASSERT_CONTAINS(output, "=== All NewType tests passed ===");
}

inline void register_new_type_tests(StdlibTestRunner& runner) {
    runner.add_test("new_type_basic", test_new_type_basic);
}
```

**配置場所:**
- `tests/stdlib/<category>/` 配下
- 例: `tests/stdlib/std/new_type_test.hpp`

#### ステップ4: main.cppに登録

```cpp
// tests/stdlib/main.cpp
#include "std/new_type_test.hpp"  // 追加

int main() {
    StdlibTestRunner runner;
    
    std::cout << "\n[std]" << std::endl;
    register_result_tests(runner);
    register_option_tests(runner);
    register_new_type_tests(runner);  // 追加
    
    // ... 他のテスト登録 ...
    
    runner.run_all();
    return runner.all_passed() ? 0 : 1;
}
```

#### ステップ5: Makefileに追加（任意）

特定のカテゴリテストを個別実行したい場合:

```makefile
stdlib-test-std: $(MAIN_TARGET)
	@echo "\n[std types]"
	@./$(MAIN_TARGET) tests/cases/stdlib/std/result_test.cb
	@./$(MAIN_TARGET) tests/cases/stdlib/std/option_test.cb
	@./$(MAIN_TARGET) tests/cases/stdlib/std/new_type_test.cb
```

#### ステップ6: テストの実行確認

```bash
# Cbテストを個別実行
./main tests/cases/stdlib/std/new_type_test.cb

# C++統合テストを実行
cd tests/stdlib && ./test_main

# 全stdlibテストを実行
make stdlib-test
```

---

## ✅ チェックリスト

新しいstdlibモジュール追加時:

- [ ] `stdlib/<category>/<module>.cb` にstdlibファイルを作成
- [ ] stdlibファイルに`export`キーワードを追加
- [ ] モジュールのカテゴリを決定（std/allocators/collections/async/io など）
- [ ] `tests/cases/stdlib/<category>/<module>_test.cb` にCbテスト作成
  - ⚠️ **必ず`import "stdlib/<category>/<module>.cb";`を記述**
  - ⚠️ **インライン定義は禁止**
- [ ] Cbテストファイルを個別実行して動作確認 (`./main tests/cases/stdlib/...`)
- [ ] `tests/stdlib/<category>/<module>_test.hpp` にC++テスト作成
  - ⚠️ **Cbテストファイルを`run_cb_test()`で実行**
  - ⚠️ **出力内容を`STDLIB_ASSERT_CONTAINS`で検証**
  - ⚠️ **exit codeを`STDLIB_ASSERT_EQ(0, exit_code)`で検証**
- [ ] `tests/stdlib/main.cpp` に`#include`とテスト登録を追加
- [ ] C++テストをビルドして実行 (`cd tests/stdlib && make && ./test_main`)
- [ ] Makefileの適切なセクションに追加（任意）
- [ ] テストが `make stdlib-test` で実行可能
- [ ] CIで自動実行される
- [ ] カバレッジ80%以上

---

## 🐛 トラブルシューティング

### エラー: "import path not found"

**原因**: import文のパスが不正

**解決**:
```cb
// ❌ 間違い
import "result.cb";
import "std/result.cb";

// ✅ 正しい（stdlib/から始める）
import "stdlib/std/result.cb";
import "stdlib/allocators/system_allocator.cb";
import "stdlib/collections/vector.cb";
```

### エラー: "symbol not exported"

**原因**: stdlibファイルにexportが不足

**解決**:
```cb
// stdlib/std/result.cb

// ❌ exportがない
enum Result<T, E> {
    Ok(T),
    Err(E)
};

// ✅ exportを追加
export enum Result<T, E> {
    Ok(T),
    Err(E)
};
```

### エラー: "Undefined variable: Result"

**原因1**: importを忘れている

**解決**:
```cb
// ❌ importがない
void main() {
    Result<int, string> r = Result<int, string>::Ok(42);
}

// ✅ importを追加
import "stdlib/std/result.cb";

void main() {
    Result<int, string> r = Result<int, string>::Ok(42);
}
```

**原因2**: インライン定義を使っている（禁止）

**解決**:
```cb
// ❌ インライン定義（stdlibテストでは禁止）
enum Result<T, E> { Ok(T), Err(E) };

void main() {
    Result<int, string> r = Result<int, string>::Ok(42);
}

// ✅ importを使用
import "stdlib/std/result.cb";

void main() {
    Result<int, string> r = Result<int, string>::Ok(42);
}
```

### C++テストが失敗する

**原因**: Cbテストの出力が期待と異なる

**デバッグ手順**:

1. **Cbテストを個別実行して出力を確認**:
```bash
./main tests/cases/stdlib/std/result_test.cb
```

2. **期待される出力を確認**:
```
=== Result<T, E> stdlib test ===
Test 1: Result<int, string>::Ok(42) - PASSED
...
=== All Result tests passed ===
```

3. **C++テストの期待値を修正**:
```cpp
STDLIB_ASSERT_CONTAINS(output, "=== Result<T, E> stdlib test ===");
STDLIB_ASSERT_CONTAINS(output, "Test 1: Result<int, string>::Ok(42) - PASSED");
STDLIB_ASSERT_CONTAINS(output, "=== All Result tests passed ===");
```

### exit codeが期待と異なる

**確認**:
```bash
./main tests/cases/stdlib/std/result_test.cb
echo $?  # exit codeを表示
```

**正常終了**: exit code == 0  
**エラー終了**: exit code != 0

**C++テストでの検証**:
```cpp
// 正常終了を期待
STDLIB_ASSERT_EQ(0, exit_code);

// エラーを期待（エラーケーステスト）
STDLIB_ASSERT_NE(0, exit_code);
```

### ファイルが見つからない

**原因**: パスが間違っている

**解決**:
```bash
# ❌ 古いパス
./main tests/cases/stdlib/test_result.cb

# ✅ 新しいパス（カテゴリフォルダ含む）
./main tests/cases/stdlib/std/result_test.cb
```

**C++テストでのパス**:
```cpp
// tests/stdlib/ から実行されることを前提
auto [output, exit_code] = run_cb_test(
    "../../tests/cases/stdlib/std/result_test.cb");
```

---

## 📚 完全な実装例

### 例: Result型のstdlibテスト

#### 1. stdlib/std/result.cb（既存）

```cb
export enum Result<T, E> {
    Ok(T),
    Err(E)
};
```

#### 2. tests/cases/stdlib/std/result_test.cb（新規作成）

```cb
import "stdlib/std/result.cb";  // ← 必須！

void main() {
    println("=== Result<T, E> stdlib test ===");
    
    // Test 1
    Result<int, string> ok_result = Result<int, string>::Ok(42);
    assert(ok_result.variant == "Ok");
    assert(ok_result.value == 42);
    println("Test 1: Result::Ok - PASSED");
    
    // Test 2
    Result<int, string> err_result = Result<int, string>::Err("error");
    assert(err_result.variant == "Err");
    assert(err_result.value == "error");
    println("Test 2: Result::Err - PASSED");
    
    // Test 3: Pattern matching
    match (ok_result) {
        Ok(value) => println("Matched Ok: ", value),
        Err(error) => println("Matched Err: ", error)
    }
    println("Test 3: Pattern matching - PASSED");
    
    println("=== All Result tests passed ===");
}
```

**実行テスト:**
```bash
$ ./main tests/cases/stdlib/std/result_test.cb
=== Result<T, E> stdlib test ===
Test 1: Result::Ok - PASSED
Test 2: Result::Err - PASSED
Matched Ok: 42
Test 3: Pattern matching - PASSED
=== All Result tests passed ===

$ echo $?
0  # 正常終了
```

#### 3. tests/stdlib/std/result_test.hpp（新規作成）

```cpp
#pragma once

#include "../framework/stdlib_test_framework.hpp"

inline void test_result_basic() {
    // Cbテストファイルを実行
    auto [output, exit_code] = run_cb_test(
        "../../tests/cases/stdlib/std/result_test.cb");
    
    // exit codeの検証
    STDLIB_ASSERT_EQ(0, exit_code);
    
    // 出力内容の検証
    STDLIB_ASSERT_CONTAINS(output, "=== Result<T, E> stdlib test ===");
    STDLIB_ASSERT_CONTAINS(output, "Test 1: Result::Ok - PASSED");
    STDLIB_ASSERT_CONTAINS(output, "Test 2: Result::Err - PASSED");
    STDLIB_ASSERT_CONTAINS(output, "Test 3: Pattern matching - PASSED");
    STDLIB_ASSERT_CONTAINS(output, "=== All Result tests passed ===");
}

inline void register_result_tests(StdlibTestRunner& runner) {
    runner.add_test("result_basic", test_result_basic);
}
```

#### 4. tests/stdlib/main.cpp（既存ファイルに追加）

```cpp
#include <iostream>
#include "framework/stdlib_test_framework.hpp"

// 追加
#include "std/result_test.hpp"

int main() {
    StdlibTestRunner runner;
    
    std::cout << "=== Stdlib Tests ===" << std::endl;
    
    // 追加
    std::cout << "\n[std]" << std::endl;
    register_result_tests(runner);
    
    // 既存テスト...
    
    runner.run_all();
    return runner.all_passed() ? 0 : 1;
}
```

#### 5. 実行確認

```bash
# Cbテスト個別実行
$ ./main tests/cases/stdlib/std/result_test.cb
=== Result<T, E> stdlib test ===
...
=== All Result tests passed ===

# C++統合テスト実行
$ cd tests/stdlib && make && ./test_main
=== Stdlib Tests ===

[std]
✓ result_basic

All tests passed!

# 全stdlibテスト実行
$ make stdlib-test
...
✅ All stdlib tests completed!
```

---

## 📚 関連ドキュメント

- **テストガイド**: `docs/testing/stdlib_testing_guide.md`
- **Vector実装状況**: `docs/todo/vector_implementation_status.md`
- **設計思想**: `docs/todo/language_vs_library_design.md`

---

## 📞 サポート

### 質問・問題報告

- **Cbテストの書き方**: tests/cases/stdlib/ のサンプル参照
- **C++テストの書き方**: tests/stdlib/ のサンプル参照
- **CI統合**: .github/workflows/ci.yml 参照

---

**最終更新**: 2025年10月28日  
**バージョン**: v1.0  
**ステータス**: ✅ 完成
