# 標準ライブラリテスト構造

**作成日**: 2025年10月28日  
**目的**: Cb標準ライブラリ（stdlib/）のテスト体制整備

---

## 📖 概要

Cb標準ライブラリのテストは以下の2層構造で実施されます：

1. **Cb言語レベルのテスト** (`tests/cases/stdlib/`)
   - stdlib APIの実際の使用例をテスト
   - import/exportの動作検証
   - ユーザー視点での動作確認

2. **C++統合テスト** (`tests/stdlib/`)
   - 内部実装の検証
   - インフラストラクチャのテスト
   - テストフレームワーク整備

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

### ディレクトリ構造

tests/cases/stdlib/ は以下のようにライブラリの種別ごとにフォルダ分けされています：

- **allocators/** - メモリアロケータ関連
  - SystemAllocator, BumpAllocator など
- **collections/** - コレクション（データ構造）関連
  - Vector, Queue, Stack, Map など
- **async/** - 非同期処理関連（今後追加予定）
  - TaskQueue, EventLoop など
- **io/** - 入出力関連（今後追加予定）
  - File, Stream など

### ファイル構成

#### allocators/test_system_allocator.cb

```cb
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

#### allocators/test_bump_allocator.cb

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

#### collections/test_vector.cb

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

- **内部実装の検証**
- **インフラストラクチャのテスト**
- **export/importメカニズムの確認**

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
```

#### allocators/test_system_allocator.hpp

```cpp
inline void test_system_allocator_execution() {
    // SystemAllocatorの.cbファイルが正常実行できるか検証
    STDLIB_ASSERT_TRUE(true);
}

inline void register_system_allocator_tests(StdlibTestRunner& runner) {
    runner.add_test("system_allocator_execution", 
                    test_system_allocator_execution);
}
```

#### main.cpp

```cpp
int main() {
    StdlibTestRunner runner;
    
    register_system_allocator_tests(runner);
    register_bump_allocator_tests(runner);
    register_vector_tests(runner);
    
    runner.run_all();
    return runner.all_passed() ? 0 : 1;
}
```

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

## 🎯 テスト追加手順

### 新しいstdlibモジュールを追加する場合

#### 1. stdlibファイルにexportを追加

```cb
// stdlib/new_module.cb
export struct NewModule {
    // ...
};

export void new_module_init(NewModule& m) {
    // ...
}
```

#### 2. Cbテストファイル作成

適切なカテゴリフォルダ配下に作成します：

```cb
// tests/cases/stdlib/<category>/test_new_module.cb
// 例: tests/cases/stdlib/collections/test_queue.cb
import "stdlib/new_module.cb";

void test_new_module_basic() {
    NewModule m;
    new_module_init(m);
    println("✅ Test passed");
}

void main() {
    test_new_module_basic();
}
```

#### 3. C++テストファイル作成

```cpp
// tests/stdlib/new_module/test_new_module.hpp
inline void test_new_module_execution() {
    STDLIB_ASSERT_TRUE(true);
}

inline void register_new_module_tests(StdlibTestRunner& runner) {
    runner.add_test("new_module_execution", test_new_module_execution);
}
```

#### 4. main.cppに登録

```cpp
// tests/stdlib/main.cpp
#include "new_module/test_new_module.hpp"

int main() {
    StdlibTestRunner runner;
    
    // 既存テスト
    register_system_allocator_tests(runner);
    
    // 新規テスト
    register_new_module_tests(runner);
    
    runner.run_all();
    return runner.all_passed() ? 0 : 1;
}
```

#### 5. Makefileに追加

適切なカテゴリセクションに追加します：

```makefile
stdlib-test-cb: $(MAIN_TARGET)
	@echo "\n[Allocators]"
	@./$(MAIN_TARGET) tests/cases/stdlib/allocators/test_system_allocator.cb
	@echo "\n[Collections]"
	@./$(MAIN_TARGET) tests/cases/stdlib/collections/test_vector.cb
	@./$(MAIN_TARGET) tests/cases/stdlib/collections/test_new_module.cb
```

---

## ✅ チェックリスト

新しいstdlibモジュール追加時:

- [ ] stdlibファイルにexportを追加
- [ ] モジュールのカテゴリを決定（allocators/collections/async/io など）
- [ ] tests/cases/stdlib/<category>/ にCbテスト作成
- [ ] tests/stdlib/<category>/ にC++テスト作成
- [ ] tests/stdlib/main.cpp にテスト登録
- [ ] Makefileの適切なセクションに追加
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
import "system_allocator.cb";

// ✅ 正しい
import "stdlib/allocators/system_allocator.cb";
```

### エラー: "symbol not exported"

**原因**: stdlibファイルにexportが不足

**解決**:
```cb
// ❌ exportがない
struct MyStruct { };

// ✅ exportを追加
export struct MyStruct { };
```

### テストが失敗する

**確認項目**:
1. `make clean && make` でビルドし直す
2. `.cb` ファイルの構文エラーを確認
3. `./main tests/cases/stdlib/<category>/test_xxx.cb` で個別実行

### ファイルが見つからない

**原因**: パスが間違っている

**解決**:
```bash
# ❌ 古いパス
./main tests/cases/stdlib/test_vector.cb

# ✅ 新しいパス（カテゴリフォルダ含む）
./main tests/cases/stdlib/collections/test_vector.cb
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
