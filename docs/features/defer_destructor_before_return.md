# return文実行前のdefer/デストラクタ実行機能の実装

## 概要

return文が実行される直前に、現在のスコープで登録されたdeferとデストラクタを自動的に実行する機能を実装しました。

## 背景

### 問題

以前の実装では、return文が実行されると即座に`ReturnException`がスローされ、現在のスコープのクリーンアップ処理（deferとデストラクタ）がスキップされていました。

```cb
int test() {
    Resource res(10);
    defer { println("cleanup"); }
    
    return 42;  // ← deferとデストラクタが実行されない
}
```

### 期待される動作

RAIIとdeferのセマンティクスでは、スコープ終了時（return文実行も含む）にクリーンアップ処理が実行される必要があります。

```cb
int test() {
    Resource res(10);
    defer { println("cleanup"); }
    
    return 42;  // ← defer実行 → デストラクタ実行 → return
}
```

## 実装

### 1. クリーンアップ関数の追加

`src/backend/interpreter/core/cleanup.cpp`に`execute_pre_return_cleanup()`関数を追加:

```cpp
void Interpreter::execute_pre_return_cleanup() {
    // 1. defer実行（LIFO順）
    if (!defer_stacks_.empty() && !defer_stacks_.back().empty()) {
        std::vector<const ASTNode *> defers = defer_stacks_.back();
        defer_stacks_.pop_back();
        for (auto it = defers.rbegin(); it != defers.rend(); ++it) {
            execute_statement(*it);
        }
    }

    // 2. デストラクタ実行（LIFO順）
    if (!destructor_stacks_.empty() && !destructor_stacks_.back().empty()) {
        const auto &destroy_list = destructor_stacks_.back();
        for (auto it = destroy_list.rbegin(); it != destroy_list.rend(); ++it) {
            const std::string &var_name = it->first;
            const std::string &struct_type_name = it->second;
            call_destructor(var_name, struct_type_name);
        }
        destructor_stacks_.pop_back();
    }
}
```

**特徴:**
- 変数スコープは`pop`しない（`ReturnException`のキャッチ時に処理される）
- defer → デストラクタの順で実行
- 両方ともLIFO（後入れ先出し）順

### 2. ヘッダーファイルの更新

`src/backend/interpreter/core/interpreter.h`に関数宣言を追加:

```cpp
// Defer管理
void push_defer_scope();
void pop_defer_scope();
void add_defer(const ASTNode *stmt);
void execute_defers();
void execute_pre_return_cleanup(); // ← 追加
```

### 3. return文処理の修正

`src/backend/interpreter/handlers/control/return.cpp`の`execute_return_statement()`を修正:

```cpp
void ReturnHandler::execute_return_statement(const ASTNode *node) {
    debug_msg(DebugMsgId::INTERPRETER_RETURN_STMT);

    // return実行前にdefer/デストラクタを実行 ← 追加
    interpreter_->execute_pre_return_cleanup();

    if (!node->left) {
        // return値なし
        throw ReturnException(static_cast<int64_t>(0));
    }
    
    // ... 既存のreturn処理 ...
}
```

## テスト

### テスト1: defer実行テスト

`tests/cases/defer/test_defer_before_return.cb`

```cb
// テスト1: 単純なreturn前のdefer
int test1() {
    println("test1: start");
    defer println("test1: defer executed");
    println("test1: before return");
    return 42;
}

// テスト2: 複数のdefer（LIFO順）
int test2() {
    println("test2: start");
    defer println("test2: defer 1");
    defer println("test2: defer 2");
    defer println("test2: defer 3");
    println("test2: before return");
    return 100;
}

// テスト3: if文内のreturn
int test3(int x) {
    println("test3: start");
    defer println("test3: outer defer");
    
    if (x > 0) {
        defer println("test3: inner defer");
        println("test3: before return in if");
        return x * 2;
    }
    
    println("test3: before return outside if");
    return 0;
}
```

**出力:**
```
=== Defer before return tests ===
--- Test 1 ---
test1: start
test1: before return
test1: defer executed
42
--- Test 2 ---
test2: start
test2: before return
test2: defer 3
test2: defer 2
test2: defer 1
100
--- Test 3 (x > 0) ---
test3: start
test3: before return in if
test3: inner defer
test3: outer defer
20
```

### テスト2: デストラクタ実行テスト

`tests/cases/defer/test_destructor_before_return.cb`

```cb
struct Resource {
    int id;
};

impl Resource {
    self(int resource_id) {
        println("Resource constructed");
        self.id = resource_id;
    }
    
    ~self() {
        println("Resource destroyed");
    }
}

// テスト1: 単純なreturn前のデストラクタ
int test1() {
    println("test1: start");
    Resource res(1);
    println("test1: before return");
    return 42;
}

// テスト4: deferとデストラクタの両方
int test4() {
    println("test4: start");
    Resource res(100);
    defer println("test4: defer 1");
    defer println("test4: defer 2");
    println("test4: before return");
    return 999;
}
```

**出力:**
```
--- Test 1 ---
test1: start
Resource constructed
test1: before return
Resource destroyed
42
--- Test 4 ---
test4: start
Resource constructed
test4: before return
test4: defer 2
test4: defer 1
Resource destroyed
999
```

**Test 4の実行順:**
1. `println("test4: start")`
2. `Resource res(100)` → "Resource constructed"
3. `defer println("test4: defer 1")` （登録）
4. `defer println("test4: defer 2")` （登録）
5. `println("test4: before return")`
6. **return実行前のクリーンアップ:**
   - `defer println("test4: defer 2")` （LIFO）
   - `defer println("test4: defer 1")` （LIFO）
   - `~Resource()` → "Resource destroyed"
7. `return 999`

## 統合テスト結果

### 統合テストへの追加

`tests/integration/defer/test_defer.hpp`に以下のテストを追加:

**Test 9: Defer before return statement**
- `test_defer_before_return.cb`の実行
- 期待値の検証:
  - Test 1: 単純なdefer前のreturn
  - Test 2: 複数のdefer（LIFO順）
  - Test 3: if文内のreturn
  - 実行順序の検証

**Test 10: Destructor before return statement**
- `test_destructor_before_return.cb`の実行
- 期待値の検証:
  - Test 1: 単純なデストラクタ前のreturn
  - Test 2: 複数のデストラクタ（LIFO順）
  - Test 3: if文内のreturn
  - Test 4: deferとデストラクタの混在
  - 実行順序の検証（defer → デストラクタ）

### テスト統計

全テストが通過:
```
✅ PASS: Defer Statement Tests (124 tests)
  - 以前: 79 tests
  - 追加: 45 tests (defer before return + destructor before return)

Integration tests: completed
Unit tests: 50 tests
Total:  30
Passed: 30
Failed: 0
```

## まとめ

### 実装したこと

1. ✅ `execute_pre_return_cleanup()`関数を追加
2. ✅ return文処理でクリーンアップを呼び出し
3. ✅ deferとデストラクタのLIFO順実行を保証
4. ✅ 包括的なテストスイートを作成

### 動作保証

- ✅ 単純なreturn文
- ✅ 複数のdefer/デストラクタ
- ✅ if文内のreturn
- ✅ ループ内のreturn
- ✅ deferとデストラクタの混在
- ✅ 既存機能の後方互換性

### 実行順序

return文実行時:
1. defer（LIFO順）
2. デストラクタ（LIFO順）
3. `ReturnException`をthrow

これにより、RAIIとdeferのセマンティクスが正しく実装されました。
