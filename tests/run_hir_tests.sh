#!/bin/bash
# v0.14.0: HIR Compiler Integration Test Runner
# HIRコンパイラを使用して統合テストを実行

set -e

# カラー出力
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# テスト統計
TOTAL_TESTS=0
PASSED_TESTS=0
FAILED_TESTS=0
SKIPPED_TESTS=0

# Cbコンパイラのパス
CB_COMPILER="./main"
CB_INTERPRETER="./main"
TEST_DIR="tests/integration"
OUTPUT_DIR="/tmp/cb_hir_test_output"

# 出力ディレクトリを作成
mkdir -p "$OUTPUT_DIR"

# ヘルプ表示
show_help() {
    echo "Usage: $0 [OPTIONS]"
    echo ""
    echo "Options:"
    echo "  -h, --help              Show this help message"
    echo "  -t, --test PATTERN      Run only tests matching PATTERN"
    echo "  -v, --verbose           Verbose output"
    echo "  -c, --cleanup           Clean up temporary files before running"
    echo ""
    echo "Examples:"
    echo "  $0                      Run all tests"
    echo "  $0 -t basic             Run only basic tests"
    echo "  $0 -t \"ffi/*\"          Run all FFI tests"
}

# テストファイルを実行
run_test() {
    local test_file="$1"
    local test_name=$(basename "$test_file" .cb)
    local test_dir=$(dirname "$test_file")
    local output_binary="$OUTPUT_DIR/${test_name}_test"
    
    TOTAL_TESTS=$((TOTAL_TESTS + 1))
    
    echo -n "Testing $test_name... "
    
    # ステップ1: コンパイル
    if ! $CB_COMPILER -c "$test_file" -o "$output_binary" 2>/dev/null; then
        echo -e "${RED}FAILED${NC} (compilation error)"
        FAILED_TESTS=$((FAILED_TESTS + 1))
        return 1
    fi
    
    # ステップ2: 実行してインタプリタと比較
    local compiler_output=$($output_binary 2>&1)
    local interpreter_output=$($CB_INTERPRETER "$test_file" 2>&1)
    
    # 出力を比較
    if [ "$compiler_output" == "$interpreter_output" ]; then
        echo -e "${GREEN}PASSED${NC}"
        PASSED_TESTS=$((PASSED_TESTS + 1))
        return 0
    else
        echo -e "${RED}FAILED${NC} (output mismatch)"
        echo "  Expected (interpreter): $interpreter_output"
        echo "  Got (compiler):         $compiler_output"
        FAILED_TESTS=$((FAILED_TESTS + 1))
        return 1
    fi
}

# メイン処理
main() {
    local test_pattern="*"
    local verbose=0
    local cleanup=0
    
    # コマンドライン引数の処理
    while [[ $# -gt 0 ]]; do
        case $1 in
            -h|--help)
                show_help
                exit 0
                ;;
            -t|--test)
                test_pattern="$2"
                shift 2
                ;;
            -v|--verbose)
                verbose=1
                shift
                ;;
            -c|--cleanup)
                cleanup=1
                shift
                ;;
            *)
                echo "Unknown option: $1"
                show_help
                exit 1
                ;;
        esac
    done
    
    # クリーンアップ
    if [ $cleanup -eq 1 ]; then
        echo "Cleaning up temporary files..."
        rm -rf "$OUTPUT_DIR"
        mkdir -p "$OUTPUT_DIR"
    fi
    
    echo -e "${BLUE}=================================${NC}"
    echo -e "${BLUE}HIR Compiler Integration Tests${NC}"
    echo -e "${BLUE}=================================${NC}"
    echo ""
    
    # Cbコンパイラが存在するか確認
    if [ ! -f "$CB_COMPILER" ]; then
        echo -e "${RED}Error: Cb compiler not found at $CB_COMPILER${NC}"
        echo "Please build the compiler first: make"
        exit 1
    fi
    
    # テストファイルを検索
    local test_files=$(find "$TEST_DIR" -name "*.cb" -type f | grep "$test_pattern")
    
    if [ -z "$test_files" ]; then
        echo -e "${YELLOW}No test files found matching pattern: $test_pattern${NC}"
        exit 0
    fi
    
    # 各テストファイルを実行
    for test_file in $test_files; do
        run_test "$test_file"
    done
    
    # 結果サマリー
    echo ""
    echo -e "${BLUE}=================================${NC}"
    echo -e "${BLUE}Test Results${NC}"
    echo -e "${BLUE}=================================${NC}"
    echo "Total:   $TOTAL_TESTS"
    echo -e "${GREEN}Passed:  $PASSED_TESTS${NC}"
    echo -e "${RED}Failed:  $FAILED_TESTS${NC}"
    echo -e "${YELLOW}Skipped: $SKIPPED_TESTS${NC}"
    echo ""
    
    if [ $FAILED_TESTS -eq 0 ]; then
        echo -e "${GREEN}All tests passed! 🎉${NC}"
        exit 0
    else
        echo -e "${RED}Some tests failed.${NC}"
        exit 1
    fi
}

main "$@"
