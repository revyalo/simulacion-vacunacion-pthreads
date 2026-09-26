CC ?= cc
CPPFLAGS ?=
CFLAGS ?= -std=c11 -Wall -Wextra -Wpedantic -Werror -O2
LDFLAGS ?=
LDLIBS ?= -pthread

TARGET := practica2
BENCHMARK := benchmark
INPUT ?= entrada.txt
OUTPUT ?= salida.txt
THREAD_COUNTS := 1 2 4 8
TEST_BINARIES := $(THREAD_COUNTS:%=build/practica2-threads-%)

.PHONY: all run clean test sanitize thread-sanitize benchmark-run

all: $(TARGET)

$(TARGET): practica2.c
	$(CC) $(CPPFLAGS) $(CFLAGS) $< $(LDFLAGS) $(LDLIBS) -o $@

$(BENCHMARK): benchmark.c
	$(CC) $(CPPFLAGS) $(CFLAGS) $< $(LDFLAGS) $(LDLIBS) -o $@

build/practica2-threads-%: practica2.c
	mkdir -p build
	$(CC) $(CPPFLAGS) $(CFLAGS) -DFABRICAS=$* $< $(LDFLAGS) $(LDLIBS) -o $@

run: $(TARGET)
	./$(TARGET) $(INPUT) $(OUTPUT)

test: $(TARGET) $(BENCHMARK) $(TEST_BINARIES)
	sh tests/run_tests.sh $(TEST_BINARIES)
	./$(BENCHMARK) --threads 2 --items 10000 --rounds 2

sanitize: CFLAGS := -std=c11 -Wall -Wextra -Wpedantic -Werror -O1 -g -fsanitize=address,undefined -fno-omit-frame-pointer
sanitize: LDFLAGS := -fsanitize=address,undefined
sanitize: clean test

thread-sanitize: CFLAGS := -std=c11 -Wall -Wextra -Wpedantic -Werror -O1 -g -fsanitize=thread -fno-omit-frame-pointer
thread-sanitize: LDFLAGS := -fsanitize=thread
thread-sanitize: clean test

benchmark-run: $(BENCHMARK)
	sh scripts/run_benchmark.sh

clean:
	rm -f $(TARGET) $(BENCHMARK) salida*.txt *.o
	rm -rf build
