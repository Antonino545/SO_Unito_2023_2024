# Compiler
CC = gcc

# Compiler flags
CFLAGS = -Wvla -Wextra -Werror

# Source directory
SRC_DIR = src

# Output binary
TARGET = main

# Source files
SRC_FILES = $(SRC_DIR)/main.c

# Default target
all: $(TARGET)

# Build target
$(TARGET): $(SRC_FILES)
	$(CC) $(CFLAGS) $(SRC_FILES) -o $(TARGET)

# Clean target
clean:
	rm -f $(TARGET)
