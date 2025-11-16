CC=g++
CXX=g++

# ディレクトリ設定
SRC_DIR=src
FRONTEND_DIR=$(SRC_DIR)/frontend
BACKEND_DIR=$(SRC_DIR)/backend
COMMON_DIR=$(SRC_DIR)/common
PLATFORM_DIR=$(SRC_DIR)/platform
NATIVE_DIR=$(PLATFORM_DIR)/native
BAREMETAL_DIR=$(PLATFORM_DIR)/baremetal
TESTS_DIR=tests
CGEN_DIR=cgen
SAMPLE_FFI_DIR=sample/ffi
STDLIB_FOREIGN_DIR=stdlib/foreign

# Interpreterサブディレクトリ
INTERPRETER_DIR=$(BACKEND_DIR)/interpreter
INTERPRETER_CORE=$(INTERPRETER_DIR)/core
INTERPRETER_EVALUATOR=$(INTERPRETER_DIR)/evaluator
INTERPRETER_EXECUTORS=$(INTERPRETER_DIR)/executors
INTERPRETER_HANDLERS=$(INTERPRETER_DIR)/handlers
INTERPRETER_MANAGERS=$(INTERPRETER_DIR)/managers
INTERPRETER_SERVICES=$(INTERPRETER_DIR)/services
INTERPRETER_OUTPUT=$(INTERPRETER_DIR)/output
INTERPRETER_EVENT_LOOP=$(INTERPRETER_DIR)/event_loop
INTERPRETER_TYPES=$(INTERPRETER_DIR)/types

# v0.14.0: IR（中間表現）サブディレクトリ
IR_DIR=$(BACKEND_DIR)/ir
IR_HIR=$(IR_DIR)/hir
IR_MIR=$(IR_DIR)/mir
IR_LIR=$(IR_DIR)/lir
IR_COMMON=$(IR_DIR)/common

# コンパイラフラグ
CXXFLAGS=-Wall -g -std=c++17
CFLAGS=$(CXXFLAGS) -I. -I$(SRC_DIR) -I$(INTERPRETER_DIR)

# AddressSanitizer用フラグ
ASAN_FLAGS=-fsanitize=address -fno-omit-frame-pointer -O1
ASAN_CXXFLAGS=$(CXXFLAGS) $(ASAN_FLAGS)
ASAN_CFLAGS=$(ASAN_CXXFLAGS) -I. -I$(SRC_DIR) -I$(INTERPRETER_DIR)

# テスト用フラグ
TEST_CXXFLAGS=$(CXXFLAGS) -I$(SRC_DIR) -I$(INTERPRETER_DIR) -I$(TESTS_DIR)/unit

# 生成されるファイル（古いbison/flexファイルは削除）
# PARSER_C=$(FRONTEND_DIR)/parser.c
# PARSER_H=$(FRONTEND_DIR)/parser.h  
# LEXER_C=$(FRONTEND_DIR)/lexer.c

# オブジェクトファイル（RecursiveParserのみ使用）
PARSER_OBJS=$(FRONTEND_DIR)/recursive_parser/parsers/expression_parser.o \
            $(FRONTEND_DIR)/recursive_parser/parsers/primary_expression_parser.o \
            $(FRONTEND_DIR)/recursive_parser/parsers/statement_parser.o \
            $(FRONTEND_DIR)/recursive_parser/parsers/declaration_parser.o \
            $(FRONTEND_DIR)/recursive_parser/parsers/variable_declaration_parser.o \
            $(FRONTEND_DIR)/recursive_parser/parsers/type_parser.o \
            $(FRONTEND_DIR)/recursive_parser/parsers/struct_parser.o \
            $(FRONTEND_DIR)/recursive_parser/parsers/enum_parser.o \
            $(FRONTEND_DIR)/recursive_parser/parsers/interface_parser.o \
            $(FRONTEND_DIR)/recursive_parser/parsers/union_parser.o \
            $(FRONTEND_DIR)/recursive_parser/parsers/type_utility_parser.o
PREPROCESSOR_OBJS=$(FRONTEND_DIR)/preprocessor/preprocessor.o
FRONTEND_OBJS=$(FRONTEND_DIR)/main.o $(FRONTEND_DIR)/help_messages.o $(FRONTEND_DIR)/recursive_parser/recursive_lexer.o $(FRONTEND_DIR)/recursive_parser/recursive_parser.o $(PARSER_OBJS) $(PREPROCESSOR_OBJS)
# Interpreterオブジェクトファイル（グループ化）
INTERPRETER_CORE_OBJS = \
	$(INTERPRETER_CORE)/interpreter.o \
	$(INTERPRETER_CORE)/initialization.o \
	$(INTERPRETER_CORE)/builtin_types.o \
	$(INTERPRETER_CORE)/cleanup.o \
	$(INTERPRETER_CORE)/utility.o \
	$(INTERPRETER_CORE)/error_handler.o \
	$(INTERPRETER_CORE)/pointer_metadata.o \
	$(INTERPRETER_CORE)/type_inference.o

