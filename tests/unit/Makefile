# Unit Test Makefile for Cb Language (Header-only)
CXX=g++
CXXFLAGS=-Wall -g -std=c++17 -I../../src -I.

# ディレクトリ設定
SRC_DIR=../../src
FRONTEND_DIR=$(SRC_DIR)/frontend
BACKEND_DIR=$(SRC_DIR)/backend
COMMON_DIR=$(SRC_DIR)/common

# 共通オブジェクトファイル（メインの実行ファイル以外）
COMMON_OBJS=$(COMMON_DIR)/type_utils.o $(FRONTEND_DIR)/parser_utils.o $(FRONTEND_DIR)/debug_impl.o $(FRONTEND_DIR)/debug_messages.o $(BACKEND_DIR)/interpreter.o

# 実行ファイル
TEST_TARGET=test_main

.PHONY: all test clean help

all: $(TEST_TARGET)

# 共通オブジェクトファイルを先にビルド
$(COMMON_DIR)/%.o: $(COMMON_DIR)/%.cpp
	@cd ../../ && $(MAKE) $@

$(FRONTEND_DIR)/%.o: $(FRONTEND_DIR)/%.cpp
	@cd ../../ && $(MAKE) $@

$(BACKEND_DIR)/%.o: $(BACKEND_DIR)/%.cpp
	@cd ../../ && $(MAKE) $@

# テスト実行ファイル（ヘッダーオンリーなのでmain.cppだけをコンパイル）
$(TEST_TARGET): main.cpp $(COMMON_OBJS)
	$(CXX) $(CXXFLAGS) -o $(TEST_TARGET) main.cpp $(COMMON_OBJS)

# テスト実行
test: $(TEST_TARGET)
	@echo "Running unit tests..."
	@./$(TEST_TARGET)

# クリーンアップ
clean:
	rm -f $(TEST_TARGET)
	rm -rf *.dSYM
	rm -rf $(TEST_TARGET).dSYM

# ヘルプ
help:
	@echo "Available targets:"
	@echo "  all     - Build test executable"
	@echo "  test    - Build and run tests"
	@echo "  clean   - Remove generated files"
	@echo "  help    - Show this help"
	@echo ""
	@echo "Test structure:"
	@echo "  framework/   - Test framework and utilities (header-only)"
	@echo "  common/      - Tests for src/common/ modules"
	@echo "  frontend/    - Tests for src/frontend/ modules"
	@echo "  backend/     - Tests for src/backend/ modules"
