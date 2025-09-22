CC=g++
LEX=flex
YACC=bison

CFLAGS=-Wall -g -std=c++17 -I.

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

# 生成されるファイル
PARSER_C=$(FRONTEND_DIR)/parser.c
PARSER_H=$(FRONTEND_DIR)/parser.h
LEXER_C=$(FRONTEND_DIR)/lexer.c

# オブジェクトファイル
FRONTEND_OBJS=$(PARSER_C:.c=.o) $(LEXER_C:.c=.o) $(FRONTEND_DIR)/parser_utils.o $(FRONTEND_DIR)/main.o $(FRONTEND_DIR)/help_messages.o $(FRONTEND_DIR)/recursive_parser/recursive_lexer.o $(FRONTEND_DIR)/recursive_parser/recursive_parser.o
BACKEND_OBJS=$(BACKEND_DIR)/interpreter.o $(BACKEND_DIR)/error_handler.o \
             $(BACKEND_DIR)/evaluator/expression_evaluator.o \
             $(BACKEND_DIR)/executor/statement_executor.o \
             $(BACKEND_DIR)/output/output_manager.o
PLATFORM_OBJS=$(NATIVE_DIR)/native_stdio_output.o $(BAREMETAL_DIR)/baremetal_uart_output.o
COMMON_OBJS=$(COMMON_DIR)/type_utils.o $(COMMON_DIR)/type_alias.o $(COMMON_DIR)/utf8_utils.o $(COMMON_DIR)/io_interface.o $(COMMON_DIR)/cb_config.o $(COMMON_DIR)/debug_impl.o $(COMMON_DIR)/debug_messages.o $(PLATFORM_OBJS)

# 実行ファイル
MAIN_TARGET=main
CGEN_TARGET=cgen_main

.PHONY: all clean lint fmt unit-test integration-test integration-test-old test debug debug-build-test setup-dirs deep-clean clean-all backup-old help

all: setup-dirs $(MAIN_TARGET)

# ディレクトリ作成
setup-dirs:
	@mkdir -p $(FRONTEND_DIR) $(BACKEND_DIR) $(COMMON_DIR) $(NATIVE_DIR) $(BAREMETAL_DIR)

# デバッグ実行例（--debugオプションでデバッグ出力有効）
debug: CFLAGS += -DYYDEBUG=1
debug: $(MAIN_TARGET)
	@echo '例: ./$(MAIN_TARGET) <file>.cb --debug'

# レキサ生成
$(LEXER_C): $(FRONTEND_DIR)/lexer.l $(PARSER_H)
	$(LEX) -o $(LEXER_C) $(FRONTEND_DIR)/lexer.l

# パーサ生成
$(PARSER_C) $(PARSER_H): $(FRONTEND_DIR)/parser.y
	$(YACC) -d -o $(PARSER_C) $(FRONTEND_DIR)/parser.y

# フロントエンドオブジェクト生成
$(FRONTEND_DIR)/%.o: $(FRONTEND_DIR)/%.c $(PARSER_H)
	$(CC) $(CFLAGS) -c -o $@ $<

$(FRONTEND_DIR)/%.o: $(FRONTEND_DIR)/%.cpp $(COMMON_DIR)/ast.h
	$(CC) $(CFLAGS) -c -o $@ $<

# バックエンドオブジェクト生成
$(BACKEND_DIR)/%.o: $(BACKEND_DIR)/%.cpp $(COMMON_DIR)/ast.h
	$(CC) $(CFLAGS) -c -o $@ $<

# バックエンドサブディレクトリのオブジェクト生成
$(BACKEND_DIR)/evaluator/%.o: $(BACKEND_DIR)/evaluator/%.cpp $(COMMON_DIR)/ast.h
	$(CC) $(CFLAGS) -c -o $@ $<
	
$(BACKEND_DIR)/executor/%.o: $(BACKEND_DIR)/executor/%.cpp $(COMMON_DIR)/ast.h
	$(CC) $(CFLAGS) -c -o $@ $<
	
$(BACKEND_DIR)/memory/%.o: $(BACKEND_DIR)/memory/%.cpp $(COMMON_DIR)/ast.h
	$(CC) $(CFLAGS) -c -o $@ $<
	
