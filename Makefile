# Compiler
CC = gcc

# Compiler flags
CFLAGS = -Wvla -Wextra -Werror

# Default target
all: master atomo alimentazione attivatore

# Linker flags
LDFLAGS = -lrt

# Build master target
master: src/master.c
	mkdir -p build
	$(CC) $(CFLAGS) -o build/master src/master.c $(LDFLAGS)

# Build atomo target
atomo: src/atomo.c
	mkdir -p build
	$(CC) $(CFLAGS) -o build/atomo src/atomo.c $(LDFLAGS)

# Build alimentazione target
alimentazione: src/alimentazione.c
	mkdir -p build
	$(CC) $(CFLAGS) -o build/alimentazione src/alimentazione.c $(LDFLAGS)

# Build attivatore target
attivatore: src/attivatore.c
	mkdir -p build
	$(CC) $(CFLAGS) -o build/attivatore src/attivatore.c $(LDFLAGS)

# Clean target
clean:
	rm -rf build

# Run target
run: all
	cd build && ./master

# Phony targets
.PHONY: all master atomo alimentazione attivatore clean run
