# Cb Test Suite Organization

## Overview

Cb言語のテストスイートは4つの独立したカテゴリーに分かれており、それぞれ異なる側面をテストします。

## Test Suites

### 1. Integration Tests (`make integration-test`)
**目的**: 言語機能のend-to-endテスト

**内容**:
- 3779個のテストケース
- 基本機能から高度な機能まで網羅
- 実際のCbプログラムを実行して動作を検証

**主要なテストカテゴリー**:
- 基本的な演算と制御フロー (printf, println, cross-type)
- 型システム (Option/Result, typedef, enum, union, pattern matching)
- v0.12.0機能 (async/await: 209テスト)
- v0.11.0機能 (ジェネリクス: 181テスト)
- v0.10.0機能 (discard variable, lambda, move constructor, rvalue reference)
- 高度な機能 (struct, interface, pointer, memory management)

### 2. Unit Tests (`make unit-test`)
**目的**: 個別コンポーネントの単体テスト

**内容**:
- 30個のC++ユニットテスト
- インタプリタの内部実装を直接テスト
- 低レベルの動作保証

**主要なテストカテゴリー**:
- インタプリタの初期化と基本評価
- 算術演算の正確性
- 型境界値テスト
- 関数定義と呼び出し
- 構造体とポインタのメタデータ

### 3. Stdlib C++ Tests (`make stdlib-cpp-test`)
**目的**: 標準ライブラリのC++インフラのテスト

**内容**:
- 29個のテストケース
- C++で実装された標準ライブラリコンポーネントのテスト
- ライブラリの基礎インフラの検証

**主要なテストカテゴリー**:
- Allocators (SystemAllocator, BumpAllocator)
- Collections (Vector<T>, Queue<T>, Map<K,V>)
- Builtin Types (Option<T>, Result<T,E>)
- Time utilities

### 4. Stdlib Cb Tests (`make stdlib-cb-test`)
**目的**: Cb言語で書かれた標準ライブラリモジュールのテスト

**内容**:
- 25個のテストケース (現在)
- Cb言語で実装されたstdlibモジュールの動作検証
- 統合的なライブラリテスト

**主要なテストカテゴリー**:
- Allocatorの使用テスト
- Vectorの基本操作とソート
- Queueの基本操作
- Mapの基本操作

**Note**: Async/Await tests are temporarily disabled due to a known issue with TestResult pointer handling after await expressions. This is tracked for future resolution.

## Running Tests

### All Tests
```bash
make test
```

4つすべてのテストスイートを順番に実行します。

### Individual Test Suites
```bash
make integration-test  # 1/4: Integration tests
make unit-test         # 2/4: Unit tests
make stdlib-cpp-test   # 3/4: Stdlib C++ tests
make stdlib-cb-test    # 4/4: Stdlib Cb tests
```

## Test Results Interpretation

### Success
```
=============================================================
=== Final Test Summary ===
=============================================================
✅ [1/4] Integration tests: PASSED
✅ [2/4] Unit tests: PASSED
✅ [3/4] Stdlib C++ tests: PASSED
✅ [4/4] Stdlib Cb tests: PASSED
=============================================================
Test suites: 4/4 passed, 0/4 failed
Total time: XXs
=============================================================

╔════════════════════════════════════════════════════════════╗
║        🎉 All 4 Test Suites Passed Successfully! 🎉       ║
╚════════════════════════════════════════════════════════════╝
```

### Failure
```
=============================================================
=== Final Test Summary ===
=============================================================
✅ [1/4] Integration tests: PASSED
✅ [2/4] Unit tests: PASSED
✅ [3/4] Stdlib C++ tests: PASSED
❌ [4/4] Stdlib Cb tests: FAILED (exit code 2)
=============================================================
Test suites: 3/4 passed, 1/4 failed
Total time: XXs
=============================================================

⚠️  1 of 4 test suite(s) failed

💡 Run individual test suites for details:
   - make stdlib-cb-test
```

## Adding New Tests

### Integration Tests
1. `tests/cases/` に新しいテストファイルを追加
2. `tests/integration/run_tests.sh` に登録
3. `make integration-test` で確認

### Unit Tests
1. `tests/unit/` に新しいテストを追加
2. `tests/unit/main.cpp` に登録
3. `make unit-test` で確認

### Stdlib C++ Tests
1. `tests/stdlib/` に新しいテストを追加
2. `tests/stdlib/main.cpp` に登録
3. `make stdlib-cpp-test` で確認

### Stdlib Cb Tests
1. `tests/cases/stdlib/test_stdlib_all.cb` に新しいテスト関数を追加
2. 対応する `run_XXX_tests()` 関数を更新
3. `main()` 関数にテストスイートを追加
4. `make stdlib-cb-test` で確認

## Design Rationale

### Why 4 Separate Suites?

1. **Clarity**: 各テストスイートの役割が明確
2. **Efficiency**: 必要なテストのみを実行可能
3. **Debugging**: 問題の発生箇所を素早く特定
4. **Scalability**: 各スイートを独立して拡張可能

### Naming Convention

- **integration-test**: 言語機能の統合テスト
- **unit-test**: コンポーネントの単体テスト
- **stdlib-cpp-test**: C++で書かれたstdlibのテスト
- **stdlib-cb-test**: Cbで書かれたstdlibのテスト

この命名により、各スイートの目的が一目で理解できます。

## Test Statistics (v0.12.0)

| Suite | Tests | Status |
|-------|-------|--------|
| Integration | 3779 | ✅ PASSED |
| Unit | 30 | ✅ PASSED |
| Stdlib C++ | 29 | ✅ PASSED |
| Stdlib Cb | 25 | ✅ PASSED |
| **Total** | **3863** | **✅ ALL PASSED** |

## Future Improvements

1. **Parallel Test Execution**: 複数のテストスイートを並列実行
2. **Coverage Reports**: コードカバレッジレポートの生成
3. **Performance Benchmarks**: パフォーマンステストの追加
4. **Async/Await Stdlib Tests**: 一時的に無効化されたasync/awaitテストの修正と再有効化

## Related Documentation

- [Testing Strategy](../testing/README.md)
- [Async/Await Implementation](../features/async_await_v0.12.0_implementation.md)
- [v0.12.0 Release Notes](../../release_notes/v0.12.0.md)
