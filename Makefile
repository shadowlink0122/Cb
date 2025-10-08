CC=g++
LEX=flex
YACC=bison

CFLAGS=-Wall -g -std=c++17 -I. -Isrc/backend/interpreter

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
BACKEND_OBJS = \
	src/backend/interpreter/core/interpreter.o \
	src/backend/interpreter/core/error_handler.o \
	src/backend/interpreter/core/pointer_metadata.o \
	src/backend/interpreter/core/type_inference.o \
	src/backend/interpreter/evaluator/expression_address_ops.o \
	src/backend/interpreter/evaluator/expression_array_access.o \
	src/backend/interpreter/evaluator/expression_assignment.o \
	src/backend/interpreter/evaluator/expression_binary_unary_typed.o \
	src/backend/interpreter/evaluator/expression_dispatcher.o \
	src/backend/interpreter/evaluator/expression_evaluator.o \
	src/backend/interpreter/evaluator/expression_function_call_impl.o \
	src/backend/interpreter/evaluator/expression_function_call.o \
	src/backend/interpreter/evaluator/expression_helpers.o \
	src/backend/interpreter/evaluator/expression_incdec.o \
	src/backend/interpreter/evaluator/expression_literal_eval.o \
	src/backend/interpreter/evaluator/expression_member_access_impl.o \
	src/backend/interpreter/evaluator/expression_member_helpers.o \
	src/backend/interpreter/evaluator/expression_receiver_resolution.o \
	src/backend/interpreter/evaluator/expression_special_access.o \
	src/backend/interpreter/evaluator/expression_ternary.o \
	src/backend/interpreter/executors/statement_executor.o \
	src/backend/interpreter/executors/control_flow_executor.o \
	src/backend/interpreter/executors/statement_list_executor.o \
	src/backend/interpreter/handlers/return_handler.o \
	src/backend/interpreter/handlers/assertion_handler.o \
	src/backend/interpreter/handlers/break_continue_handler.o \
	src/backend/interpreter/handlers/function_declaration_handler.o \
	src/backend/interpreter/handlers/struct_declaration_handler.o \
	src/backend/interpreter/handlers/interface_declaration_handler.o \
	src/backend/interpreter/handlers/impl_declaration_handler.o \
	src/backend/interpreter/handlers/expression_statement_handler.o \
	src/backend/interpreter/managers/variables/manager.o \
	src/backend/interpreter/managers/variables/declaration.o \
	src/backend/interpreter/managers/variables/assignment.o \
	src/backend/interpreter/managers/variables/initialization.o \
	src/backend/interpreter/managers/variables/static.o \
	src/backend/interpreter/managers/arrays/manager.o \
	src/backend/interpreter/managers/types/manager.o \
	src/backend/interpreter/managers/types/enums.o \
	src/backend/interpreter/managers/types/interfaces.o \
	src/backend/interpreter/managers/structs/operations.o \
	src/backend/interpreter/managers/structs/member_variables.o \
	src/backend/interpreter/managers/structs/assignment.o \
	src/backend/interpreter/managers/structs/sync.o \
	src/backend/interpreter/managers/common/global_init.o \
	src/backend/interpreter/managers/common/operations.o \
	src/backend/interpreter/output/output_manager.o \
	src/backend/interpreter/services/expression_service.o \
	src/backend/interpreter/services/variable_access_service.o \
	src/backend/interpreter/services/debug_service.o \
	src/backend/interpreter/services/array_processing_service.o
PLATFORM_OBJS=$(NATIVE_DIR)/native_stdio_output.o $(BAREMETAL_DIR)/baremetal_uart_output.o
COMMON_OBJS=$(COMMON_DIR)/type_utils.o $(COMMON_DIR)/type_alias.o $(COMMON_DIR)/array_type_info.o $(COMMON_DIR)/utf8_utils.o $(COMMON_DIR)/io_interface.o $(COMMON_DIR)/debug_impl.o $(COMMON_DIR)/debug_messages.o $(PLATFORM_OBJS)

# 実行ファイル
MAIN_TARGET=main
CGEN_TARGET=cgen_main

.PHONY: all clean lint fmt unit-test integration-test integration-test-verbose integration-test-old test debug debug-build-test setup-dirs deep-clean clean-all backup-old help

all: setup-dirs $(MAIN_TARGET)

