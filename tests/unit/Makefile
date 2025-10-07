# Unit Test Makefile for Cb Language (Reorganized)
CXX=g++
# CXXFLAGSを新しいincludeパスに更新
CXXFLAGS=-Wall -g -std=c++17 -I../../src -I../../src/backend/interpreter -I.

# ディレクトリ設定
SRC_DIR=../../src
FRONTEND_DIR=$(SRC_DIR)/frontend
BACKEND_DIR=$(SRC_DIR)/backend
COMMON_DIR=$(SRC_DIR)/common

# 共通オブジェクトファイル（メインの実行ファイル以外）
COMMON_OBJS=$(COMMON_DIR)/type_utils.o $(COMMON_DIR)/type_alias.o $(COMMON_DIR)/array_type_info.o $(COMMON_DIR)/debug_impl.o $(COMMON_DIR)/debug_messages.o $(BACKEND_DIR)/interpreter/core/interpreter.o $(BACKEND_DIR)/interpreter/core/error_handler.o $(BACKEND_DIR)/interpreter/core/type_inference.o $(BACKEND_DIR)/interpreter/core/pointer_metadata.o $(BACKEND_DIR)/interpreter/output/output_manager.o $(BACKEND_DIR)/interpreter/managers/variable_manager.o $(BACKEND_DIR)/interpreter/managers/array_manager.o $(BACKEND_DIR)/interpreter/managers/type_manager.o $(BACKEND_DIR)/interpreter/managers/enum_manager.o $(BACKEND_DIR)/interpreter/managers/common_operations.o $(BACKEND_DIR)/interpreter/services/expression_service.o $(BACKEND_DIR)/interpreter/services/variable_access_service.o $(BACKEND_DIR)/interpreter/services/debug_service.o $(BACKEND_DIR)/interpreter/services/array_processing_service.o $(BACKEND_DIR)/interpreter/evaluator/expression_evaluator.o $(BACKEND_DIR)/interpreter/evaluator/expression_helpers.o $(BACKEND_DIR)/interpreter/evaluator/expression_address_ops.o $(BACKEND_DIR)/interpreter/evaluator/expression_array_access.o $(BACKEND_DIR)/interpreter/evaluator/expression_function_call.o $(BACKEND_DIR)/interpreter/evaluator/expression_incdec.o $(BACKEND_DIR)/interpreter/evaluator/expression_assignment.o $(BACKEND_DIR)/interpreter/executor/statement_executor.o $(COMMON_DIR)/io_interface.o $(COMMON_DIR)/utf8_utils.o $(SRC_DIR)/platform/native/native_stdio_output.o $(SRC_DIR)/platform/baremetal/baremetal_uart_output.o

# 実行ファイル
TEST_TARGET=test_main

# ダミー関数オブジェクト
DUMMY_OBJS=dummy.o

.PHONY: all test clean help

all: $(TEST_TARGET)

# 共通オブジェクトファイルを先にビルド
$(COMMON_DIR)/%.o: $(COMMON_DIR)/%.cpp
	@cd ../../ && $(MAKE) $@



$(BACKEND_DIR)/%.o: $(BACKEND_DIR)/%.cpp
	@cd ../../ && $(MAKE) $@

# Platform object files
$(SRC_DIR)/platform/native/%.o: $(SRC_DIR)/platform/native/%.cpp
	@cd ../../ && $(MAKE) $@

$(SRC_DIR)/platform/baremetal/%.o: $(SRC_DIR)/platform/baremetal/%.cpp
	@cd ../../ && $(MAKE) $@

# Backend subdirectory object files
$(BACKEND_DIR)/interpreter/core/%.o: $(BACKEND_DIR)/interpreter/core/%.cpp
	@cd ../../ && $(MAKE) $@

$(BACKEND_DIR)/interpreter/managers/%.o: $(BACKEND_DIR)/interpreter/managers/%.cpp
	@cd ../../ && $(MAKE) $@

$(BACKEND_DIR)/interpreter/services/%.o: $(BACKEND_DIR)/interpreter/services/%.cpp
	@cd ../../ && $(MAKE) $@

$(BACKEND_DIR)/interpreter/output/%.o: $(BACKEND_DIR)/interpreter/output/%.cpp
	@cd ../../ && $(MAKE) $@

$(BACKEND_DIR)/interpreter/evaluator/%.o: $(BACKEND_DIR)/interpreter/evaluator/%.cpp
	@cd ../../ && $(MAKE) $@

$(BACKEND_DIR)/interpreter/executor/%.o: $(BACKEND_DIR)/interpreter/executor/%.cpp
	@cd ../../ && $(MAKE) $@

# ダミー関数オブジェクト
dummy.o: dummy.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $<

# テスト実行ファイル（ヘッダーオンリーなのでmain.cppだけをコンパイル）
$(TEST_TARGET): main.cpp $(COMMON_OBJS) $(DUMMY_OBJS)
	$(CXX) $(CXXFLAGS) -o $(TEST_TARGET) main.cpp $(COMMON_OBJS) $(DUMMY_OBJS)

# テスト実行
test: $(TEST_TARGET)
	@echo "Running unit tests..."
	@./$(TEST_TARGET)

# クリーンアップ
clean:
	rm -f $(TEST_TARGET)
	rm -f $(DUMMY_OBJS)
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
	@echo "  framework/   - Test framework and utilities"
	@echo "  common/      - Tests for src/common/ modules"
	@echo "  frontend/    - Tests for src/frontend/ modules"
	@echo "  backend/     - Tests for src/backend/ modules"
