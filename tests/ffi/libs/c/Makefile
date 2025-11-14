.PHONY: all clean

all: libclib.so

libclib.so: clib.c
	gcc -shared -fPIC -O2 clib.c -o libclib.so -lm

clean:
	rm -f libclib.so