# ディレクトリ作成
setup-dirs:
	@mkdir -p $(FRONTEND_DIR) $(BACKEND_DIR) $(COMMON_DIR) $(NATIVE_DIR) $(BAREMETAL_DIR)
	@mkdir -p $(BACKEND_DIR)/interpreter/core $(BACKEND_DIR)/interpreter/managers
	@mkdir -p $(BACKEND_DIR)/interpreter/managers/variables $(BACKEND_DIR)/interpreter/managers/arrays
	@mkdir -p $(BACKEND_DIR)/interpreter/managers/structs $(BACKEND_DIR)/interpreter/managers/types
	@mkdir -p $(BACKEND_DIR)/interpreter/managers/common
	@mkdir -p $(BACKEND_DIR)/interpreter/evaluator $(BACKEND_DIR)/interpreter/executors
	@mkdir -p $(BACKEND_DIR)/interpreter/handlers $(BACKEND_DIR)/interpreter/output
	@mkdir -p $(BACKEND_DIR)/interpreter/services
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
	$(CC) $(CFLAGS) -c -o $@ $<

# 単体テスト
unit-test: $(MAIN_TARGET) $(FRONTEND_OBJS) $(BACKEND_OBJS) $(COMMON_OBJS) $(PLATFORM_OBJS) $(TESTS_DIR)/unit/dummy.o
	@echo "Running unit tests..."
	@cd tests/unit && $(CC) $(CFLAGS) -o test_main main.cpp dummy.o \
		../../$(FRONTEND_DIR)/recursive_parser/recursive_parser.o \
		../../$(FRONTEND_DIR)/recursive_parser/recursive_lexer.o \
		../../$(FRONTEND_DIR)/recursive_parser/parsers/expression_parser.o \
		../../$(FRONTEND_DIR)/recursive_parser/parsers/primary_expression_parser.o \
		../../$(FRONTEND_DIR)/recursive_parser/parsers/statement_parser.o \
		../../$(FRONTEND_DIR)/recursive_parser/parsers/declaration_parser.o \
		../../$(FRONTEND_DIR)/recursive_parser/parsers/variable_declaration_parser.o \
		../../$(FRONTEND_DIR)/recursive_parser/parsers/type_parser.o \
		../../$(FRONTEND_DIR)/recursive_parser/parsers/struct_parser.o \
		../../$(FRONTEND_DIR)/recursive_parser/parsers/enum_parser.o \
		../../$(FRONTEND_DIR)/recursive_parser/parsers/interface_parser.o \
		../../$(FRONTEND_DIR)/recursive_parser/parsers/union_parser.o \
		../../$(FRONTEND_DIR)/recursive_parser/parsers/type_utility_parser.o \
		../../$(BACKEND_DIR)/interpreter/core/interpreter.o \
		../../$(BACKEND_DIR)/interpreter/core/error_handler.o \
		../../$(BACKEND_DIR)/interpreter/core/type_inference.o \
		../../$(BACKEND_DIR)/interpreter/core/pointer_metadata.o \
		../../$(BACKEND_DIR)/interpreter/output/output_manager.o \
		../../$(BACKEND_DIR)/interpreter/managers/variable_manager.o \
		../../$(BACKEND_DIR)/interpreter/managers/array_manager.o \
		../../$(BACKEND_DIR)/interpreter/managers/type_manager.o \
		../../$(BACKEND_DIR)/interpreter/managers/enum_manager.o \
		../../$(BACKEND_DIR)/interpreter/managers/static_variable_manager.o \
		../../$(BACKEND_DIR)/interpreter/managers/interface_operations.o \
		../../$(BACKEND_DIR)/interpreter/managers/struct_operations.o \
		../../$(BACKEND_DIR)/interpreter/managers/struct_variable_manager.o \
		../../$(BACKEND_DIR)/interpreter/managers/struct_assignment_manager.o \
		../../$(BACKEND_DIR)/interpreter/managers/struct_sync_manager.o \
		../../$(BACKEND_DIR)/interpreter/managers/global_initialization_manager.o \
		../../$(BACKEND_DIR)/interpreter/managers/common_operations.o \
		../../$(BACKEND_DIR)/interpreter/services/expression_service.o \
		../../$(BACKEND_DIR)/interpreter/services/variable_access_service.o \
		../../$(BACKEND_DIR)/interpreter/services/debug_service.o \
		../../$(BACKEND_DIR)/interpreter/services/array_processing_service.o \
		../../$(BACKEND_DIR)/interpreter/evaluator/expression_evaluator.o \
		../../$(BACKEND_DIR)/interpreter/evaluator/expression_dispatcher.o \
		../../$(BACKEND_DIR)/interpreter/evaluator/expression_function_call_impl.o \
		../../$(BACKEND_DIR)/interpreter/evaluator/expression_member_access_impl.o \
		../../$(BACKEND_DIR)/interpreter/evaluator/expression_helpers.o \
		../../$(BACKEND_DIR)/interpreter/evaluator/expression_address_ops.o \
		../../$(BACKEND_DIR)/interpreter/evaluator/expression_array_access.o \
		../../$(BACKEND_DIR)/interpreter/evaluator/expression_function_call.o \
		../../$(BACKEND_DIR)/interpreter/evaluator/expression_incdec.o \
		../../$(BACKEND_DIR)/interpreter/evaluator/expression_assignment.o \
		../../$(BACKEND_DIR)/interpreter/evaluator/expression_binary_unary_typed.o \
		../../$(BACKEND_DIR)/interpreter/evaluator/expression_special_access.o \
		../../$(BACKEND_DIR)/interpreter/evaluator/expression_literal_eval.o \
		../../$(BACKEND_DIR)/interpreter/evaluator/expression_ternary.o \
		../../$(BACKEND_DIR)/interpreter/evaluator/expression_member_helpers.o \
		../../$(BACKEND_DIR)/interpreter/evaluator/expression_receiver_resolution.o \
		../../$(BACKEND_DIR)/interpreter/executors/statement_executor.o \
		../../$(BACKEND_DIR)/interpreter/executors/control_flow_executor.o \
		../../$(BACKEND_DIR)/interpreter/executors/statement_list_executor.o \
		../../$(BACKEND_DIR)/interpreter/handlers/return_handler.o \
		../../$(BACKEND_DIR)/interpreter/handlers/assertion_handler.o \
		../../$(BACKEND_DIR)/interpreter/handlers/break_continue_handler.o \
		../../$(BACKEND_DIR)/interpreter/handlers/function_declaration_handler.o \
		../../$(BACKEND_DIR)/interpreter/handlers/struct_declaration_handler.o \
		../../$(BACKEND_DIR)/interpreter/handlers/interface_declaration_handler.o \
		../../$(BACKEND_DIR)/interpreter/handlers/impl_declaration_handler.o \
		../../$(BACKEND_DIR)/interpreter/handlers/expression_statement_handler.o \
		../../$(COMMON_DIR)/type_utils.o \
		../../$(COMMON_DIR)/type_alias.o \
		../../$(COMMON_DIR)/array_type_info.o \
		../../$(COMMON_DIR)/utf8_utils.o \
		../../$(COMMON_DIR)/io_interface.o \
		../../$(COMMON_DIR)/debug_impl.o \
		../../$(COMMON_DIR)/debug_messages.o \
		../../$(PLATFORM_DIR)/native/native_stdio_output.o \
		../../$(PLATFORM_DIR)/baremetal/baremetal_uart_output.o
	@cd tests/unit && ./test_main

