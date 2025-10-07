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
COMMON_OBJS=COMMON_OBJS = \
	../../src/backend/interpreter/core/interpreter.o \
	../../src/backend/interpreter/core/error_handler.o \
	../../src/backend/interpreter/core/pointer_metadata.o \
	../../src/backend/interpreter/core/type_inference.o \
	../../src/backend/interpreter/evaluator/expression_address_ops.o \
	../../src/backend/interpreter/evaluator/expression_array_access.o \
	../../src/backend/interpreter/evaluator/expression_assignment.o \
	../../src/backend/interpreter/evaluator/expression_binary_unary_typed.o \
	../../src/backend/interpreter/evaluator/expression_dispatcher.o \
	../../src/backend/interpreter/evaluator/expression_evaluator.o \
	../../src/backend/interpreter/evaluator/expression_function_call_impl.o \
	../../src/backend/interpreter/evaluator/expression_function_call.o \
	../../src/backend/interpreter/evaluator/expression_helpers.o \
	../../src/backend/interpreter/evaluator/expression_incdec.o \
	../../src/backend/interpreter/evaluator/expression_literal_eval.o \
	../../src/backend/interpreter/evaluator/expression_member_access_impl.o \
	../../src/backend/interpreter/evaluator/expression_member_helpers.o \
	../../src/backend/interpreter/evaluator/expression_receiver_resolution.o \
	../../src/backend/interpreter/evaluator/expression_special_access.o \
	../../src/backend/interpreter/evaluator/expression_ternary.o \
	../../src/backend/interpreter/executors/statement_executor.o \
	../../src/backend/interpreter/executors/control_flow_executor.o \
	../../src/backend/interpreter/executors/statement_list_executor.o \
	../../src/backend/interpreter/handlers/return_handler.o \
	../../src/backend/interpreter/handlers/assertion_handler.o \
	../../src/backend/interpreter/handlers/break_continue_handler.o \
	../../src/backend/interpreter/handlers/function_declaration_handler.o \
	../../src/backend/interpreter/handlers/struct_declaration_handler.o \
	../../src/backend/interpreter/handlers/interface_declaration_handler.o \
	../../src/backend/interpreter/handlers/impl_declaration_handler.o \
	../../src/backend/interpreter/handlers/expression_statement_handler.o \
	../../src/backend/interpreter/managers/variable_manager.o \
	../../src/backend/interpreter/managers/array_manager.o \
	../../src/backend/interpreter/managers/type_manager.o \
	../../src/backend/interpreter/managers/enum_manager.o \
	../../src/backend/interpreter/managers/static_variable_manager.o \
	../../src/backend/interpreter/managers/interface_operations.o \
	../../src/backend/interpreter/managers/struct_operations.o \
	../../src/backend/interpreter/managers/common_operations.o \
	../../src/backend/interpreter/output/output_manager.o \
	../../src/backend/interpreter/services/expression_service.o \
	../../src/backend/interpreter/services/variable_access_service.o \
	../../src/backend/interpreter/services/debug_service.o \
	../../src/backend/interpreter/services/array_processing_service.o

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
