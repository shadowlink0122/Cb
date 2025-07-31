CC=g++
LEX=flex
YACC=bison

CFLAGS=-Wall -g

LINT_FILES=src/**/*.cpp src/**/*.h tests/**/*.cpp

all: main

lint:
	clang-format --dry-run --Werror $(LINT_FILES)

fmt:
	clang-format -i $(LINT_FILES)

# unitテスト
unit-test:
	$(CC) $(CFLAGS) -I. -o tests/unit/test_main \
		tests/unit/test_main.cpp \
		tests/unit/type/test_type.cpp \
		tests/unit/type/tiny/test_tiny.cpp \
		tests/unit/type/short/test_short.cpp \
		tests/unit/type/int/test_int.cpp \
		tests/unit/type/long/test_long.cpp \
		tests/unit/assign/test_assign.cpp \
		tests/unit/boundary/test_boundary.cpp \
		tests/unit/arithmetic/test_arithmetic.cpp \
		tests/unit/cross_type/test_cross_type.cpp \
		src/eval/eval.cpp
	tests/unit/test_main

# 結合テスト
integration-test: main
	$(CC) $(CFLAGS) -I. -o tests/integration/test_main \
		tests/integration/test_main.cpp \
		tests/integration/arithmetic/test_arithmetic.cpp \
		tests/integration/boundary/test_boundary.cpp \
		tests/integration/assign/test_assign.cpp \
		tests/integration/cross_type/test_cross_type.cpp \
		src/eval/eval.cpp
	tests/integration/test_main

# 両方まとめて実行
test:
	$(MAKE) unit-test
	$(MAKE) integration-test

src/lexer.c: src/lexer.l
	$(LEX) -o src/lexer.c src/lexer.l

src/parser.c src/parser.h: src/parser.y
	$(YACC) -d -o src/parser.c src/parser.y


main: src/parser.c src/lexer.c src/main.cpp src/eval/eval.cpp src/ast/util.cpp
	$(CC) $(CFLAGS) -I. -o main src/parser.c src/lexer.c src/main.cpp src/eval/eval.cpp src/ast/util.cpp

# Cb→Cコード変換ツール
.PHONY: cgen
cgen:
	$(CC) $(CFLAGS) -I. -o cgen_main cgen/cgen_main.cpp src/ast/util.cpp src/parser.c src/lexer.c

clean:
	rm -rf src/*.o src/*.c src/parser.h main cgen_main \
		tests/integration/test_main tests/unit/test_main \
		tests/integration/*.dSYM tests/unit/*.dSYM *.dSYM \
		cgen/*.dSYM
