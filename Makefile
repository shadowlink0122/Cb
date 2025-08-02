CC=g++
LEX=flex
YACC=bison

CFLAGS=-Wall -g

LINT_FILES=src/**/*.cpp src/**/*.h tests/**/*.cpp


.PHONY: all clean lint fmt unit-test integration-test test cgen main debug debug-build-test
all: main


# デバッグ実行例（--debugオプションでデバッグ出力有効）
debug:
	CB_DEBUG_MODE=1 $(MAKE) main
	@echo '例: ./main <file>.cb --debug'

lint:
	clang-format --dry-run --Werror $(LINT_FILES)

fmt:
	clang-format -i $(LINT_FILES)

# unitテスト
unit-test: src/parser.c src/lexer.c
	$(CC) $(CFLAGS) -I. -o tests/unit/test_main \
		tests/unit/assign/tiny/test_assign_tiny.cpp \
		tests/unit/assign/short/test_assign_short.cpp \
		tests/unit/assign/int/test_assign_int.cpp \
		tests/unit/assign/long/test_assign_long.cpp \
		tests/unit/assign/test_assign.cpp \
		tests/unit/type/test_type.cpp \
		tests/unit/type/tiny/test_tiny.cpp \
		tests/unit/type/short/test_short.cpp \
		tests/unit/type/int/test_int.cpp \
		tests/unit/type/long/test_long.cpp \
		tests/unit/boundary/tiny/test_boundary_tiny.cpp \
		tests/unit/boundary/short/test_boundary_short.cpp \
		tests/unit/boundary/int/test_boundary_int.cpp \
		tests/unit/boundary/long/test_boundary_long.cpp \
		tests/unit/boundary/test_boundary.cpp \
		tests/unit/arithmetic/test_arithmetic.cpp \
		tests/unit/cross_type/tiny/test_cross_type_tiny.cpp \
		tests/unit/cross_type/short/test_cross_type_short.cpp \
		tests/unit/cross_type/int/test_cross_type_int.cpp \
		tests/unit/cross_type/long/test_cross_type_long.cpp \
		tests/unit/cross_type/test_cross_type.cpp \
		tests/unit/func/test_func.cpp \
		src/eval/eval.cpp \
		src/parser.c \
		src/lexer.c \
		src/ast/util.cpp \
		tests/unit/test_main.cpp
	tests/unit/test_main

# 結合テスト
integration-test: main
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
			   src/eval/eval.cpp \
			   src/parser.c \
			   src/lexer.c \
			   src/ast/util.cpp
	tests/integration/test_main

# debug_buildテスト
debug-build-test: debug
	$(CC) $(CFLAGS_DEBUG) -I. -o tests/debug_build/test_debug_main tests/debug_build/test_debug_main.cpp src/eval/eval.cpp src/ast/util.cpp src/parser.c src/lexer.c
	tests/debug_build/test_debug_main

# 両方まとめて実行
test:
	$(MAKE) unit-test
	$(MAKE) integration-test
	$(MAKE) debug-build-test

src/lexer.c: src/lexer.l
	$(LEX) -o src/lexer.c src/lexer.l

src/parser.c src/parser.h: src/parser.y
	$(YACC) -d -o src/parser.c src/parser.y

main: src/parser.c src/lexer.c src/main.cpp src/eval/eval.cpp src/ast/util.cpp
	$(CC) $(CFLAGS) -I. -o main src/parser.c src/lexer.c src/main.cpp src/eval/eval.cpp src/ast/util.cpp

# Cb→Cコード変換ツール
cgen:
	$(CC) $(CFLAGS) -I. -o cgen_main cgen/cgen_main.cpp src/ast/util.cpp src/parser.c src/lexer.c

clean:
	rm -rf src/*.o src/*.c src/parser.h main cgen_main \
	  tests/*/test_main tests/debug_build/test_debug_main \
	  **/*.dSYM *.dSYM
