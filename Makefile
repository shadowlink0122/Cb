CC=g++
LEX=flex
YACC=bison

CFLAGS=-Wall -g

UNIT_TEST=test/unit/test_eval
INTEGRATION_TEST=test/integration/test_integration


LINT_FILES=src/*.cpp src/*.h test/**/*.cpp


all: main

lint:
	clang-format --dry-run --Werror $(LINT_FILES)

fmt:
	clang-format -i $(LINT_FILES)

test: $(UNIT_TEST) $(INTEGRATION_TEST)
	./$(UNIT_TEST)
	./$(INTEGRATION_TEST)

src/lexer.c: src/lexer.l
	$(LEX) -o src/lexer.c src/lexer.l

src/parser.c src/parser.h: src/parser.y
	$(YACC) -d -o src/parser.c src/parser.y

main: src/parser.c src/lexer.c src/main.cpp src/eval.cpp
	$(CC) $(CFLAGS) -o main src/parser.c src/lexer.c src/main.cpp src/eval.cpp

$(UNIT_TEST): test/unit/test_eval.cpp src/eval.cpp src/ast.h src/eval.h
	$(CC) $(CFLAGS) -I./src -DUNIT_TEST_BUILD -o $(UNIT_TEST) test/unit/test_eval.cpp src/eval.cpp

$(INTEGRATION_TEST): test/integration/test_integration.cpp main
	$(CC) $(CFLAGS) -o $(INTEGRATION_TEST) test/integration/test_integration.cpp


clean:
	rm -f src/*.o src/*.c src/parser.h main $(UNIT_TEST) $(INTEGRATION_TEST)
