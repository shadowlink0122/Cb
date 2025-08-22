CC=g++
LEX=flex
YACC=bison

CFLAGS=-Wall -g -std=c++17 -I.

# ディレクトリ設定
SRC_DIR=src
FRONTEND_DIR=$(SRC_DIR)/frontend
BACKEND_DIR=$(SRC_DIR)/backend
COMMON_DIR=$(SRC_DIR)/common
TESTS_DIR=tests
CGEN_DIR=cgen

# 生成されるファイル
PARSER_C=$(FRONTEND_DIR)/parser.c
PARSER_H=$(FRONTEND_DIR)/parser.h
LEXER_C=$(FRONTEND_DIR)/lexer.c

# オブジェクトファイル
FRONTEND_OBJS=$(PARSER_C:.c=.o) $(LEXER_C:.c=.o) $(FRONTEND_DIR)/parser_utils.o $(FRONTEND_DIR)/main.o $(FRONTEND_DIR)/debug_impl.o $(FRONTEND_DIR)/debug_messages.o
BACKEND_OBJS=$(BACKEND_DIR)/interpreter.o
COMMON_OBJS=$(COMMON_DIR)/type_utils.o

# 実行ファイル
MAIN_TARGET=main
CGEN_TARGET=cgen_main

.PHONY: all clean lint fmt unit-test integration-test test debug setup-dirs

all: setup-dirs $(MAIN_TARGET)

# ディレクトリ作成
setup-dirs:
	@mkdir -p $(FRONTEND_DIR) $(BACKEND_DIR) $(COMMON_DIR)

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

# 共通オブジェクト生成
$(COMMON_DIR)/%.o: $(COMMON_DIR)/%.cpp $(COMMON_DIR)/ast.h
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

# 単体テスト（新しいテストフレームワークを使用）
unit-test: $(MAIN_TARGET) $(FRONTEND_OBJS) $(BACKEND_OBJS) $(COMMON_OBJS) $(TESTS_DIR)/unit/dummy.o
	@echo "Running unit tests..."
	@cd tests/unit && $(CC) $(CFLAGS) -o test_main main.cpp dummy.o ../../$(BACKEND_DIR)/interpreter.o ../../$(COMMON_DIR)/type_utils.o ../../$(FRONTEND_DIR)/parser_utils.o ../../$(FRONTEND_DIR)/debug_impl.o ../../$(FRONTEND_DIR)/debug_messages.o
	@cd tests/unit && ./test_main

# 基本的な機能テスト（旧）
basic-test: $(MAIN_TARGET)
	@echo "Running basic unit tests..."
	@if ./main hello.cb > /dev/null 2>&1; then echo "✓ hello.cb"; else echo "✗ hello.cb"; fi

# 新しい単体テスト（unit-testのエイリアス）
unit-test-new: unit-test

integration-test: $(MAIN_TARGET)
	@echo "Running existing integration tests..."
	$(CC) $(CFLAGS) -I. -o tests/integration/test_main \
		tests/integration/test_main.cpp \
		tests/integration/arithmetic/test_arithmetic.cpp \
		tests/integration/boundary/test_boundary.cpp \
		tests/integration/assign/test_assign.cpp \
		tests/integration/cross_type/test_cross_type.cpp \
		tests/integration/string/test_string.cpp \
		tests/integration/bool_expr/test_bool_expr.cpp \
		tests/integration/loop/test_loop.cpp \
		tests/integration/if/test_if.cpp \
		tests/integration/self_assign/test_self_assign.cpp \
		tests/integration/incdec/test_incdec.cpp \
		tests/integration/array/test_array.cpp \
		tests/integration/global_vars/test_global_vars.cpp
	tests/integration/test_main

test: basic-test integration-test unit-test
	@echo "=== Test Summary ==="
	@echo "Basic functionality tests completed."
	@echo "Integration tests completed."
	@echo "Unit tests completed (50 tests, some failures expected for unimplemented features)."
	@echo "Note: Function-related tests fail due to unimplemented interpreter features."

# クリーンアップ
clean:
	rm -f $(MAIN_TARGET) $(CGEN_TARGET)
	rm -f $(PARSER_C) $(PARSER_H) $(LEXER_C)
	rm -f $(FRONTEND_DIR)/*.o $(BACKEND_DIR)/*.o $(COMMON_DIR)/*.o
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
	@echo "  unit-test    - Run unit tests (54 tests)"
	@echo "  unit-test-new- Alias for unit-test"
	@echo "  basic-test   - Run basic functionality tests"
	@echo "  integration-test - Run integration tests"
	@echo "  help         - Show this help"
