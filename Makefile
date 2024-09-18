# Compiler
CC = gcc

# Compiler flags
CFLAGS = -std=c99 -D_GNU_SOURCE -Wvla -Wextra -Werror

# Default target
all: build master atomo alimentazione attivatore

# Linker flags
LDFLAGS = -lrt

# Build master target
master: build/lib.o src/master.c
	$(CC) $(CFLAGS) -o build/master src/master.c build/lib.o $(LDFLAGS)

# Build atomo target
atomo: build/lib.o src/atomo.c
	$(CC) $(CFLAGS) -o build/atomo src/atomo.c build/lib.o $(LDFLAGS)

# Build alimentazione target
alimentazione: build/lib.o src/alimentazione.c
	$(CC) $(CFLAGS) -o build/alimentazione src/alimentazione.c build/lib.o $(LDFLAGS)

# Build attivatore target
attivatore: build/lib.o src/attivatore.c
	$(CC) $(CFLAGS) -o build/attivatore src/attivatore.c build/lib.o $(LDFLAGS)

# Build lib target
build/lib.o: src/lib.c src/lib.h
	$(CC) $(CFLAGS) -c -o build/lib.o src/lib.c

# Create build directory
build:
	mkdir -p build
# Clean target
clean:
	rm -rf build

# Run target
run: all
	cd build && ./master
