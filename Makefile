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
FRONTEND_OBJS=$(PARSER_C:.c=.o) $(LEXER_C:.c=.o) $(FRONTEND_DIR)/parser_utils.o $(FRONTEND_DIR)/main.o
BACKEND_OBJS=$(BACKEND_DIR)/interpreter.o

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

# メイン実行ファイル
$(MAIN_TARGET): $(FRONTEND_OBJS) $(BACKEND_OBJS)
	$(CC) $(CFLAGS) -o $(MAIN_TARGET) $(FRONTEND_OBJS) $(BACKEND_OBJS)

# Cb→Cコード変換ツール（将来の拡張）
$(CGEN_TARGET):
	@echo "Code generator is not implemented yet in new architecture"
	@echo "Use old structure for now: make -f Makefile.old cgen"

# フォーマット
lint:
	clang-format --dry-run --Werror $(SRC_DIR)/**/*.cpp $(SRC_DIR)/**/*.h $(TESTS_DIR)/**/*.cpp

fmt:
	clang-format -i $(SRC_DIR)/**/*.cpp $(SRC_DIR)/**/*.h $(TESTS_DIR)/**/*.cpp

# テスト（基本的な機能テスト）
unit-test: $(MAIN_TARGET)
	@echo "Running basic unit tests..."
	@if ./main hello.cb > /dev/null 2>&1; then echo "✓ hello.cb"; else echo "✗ hello.cb"; fi

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

test: unit-test integration-test
	@echo "=== Test Summary ==="
	@echo "Basic functionality tests completed."
	@echo "Note: Advanced features (arrays, functions) need implementation."

# クリーンアップ
clean:
	rm -f $(MAIN_TARGET) $(CGEN_TARGET)
	rm -f $(PARSER_C) $(PARSER_H) $(LEXER_C)
	rm -f $(FRONTEND_DIR)/*.o $(BACKEND_DIR)/*.o
	rm -rf **/*.dSYM *.dSYM

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
	@echo "  lint         - Check code formatting"
	@echo "  fmt          - Format code"
	@echo "  test         - Run tests (old architecture for now)"
	@echo "  help         - Show this help"
