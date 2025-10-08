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
COMMON_OBJS = \
	../../src/backend/interpreter/core/interpreter.o \
	../../src/backend/interpreter/core/error_handler.o \
	../../src/backend/interpreter/core/pointer_metadata.o \
	../../src/backend/interpreter/core/type_inference.o \
	../../src/backend/interpreter/evaluator/core/evaluator.o \
	../../src/backend/interpreter/evaluator/core/dispatcher.o \
	../../src/backend/interpreter/evaluator/core/helpers.o \
	../../src/backend/interpreter/evaluator/operators/binary_unary.o \
	../../src/backend/interpreter/evaluator/operators/assignment.o \
	../../src/backend/interpreter/evaluator/operators/incdec.o \
	../../src/backend/interpreter/evaluator/operators/ternary.o \
	../../src/backend/interpreter/evaluator/access/array.o \
	../../src/backend/interpreter/evaluator/access/member.o \
	../../src/backend/interpreter/evaluator/access/member_helpers.o \
	../../src/backend/interpreter/evaluator/access/special.o \
	../../src/backend/interpreter/evaluator/access/address_ops.o \
	../../src/backend/interpreter/evaluator/access/receiver_resolution.o \
	../../src/backend/interpreter/evaluator/functions/call.o \
	../../src/backend/interpreter/evaluator/functions/call_impl.o \
	../../src/backend/interpreter/evaluator/literals/eval.o \
	../../src/backend/interpreter/executors/statement_executor.o \
	../../src/backend/interpreter/executors/control_flow_executor.o \
	../../src/backend/interpreter/executors/statement_list_executor.o \
	../../src/backend/interpreter/executors/assignments/simple_assignment.o \
	../../src/backend/interpreter/executors/assignments/member_assignment.o \
	../../src/backend/interpreter/executors/declarations/array_declaration.o \
	../../src/backend/interpreter/executors/declarations/variable_declaration.o \
	../../src/backend/interpreter/handlers/control/return.o \
	../../src/backend/interpreter/handlers/control/assertion.o \
	../../src/backend/interpreter/handlers/control/break_continue.o \
	../../src/backend/interpreter/handlers/declarations/function.o \
	../../src/backend/interpreter/handlers/declarations/struct.o \
	../../src/backend/interpreter/handlers/declarations/interface.o \
	../../src/backend/interpreter/handlers/declarations/impl.o \
	../../src/backend/interpreter/handlers/statements/expression.o \
	../../src/backend/interpreter/managers/variables/manager.o \
	../../src/backend/interpreter/managers/variables/declaration.o \
	../../src/backend/interpreter/managers/variables/assignment.o \
	../../src/backend/interpreter/managers/variables/initialization.o \
	../../src/backend/interpreter/managers/variables/static.o \
	../../src/backend/interpreter/managers/arrays/manager.o \
	../../src/backend/interpreter/managers/types/manager.o \
	../../src/backend/interpreter/managers/types/enums.o \
	../../src/backend/interpreter/managers/types/interfaces.o \
	../../src/backend/interpreter/managers/structs/operations.o \
	../../src/backend/interpreter/managers/structs/member_variables.o \
	../../src/backend/interpreter/managers/structs/assignment.o \
	../../src/backend/interpreter/managers/structs/sync.o \
	../../src/backend/interpreter/managers/common/operations.o \
	../../src/backend/interpreter/managers/common/global_init.o \
	../../src/backend/interpreter/output/output_manager.o \
	../../src/backend/interpreter/services/expression_service.o \
	../../src/backend/interpreter/services/variable_access_service.o \
	../../src/backend/interpreter/services/debug_service.o \
	../../src/backend/interpreter/services/array_processing_service.o \
	../../src/common/type_utils.o \
	../../src/common/type_alias.o \
	../../src/common/array_type_info.o \
	../../src/common/utf8_utils.o \
	../../src/common/io_interface.o \
	../../src/common/debug_impl.o \
	../../src/common/debug_messages.o \
	../../src/platform/native/native_stdio_output.o \
	../../src/platform/baremetal/baremetal_uart_output.o

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

$(BACKEND_DIR)/interpreter/executors/%.o: $(BACKEND_DIR)/interpreter/executors/%.cpp
	@cd ../../ && $(MAKE) $@

$(BACKEND_DIR)/interpreter/handlers/%.o: $(BACKEND_DIR)/interpreter/handlers/%.cpp
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
