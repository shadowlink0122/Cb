#!/bin/bash

# Unified Integration Test Runner for Cb Language
# This script runs the same test cases in both INTERPRETER and COMPILER modes
# to ensure consistency between both execution methods

set -e

# Color codes
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
CYAN='\033[0;36m'
MAGENTA='\033[0;35m'
NC='\033[0m' # No Color

# Get script directory
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"

# Paths
CB_BINARY="$PROJECT_ROOT/cb"
TEST_CASES_DIR="$PROJECT_ROOT/tests/cases"
TEMP_DIR="/tmp/cb_unified_tests_$$"

# Mode selection
MODE="${1:-both}"  # interpreter, compiler, or both (default)

# Test statistics
INTERPRETER_PASSED=0
INTERPRETER_FAILED=0
COMPILER_PASSED=0
COMPILER_FAILED=0
TOTAL_TESTS=0
SKIPPED_TESTS=0

mkdir -p "$TEMP_DIR"

# Cleanup on exit
cleanup() {
    rm -rf "$TEMP_DIR"
}
trap cleanup EXIT

# Print header
print_header() {
    echo ""
    echo -e "${CYAN}=============================================================${NC}"
    echo -e "${CYAN}  Cb Unified Integration Test Suite${NC}"
    echo -e "${CYAN}  Mode: $MODE${NC}"
    echo -e "${CYAN}=============================================================${NC}"
    echo ""
}

# Run a single test in interpreter mode
run_interpreter_test() {
    local test_file="$1"
    local test_name="$2"
    
    if "$CB_BINARY" run "$test_file" > "$TEMP_DIR/interp_${test_name}.log" 2>&1; then
        return 0
    else
        return 1
    fi
}

# Run a single test in compiler mode
run_compiler_test() {
    local test_file="$1"
    local test_name="$2"
    local output_binary="$TEMP_DIR/${test_name}_compiled"
    
    # Compile
    if ! "$CB_BINARY" compile "$test_file" -o "$output_binary" > "$TEMP_DIR/compile_${test_name}.log" 2>&1; then
        return 1
    fi
    
    # Run
    if ! "$output_binary" > "$TEMP_DIR/run_${test_name}.log" 2>&1; then
        rm -f "$output_binary"
        return 1
    fi
    
    rm -f "$output_binary"
    return 0
}

# Compare outputs
compare_outputs() {
    local interp_log="$1"
    local compiler_log="$2"
    
    if diff -q "$interp_log" "$compiler_log" > /dev/null 2>&1; then
        return 0
    else
        return 1
    fi
}

# Test a single case
test_case() {
    local test_path="$1"
    local test_name=$(basename "$test_path" .cb)
    local rel_path="${test_path#$TEST_CASES_DIR/}"
    
    TOTAL_TESTS=$((TOTAL_TESTS + 1))
    
    local interp_result=""
    local compiler_result=""
    local status_line=""
    
    # Test in interpreter mode
    if [ "$MODE" = "interpreter" ] || [ "$MODE" = "both" ]; then
        if run_interpreter_test "$test_path" "$test_name"; then
            INTERPRETER_PASSED=$((INTERPRETER_PASSED + 1))
            interp_result="${GREEN}✓ INTERP${NC}"
        else
            INTERPRETER_FAILED=$((INTERPRETER_FAILED + 1))
            interp_result="${RED}✗ INTERP${NC}"
        fi
        status_line="$interp_result"
    fi
    
    # Test in compiler mode
    if [ "$MODE" = "compiler" ] || [ "$MODE" = "both" ]; then
        if run_compiler_test "$test_path" "$test_name"; then
            COMPILER_PASSED=$((COMPILER_PASSED + 1))
            compiler_result="${GREEN}✓ COMP${NC}"
        else
            COMPILER_FAILED=$((COMPILER_FAILED + 1))
            compiler_result="${RED}✗ COMP${NC}"
        fi
        
        if [ -n "$status_line" ]; then
            status_line="$status_line | $compiler_result"
        else
            status_line="$compiler_result"
        fi
    fi
    
    # Compare outputs if both modes were run and passed
    if [ "$MODE" = "both" ] && [ $INTERPRETER_PASSED -gt 0 ] && [ $COMPILER_PASSED -gt 0 ]; then
        if compare_outputs "$TEMP_DIR/interp_${test_name}.log" "$TEMP_DIR/run_${test_name}.log"; then
            status_line="$status_line ${BLUE}[OUTPUT MATCH]${NC}"
        else
            status_line="$status_line ${YELLOW}[OUTPUT DIFF]${NC}"
        fi
    fi
    
    echo -e "$status_line $rel_path"
}

# Discover and run all test files
run_all_tests() {
    echo -e "${BLUE}Discovering test files...${NC}"
    
    local test_files=()
    while IFS= read -r -d '' file; do
        test_files+=("$file")
    done < <(find "$TEST_CASES_DIR" -name "*.cb" -type f -print0 | sort -z)
    
    echo -e "${BLUE}Found ${#test_files[@]} test files${NC}"
    echo ""
    
    for test_file in "${test_files[@]}"; do
        test_case "$test_file"
    done
}

# Print summary
print_summary() {
    echo ""
    echo -e "${CYAN}=============================================================${NC}"
    echo -e "${CYAN}  Test Summary${NC}"
    echo -e "${CYAN}=============================================================${NC}"
    
    if [ "$MODE" = "interpreter" ] || [ "$MODE" = "both" ]; then
        echo -e "${BLUE}Interpreter Mode:${NC}"
        echo -e "  Total:  $((INTERPRETER_PASSED + INTERPRETER_FAILED)) tests"
        echo -e "  ${GREEN}Passed: $INTERPRETER_PASSED${NC}"
        echo -e "  ${RED}Failed: $INTERPRETER_FAILED${NC}"
        echo ""
    fi
    
    if [ "$MODE" = "compiler" ] || [ "$MODE" = "both" ]; then
        echo -e "${BLUE}Compiler Mode:${NC}"
        echo -e "  Total:  $((COMPILER_PASSED + COMPILER_FAILED)) tests"
        echo -e "  ${GREEN}Passed: $COMPILER_PASSED${NC}"
        echo -e "  ${RED}Failed: $COMPILER_FAILED${NC}"
        echo ""
    fi
    
    echo -e "Total Tests: $TOTAL_TESTS"
    
    # Exit with error if any tests failed
    local total_failed=$((INTERPRETER_FAILED + COMPILER_FAILED))
    if [ $total_failed -gt 0 ]; then
        echo -e "${RED}❌ $total_failed test(s) failed${NC}"
        exit 1
    else
        echo -e "${GREEN}✅ All tests passed!${NC}"
        exit 0
    fi
}

# Main execution
print_header
run_all_tests
print_summary
