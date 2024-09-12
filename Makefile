# Compiler
CC = gcc

# Compiler flags
CFLAGS = -Wvla -Wextra -Werror

# Default target
all: master atomo alimentazione attivatore

# Linker flags
LDFLAGS = -lrt

# Build master target
master: build/lib.o src/master.c
	mkdir -p build
	$(CC) $(CFLAGS) -o build/master src/master.c build/lib.o $(LDFLAGS)

# Build atomo target
atomo: build/lib.o src/atomo.c
	mkdir -p build
	$(CC) $(CFLAGS) -o build/atomo src/atomo.c build/lib.o $(LDFLAGS)

# Build alimentazione target
alimentazione: build/lib.o src/alimentazione.c
	mkdir -p build
	$(CC) $(CFLAGS) -o build/alimentazione src/alimentazione.c build/lib.o $(LDFLAGS)

# Build attivatore target
attivatore: build/lib.o src/attivatore.c
	mkdir -p build
	$(CC) $(CFLAGS) -o build/attivatore src/attivatore.c build/lib.o $(LDFLAGS)

# Build lib target
build/lib.o: src/lib.c src/lib.h
	mkdir -p build
	$(CC) $(CFLAGS) -c -o build/lib.o src/lib.c

# Clean target
clean:
	rm -rf build

# Run target
run: all
	cd build && ./master