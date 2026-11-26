CC = cc
CFLAGS = -std=c11 -O2 -Wall -Wextra -Wpedantic -Werror
TARGET = build/timsort
SOURCES = src/main.c src/io.c src/parallel.c src/timsort.c
HEADERS = src/io.h src/parallel.h src/timsort.h
TRACE_FILE = report/strace.log

.PHONY: all test sanitize trace clean
all: $(TARGET)

$(TARGET): $(SOURCES) $(HEADERS) Makefile
	mkdir -p $(@D)
	$(CC) $(CFLAGS) -pthread $(SOURCES) -o $@

test: $(TARGET)
	python3 tests/test_sort.py --binary $(TARGET)

sanitize:
	mkdir -p build
	$(CC) -std=c11 -O1 -g -Wall -Wextra -Wpedantic -Werror -fsanitize=address,undefined -fno-omit-frame-pointer -pthread $(SOURCES) -o build/timsort-sanitize
	python3 tests/test_sort.py --binary build/timsort-sanitize

trace: $(TARGET)
	strace -f -e trace=clone,clone3,futex -o "$(TRACE_FILE)" $(TARGET) --threads 4 < tests/trace_input.txt

clean:
	rm -f build/timsort build/timsort-sanitize