INTERPRETER_EVALUATOR_OBJS = \
	$(INTERPRETER_EVALUATOR)/core/evaluator.o \
	$(INTERPRETER_EVALUATOR)/core/dispatcher.o \
	$(INTERPRETER_EVALUATOR)/core/helpers.o \
	$(INTERPRETER_EVALUATOR)/operators/binary_unary.o \
	$(INTERPRETER_EVALUATOR)/operators/assignment.o \
	$(INTERPRETER_EVALUATOR)/operators/incdec.o \
	$(INTERPRETER_EVALUATOR)/operators/ternary.o \
	$(INTERPRETER_EVALUATOR)/operators/error_handling.o \
	$(INTERPRETER_EVALUATOR)/operators/memory_operators.o \
	$(INTERPRETER_EVALUATOR)/access/array.o \
	$(INTERPRETER_EVALUATOR)/access/member.o \
	$(INTERPRETER_EVALUATOR)/access/member_helpers.o \
	$(INTERPRETER_EVALUATOR)/access/special.o \
	$(INTERPRETER_EVALUATOR)/access/address_ops.o \
	$(INTERPRETER_EVALUATOR)/access/receiver_resolution.o \
	$(INTERPRETER_EVALUATOR)/functions/call.o \
	$(INTERPRETER_EVALUATOR)/functions/call_impl.o \
	$(INTERPRETER_EVALUATOR)/functions/generic_instantiation.o \
	$(INTERPRETER_EVALUATOR)/literals/eval.o

INTERPRETER_EXECUTORS_OBJS = \
	$(INTERPRETER_EXECUTORS)/statement_executor.o \
	$(INTERPRETER_EXECUTORS)/control_flow_executor.o \
	$(INTERPRETER_EXECUTORS)/statement_list_executor.o \
	$(INTERPRETER_EXECUTORS)/assignments/simple_assignment.o \
	$(INTERPRETER_EXECUTORS)/assignments/member_assignment.o \
	$(INTERPRETER_EXECUTORS)/declarations/array_declaration.o \
	$(INTERPRETER_EXECUTORS)/declarations/variable_declaration.o

INTERPRETER_HANDLERS_OBJS = \
	$(INTERPRETER_HANDLERS)/control/return.o \
	$(INTERPRETER_HANDLERS)/control/assertion.o \
	$(INTERPRETER_HANDLERS)/control/break_continue.o \
	$(INTERPRETER_HANDLERS)/declarations/function.o \
	$(INTERPRETER_HANDLERS)/declarations/struct.o \
	$(INTERPRETER_HANDLERS)/declarations/interface.o \
	$(INTERPRETER_HANDLERS)/declarations/impl.o \
	$(INTERPRETER_HANDLERS)/statements/expression.o

INTERPRETER_MANAGERS_OBJS = \
	$(INTERPRETER_MANAGERS)/variables/manager.o \
	$(INTERPRETER_MANAGERS)/variables/declaration.o \
	$(INTERPRETER_MANAGERS)/variables/assignment.o \
	$(INTERPRETER_MANAGERS)/variables/initialization.o \
	$(INTERPRETER_MANAGERS)/variables/static.o \
	$(INTERPRETER_MANAGERS)/arrays/manager.o \
	$(INTERPRETER_MANAGERS)/types/manager.o \
	$(INTERPRETER_MANAGERS)/types/enums.o \
	$(INTERPRETER_MANAGERS)/types/interfaces.o \
	$(INTERPRETER_MANAGERS)/structs/operations.o \
	$(INTERPRETER_MANAGERS)/structs/member_variables.o \
	$(INTERPRETER_MANAGERS)/structs/assignment.o \
	$(INTERPRETER_MANAGERS)/structs/sync.o \
	$(INTERPRETER_MANAGERS)/common/global_init.o \
	$(INTERPRETER_MANAGERS)/common/operations.o

INTERPRETER_SERVICES_OBJS = \
	$(INTERPRETER_SERVICES)/expression_service.o \
	$(INTERPRETER_SERVICES)/variable_access_service.o \
	$(INTERPRETER_SERVICES)/debug_service.o \
	$(INTERPRETER_SERVICES)/array_processing_service.o

INTERPRETER_OUTPUT_OBJS = \
	$(INTERPRETER_OUTPUT)/output_manager.o

INTERPRETER_EVENT_LOOP_OBJS = \
	$(INTERPRETER_EVENT_LOOP)/event_loop.o \
	$(INTERPRETER_EVENT_LOOP)/simple_event_loop.o

INTERPRETER_TYPES_OBJS = \
	$(INTERPRETER_TYPES)/future.o

INTERPRETER_FFI_OBJS = \
	$(INTERPRETER_DIR)/ffi_manager.o

# v0.14.0: IRオブジェクトファイル
IR_HIR_OBJS = \
	$(IR_HIR)/hir_generator.o \
	$(IR_HIR)/hir_node.o \
	$(IR_HIR)/hir_builder.o

# v0.14.0: Codegen (Code Generation) のオブジェクトファイル
CODEGEN_DIR=$(BACKEND_DIR)/codegen
CODEGEN_OBJS = \
	$(CODEGEN_DIR)/hir_to_cpp.o

# 	$(IR_HIR)/hir_visitor.o \
# 	$(IR_HIR)/hir_dumper.o

# IR_MIR_OBJS = \
# 	$(IR_MIR)/mir_generator.o \
# 	$(IR_MIR)/cfg_builder.o \
# 	$(IR_MIR)/ssa_builder.o

