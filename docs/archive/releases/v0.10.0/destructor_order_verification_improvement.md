# デストラクタ順序検証の改善

## 変更内容

デストラクタの実行順序（LIFO）を明確に確認できるよう、デストラクタの出力にリソースIDを追加しました。

## 変更箇所

### テストファイル: `tests/cases/defer/test_destructor_before_return.cb`

**変更前:**
```cb
~self() {
    println("Resource destroyed");
}
```

**変更後:**
```cb
~self() {
    println("Resource destroyed", self.id);
}
```

## 出力の改善

### Test 1: 単純なデストラクタ
```
test1: start
Resource constructed
test1: before return
Resource destroyed 1      ← IDが表示される
42
```

### Test 2: 複数のデストラクタ (LIFO順)
```
test2: start
Resource constructed
Resource constructed
Resource constructed
test2: before return
Resource destroyed 3      ← 最後に構築されたものが最初に破棄
Resource destroyed 2
Resource destroyed 1      ← 最初に構築されたものが最後に破棄
100
```

**LIFO順の確認:**
- Resource 3 → Resource 2 → Resource 1の順で破棄
- 構築順の逆順が明確に確認できる

### Test 3: if文内のデストラクタ (x > 0の場合)
```
test3: start
Resource constructed
Resource constructed
test3: before return in if
Resource destroyed 20     ← 内側のスコープ (inner)
Resource destroyed 10     ← 外側のスコープ (outer)
20
```

**スコープ階層の確認:**
- 内側のスコープ (ID: 20) が先に破棄される
- 外側のスコープ (ID: 10) が後に破棄される

### Test 4: deferとデストラクタの両方
```
test4: start
Resource constructed
test4: before return
test4: defer 2            ← deferが先 (LIFO)
test4: defer 1            ← deferが先 (LIFO)
Resource destroyed 100    ← デストラクタは後
999
```

**実行順序の確認:**
1. defer (LIFO順)
2. デストラクタ
3. return

## 統合テストの更新

### `tests/integration/defer/test_defer.hpp`

より厳密な検証を追加:

#### Test 1の期待値
```cpp
INTEGRATION_ASSERT_CONTAINS(output, "Resource destroyed 1", "Should destroy resource 1 before return");

// インデックスベースの順序検証
if (lines[i].find("Resource destroyed 1") != std::string::npos && destroyed_idx == -1) 
    destroyed_idx = i;
```

#### Test 2の期待値（LIFO順の詳細検証）
```cpp
// IDを含む出力の検証
INTEGRATION_ASSERT_CONTAINS(output, "Resource destroyed 3", "Should destroy resource 3");
INTEGRATION_ASSERT_CONTAINS(output, "Resource destroyed 2", "Should destroy resource 2");
INTEGRATION_ASSERT_CONTAINS(output, "Resource destroyed 1", "Should destroy resource 1");

// LIFO順の検証
size_t pos_destroy3 = output.find("Resource destroyed 3");
size_t pos_destroy2 = output.find("Resource destroyed 2");
size_t pos_destroy1 = output.find("Resource destroyed 1");
INTEGRATION_ASSERT(pos_destroy3 < pos_destroy2, "Resource 3 should be destroyed first (LIFO)");
INTEGRATION_ASSERT(pos_destroy2 < pos_destroy1, "Resource 2 should be destroyed second (LIFO)");
```

#### Test 3の期待値（スコープ階層の検証）
```cpp
INTEGRATION_ASSERT_CONTAINS(output, "Resource destroyed 20", "Should destroy inner resource 20");
INTEGRATION_ASSERT_CONTAINS(output, "Resource destroyed 10", "Should destroy outer resource 10");

// スコープ階層の順序検証
size_t pos_destroy20 = output.find("Resource destroyed 20");
size_t pos_destroy10 = output.find("Resource destroyed 10");
INTEGRATION_ASSERT(pos_destroy20 < pos_destroy10, "Inner resource should be destroyed before outer (LIFO)");
```

#### Test 4の期待値（deferとデストラクタの順序）
```cpp
INTEGRATION_ASSERT_CONTAINS(output, "Resource destroyed 100", "Should destroy resource 100");

size_t pos_defer2 = output.find("test4: defer 2");
size_t pos_defer1 = output.find("test4: defer 1");
size_t pos_destroyed100 = output.find("Resource destroyed 100");

INTEGRATION_ASSERT(pos_defer2 < pos_defer1, "defer 2 should execute first (LIFO)");
INTEGRATION_ASSERT(pos_defer1 < pos_destroyed100, "Defer should execute before destructor");
```

## 改善のメリット

1. **デバッグの容易性**
   - どのリソースが破棄されているか一目瞭然
   - LIFO順が視覚的に確認できる

2. **テストの信頼性向上**
   - IDベースの検証により、特定のリソースの破棄を確認
   - 曖昧な検証（"Resource destroyed"の出現回数）から明確な検証へ

3. **実行順序の証明**
   - 構築順: 1 → 2 → 3
   - 破棄順: 3 → 2 → 1
   - LIFO（後入れ先出し）が明確に証明される

## テスト結果

```
✅ PASS: Defer Statement Tests (124 tests)
  - Test 9: Defer before return statement
  - Test 10: Destructor before return statement (更新済み)

Integration tests: completed
All tests passed!
```

すべてのテストが通過し、デストラクタのLIFO順序が正しく検証されることが確認されました。
