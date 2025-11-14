.PHONY: all clean

all: libziglib.so

libziglib.so: ziglib.zig
	zig build-lib ziglib.zig -dynamic -O ReleaseFast
	if [ -f libziglib.so ]; then \
		echo "libziglib.so created"; \
	elif ls libziglib.so.* >/dev/null 2>&1; then \
		mv libziglib.so.* libziglib.so; \
	else \
		echo "Error: No library file found"; \
		exit 1; \
	fi

clean:
	rm -f libziglib.so* ziglib.o *.o