IR_OBJS = $(IR_HIR_OBJS) $(CODEGEN_OBJS)
# $(IR_MIR_OBJS)

# Backendオブジェクト（全て統合）
BACKEND_OBJS = \
	$(INTERPRETER_CORE_OBJS) \
	$(INTERPRETER_EVALUATOR_OBJS) \
	$(INTERPRETER_EXECUTORS_OBJS) \
	$(INTERPRETER_HANDLERS_OBJS) \
	$(INTERPRETER_MANAGERS_OBJS) \
	$(INTERPRETER_SERVICES_OBJS) \
	$(INTERPRETER_OUTPUT_OBJS) \
	$(INTERPRETER_EVENT_LOOP_OBJS) \
	$(INTERPRETER_TYPES_OBJS) \
	$(INTERPRETER_FFI_OBJS) \
	$(IR_OBJS)
PLATFORM_OBJS=$(NATIVE_DIR)/native_stdio_output.o $(BAREMETAL_DIR)/baremetal_uart_output.o
# デバッグメッセージモジュール
DEBUG_DIR=$(COMMON_DIR)/debug
DEBUG_OBJS = \
	$(DEBUG_DIR)/debug_parser_messages.o \
	$(DEBUG_DIR)/debug_ast_messages.o \
	$(DEBUG_DIR)/debug_interpreter_messages.o \
	$(DEBUG_DIR)/debug_hir_messages.o

COMMON_OBJS=$(COMMON_DIR)/type_utils.o $(COMMON_DIR)/type_alias.o $(COMMON_DIR)/array_type_info.o $(COMMON_DIR)/utf8_utils.o $(COMMON_DIR)/io_interface.o $(COMMON_DIR)/debug_impl.o $(COMMON_DIR)/debug_messages.o $(DEBUG_OBJS) $(COMMON_DIR)/ast.o $(PLATFORM_OBJS)

# 実行ファイル
MAIN_TARGET=cb
CGEN_TARGET=cgen_main

# OSごとのライブラリ拡張子
UNAME_S := $(shell uname -s)
UNAME_M := $(shell uname -m)
ifeq ($(UNAME_S),Darwin)
    LIB_EXT=dylib
    FFI_CC=/usr/bin/clang++
    # Apple Silicon (arm64) または Intel (x86_64) に対応
    FFI_CXXFLAGS=-arch $(UNAME_M) -std=c++17
else ifeq ($(UNAME_S),Linux)
    LIB_EXT=so
    FFI_CC=g++
    FFI_CXXFLAGS=-std=c++17
else
    LIB_EXT=dll
    FFI_CC=g++
    FFI_CXXFLAGS=-std=c++17
endif

# FFIライブラリ設定
FFI_LIBS=$(STDLIB_FOREIGN_DIR)/libcppexample.$(LIB_EXT) \
         $(STDLIB_FOREIGN_DIR)/libadvanced.$(LIB_EXT)

.PHONY: all clean lint fmt unit-test integration-test integration-test-interpreter integration-test-compiler integration-test-verbose hir-integration-test integration-test-old test test-interpreter test-compiler test-all debug setup-dirs deep-clean clean-all backup-old help install-vscode-extension build-extension clean-extension update-extension-version verify-extension-version ffi-libs clean-ffi test-ffi stdlib-test stdlib-cpp-test stdlib-cb-test stdlib-cb-test-interpreter stdlib-cb-test-compiler

all: setup-dirs $(MAIN_TARGET) ffi-libs

# ディレクトリ作成
setup-dirs:
	@mkdir -p $(FRONTEND_DIR) $(BACKEND_DIR) $(COMMON_DIR) $(NATIVE_DIR) $(BAREMETAL_DIR)
	@mkdir -p $(INTERPRETER_CORE) $(INTERPRETER_EVALUATOR) $(INTERPRETER_EXECUTORS)
	@mkdir -p $(INTERPRETER_HANDLERS) $(INTERPRETER_OUTPUT) $(INTERPRETER_SERVICES)
	@mkdir -p $(INTERPRETER_MANAGERS)/variables $(INTERPRETER_MANAGERS)/arrays
	@mkdir -p $(INTERPRETER_MANAGERS)/structs $(INTERPRETER_MANAGERS)/types $(INTERPRETER_MANAGERS)/common
	@mkdir -p $(INTERPRETER_EVALUATOR)/core $(INTERPRETER_EVALUATOR)/operators
	@mkdir -p $(INTERPRETER_EVALUATOR)/access $(INTERPRETER_EVALUATOR)/functions $(INTERPRETER_EVALUATOR)/literals
	@mkdir -p $(INTERPRETER_EXECUTORS)/declarations $(INTERPRETER_EXECUTORS)/assignments
	@mkdir -p $(INTERPRETER_HANDLERS)/control $(INTERPRETER_HANDLERS)/declarations $(INTERPRETER_HANDLERS)/statements
	@mkdir -p $(INTERPRETER_EVENT_LOOP)
	@mkdir -p $(INTERPRETER_TYPES)
	@mkdir -p $(BACKEND_DIR)/ir $(BACKEND_DIR)/optimizer $(BACKEND_DIR)/codegen
	@mkdir -p $(STDLIB_FOREIGN_DIR)
	@mkdir -p $(SAMPLE_FFI_DIR)

