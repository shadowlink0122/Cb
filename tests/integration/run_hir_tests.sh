#!/bin/bash

# HIR統合テストスクリプト
# integration-testと同じテストケースをHIRコンパイル経由で実行

echo "=============================================="
echo "  HIR Integration Test Runner"
echo "=============================================="
echo ""

# テストディレクトリ
TEST_DIR="$(cd "$(dirname "$0")" && pwd)"
ROOT_DIR="$(cd "$TEST_DIR/../.." && pwd)"
MAIN_BINARY="$ROOT_DIR/cb"
CASES_DIR="$ROOT_DIR/tests/cases"
INTEGRATION_CASES_DIR="$TEST_DIR/cases"

# 結果カウンター
TOTAL=0
PASSED=0
FAILED=0
SKIPPED=0

# カラー出力
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# テンポラリディレクトリ
TMP_DIR="/tmp/cb_hir_integration_$$"
mkdir -p "$TMP_DIR"

# クリーンアップ
cleanup() {
    rm -rf "$TMP_DIR"
}
trap cleanup EXIT

# テスト実行関数
run_test() {
    local test_file="$1"
    local test_name=$(basename "$test_file" .cb)
    local rel_path="${test_file#$ROOT_DIR/}"
    
    TOTAL=$((TOTAL + 1))
    
    # テスト実行
    if "$MAIN_BINARY" "$test_file" > "$TMP_DIR/output_$TOTAL.txt" 2>&1; then
        echo -e "${GREEN}✓${NC} PASS: $rel_path"
        PASSED=$((PASSED + 1))
        return 0
    else
        echo -e "${RED}✗${NC} FAIL: $rel_path"
        FAILED=$((FAILED + 1))
        # エラー詳細を表示（最初の3行）
        if [ -f "$TMP_DIR/output_$TOTAL.txt" ]; then
            head -3 "$TMP_DIR/output_$TOTAL.txt" | sed 's/^/    /'
        fi
        return 1
    fi
}

# ディレクトリ内のすべてのCbファイルをテスト
run_directory() {
    local dir="$1"
    local category="$2"
    
    if [ ! -d "$dir" ]; then
        return
    fi
    
    echo ""
    echo -e "${BLUE}=== $category ===${NC}"
    
    local test_files=()
    while IFS= read -r -d '' file; do
        test_files+=("$file")
    done < <(find "$dir" -name "*.cb" -type f -print0 2>/dev/null | sort -z)
    
    if [ ${#test_files[@]} -eq 0 ]; then
        echo -e "${YELLOW}⊘${NC} No tests found in: $dir"
        return
    fi
    
    for test_file in "${test_files[@]}"; do
        run_test "$test_file"
    done
}

# パターンマッチでテストを実行
run_pattern() {
    local base_dir="$1"
    local pattern="$2"
    local category="$3"
    
    if [ ! -d "$base_dir" ]; then
        return
    fi
    
    echo ""
    echo -e "${BLUE}=== $category ===${NC}"
    
    local test_files=()
    while IFS= read -r -d '' file; do
        test_files+=("$file")
    done < <(find "$base_dir" -name "$pattern" -type f -print0 2>/dev/null | sort -z)
    
    if [ ${#test_files[@]} -eq 0 ]; then
        echo -e "${YELLOW}⊘${NC} No tests found for pattern: $pattern"
        return
    fi
    
    for test_file in "${test_files[@]}"; do
        run_test "$test_file"
    done
}

# メインバイナリの確認
if [ ! -f "$MAIN_BINARY" ]; then
    echo -e "${RED}ERROR:${NC} Main binary not found: $MAIN_BINARY"
    echo "Please run 'make' first to build the compiler."
    exit 1
fi

echo "Using compiler: $MAIN_BINARY"
echo ""

# ========================================
# Part 1: tests/cases からのテスト
# ========================================
echo "=============================================="
echo "  Part 1: Testing from tests/cases/"
echo "=============================================="

# HIR専用テスト
run_pattern "$CASES_DIR" "hir_*.cb" "HIR Basic Tests"

# println テスト
run_directory "$CASES_DIR/println" "println Tests"

# ジェネリクステスト
run_directory "$CASES_DIR/generics" "Generics Tests"

# ========================================
# Part 2: tests/integration/cases からのテスト
# ========================================
echo ""
echo "=============================================="
echo "  Part 2: Testing from tests/integration/cases/"
echo "=============================================="

# FFI テスト
run_directory "$INTEGRATION_CASES_DIR/ffi" "FFI Tests"

# プリプロセッサテスト
run_directory "$INTEGRATION_CASES_DIR/preprocessor" "Preprocessor Tests"

# その他のintegrationテストケース
if [ -f "$INTEGRATION_CASES_DIR/syntax_highlighting_test.cb" ]; then
    echo ""
    echo -e "${BLUE}=== Other Integration Cases ===${NC}"
    run_test "$INTEGRATION_CASES_DIR/syntax_highlighting_test.cb"
fi

# 結果サマリー
echo ""
echo "=============================================="
echo "  Test Results Summary"
echo "=============================================="
echo "Total:   $TOTAL"
echo -e "Passed:  ${GREEN}$PASSED${NC}"
if [ $FAILED -gt 0 ]; then
    echo -e "Failed:  ${RED}$FAILED${NC}"
else
    echo -e "Failed:  $FAILED"
fi

if [ $SKIPPED -gt 0 ]; then
    echo -e "Skipped: ${YELLOW}$SKIPPED${NC}"
fi

echo ""

if [ $FAILED -eq 0 ]; then
    echo -e "${GREEN}✓ All tests passed!${NC}"
    echo ""
    echo "HIR integration tests completed successfully."
    exit 0
else
    echo -e "${RED}✗ Some tests failed.${NC}"
    echo ""
    echo "HIR integration: $FAILED/$TOTAL tests failed."
    exit 1
fi
