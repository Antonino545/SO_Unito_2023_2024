# Compiler
CC = gcc

# Compiler flags
CFLAGS = -Wvla -Wextra -Werror

# Default target
all: manster atomo alimentazione attivatore

# Build manster target
manster: build/lib.o src/manster.c
	mkdir -p build
	$(CC) $(CFLAGS) -o build/manster build/lib.o src/manster.c

# Build atomo target
atomo: build/lib.o src/atomo.c
	mkdir -p build
	$(CC) $(CFLAGS) -o build/atomo build/lib.o src/atomo.c
alimentazione: build/lib.o src/alimentazione.c
	mkdir -p build
	$(CC) $(CFLAGS) -o build/alimentazione build/lib.o src/alimentazione.c

attivatore: build/lib.o src/attivatore.c
	mkdir -p build
	$(CC) $(CFLAGS) -o build/attivatore build/lib.o src/attivatore.c
# Build lib.o target
build/lib.o: src/lib.c src/lib.h
	mkdir -p build
	$(CC) $(CFLAGS) -c src/lib.c -o build/lib.o

# Clean target
clean:
	rm -rf build

# Run target
run: all
	cd build && ./manster

# Phony targets
.PHONY: all manster atomo clean run
