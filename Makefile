# Compiler
CC = gcc

# Compiler flags
CFLAGS = -Wvla -Wextra -Werror

# Source directory
SRC_DIR = src
Dest_DIR = build

# Output binary
TARGET = main

# Source files
SRC_FILES = $(SRC_DIR)/main.c

# Default target
all: $(TARGET)

# Build target
$(TARGET): $(SRC_FILES)
	$(CC) $(CFLAGS) -o  $(Dest_DIR)/$(TARGET) $(SRC_FILES)
# Clean target
clean:
	rm -rf $(Dest_DIR)
