# Compiler
CC = gcc

# Compiler flags
CFLAGS = -Wvla -Wextra -Werror

# Source and build directories
SRC_DIR = src
DEST_DIR = build

# Output binary
TARGET = manster

# Source files
SRC_FILES = $(SRC_DIR)/manster.c $(SRC_DIR)/utility.c

# Default target
all: $(TARGET)

# Build target
$(TARGET): $(SRC_FILES)
	mkdir -p $(DEST_DIR)
	$(CC) $(CFLAGS) -o $(DEST_DIR)/$(TARGET) $(SRC_FILES)

# Clean target
clean:
	rm -rf $(DEST_DIR)
