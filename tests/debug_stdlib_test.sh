#!/bin/bash

# Debug script for stdlib tests
# This script helps identify which specific test is causing segfaults

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"
CB_BINARY="$PROJECT_ROOT/cb"
STDLIB_TEST="$PROJECT_ROOT/tests/cases/stdlib/test_stdlib_all.cb"

# Color codes
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m'

echo -e "${BLUE}=============================================================${NC}"
echo -e "${BLUE}  Stdlib Test Debugger${NC}"
echo -e "${BLUE}=============================================================${NC}"
echo ""

# Test 1: Try interpreter mode first
echo -e "${YELLOW}[1/3] Testing with interpreter mode...${NC}"
if "$CB_BINARY" run "$STDLIB_TEST"; then
    echo -e "${GREEN}✓ Interpreter mode: PASSED${NC}"
else
    echo -e "${RED}✗ Interpreter mode: FAILED${NC}"
fi
echo ""

# Test 2: Compile only (no run)
echo -e "${YELLOW}[2/3] Testing compilation only...${NC}"
if "$CB_BINARY" compile "$STDLIB_TEST" -o /tmp/cb_stdlib_debug > /tmp/cb_stdlib_compile.log 2>&1; then
    echo -e "${GREEN}✓ Compilation: SUCCEEDED${NC}"
    echo "Binary size: $(wc -c < /tmp/cb_stdlib_debug) bytes"
else
    echo -e "${RED}✗ Compilation: FAILED${NC}"
    echo "Error log:"
    cat /tmp/cb_stdlib_compile.log
    exit 1
fi
echo ""

# Test 3: Run with debugger hints
echo -e "${YELLOW}[3/3] Testing execution with debugging...${NC}"
echo "If this crashes, the crash location will help identify the issue."
echo ""

# Try to run with timeout to avoid infinite loops
if timeout 10s /tmp/cb_stdlib_debug 2>&1; then
    echo -e "${GREEN}✓ Execution: COMPLETED${NC}"
else
    EXIT_CODE=$?
    echo ""
    if [ $EXIT_CODE -eq 124 ]; then
        echo -e "${RED}✗ Execution: TIMEOUT (infinite loop?)${NC}"
    elif [ $EXIT_CODE -eq 139 ]; then
        echo -e "${RED}✗ Execution: SEGMENTATION FAULT${NC}"
    elif [ $EXIT_CODE -eq 134 ]; then
        echo -e "${RED}✗ Execution: ABORTED (assertion failure)${NC}"
    else
        echo -e "${RED}✗ Execution: FAILED (exit code: $EXIT_CODE)${NC}"
    fi
    echo ""
    echo "To debug further, run:"
    echo "  lldb /tmp/cb_stdlib_debug"
    echo "  (lldb) run"
    echo "  (lldb) bt"
fi

# Cleanup
rm -f /tmp/cb_stdlib_debug

echo ""
echo -e "${BLUE}=============================================================${NC}"
