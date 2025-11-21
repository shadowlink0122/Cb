#!/bin/bash

# Compiler mode integration test runner
# This script runs integration tests using the compiler mode

set -e

# Get script directory for proper path resolution
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"

CB_BINARY="$PROJECT_ROOT/cb"
TEST_CASES_DIR="$PROJECT_ROOT/tests/cases"
TEMP_DIR="/tmp/cb_compiler_tests_$$"
FAILED_TESTS=0
PASSED_TESTS=0
TOTAL_TESTS=0

mkdir -p "$TEMP_DIR"

echo "[compiler-test] Starting Compiler Mode Integration Tests"
echo ""

# テストケースを選択（コンパイラで動作するもののみ）
TEST_FILES=(
    "basic/simple_main.cb"
    "arithmetic/ok.cb"
    "println/single_arg.cb"
    "if/basic.cb"
)

for test_file in "${TEST_FILES[@]}"; do
    TOTAL_TESTS=$((TOTAL_TESTS + 1))
    test_path="$TEST_CASES_DIR/$test_file"
    test_name=$(basename "$test_file" .cb)
    output_binary="$TEMP_DIR/${test_name}_$$"
    
    if [ ! -f "$test_path" ]; then
        echo "[compiler-test] ⚠️  SKIP: $test_file (file not found)"
        continue
    fi
    
    echo -n "[compiler-test] Testing $test_file ... "
    
    # コンパイル
    if ! $CB_BINARY compile "$test_path" -o "$output_binary" > "$TEMP_DIR/compile_${test_name}.log" 2>&1; then
        echo "❌ FAILED (compilation error)"
        FAILED_TESTS=$((FAILED_TESTS + 1))
        cat "$TEMP_DIR/compile_${test_name}.log"
        continue
    fi
    
    # 実行
    if ! "$output_binary" > "$TEMP_DIR/run_${test_name}.log" 2>&1; then
        echo "❌ FAILED (runtime error)"
        FAILED_TESTS=$((FAILED_TESTS + 1))
        cat "$TEMP_DIR/run_${test_name}.log"
        rm -f "$output_binary"
        continue
    fi
    
    echo "✅ PASSED"
    PASSED_TESTS=$((PASSED_TESTS + 1))
    rm -f "$output_binary"
done

# クリーンアップ
rm -rf "$TEMP_DIR"

echo ""
echo "============================================================="
echo "Compiler Mode Integration Test Summary"
echo "============================================================="
echo "Total:  $TOTAL_TESTS tests"
echo "Passed: $PASSED_TESTS tests"
echo "Failed: $FAILED_TESTS tests"
echo "============================================================="

if [ $FAILED_TESTS -eq 0 ]; then
    echo "✅ All compiler mode tests PASSED"
    exit 0
else
    echo "❌ Some compiler mode tests FAILED"
    exit 1
fi
