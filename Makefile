CC=g++
CXX=g++
LEX=flex
YACC=bison

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

# Interpreterサブディレクトリ
INTERPRETER_DIR=$(BACKEND_DIR)/interpreter
INTERPRETER_CORE=$(INTERPRETER_DIR)/core
INTERPRETER_EVALUATOR=$(INTERPRETER_DIR)/evaluator
INTERPRETER_EXECUTORS=$(INTERPRETER_DIR)/executors
INTERPRETER_HANDLERS=$(INTERPRETER_DIR)/handlers
INTERPRETER_MANAGERS=$(INTERPRETER_DIR)/managers
INTERPRETER_SERVICES=$(INTERPRETER_DIR)/services
INTERPRETER_OUTPUT=$(INTERPRETER_DIR)/output

# コンパイラフラグ
CXXFLAGS=-Wall -g -std=c++17
CFLAGS=$(CXXFLAGS) -I. -I$(SRC_DIR) -I$(INTERPRETER_DIR)

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
FRONTEND_OBJS=$(FRONTEND_DIR)/main.o $(FRONTEND_DIR)/help_messages.o $(FRONTEND_DIR)/recursive_parser/recursive_lexer.o $(FRONTEND_DIR)/recursive_parser/recursive_parser.o $(PARSER_OBJS)
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

# Backendオブジェクト（全て統合）
BACKEND_OBJS = \
	$(INTERPRETER_CORE_OBJS) \
	$(INTERPRETER_EVALUATOR_OBJS) \
	$(INTERPRETER_EXECUTORS_OBJS) \
	$(INTERPRETER_HANDLERS_OBJS) \
	$(INTERPRETER_MANAGERS_OBJS) \
	$(INTERPRETER_SERVICES_OBJS) \
	$(INTERPRETER_OUTPUT_OBJS)
PLATFORM_OBJS=$(NATIVE_DIR)/native_stdio_output.o $(BAREMETAL_DIR)/baremetal_uart_output.o
COMMON_OBJS=$(COMMON_DIR)/type_utils.o $(COMMON_DIR)/type_alias.o $(COMMON_DIR)/array_type_info.o $(COMMON_DIR)/utf8_utils.o $(COMMON_DIR)/io_interface.o $(COMMON_DIR)/debug_impl.o $(COMMON_DIR)/debug_messages.o $(COMMON_DIR)/ast.o $(PLATFORM_OBJS)

# 実行ファイル
MAIN_TARGET=main
CGEN_TARGET=cgen_main

.PHONY: all clean lint fmt unit-test integration-test integration-test-verbose integration-test-old test debug setup-dirs deep-clean clean-all backup-old help

all: setup-dirs $(MAIN_TARGET)

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
	@mkdir -p $(BACKEND_DIR)/ir $(BACKEND_DIR)/optimizer $(BACKEND_DIR)/codegen

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

# プラットフォーム固有オブジェクト生成
$(NATIVE_DIR)/%.o: $(NATIVE_DIR)/%.cpp
	$(CC) $(CFLAGS) -c -o $@ $<

$(BAREMETAL_DIR)/%.o: $(BAREMETAL_DIR)/%.cpp
	$(CC) $(CFLAGS) -c -o $@ $<

# メイン実行ファイル
$(MAIN_TARGET): $(FRONTEND_OBJS) $(BACKEND_OBJS) $(COMMON_OBJS)
	$(CC) $(CFLAGS) -o $(MAIN_TARGET) $(FRONTEND_OBJS) $(BACKEND_OBJS) $(COMMON_OBJS)

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

integration-test: $(TESTS_DIR)/integration/test_main
	@echo "============================================================="
	@echo "Running Cb Integration Test Suite"
	@echo "============================================================="
	@bash -c "set -o pipefail; cd tests/integration && ./test_main 2>&1 | fold -s -w 80"

# より詳細な出力が必要な場合の統合テスト（フル出力）
integration-test-verbose: $(TESTS_DIR)/integration/test_main
	@echo "Running integration tests (verbose mode)..."
	@cd tests/integration && ./test_main

# Stdlib test binary target
$(TESTS_DIR)/stdlib/test_main: $(TESTS_DIR)/stdlib/main.cpp $(MAIN_TARGET)
	@cd tests/stdlib && $(CC) $(CFLAGS) -I../../$(SRC_DIR) -I. -o test_main main.cpp

# Stdlib tests (C++ infrastructure tests)
stdlib-test-cpp: $(TESTS_DIR)/stdlib/test_main
	@echo "============================================================="
	@echo "Running Cb Standard Library Tests (C++)"
	@echo "============================================================="
	@cd tests/stdlib && ./test_main

# Stdlib tests (Cb language tests)
stdlib-test-cb: $(MAIN_TARGET)
	@echo "============================================================="
	@echo "Running Cb Standard Library Tests (Cb)"
	@echo "============================================================="
	@echo "\n[Allocators]"
	@echo "[1/2] Testing SystemAllocator..."
	@./$(MAIN_TARGET) tests/cases/stdlib/allocators/test_system_allocator.cb
	@echo "\n[2/2] Testing BumpAllocator..."
	@./$(MAIN_TARGET) tests/cases/stdlib/allocators/test_bump_allocator.cb
	@echo "\n[Collections]"
	@echo "[1/1] Testing Vector..."
	@./$(MAIN_TARGET) tests/cases/stdlib/collections/test_vector.cb
	@echo "\n✅ All stdlib .cb tests passed!"

# Run both C++ and Cb stdlib tests
stdlib-test: stdlib-test-cpp stdlib-test-cb
	@echo "\n╔════════════════════════════════════════════════════════════╗"
	@echo "║    All Standard Library Tests Completed Successfully!     ║"
	@echo "╚════════════════════════════════════════════════════════════╝"

test: integration-test unit-test stdlib-test
	@echo "=== Test Summary ==="
	@echo "Integration tests: completed"
	@echo "Unit tests: completed"
	@echo "Stdlib tests: completed"

# クリーンアップ
clean:
	@echo "Cleaning up build artifacts..."
	rm -f $(MAIN_TARGET) $(CGEN_TARGET)
	rm -f tests/integration/test_main
	rm -f tests/unit/test_main tests/unit/dummy.o
	rm -f tests/stdlib/test_main
	find . -name "*.o" -type f -delete
	rm -rf **/*.dSYM *.dSYM
	rm -rf tests/integration/*.dSYM
	rm -rf tests/unit/*.dSYM
	rm -rf tests/stdlib/*.dSYM
	@echo "Clean completed."

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

# 開発用のヘルプ
help:
	@echo "Available targets:"
	@echo "  all                    - Build main executable"
	@echo "  main                   - Build main executable"
	@echo "  debug                  - Build with debug flags"
	@echo "  clean                  - Remove generated files"
	@echo "  deep-clean             - Remove all generated files (thorough cleanup)"
	@echo "  clean-all              - Clean all subdirectories too"
	@echo "  lint                   - Check code formatting"
	@echo "  fmt                    - Format code"
	@echo "  test                   - Run all tests (integration + unit + stdlib)"
	@echo "  unit-test              - Run unit tests (30 tests)"
	@echo "  integration-test       - Run integration tests (formatted output)"
	@echo "  integration-test-verbose - Run integration tests (full output)"
	@echo "  stdlib-test            - Run stdlib tests (C++ + Cb)"
	@echo "  stdlib-test-cpp        - Run stdlib C++ infrastructure tests"
	@echo "  stdlib-test-cb         - Run stdlib Cb language tests"
	@echo "  help                   - Show this help"