# デバッグ実行例（--debugオプションでデバッグ出力有効）
debug: CFLAGS += -DYYDEBUG=1
debug: $(MAIN_TARGET)
	@echo '例: ./$(MAIN_TARGET) <file>.cb --debug'

# 古いbison/flexルールは削除（RecursiveParserを使用）
# $(LEXER_C): $(FRONTEND_DIR)/lexer.l $(PARSER_H)
# 	$(LEX) -o $(LEXER_C) $(FRONTEND_DIR)/lexer.l

# $(PARSER_C) $(PARSER_H): $(FRONTEND_DIR)/parser.y
# 	$(YACC) -d -o $(PARSER_C) $(FRONTEND_DIR)/parser.y

# RecursiveParserオブジェクト生成
$(FRONTEND_DIR)/recursive_parser/%.o: $(FRONTEND_DIR)/recursive_parser/%.cpp
	$(CC) $(CFLAGS) -c -o $@ $<

# 分離されたパーサーオブジェクト生成
$(FRONTEND_DIR)/recursive_parser/parsers/%.o: $(FRONTEND_DIR)/recursive_parser/parsers/%.cpp
	$(CC) $(CFLAGS) -c -o $@ $<

# フロントエンドオブジェクト生成（古いparser.hの依存関係を削除）
$(FRONTEND_DIR)/%.o: $(FRONTEND_DIR)/%.cpp $(COMMON_DIR)/ast.h
	$(CC) $(CFLAGS) -c -o $@ $<

# バックエンドオブジェクト生成
$(BACKEND_DIR)/%.o: $(BACKEND_DIR)/%.cpp $(COMMON_DIR)/ast.h
	$(CC) $(CFLAGS) -c -o $@ $<

# バックエンドサブディレクトリのオブジェクト生成
$(BACKEND_DIR)/evaluator/%.o: $(BACKEND_DIR)/evaluator/%.cpp $(COMMON_DIR)/ast.h
	$(CC) $(CFLAGS) -c -o $@ $<

$(BACKEND_DIR)/executors/%.o: $(BACKEND_DIR)/executors/%.cpp $(COMMON_DIR)/ast.h
	$(CC) $(CFLAGS) -c -o $@ $<

$(BACKEND_DIR)/handlers/%.o: $(BACKEND_DIR)/handlers/%.cpp $(COMMON_DIR)/ast.h
	$(CC) $(CFLAGS) -c -o $@ $<
	
$(BACKEND_DIR)/output/%.o: $(BACKEND_DIR)/output/%.cpp $(COMMON_DIR)/ast.h
	$(CC) $(CFLAGS) -c -o $@ $<

# マネージャーサブディレクトリのオブジェクト生成
$(BACKEND_DIR)/managers/variables/%.o: $(BACKEND_DIR)/managers/variables/%.cpp $(COMMON_DIR)/ast.h
	$(CC) $(CFLAGS) -c -o $@ $<

$(BACKEND_DIR)/managers/arrays/%.o: $(BACKEND_DIR)/managers/arrays/%.cpp $(COMMON_DIR)/ast.h
	$(CC) $(CFLAGS) -c -o $@ $<

$(BACKEND_DIR)/managers/structs/%.o: $(BACKEND_DIR)/managers/structs/%.cpp $(COMMON_DIR)/ast.h
	$(CC) $(CFLAGS) -c -o $@ $<

$(BACKEND_DIR)/managers/types/%.o: $(BACKEND_DIR)/managers/types/%.cpp $(COMMON_DIR)/ast.h
	$(CC) $(CFLAGS) -c -o $@ $<

$(BACKEND_DIR)/managers/common/%.o: $(BACKEND_DIR)/managers/common/%.cpp $(COMMON_DIR)/ast.h
	$(CC) $(CFLAGS) -c -o $@ $<

# 共通オブジェクト生成
$(COMMON_DIR)/%.o: $(COMMON_DIR)/%.cpp $(COMMON_DIR)/ast.h
	$(CC) $(CFLAGS) -c -o $@ $<

# デバッグメッセージモジュールのコンパイル
$(DEBUG_DIR)/%.o: $(DEBUG_DIR)/%.cpp
	$(CC) $(CFLAGS) -c -o $@ $<

# プラットフォーム固有オブジェクト生成
$(NATIVE_DIR)/%.o: $(NATIVE_DIR)/%.cpp
	$(CC) $(CFLAGS) -c -o $@ $<

$(BAREMETAL_DIR)/%.o: $(BAREMETAL_DIR)/%.cpp
	$(CC) $(CFLAGS) -c -o $@ $<

# メイン実行ファイル
$(MAIN_TARGET): $(FRONTEND_OBJS) $(BACKEND_OBJS) $(COMMON_OBJS)
	$(CC) $(CFLAGS) -o $(MAIN_TARGET) $(FRONTEND_OBJS) $(BACKEND_OBJS) $(COMMON_OBJS)

# AddressSanitizer有効版（メモリ破損検出用）
main-asan: CFLAGS=$(ASAN_CFLAGS)
main-asan: clean
	@echo "Building with AddressSanitizer..."
	$(MAKE) $(MAIN_TARGET) CFLAGS="$(ASAN_CFLAGS)"
	@mv $(MAIN_TARGET) main-asan
	@echo "Built main-asan with AddressSanitizer"