# Integration test binary target
$(TESTS_DIR)/integration/test_main: $(TESTS_DIR)/integration/main.cpp $(MAIN_TARGET)
	@cd tests/integration && $(CC) $(CFLAGS) -I. -o test_main main.cpp

integration-test: $(TESTS_DIR)/integration/test_main
	@echo "============================================================="
	@echo "Running Cb Integration Test Suite"
	@echo "============================================================="
	@cd tests/integration && ./test_main 2>&1 | fold -s -w 80

# より詳細な出力が必要な場合の統合テスト（フル出力）
integration-test-verbose: $(TESTS_DIR)/integration/test_main
	@echo "Running integration tests (verbose mode)..."
	@cd tests/integration && ./test_main

test: integration-test unit-test
	@echo "=== Test Summary ==="
	@echo "Integration tests: completed"
	@echo "Unit tests: 50 tests"

# デバッグ版のテスト実行
debug-build-test: CFLAGS += -DYYDEBUG=1 -DDEBUG=1
debug-build-test: clean $(MAIN_TARGET) integration-test unit-test
	@echo "=== Debug Build Test Summary ==="
	@echo "Integration tests: completed (debug mode)"
	@echo "Unit tests: completed (debug mode)"

# クリーンアップ
clean:
	@echo "Cleaning up build artifacts..."
	rm -f $(MAIN_TARGET) $(CGEN_TARGET)
	rm -f tests/integration/test_main
	rm -f tests/unit/test_main tests/unit/dummy.o
	find . -name "*.o" -type f -delete
	rm -rf **/*.dSYM *.dSYM
	rm -rf tests/integration/*.dSYM
	rm -rf tests/unit/*.dSYM
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
	@echo "  test                   - Run all tests"
	@echo "  debug-build-test       - Build with debug flags and run all tests"
	@echo "  unit-test              - Run unit tests (50 tests)"
	@echo "  integration-test       - Run integration tests (formatted output)"
	@echo "  integration-test-verbose - Run integration tests (full output)"
	@echo "  help                   - Show this help"
