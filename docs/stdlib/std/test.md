# Test - テストフレームワーク

## 概要

`TestResult`と`TestFramework`は、Cbでユニットテストを書くためのシンプルなテストフレームワークです。

## インポート

```cb
import stdlib.std.test;
```

## 基本的な使い方

```cb
import stdlib.std.test;

void main() {
    TestResult result;
    result.passed = 0;
    result.failed = 0;
    result.total = 0;
    
    result.assert_eq_int(1 + 1, 2, "addition works");
    result.assert_true(true, "boolean test");
    
    result.print_summary();
}
```

## TestResult構造体

```cb
struct TestResult {
    int passed;   // 成功したテスト数
    int failed;   // 失敗したテスト数
    int total;    // 総テスト数
};
```

## アサーションメソッド

### 真偽値のテスト

| メソッド | 説明 | 引数 |
|---------|------|-----|
| `assert_true(bool condition, string message)` | 条件が真であることを検証 | 条件、メッセージ |
| `assert_false(bool condition, string message)` | 条件が偽であることを検証 | 条件、メッセージ |

### 等価性のテスト

| メソッド | 説明 | 引数 |
|---------|------|-----|
| `assert_eq_int(int actual, int expected, string message)` | int値が等しいか検証 | 実際の値、期待値、メッセージ |
| `assert_eq_long(long actual, long expected, string message)` | long値が等しいか検証 | 実際の値、期待値、メッセージ |
| `assert_eq_bool(bool actual, bool expected, string message)` | bool値が等しいか検証 | 実際の値、期待値、メッセージ |
| `assert_eq_str(string actual, string expected, string message)` | string値が等しいか検証 | 実際の値、期待値、メッセージ |

### ユーティリティ

| メソッド | 説明 | 戻り値 |
|---------|------|-------|
| `get_result()` | テスト結果を取得 | `TestResult` |
| `print_summary()` | テストサマリーを出力 | `void` |
| `reset()` | カウンターをリセット | `void` |

## 使用例

### 基本的なテスト

```cb
import stdlib.std.test;

void main() {
    TestResult t;
    t.passed = 0;
    t.failed = 0;
    t.total = 0;
    
    // 真偽値テスト
    t.assert_true(5 > 3, "5 is greater than 3");
    t.assert_false(2 > 10, "2 is not greater than 10");
    
    // 整数テスト
    t.assert_eq_int(10, 10, "integers equal");
    t.assert_eq_int(5 + 5, 10, "arithmetic works");
    
    t.print_summary();
}
```

**出力:**
```
  ✅ 5 is greater than 3
  ✅ 2 is not greater than 10
  ✅ integers equal
  ✅ arithmetic works

═══════════════════════════════════════════════════════
Test Results: 4 passed, 0 failed, 4 total
═══════════════════════════════════════════════════════
✅ All tests passed!
```

### 文字列テスト

```cb
import stdlib.std.test;

void main() {
    TestResult t;
    t.passed = 0;
    t.failed = 0;
    t.total = 0;
    
    string hello = "Hello";
    string world = "World";
    
    t.assert_eq_str(hello, "Hello", "string matches");
    t.assert_eq_str(world, "World", "another string matches");
    
    t.print_summary();
}
```

### カスタム関数のテスト

```cb
import stdlib.std.test;

int add(int a, int b) {
    return a + b;
}

int multiply(int a, int b) {
    return a * b;
}

void test_math() {
    TestResult t;
    t.passed = 0;
    t.failed = 0;
    t.total = 0;
    
    println("Testing math functions:");
    
    t.assert_eq_int(add(2, 3), 5, "add(2, 3) = 5");
    t.assert_eq_int(add(0, 0), 0, "add(0, 0) = 0");
    t.assert_eq_int(add(-1, 1), 0, "add(-1, 1) = 0");
    
    t.assert_eq_int(multiply(3, 4), 12, "multiply(3, 4) = 12");
    t.assert_eq_int(multiply(0, 100), 0, "multiply(0, 100) = 0");
    
    t.print_summary();
}

void main() {
    test_math();
}
```