# Cb→Cコード変換ツール（将来の拡張）
$(CGEN_TARGET):
	@echo "Code generator is not implemented yet in new architecture"
	@echo "Use old structure for now: make -f Makefile.old cgen"

# フォーマット
lint:
	@echo "Checking code formatting (dry-run)..."
	@find $(SRC_DIR) -type f \( -name "*.cpp" -o -name "*.h" \) -exec clang-format --dry-run --Werror {} +
	@find $(TESTS_DIR) -type f \( -name "*.cpp" -o -name "*.h" \) -exec clang-format --dry-run --Werror {} + 2>/dev/null || true

fmt:
	@echo "Formatting all source files in src/ and tests/..."
	@find $(SRC_DIR) -type f \( -name "*.cpp" -o -name "*.h" \) -exec clang-format -i {} +
	@find $(TESTS_DIR) -type f \( -name "*.cpp" -o -name "*.h" \) -exec clang-format -i {} + 2>/dev/null || true
	@echo "Formatting complete!"

# 単体テスト用のダミーオブジェクト
$(TESTS_DIR)/unit/dummy.o: $(TESTS_DIR)/unit/dummy.cpp
	$(CXX) $(TEST_CXXFLAGS) -c -o $@ $<

# Unit test binary target
$(TESTS_DIR)/unit/test_main: $(TESTS_DIR)/unit/main.cpp $(TESTS_DIR)/unit/dummy.o $(BACKEND_OBJS) $(COMMON_OBJS) $(PARSER_OBJS) $(FRONTEND_DIR)/recursive_parser/recursive_parser.o $(FRONTEND_DIR)/recursive_parser/recursive_lexer.o
	$(CXX) $(TEST_CXXFLAGS) -o $@ $^

unit-test: $(TESTS_DIR)/unit/test_main
	@echo "============================================================="
	@echo "Running Cb Unit Test Suite"
	@echo "============================================================="
	cd tests/unit && ./test_main

# Integration test binary target
$(TESTS_DIR)/integration/test_main: $(TESTS_DIR)/integration/main.cpp $(MAIN_TARGET)
	@cd tests/integration && $(CC) $(CFLAGS) -I. -o test_main main.cpp

# Integration test - Interpreter mode only
integration-test-interpreter: $(TESTS_DIR)/integration/test_main
	@echo "============================================================="
	@echo "Running Cb Integration Test Suite (INTERPRETER MODE)"
	@echo "============================================================="
	@bash -c "set -o pipefail; cd tests/integration && ./test_main 2>&1 | tee /tmp/cb_integration_interpreter.log | fold -s -w 80"; \
	if grep -q "^Failed: [1-9]" /tmp/cb_integration_interpreter.log; then \
		exit 1; \
	fi

# Integration test - Compiler mode only
integration-test-compiler: $(MAIN_TARGET)
	@echo "============================================================="
	@echo "Running Cb Integration Test Suite (COMPILER MODE)"
	@echo "============================================================="
	@echo "Testing all integration test cases via compilation..."
	@bash tests/integration/run_compiler_tests.sh 2>&1 | tee /tmp/cb_integration_compiler.log | fold -s -w 80; \
	if grep -q "FAILED" /tmp/cb_integration_compiler.log; then \
		exit 1; \
	fi

# Integration test - Both modes (default)
integration-test: integration-test-interpreter integration-test-compiler
	@echo ""
	@echo "✅ Integration tests completed for both INTERPRETER and COMPILER modes"

# より詳細な出力が必要な場合の統合テスト（フル出力）
integration-test-verbose: $(TESTS_DIR)/integration/test_main
	@echo "Running integration tests (verbose mode)..."
	@cd tests/integration && ./test_main

# HIR統合テスト：HIRコンパイル経由で統合テストを実行
# integration-testと同じテストケースをHIRコンパイルしたバイナリで実行
hir-integration-test: $(MAIN_TARGET)
	@echo "============================================================="
	@echo "Running HIR Integration Test Suite"
	@echo "============================================================="
	@echo "Compiling integration test cases via HIR..."
	@bash tests/integration/run_hir_tests.sh 2>&1 | tee /tmp/cb_hir_integration_raw.log | fold -s -w 80; \
	if grep -q "FAILED" /tmp/cb_hir_integration_raw.log; then \
		exit 1; \
	fi

# Stdlib test binary target
$(TESTS_DIR)/stdlib/test_main: $(TESTS_DIR)/stdlib/main.cpp $(MAIN_TARGET)
	@cd tests/stdlib && $(CC) $(CFLAGS) -I../../$(SRC_DIR) -I. -o test_main main.cpp

# Stdlib tests (C++ infrastructure tests)
stdlib-cpp-test: $(TESTS_DIR)/stdlib/test_main
	@echo "============================================================="
	@echo "[1/4] Running Standard Library Tests (C++ Infrastructure)"
	@echo "============================================================="
	@cd tests/stdlib && ./test_main

# Stdlib tests (Cb language tests) - Interpreter mode
stdlib-cb-test-interpreter: $(MAIN_TARGET)
	@echo "============================================================="
	@echo "Running Standard Library Tests (Cb Language - INTERPRETER)"
	@echo "Testing stdlib modules written in Cb"
	@echo "============================================================="
	@./$(MAIN_TARGET) run tests/cases/stdlib/test_stdlib_all.cb || exit 1

