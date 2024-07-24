# Compiler
CC = gcc

# Compiler flags
CFLAGS = -Wvla -Wextra -Werror

# Source directory
SRC_DIR = src
Dest_DIR = build

# Output binary
TARGET = manster

# Source files
SRC_FILES = $(SRC_DIR)/$(TARGET).c

# Default target
all: $(TARGET)

# Build target
$(TARGET): $(SRC_FILES)
	mkdir -p $(Dest_DIR)
	$(CC) $(CFLAGS) -o  $(Dest_DIR)/$(TARGET) $(SRC_FILES)
# Clean target
clean:
	rm -rf $(Dest_DIR)
