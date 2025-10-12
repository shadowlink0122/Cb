# return文実行前のdefer/デストラクタ実行 - 統合テスト仕様

## 概要

`tests/integration/defer/test_defer.hpp`に追加された2つの統合テストの詳細仕様。

## Test 9: Defer before return statement

### テストファイル
`tests/cases/defer/test_defer_before_return.cb`

### テストケース

#### Test 1: Simple defer before return
```cb
int test1() {
    println("test1: start");
    defer println("test1: defer executed");
    println("test1: before return");
    return 42;
}
```

**期待される出力順序:**
1. `test1: start`
2. `test1: before return`
3. `test1: defer executed` ← return前に実行
4. `42` ← 戻り値

**検証項目:**
- ✅ `test1: before return`の後に`test1: defer executed`が出力される
- ✅ deferがreturn値の前に実行される

#### Test 2: Multiple defers (LIFO order)
```cb
int test2() {
    println("test2: start");
    defer println("test2: defer 1");
    defer println("test2: defer 2");
    defer println("test2: defer 3");
    println("test2: before return");
    return 100;
}
```

**期待される出力順序:**
1. `test2: start`
2. `test2: before return`
3. `test2: defer 3` ← LIFO (Last In, First Out)
4. `test2: defer 2`
5. `test2: defer 1`
6. `100` ← 戻り値

**検証項目:**
- ✅ defer 3 → defer 2 → defer 1 の順で実行（LIFO）
- ✅ すべてのdeferがreturn前に実行される

#### Test 3: Defer in if statement with return
```cb
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

**ケース3a: x > 0 (x=10)**
1. `test3: start`
2. `test3: before return in if`
3. `test3: inner defer` ← if内のdefer
4. `test3: outer defer` ← 外側のdefer
5. `20` ← 戻り値

**ケース3b: x = 0**
1. `test3: start`
2. `test3: before return outside if`
3. `test3: outer defer` ← 外側のdeferのみ
4. `0` ← 戻り値

**検証項目:**
- ✅ 内側のスコープのdeferが先に実行される
- ✅ 外側のスコープのdeferが後に実行される
- ✅ スコープの階層が正しく処理される

## Test 10: Destructor before return statement

### テストファイル
`tests/cases/defer/test_destructor_before_return.cb`

### 構造体定義
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
```

### テストケース

#### Test 1: Simple destructor before return
```cb
int test1() {
    println("test1: start");
    Resource res(1);
    println("test1: before return");
    return 42;
}
```

**期待される出力順序:**
1. `test1: start`
2. `Resource constructed` ← コンストラクタ
3. `test1: before return`
4. `Resource destroyed` ← return前にデストラクタ
5. `42` ← 戻り値

**検証項目:**
- ✅ `test1: before return`の後に`Resource destroyed`が出力される
- ✅ デストラクタがreturn値の前に実行される

#### Test 2: Multiple destructors (LIFO order)
```cb
int test2() {
    println("test2: start");
    Resource res1(1);
    Resource res2(2);
    Resource res3(3);
    println("test2: before return");
    return 100;
}
```

**期待される出力順序:**
1. `test2: start`
2. `Resource constructed` (res1)
3. `Resource constructed` (res2)
4. `Resource constructed` (res3)
5. `test2: before return`
6. `Resource destroyed` ← res3 (LIFO)
7. `Resource destroyed` ← res2
8. `Resource destroyed` ← res1
9. `100` ← 戻り値

**検証項目:**
- ✅ デストラクタがLIFO順で実行される
- ✅ すべてのデストラクタがreturn前に実行される
- ✅ 少なくとも3つのデストラクタ呼び出しが確認される

#### Test 3: Destructor in if statement
```cb
int test3(int x) {
    println("test3: start");
    Resource outer_res(10);
    
    if (x > 0) {
        Resource inner_res(20);
        println("test3: before return in if");
        return x * 2;
    }
    
    println("test3: before return outside if");
    return 0;
}
```

**ケース3a: x > 0 (x=10)**
1. `test3: start`
2. `Resource constructed` (outer_res)
3. `Resource constructed` (inner_res)
4. `test3: before return in if`
5. `Resource destroyed` ← inner_res
6. `Resource destroyed` ← outer_res
7. `20` ← 戻り値

**ケース3b: x = 0**
1. `test3: start`
2. `Resource constructed` (outer_res)
3. `test3: before return outside if`
4. `Resource destroyed` ← outer_resのみ
5. `0` ← 戻り値

**検証項目:**
- ✅ 内側のスコープのデストラクタが先に実行される
- ✅ 外側のスコープのデストラクタが後に実行される

#### Test 4: Defer and destructor together
```cb
int test4() {
    println("test4: start");
    Resource res(100);
    defer println("test4: defer 1");
    defer println("test4: defer 2");
    println("test4: before return");
    return 999;
}
```

**期待される出力順序:**
1. `test4: start`
2. `Resource constructed`
3. `test4: before return`
4. `test4: defer 2` ← defer (LIFO)
5. `test4: defer 1` ← defer (LIFO)
6. `Resource destroyed` ← デストラクタはdeferの後
7. `999` ← 戻り値

**検証項目:**
- ✅ deferが先に実行される
- ✅ デストラクタがdeferの後に実行される
- ✅ deferはLIFO順で実行される
- ✅ **重要**: defer → デストラクタの順序が保証される

## 実行順序の仕様

### クリーンアップの実行順序

return文が実行される際、以下の順序でクリーンアップが行われる:

1. **Defer実行** (LIFO順)
   - 現在のスコープで登録された全てのdefer
   - 後から登録されたものが先に実行される

2. **デストラクタ実行** (LIFO順)
   - 現在のスコープで構築された全てのオブジェクト
   - 後から構築されたものが先に破棄される

3. **ReturnException送出**
   - return値を持つ例外をthrow
   - 呼び出し元に制御を返す

### スコープ階層の処理

ネストしたスコープでreturnが実行された場合:

```cb
void func() {
    // Scope 1
    Resource outer(1);
    defer println("outer defer");
    
    {
        // Scope 2
        Resource inner(2);
        defer println("inner defer");
        
        return;  // ← ここでreturn
        // 実行順序:
        // 1. inner defer (Scope 2)
        // 2. inner destructor (Scope 2)
        // 3. outer defer (Scope 1)
        // 4. outer destructor (Scope 1)
    }
}
```

## 統合テストの実装

### アサーション項目

**Defer before return:**
- `INTEGRATION_ASSERT_CONTAINS`: 必要な出力が含まれているか
- `INTEGRATION_ASSERT`: 出力の順序が正しいか
- `INTEGRATION_ASSERT_EQ`: exit codeが0か

**Destructor before return:**
- `INTEGRATION_ASSERT_CONTAINS`: コンストラクタ/デストラクタの出力
- `split_lines()`: 行単位で出力を解析
- インデックスベースの順序検証
- デストラクタ呼び出し回数のカウント
- `rfind()`: 最後のデストラクタ出力を検索

### テスト統計

```
✅ PASS: Defer Statement Tests (124 tests)
  - Test 9 (defer before return): ~23 tests
  - Test 10 (destructor before return): ~22 tests
  - Total added: 45 tests
```

## まとめ

両方のテストが統合テストに追加され、以下を保証:

1. ✅ return前にdeferが実行される
2. ✅ return前にデストラクタが実行される
3. ✅ defer → デストラクタの順序が保証される
4. ✅ LIFO順が正しく機能する
5. ✅ スコープ階層が正しく処理される
6. ✅ 既存機能に影響がない（全テスト通過）
