CC ?= cc
CFLAGS ?= -Wall -Wextra -Wpedantic -O2
LDFLAGS ?= -pthread

TARGET := practica2
SRC := practica2.c
INPUT ?= entrada.txt
OUTPUT ?= salida.txt

.PHONY: all run clean

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) $(SRC) $(LDFLAGS) -o $(TARGET)

run: $(TARGET)
	./$(TARGET) $(INPUT) $(OUTPUT)

clean:
	rm -f $(TARGET) salida*.txt *.o
