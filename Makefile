# Compiler
CC = gcc

# Compiler flags
CFLAGS = -Wvla -Wextra -Werror
# Source and build directories
SRC_DIR = src
DEST_DIR = build

# Source files
SRC_FILES_MANSTER = $(SRC_DIR)/manster.c 
SRC_FILE_ATOMO = $(SRC_DIR)/atomo.c

# Default target
all: manster atomo

# Build manster target
manster: $(SRC_FILES_MANSTER)
	mkdir -p $(DEST_DIR)
	$(CC) $(CFLAGS) -o $(DEST_DIR)/manster $(SRC_FILES_MANSTER)

# Build atomo target
atomo: $(SRC_FILE_ATOMO)
	mkdir -p $(DEST_DIR)
	$(CC) $(CFLAGS) -o $(DEST_DIR)/atomo $(SRC_FILE_ATOMO)

# Clean target
clean:
	rm -rf $(DEST_DIR)

# Run target
run: all
	./$(DEST_DIR)/manster

# Phony targets
.PHONY: all manster atomo clean run
