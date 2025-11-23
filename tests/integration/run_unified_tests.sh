#!/bin/bash

# Unified Test Runner for Cb Language
# This script runs the same test cases in both INTERPRETER and COMPILER modes
# to ensure consistency between both execution methods

set -e

# Color codes
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
CYAN='\033[0;36m'
NC='\033[0m' # No Color

# Get script directory
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"

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
    echo -e "${CYAN}  Cb Unified Test Suite${NC}"
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

# Test a single case
test_case() {
    local test_path="$1"
    local test_name=$(basename "$test_path" .cb)
    local rel_path="${test_path#$TEST_CASES_DIR/}"
    
    TOTAL_TESTS=$((TOTAL_TESTS + 1))
    
    local interp_result=""
    local compiler_result=""
    
    # Test in interpreter mode
    if [ "$MODE" = "interpreter" ] || [ "$MODE" = "both" ]; then
        if run_interpreter_test "$test_path" "$test_name"; then
            INTERPRETER_PASSED=$((INTERPRETER_PASSED + 1))
            interp_result="${GREEN}✓ INT${NC}"
        else
            INTERPRETER_FAILED=$((INTERPRETER_FAILED + 1))
            interp_result="${RED}✗ INT${NC}"
        fi
    fi
    
    # Test in compiler mode
    if [ "$MODE" = "compiler" ] || [ "$MODE" = "both" ]; then
        if run_compiler_test "$test_path" "$test_name"; then
            COMPILER_PASSED=$((COMPILER_PASSED + 1))
            compiler_result="${GREEN}✓ COM${NC}"
        else
            COMPILER_FAILED=$((COMPILER_FAILED + 1))
            compiler_result="${RED}✗ COM${NC}"
        fi
    fi
    
    # Print result
    if [ "$MODE" = "both" ]; then
        echo -e "  $interp_result $compiler_result  $rel_path"
    elif [ "$MODE" = "interpreter" ]; then
        echo -e "  $interp_result  $rel_path"
    else
        echo -e "  $compiler_result  $rel_path"
    fi
}

# Main execution
print_header

# Define test categories and files
# These are tests that should work in both modes
declare -A TEST_CATEGORIES=(
    ["basic"]="simple_main.cb hello_world.cb"
    ["arithmetic"]="ok.cb simple.cb"
    ["println"]="single_arg.cb multiple_args.cb"
    ["if"]="basic.cb else_branch.cb nested.cb"
    ["loop"]="while_basic.cb for_basic.cb"
    ["function"]="simple.cb with_params.cb return_value.cb"
    ["variable"]="declaration.cb initialization.cb"
    ["string"]="concat.cb comparison.cb"
    ["array"]="declaration.cb access.cb"
)

echo -e "${BLUE}Running test cases...${NC}"
echo ""

for category in "${!TEST_CATEGORIES[@]}"; do
    echo -e "${YELLOW}[$category]${NC}"
    
    for test_file in ${TEST_CATEGORIES[$category]}; do
        test_path="$TEST_CASES_DIR/$category/$test_file"
        
        if [ -f "$test_path" ]; then
            test_case "$test_path"
        else
            echo -e "  ${YELLOW}⚠ SKIP${NC}  $category/$test_file (not found)"
        fi
    done
    
    echo ""
done

# Print summary
echo -e "${CYAN}=============================================================${NC}"
echo -e "${CYAN}  Test Summary${NC}"
echo -e "${CYAN}=============================================================${NC}"
echo ""
echo "Total tests: $TOTAL_TESTS"
echo ""

if [ "$MODE" = "interpreter" ] || [ "$MODE" = "both" ]; then
    echo -e "${BLUE}Interpreter Mode:${NC}"
    echo -e "  ${GREEN}Passed: $INTERPRETER_PASSED${NC}"
    if [ $INTERPRETER_FAILED -gt 0 ]; then
        echo -e "  ${RED}Failed: $INTERPRETER_FAILED${NC}"
    else
        echo -e "  Failed: $INTERPRETER_FAILED"
    fi
    echo ""
fi

if [ "$MODE" = "compiler" ] || [ "$MODE" = "both" ]; then
    echo -e "${BLUE}Compiler Mode:${NC}"
    echo -e "  ${GREEN}Passed: $COMPILER_PASSED${NC}"
    if [ $COMPILER_FAILED -gt 0 ]; then
        echo -e "  ${RED}Failed: $COMPILER_FAILED${NC}"
    else
        echo -e "  Failed: $COMPILER_FAILED"
    fi
    echo ""
fi

echo -e "${CYAN}=============================================================${NC}"

# Exit with appropriate code
if [ "$MODE" = "interpreter" ] && [ $INTERPRETER_FAILED -gt 0 ]; then
    exit 1
elif [ "$MODE" = "compiler" ] && [ $COMPILER_FAILED -gt 0 ]; then
    exit 1
elif [ "$MODE" = "both" ] && [ $((INTERPRETER_FAILED + COMPILER_FAILED)) -gt 0 ]; then
    exit 1
else
    exit 0
fi