$(BACKEND_DIR)/output/%.o: $(BACKEND_DIR)/output/%.cpp $(COMMON_DIR)/ast.h
	$(CC) $(CFLAGS) -c -o $@ $<
	
$(BACKEND_DIR)/variables/%.o: $(BACKEND_DIR)/variables/%.cpp $(COMMON_DIR)/ast.h
	$(CC) $(CFLAGS) -c -o $@ $<
	
$(BACKEND_DIR)/modules/%.o: $(BACKEND_DIR)/modules/%.cpp $(COMMON_DIR)/ast.h
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
	clang-format --dry-run --Werror $(SRC_DIR)/**/*.cpp $(SRC_DIR)/**/*.h $(TESTS_DIR)/**/*.cpp

fmt:
	clang-format -i $(SRC_DIR)/**/*.cpp $(SRC_DIR)/**/*.h $(TESTS_DIR)/**/*.cpp

# 単体テスト用のダミーオブジェクト
$(TESTS_DIR)/unit/dummy.o: $(TESTS_DIR)/unit/dummy.cpp
	$(CC) $(CFLAGS) -c -o $@ $<

# 単体テスト
unit-test: $(MAIN_TARGET) $(FRONTEND_OBJS) $(BACKEND_OBJS) $(COMMON_OBJS) $(PLATFORM_OBJS) $(TESTS_DIR)/unit/dummy.o
	@echo "Running unit tests..."
	@cd tests/unit && $(CC) $(CFLAGS) -o test_main main.cpp dummy.o ../../$(BACKEND_DIR)/interpreter.o ../../$(BACKEND_DIR)/output/output_manager.o ../../$(BACKEND_DIR)/evaluator/expression_evaluator.o ../../$(BACKEND_DIR)/executor/statement_executor.o ../../$(COMMON_DIR)/type_utils.o ../../$(COMMON_DIR)/type_alias.o ../../$(COMMON_DIR)/utf8_utils.o ../../$(COMMON_DIR)/io_interface.o ../../$(FRONTEND_DIR)/parser_utils.o ../../$(COMMON_DIR)/debug_impl.o ../../$(COMMON_DIR)/debug_messages.o ../../$(PLATFORM_DIR)/native/native_stdio_output.o ../../$(PLATFORM_DIR)/baremetal/baremetal_uart_output.o
	@cd tests/unit && ./test_main

integration-test: $(MAIN_TARGET)
	@echo "Running integration tests..."
	@cd tests/integration && $(CC) $(CFLAGS) -I. -o test_main main.cpp
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
	rm -f $(MAIN_TARGET) $(CGEN_TARGET)
	rm -f $(PARSER_C) $(PARSER_H) $(LEXER_C)
	rm -f $(FRONTEND_DIR)/*.o $(BACKEND_DIR)/*.o $(COMMON_DIR)/*.o $(NATIVE_DIR)/*.o $(BAREMETAL_DIR)/*.o
	rm -f tests/integration/test_main
	rm -f tests/unit/test_main tests/unit/dummy.o
	rm -rf **/*.dSYM *.dSYM
	rm -rf tests/integration/*.dSYM
	rm -rf tests/unit/*.dSYM

# ディープクリーン（すべての生成ファイルを削除）
deep-clean: clean
	@echo "Removing all generated files..."
	find . -name "*.o" -type f -delete
	find . -name "test_main" -type f -delete
	find . -name "*.dSYM" -type d -exec rm -rf {} + 2>/dev/null || true
	rm -f $(FRONTEND_DIR)/parser.output

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
	@echo "  all          - Build main executable"
	@echo "  main         - Build main executable"
	@echo "  debug        - Build with debug flags"
	@echo "  clean        - Remove generated files"
	@echo "  deep-clean   - Remove all generated files (thorough cleanup)"
	@echo "  clean-all    - Clean all subdirectories too"
	@echo "  lint         - Check code formatting"
	@echo "  fmt          - Format code"
	@echo "  test         - Run all tests"
	@echo "  debug-build-test - Build with debug flags and run all tests"
	@echo "  unit-test    - Run unit tests (50 tests)"
	@echo "  integration-test - Run integration tests"
	@echo "  help         - Show this help"
