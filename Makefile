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
SRC_FILE_LIB = $(SRC_DIR)/lib.c

# Object files
OBJ_FILE_LIB = $(DEST_DIR)/lib.o

# Default target
all: manster atomo

# Build manster target
manster: $(OBJ_FILE_LIB) $(SRC_FILES_MANSTER)
	mkdir -p $(DEST_DIR)
	$(CC) $(CFLAGS) -o $(DEST_DIR)/manster $(OBJ_FILE_LIB) $(SRC_FILES_MANSTER)

# Build atomo target
atomo: $(OBJ_FILE_LIB) $(SRC_FILE_ATOMO)
	mkdir -p $(DEST_DIR)
	$(CC) $(CFLAGS) -o $(DEST_DIR)/atomo $(OBJ_FILE_LIB) $(SRC_FILE_ATOMO)

# Build lib.o target
$(OBJ_FILE_LIB): $(SRC_FILE_LIB) $(SRC_DIR)/lib.h
	mkdir -p $(DEST_DIR)
	$(CC) $(CFLAGS) -c $(SRC_FILE_LIB) -o $(OBJ_FILE_LIB)

# Clean target
clean:
	rm -rf $(DEST_DIR)

# Run target
run: all
	cd $(DEST_DIR) && ./manster

# Phony targets
.PHONY: all manster atomo clean run
