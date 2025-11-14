.PHONY: all clean

all: libgolib.so

libgolib.so: golib.go
	go build -buildmode=c-shared -o libgolib.so golib.go

clean:
	rm -f libgolib.so libgolib.h