# Stdlib tests (Cb language tests) - Compiler mode
stdlib-cb-test-compiler: $(MAIN_TARGET)
	@echo "============================================================="
	@echo "Running Standard Library Tests (Cb Language - COMPILER)"
	@echo "Testing stdlib modules written in Cb"
	@echo "============================================================="
	@./$(MAIN_TARGET) compile tests/cases/stdlib/test_stdlib_all.cb -o /tmp/cb_stdlib_test && \
	/tmp/cb_stdlib_test && \
	rm -f /tmp/cb_stdlib_test || exit 1

# Stdlib tests (Cb language tests) - Both modes
stdlib-cb-test: stdlib-cb-test-interpreter stdlib-cb-test-compiler
	@echo ""
	@echo "✅ Stdlib Cb tests completed for both INTERPRETER and COMPILER modes"

# Run both C++ and Cb stdlib tests
stdlib-test:
	@echo "============================================================="
	@echo "Running Standard Library Test Suite"
	@echo "============================================================="
	@CPP_RESULT=0; CB_RESULT=0; \
	$(MAKE) stdlib-cpp-test || CPP_RESULT=$$?; \
	$(MAKE) stdlib-cb-test || CB_RESULT=$$?; \
	if [ $$CPP_RESULT -eq 0 ] && [ $$CB_RESULT -eq 0 ]; then \
		echo ""; \
		echo "✅ All Standard Library Tests PASSED"; \
		exit 0; \
	else \
		echo ""; \
		echo "❌ Some stdlib tests FAILED:"; \
		if [ $$CPP_RESULT -ne 0 ]; then echo "   - C++ infrastructure tests: FAILED"; fi; \
		if [ $$CB_RESULT -ne 0 ]; then echo "   - Cb language tests: FAILED"; fi; \
		exit 1; \
	fi

test:
	@echo "============================================================="
	@echo "Running All Cb Test Suites (4 suites)"
	@echo "============================================================="
	@START_TIME=$$(date +%s); \
	INTEGRATION_RESULT=0; UNIT_RESULT=0; STDLIB_CPP_RESULT=0; STDLIB_CB_RESULT=0; \
	echo ""; \
	echo "[1/4] Running Integration Tests..."; \
	$(MAKE) integration-test || INTEGRATION_RESULT=$$?; \
	echo ""; \
	echo "[2/4] Running Unit Tests..."; \
	$(MAKE) unit-test || UNIT_RESULT=$$?; \
	echo ""; \
	echo "[3/4] Running Stdlib C++ Tests..."; \
	$(MAKE) stdlib-cpp-test || STDLIB_CPP_RESULT=$$?; \
	echo ""; \
	echo "[4/4] Running Stdlib Cb Tests..."; \
	$(MAKE) stdlib-cb-test || STDLIB_CB_RESULT=$$?; \
	END_TIME=$$(date +%s); \
	ELAPSED=$$((END_TIME - START_TIME)); \
	echo ""; \
	echo "============================================================="; \
	echo "=== Final Test Summary ==="; \
	echo "============================================================="; \
	TOTAL_PASS=0; TOTAL_FAIL=0; \
	if [ $$INTEGRATION_RESULT -eq 0 ]; then \
		echo "✅ [1/4] Integration tests: PASSED"; \
		TOTAL_PASS=$$((TOTAL_PASS + 1)); \
	else \
		echo "❌ [1/4] Integration tests: FAILED (exit code $$INTEGRATION_RESULT)"; \
		TOTAL_FAIL=$$((TOTAL_FAIL + 1)); \
		if [ -f /tmp/cb_integration_raw.log ]; then \
			FAILED_COUNT=$$(grep "^Failed:" /tmp/cb_integration_raw.log | head -1 | awk '{print $$2}' || echo "0"); \
			if [ "$$FAILED_COUNT" != "0" ]; then \
				echo "   └─ $$FAILED_COUNT integration tests failed"; \
			fi; \
		fi; \
	fi; \
	if [ $$UNIT_RESULT -eq 0 ]; then \
		echo "✅ [2/4] Unit tests: PASSED"; \
		TOTAL_PASS=$$((TOTAL_PASS + 1)); \
	else \
		echo "❌ [2/4] Unit tests: FAILED (exit code $$UNIT_RESULT)"; \
		TOTAL_FAIL=$$((TOTAL_FAIL + 1)); \
	fi; \
	if [ $$STDLIB_CPP_RESULT -eq 0 ]; then \
		echo "✅ [3/4] Stdlib C++ tests: PASSED"; \
		TOTAL_PASS=$$((TOTAL_PASS + 1)); \
	else \
		echo "❌ [3/4] Stdlib C++ tests: FAILED (exit code $$STDLIB_CPP_RESULT)"; \
		TOTAL_FAIL=$$((TOTAL_FAIL + 1)); \
	fi; \
	if [ $$STDLIB_CB_RESULT -eq 0 ]; then \
		echo "✅ [4/4] Stdlib Cb tests: PASSED"; \
		TOTAL_PASS=$$((TOTAL_PASS + 1)); \
	else \
		echo "❌ [4/4] Stdlib Cb tests: FAILED (exit code $$STDLIB_CB_RESULT)"; \
		TOTAL_FAIL=$$((TOTAL_FAIL + 1)); \
	fi; \
	echo "============================================================="; \
	echo "Test suites: $$TOTAL_PASS/4 passed, $$TOTAL_FAIL/4 failed"; \
	echo "Total time: $${ELAPSED}s"; \
	echo "============================================================="; \
	if [ $$TOTAL_FAIL -eq 0 ]; then \
		echo ""; \
		echo "╔════════════════════════════════════════════════════════════╗"; \
		echo "║        🎉 All 4 Test Suites Passed Successfully! 🎉       ║"; \
		echo "╚════════════════════════════════════════════════════════════╝"; \
		exit 0; \
	else \
		echo ""; \
		echo "⚠️  $$TOTAL_FAIL of 4 test suite(s) failed"; \
		echo ""; \
		echo "💡 Run individual test suites for details:"; \
		if [ $$INTEGRATION_RESULT -ne 0 ]; then echo "   - make integration-test"; fi; \
		if [ $$UNIT_RESULT -ne 0 ]; then echo "   - make unit-test"; fi; \
		if [ $$STDLIB_CPP_RESULT -ne 0 ]; then echo "   - make stdlib-cpp-test"; fi; \
		if [ $$STDLIB_CB_RESULT -ne 0 ]; then echo "   - make stdlib-cb-test"; fi; \
		exit 1; \
	fi

