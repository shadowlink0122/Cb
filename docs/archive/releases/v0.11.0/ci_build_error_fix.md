# CIビルドエラー修正

## 問題

CIでビルドエラーが発生:
```
src/frontend/../backend/interpreter/core/../../../common/ast.h:883:10: error: 
'unordered_map' in namespace 'std' does not name a template type
  883 |     std::unordered_map<std::string, std::string>
      |          ^~~~~~~~~~~~~
```

## 原因

`src/common/ast.h`で`std::unordered_map`を使用しているが、必要なヘッダーファイル`<unordered_map>`がインクルードされていなかった。

## 修正内容

### 1. ヘッダーファイルの追加

**ファイル**: `src/common/ast.h`

**変更前**:
```cpp
#pragma once
#include <cstdint>
#include <iostream>
#include <memory>
#include <string>
#include <vector>
```

**変更後**:
```cpp
#pragma once
#include <cstdint>
#include <iostream>
#include <memory>
#include <string>
#include <unordered_map>  // ← 追加
#include <vector>
```

### 2. 統合テストの修正

デストラクタテストで、複数のテストケースが同じリソースIDを使用しているため、`find()`が意図しない位置を返す問題を修正。

**ファイル**: `tests/integration/defer/test_defer.hpp`

#### Test 2の修正

**問題**: "Resource destroyed 1"がTest 1とTest 2の両方で出力されるため、`find()`が最初の出現位置（Test 1）を返してしまう。

**修正**: Test 2のセクション内で検索するよう変更。

```cpp
// Test 2のセクションを抽出
size_t test2_start = output.find("test2: start");
size_t test2_end = output.find("--- Test 3", test2_start);
std::string test2_output = output.substr(test2_start, test2_end - test2_start);

// test2_output内で検索
size_t pos_destroy3 = test2_output.find("Resource destroyed 3");
size_t pos_destroy2 = test2_output.find("Resource destroyed 2");
size_t pos_destroy1 = test2_output.find("Resource destroyed 1");
```

#### Test 3の修正

**問題**: "Resource destroyed 10"がTest 3の2つのケース（x > 0とx = 0）で出力される。

**修正**: 最初のTest 3セクション（x > 0）内で検索するよう変更。

```cpp
// 最初のTest 3セクションを抽出
size_t test3_start = output.find("--- Test 3 (x > 0) ---");
size_t test3_end = output.find("--- Test 3 (x = 0) ---", test3_start);
std::string test3_output = output.substr(test3_start, test3_end - test3_start);

// test3_output内で検索
size_t pos_destroy20 = test3_output.find("Resource destroyed 20");
size_t pos_destroy10 = test3_output.find("Resource destroyed 10");
```

## テスト結果

### ビルド成功
```bash
g++ -Wall -g -std=c++17 -I. -Isrc -Isrc/backend/interpreter -c -o src/frontend/main.o src/frontend/main.cpp
# エラーなし
```

### 統合テスト成功
```
[integration-test] [PASS] Defer before return statement 
(test_defer_before_return.cb)
[integration-test] [PASS] Destructor before return statement 
(test_destructor_before_return.cb)
[integration-test] Defer tests completed successfully
[integration-test] ✅ PASS: Defer Statement Tests (131 tests)
```

### 全テスト成功
```
=== Test Summary ===
Total:  30
Passed: 30
Failed: 0

All tests passed!
Integration tests: completed
Unit tests: 50 tests
```

## まとめ

1. ✅ `<unordered_map>`ヘッダーを`ast.h`に追加
2. ✅ 統合テストの検索ロジックを修正（セクション内検索）
3. ✅ すべてのテストが通過
4. ✅ CIビルドエラーが解決

これにより、CI環境でも正常にビルドとテストが実行できるようになりました。
