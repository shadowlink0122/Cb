.PHONY: all clean

all: libcpplib.so

libcpplib.so: cpplib.cpp
	g++ -std=c++17 -shared -fPIC -O2 cpplib.cpp -o libcpplib.so

clean:
	rm -f libcpplib.so