# FFI ライブラリのビルド
ffi-libs: $(FFI_LIBS)

$(STDLIB_FOREIGN_DIR)/libcppexample.$(LIB_EXT): $(SAMPLE_FFI_DIR)/ffi_cpp_example.cpp
	@echo "Building FFI library: libcppexample.$(LIB_EXT)"
	$(FFI_CC) $(FFI_CXXFLAGS) -shared -fPIC -o $@ $<

$(STDLIB_FOREIGN_DIR)/libadvanced.$(LIB_EXT): $(SAMPLE_FFI_DIR)/advanced_cpp_ffi.cpp
	@echo "Building FFI library: libadvanced.$(LIB_EXT)"
	$(FFI_CC) $(FFI_CXXFLAGS) -shared -fPIC -o $@ $<

# FFI テスト実行
test-ffi: $(MAIN_TARGET) ffi-libs
	@echo "============================================================="
	@echo "Running FFI Tests"
	@echo "============================================================="
	@echo "Testing basic FFI example..."
	@DYLD_LIBRARY_PATH=$(STDLIB_FOREIGN_DIR):$$DYLD_LIBRARY_PATH LD_LIBRARY_PATH=$(STDLIB_FOREIGN_DIR):$$LD_LIBRARY_PATH ./$(MAIN_TARGET) $(SAMPLE_FFI_DIR)/ffi_cpp_example.cb
	@echo ""
	@echo "Testing advanced FFI example..."
	@DYLD_LIBRARY_PATH=$(STDLIB_FOREIGN_DIR):$$DYLD_LIBRARY_PATH LD_LIBRARY_PATH=$(STDLIB_FOREIGN_DIR):$$LD_LIBRARY_PATH ./$(MAIN_TARGET) $(SAMPLE_FFI_DIR)/advanced_cpp_ffi.cb

