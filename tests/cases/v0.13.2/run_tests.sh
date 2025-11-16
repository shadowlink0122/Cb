#!/bin/bash
# v0.13.2 Test Runner
# Comprehensive test suite for v0.13.2 features

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/../../.." && pwd)"
cd "$PROJECT_ROOT"

# Ensure binary exists
if [ ! -f "./main" ]; then
    echo "Error: ./main binary not found. Please run 'make' first."
    exit 1
fi

echo "╔═══════════════════════════════════════════════════════════╗"
echo "║         Cb v0.13.2 Comprehensive Test Suite              ║"
echo "╚═══════════════════════════════════════════════════════════╝"
echo ""

PASSED=0
FAILED=0
TOTAL=0

run_test() {
    local test_file="$1"
    local test_name="$2"
    TOTAL=$((TOTAL + 1))
    
    echo -n "[$TOTAL] Testing $test_name... "
    
    if ./main "$test_file" > /tmp/test_output_$$.txt 2>&1; then
        echo "✅ PASS"
        PASSED=$((PASSED + 1))
        return 0
    else
        echo "❌ FAIL"
        FAILED=$((FAILED + 1))
        echo "    Output:"
        cat /tmp/test_output_$$.txt | head -10 | sed 's/^/    /'
        return 1
    fi
}

echo "=== Core v0.13.2 Features ==="
echo ""

# 1. Async Lambda Tests
run_test "tests/cases/async/test_async_lambda_basic.cb" "Async Lambda - Basic"
run_test "tests/cases/async/test_async_lambda_complex.cb" "Async Lambda - Complex"
run_test "tests/cases/async/test_async_lambda_params.cb" "Async Lambda - Parameters"

# 2. Generic String Array Tests
run_test "tests/cases/v0.13.2/test_comprehensive.cb" "v0.13.2 - Comprehensive"
run_test "tests/cases/v0.13.2/test_edge_cases.cb" "v0.13.2 - Edge Cases"

echo ""
echo "=== Regression Tests ==="
echo ""

# 3. Make sure existing tests still pass
echo -n "[$((TOTAL + 1))] Running full test suite... "
if make test > /tmp/full_test_$$.log 2>&1; then
    echo "✅ PASS"
    PASSED=$((PASSED + 1))
    TOTAL=$((TOTAL + 1))
    
    # Extract summary
    grep "Test suites:" /tmp/full_test_$$.log | tail -1 | sed 's/^/    /'
else
    echo "❌ FAIL"
    FAILED=$((FAILED + 1))
    TOTAL=$((TOTAL + 1))
    echo "    Last 20 lines:"
    tail -20 /tmp/full_test_$$.log | sed 's/^/    /'
fi

echo ""
echo "╔═══════════════════════════════════════════════════════════╗"
echo "║                   Test Summary                            ║"
echo "╠═══════════════════════════════════════════════════════════╣"
printf "║  Total Tests:  %-42s ║\n" "$TOTAL tests"
printf "║  Passed:       %-42s ║\n" "$PASSED tests (✅)"
printf "║  Failed:       %-42s ║\n" "$FAILED tests (❌)"
echo "╠═══════════════════════════════════════════════════════════╣"

if [ $FAILED -eq 0 ]; then
    echo "║  🎉 All v0.13.2 Tests Passed Successfully! 🎉            ║"
    echo "╚═══════════════════════════════════════════════════════════╝"
    exit 0
else
    echo "║  ⚠️  Some tests failed. Please review the output.        ║"
    echo "╚═══════════════════════════════════════════════════════════╝"
    exit 1
fi
