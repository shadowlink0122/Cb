#!/bin/bash

# Cb FFI Test Runner
# 各言語で実装されたライブラリをテストする

set -e

# カラー出力
GREEN='\033[0;32m'
RED='\033[0;31m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

echo "================================"
echo "Cb FFI Test Suite"
echo "================================"
echo ""

# Cbインタプリタのビルド
echo -e "${YELLOW}Building Cb interpreter...${NC}"
cd /cb
make clean && make
if [ ! -f "/cb/main" ]; then
    echo -e "${RED}Failed to build Cb interpreter${NC}"
    exit 1
fi
echo -e "${GREEN}✓ Cb interpreter built successfully${NC}"
echo ""

# テストディレクトリへ移動
cd /cb/tests/ffi

# stdlib/foreignディレクトリを作成
mkdir -p /cb/stdlib/foreign

# テスト実行関数
run_test() {
    local lang=$1
    local test_name=$2
    
    echo -e "${YELLOW}Testing ${lang}: ${test_name}${NC}"
    
    if [ -f "/cb/tests/ffi/tests/${lang}/${test_name}.cb" ]; then
        cd /cb
        /cb/main /cb/tests/ffi/tests/${lang}/${test_name}.cb
        if [ $? -eq 0 ]; then
            echo -e "${GREEN}✓ ${lang}/${test_name} passed${NC}"
            return 0
        else
            echo -e "${RED}✗ ${lang}/${test_name} failed${NC}"
            return 1
        fi
    else
        echo -e "${YELLOW}⊘ ${lang}/${test_name}.cb not found${NC}"
        return 0
    fi
}

# C言語のテスト
test_c() {
    echo "================================"
    echo "Testing C FFI"
    echo "================================"
    
    cd /cb/tests/ffi/libs/c
    echo "Building C library..."
    make clean
    make
    
    echo "Checking if library was built..."
    ls -la *.so || echo "ERROR: No .so files found!"
    
    echo "Creating foreign directory..."
    mkdir -p /cb/stdlib/foreign
    
    echo "Copying library..."
    cp -v *.so /cb/stdlib/foreign/ || echo "ERROR: Failed to copy library!"
    
    echo "Verifying library in foreign directory..."
    ls -la /cb/stdlib/foreign/*.so || echo "ERROR: No libraries in foreign directory!"
    
    cd /cb/tests/ffi
    
    run_test "c" "basic_test"
    run_test "c" "math_test"
    run_test "c" "stdlib_test"
    
    echo ""
}

# C++のテスト
test_cpp() {
    echo "================================"
    echo "Testing C++ FFI"
    echo "================================"
    
    cd /cb/tests/ffi/libs/cpp
    echo "Building C++ library..."
    make clean
    make
    
    echo "Copying library..."
    mkdir -p /cb/stdlib/foreign
    cp -v *.so /cb/stdlib/foreign/ || echo "ERROR: Failed to copy library!"
    
    cd /cb/tests/ffi
    
    run_test "cpp" "basic_test"
    run_test "cpp" "std_test"
    
    echo ""
}

# Rustのテスト
test_rust() {
    echo "================================"
    echo "Testing Rust FFI"
    echo "================================"
    
    cd /cb/tests/ffi/libs/rust
    echo "Building Rust library..."
    cargo clean
    cargo build --release
    
    echo "Copying library..."
    mkdir -p /cb/stdlib/foreign
    cp -v target/release/*.so /cb/stdlib/foreign/ 2>/dev/null || echo "ERROR: Failed to copy library!"
    
    cd /cb/tests/ffi
    
    run_test "rust" "basic_test"
    run_test "rust" "advanced_test"
    
    echo ""
}

# Goのテスト
test_go() {
    echo "================================"
    echo "Testing Go FFI"
    echo "================================"
    
    cd /cb/tests/ffi/libs/go
    echo "Building Go library..."
    make clean
    make
    
    echo "Copying library..."
    mkdir -p /cb/stdlib/foreign
    cp -v *.so /cb/stdlib/foreign/ || echo "ERROR: Failed to copy library!"
    
    cd /cb/tests/ffi
    
    run_test "go" "basic_test"
    run_test "go" "concurrent_test"
    
    echo ""
}

# Zigのテスト
test_zig() {
    echo "================================"
    echo "Testing Zig FFI"
    echo "================================"
    
    cd /cb/tests/ffi/libs/zig
    echo "Building Zig library..."
    make clean
    make
    
    echo "Copying library..."
    mkdir -p /cb/stdlib/foreign
    cp -v *.so /cb/stdlib/foreign/ || echo "ERROR: Failed to copy library!"
    
    cd /cb/tests/ffi
    
    run_test "zig" "basic_test"
    run_test "zig" "math_test"
    
    echo ""
}

# メイン処理
if [ $# -eq 0 ] || [ "$1" == "all" ]; then
    # すべてのテストを実行
    test_c
    test_cpp
    test_rust
    test_go
    test_zig
else
    # 指定された言語のみテスト
    case "$1" in
        c)
            test_c
            ;;
        cpp)
            test_cpp
            ;;
        rust)
            test_rust
            ;;
        go)
            test_go
            ;;
        zig)
            test_zig
            ;;
        *)
            echo "Unknown language: $1"
            echo "Usage: $0 [all|c|cpp|rust|go|zig]"
            exit 1
            ;;
    esac
fi

echo "================================"
echo -e "${GREEN}All FFI tests completed!${NC}"
echo "================================"