# クリーンアップ
clean: clean-ffi
	@echo "Cleaning up build artifacts..."
	rm -f $(MAIN_TARGET) $(CGEN_TARGET)
	rm -f main_asan
	rm -f tests/integration/test_main
	rm -f tests/unit/test_main tests/unit/dummy.o
	rm -f tests/stdlib/test_main
	rm -f /tmp/cb_integration_test.log
	find src -name "*.o" -type f -delete
	find tests -name "*.o" -type f -delete
	find sample -name "*.o" -type f -delete
	rm -rf tmp/*
	rm -rf **/*.dSYM *.dSYM
	rm -rf tests/integration/*.dSYM
	rm -rf tests/unit/*.dSYM
	rm -rf tests/stdlib/*.dSYM
	@echo "Clean completed."

# FFI ライブラリのクリーンアップ
clean-ffi:
	@echo "Cleaning FFI libraries..."
	rm -f $(STDLIB_FOREIGN_DIR)/*.$(LIB_EXT)
	rm -f $(STDLIB_FOREIGN_DIR)/*.dylib
	rm -f $(STDLIB_FOREIGN_DIR)/*.so
	rm -f $(STDLIB_FOREIGN_DIR)/*.dll
	@echo "FFI libraries cleaned."

# ディープクリーン（すべての生成ファイルを削除）
deep-clean: clean
	@echo "Removing all generated files..."
	find . -name "*.o" -type f -delete
	find . -name "test_main" -type f -delete
	find . -name "*.dSYM" -type d -exec rm -rf {} + 2>/dev/null || true
	# 古いparser関連ファイルも削除
	rm -f $(FRONTEND_DIR)/parser.c $(FRONTEND_DIR)/parser.h $(FRONTEND_DIR)/lexer.c $(FRONTEND_DIR)/parser.output

# サブディレクトリも含む完全クリーンアップ
clean-all: deep-clean
	@echo "Cleaning subdirectories..."
	@echo "All directories cleaned."

# 古いMakefileのバックアップ
backup-old:
	@if [ -f Makefile.old ]; then \
		echo "Makefile.old already exists"; \
	else \
		echo "Creating backup of old Makefile"; \
		cp Makefile Makefile.old; \
	fi

# VSCode拡張機能のインストール
install-vscode-extension:
	@echo "Installing Cb Language VSCode extension..."
	@if [ ! -d vscode-extension ]; then \
		echo "Error: vscode-extension directory not found"; \
		exit 1; \
	fi
	@VSCODE_EXT_DIR=""; \
	if [ "$$(uname)" = "Darwin" ] || [ "$$(uname)" = "Linux" ]; then \
		VSCODE_EXT_DIR="$$HOME/.vscode/extensions/cb-language-0.13.0"; \
	elif [ "$$(uname -o 2>/dev/null)" = "Msys" ] || [ "$$(uname -o 2>/dev/null)" = "Cygwin" ]; then \
		VSCODE_EXT_DIR="$$USERPROFILE/.vscode/extensions/cb-language-0.13.0"; \
	else \
		echo "Error: Unsupported OS"; \
		exit 1; \
	fi; \
	echo "Installing to: $$VSCODE_EXT_DIR"; \
	rm -rf "$$VSCODE_EXT_DIR"; \
	mkdir -p "$$VSCODE_EXT_DIR"; \
	cp -r vscode-extension/* "$$VSCODE_EXT_DIR/"; \
	echo "✅ Cb Language extension installed successfully!"; \
	echo ""; \
	echo "Please restart VSCode to activate the extension."

# VSCode拡張機能のビルド（.vsixファイル作成）
build-extension:
	@echo "Building VSCode extension..."
	@if [ ! -d vscode-extension ]; then \
		echo "Error: vscode-extension directory not found"; \
		exit 1; \
	fi
	@if ! command -v vsce >/dev/null 2>&1; then \
		echo "Error: vsce is not installed."; \
		echo "Install it with: npm install -g @vscode/vsce"; \
		exit 1; \
	fi
	@echo "Verifying version consistency..."
	@cd vscode-extension && node scripts/verify-version.js
	@echo "Packaging extension..."
	@cd vscode-extension && vsce package
	@echo "✅ Extension packaged successfully!"
	@echo ""
	@echo "Install with:"
	@echo "  code --install-extension vscode-extension/cb-language-*.vsix"
	@echo "Or:"
	@echo "  VSCode → Extensions → ... → Install from VSIX..."

update-extension-version:
	@echo "Updating VSCode extension version from VERSION file..."
	@cd vscode-extension && node scripts/update-version.js

verify-extension-version:
	@echo "Verifying VSCode extension version..."
	@cd vscode-extension && node scripts/verify-version.js

# VSCode拡張機能のクリーンアップ
clean-extension:
	@echo "Cleaning VSCode extension build artifacts..."
	@rm -f vscode-extension/*.vsix
	@echo "Extension build artifacts cleaned."

# 開発用のヘルプ
help:
	@echo "Available targets:"
	@echo ""
	@echo "Build targets:"
	@echo "  all                    - Build main executable"
	@echo "  main                   - Build main executable"
	@echo "  debug                  - Build with debug flags"
	@echo "  main-asan              - Build with AddressSanitizer"
	@echo ""
	@echo "Test targets:"
	@echo "  test                   - Run all 4 test suites"
	@echo "  integration-test       - Run integration tests"
	@echo "  hir-integration-test   - Run HIR integration tests (89 tests, 97.8% pass)"
	@echo "  unit-test              - Run unit tests (30 tests)"
	@echo "  stdlib-cpp-test        - Run stdlib C++ infrastructure tests"
	@echo "  stdlib-cb-test         - Run stdlib Cb language tests"
	@echo "  stdlib-test            - Run both stdlib C++ and Cb tests"
	@echo ""
	@echo "Code quality:"
	@echo "  lint                   - Check code formatting"
	@echo "  fmt                    - Format code"
	@echo ""
	@echo "VSCode extension:"
	@echo "  build-extension        - Build VSCode extension (.vsix file)"
	@echo "  install-vscode-extension - Install Cb syntax highlighting for VSCode"
	@echo "  clean-extension        - Remove .vsix files"
	@echo "  update-extension-version - Update extension version from VERSION file"
	@echo "  verify-extension-version - Verify extension version matches VERSION file"
	@echo ""
	@echo "FFI (Foreign Function Interface):"
	@echo "  ffi-libs               - Build all FFI example libraries"
	@echo "  test-ffi               - Run FFI example tests"
	@echo "  clean-ffi              - Remove compiled FFI libraries"
	@echo ""
	@echo "Cleanup:"
	@echo "  clean                  - Remove generated files"
	@echo "  deep-clean             - Remove all generated files (thorough cleanup)"
	@echo "  clean-all              - Clean all subdirectories too"
	@echo ""
	@echo "Test suites breakdown:"
	@echo "  1. Integration tests   - End-to-end testing of language features"
	@echo "  2. Unit tests          - Testing of individual components"
	@echo "  3. Stdlib C++ tests    - Testing of C++ infrastructure for stdlib"
	@echo "  4. Stdlib Cb tests     - Testing of Cb stdlib modules"