### 構造体のテスト

```cb
import stdlib.std.test;

struct Point {
    int x;
    int y;
};

int distance_sq(Point p1, Point p2) {
    int dx = p2.x - p1.x;
    int dy = p2.y - p1.y;
    return dx * dx + dy * dy;
}

void main() {
    TestResult t;
    t.passed = 0;
    t.failed = 0;
    t.total = 0;
    
    Point origin;
    origin.x = 0;
    origin.y = 0;
    
    Point p;
    p.x = 3;
    p.y = 4;
    
    int dist = distance_sq(origin, p);
    t.assert_eq_int(dist, 25, "distance^2 from (0,0) to (3,4) is 25");
    
    t.print_summary();
}
```

## 失敗例の出力

テストが失敗すると、期待値と実際の値が表示されます：

```cb
import stdlib.std.test;

void main() {
    TestResult t;
    t.passed = 0;
    t.failed = 0;
    t.total = 0;
    
    t.assert_eq_int(5, 10, "this will fail");
    t.print_summary();
}
```

**出力:**
```
  ❌ this will fail (expected: 10, got: 5)

═══════════════════════════════════════════════════════
Test Results: 0 passed, 1 failed, 1 total
═══════════════════════════════════════════════════════
❌ Some tests failed
```

## ベストプラクティス

### ✅ 推奨

```cb
// テスト関数を分割
void test_addition() {
    TestResult t;
    t.passed = 0;
    t.failed = 0;
    t.total = 0;
    
    t.assert_eq_int(1 + 1, 2, "1 + 1 = 2");
    t.print_summary();
}

void test_multiplication() {
    TestResult t;
    t.passed = 0;
    t.failed = 0;
    t.total = 0;
    
    t.assert_eq_int(2 * 3, 6, "2 * 3 = 6");
    t.print_summary();
}

void main() {
    test_addition();
    test_multiplication();
}
```

### ❌ 非推奨

```cb
// すべてを一つの関数に詰め込む
void main() {
    TestResult t;
    t.passed = 0;
    t.failed = 0;
    t.total = 0;
    
    t.assert_eq_int(1 + 1, 2, "test 1");
    t.assert_eq_int(2 * 3, 6, "test 2");
    t.assert_eq_int(5 - 2, 3, "test 3");
    // ... 100個のテスト
    t.print_summary();
}
```

## 実際のテスト例

実際のテストコードは以下のディレクトリにあります：

```
tests/cases/stdlib/
├── string/         # 文字列ライブラリのテスト
├── vector/         # Vectorのテスト
├── queue/          # Queueのテスト
└── collections/
    └── map/        # Mapのテスト
```

例: `tests/cases/stdlib/string/test_basic.cb`

```cb
import stdlib.std.test;
import stdlib.std.string;

void main() {
    TestResult t;
    t.passed = 0;
    t.failed = 0;
    t.total = 0;
    
    String s;
    s.data = "Hello";
    s.length = 5;
    
    t.assert_eq_int(s.size(), 5, "size returns correct length");
    t.assert_false(s.is_empty(), "non-empty string");
    t.assert_eq_str(s.get(), "Hello", "get returns raw string");
    
    t.print_summary();
}
```

## 制限事項

1. **カスタムアサーション**: 現在サポートされていません
2. **セットアップ/ティアダウン**: before/afterフックは未実装
3. **テストスイート**: テストのグループ化機能はありません

## 将来の拡張

- [ ] カスタムアサーションマクロ
- [ ] before/afterフック
- [ ] テストスイート機能
- [ ] パフォーマンステスト

## 関連項目

- [Vector](./vector.md)
- [Queue](./queue.md)
- [Map](./map.md)
- [String](./string.md)
