# Makefile for Codeless IDE

# Compiler and flags
CC = gcc
PKG_CONFIG_LIBS = gtk+-3.0 gtksourceview-4 vte-2.91 sqlite3
CFLAGS = -Wall -g $(shell pkg-config --cflags $(PKG_CONFIG_LIBS))
LDFLAGS = $(shell pkg-config --libs $(PKG_CONFIG_LIBS))
INCLUDES = -Iinclude

# Project structure
SRC_DIR = src
OBJ_DIR = obj
BIN_DIR = bin

# Source files
SRCS = $(wildcard $(SRC_DIR)/*.c) $(wildcard $(SRC_DIR)/db/*.c) $(wildcard $(SRC_DIR)/ui/*.c)
OBJS = $(patsubst $(SRC_DIR)/%.c,$(OBJ_DIR)/%.o,$(SRCS))

# Target executable
TARGET = $(BIN_DIR)/codeless_ide

# Default target
all: $(TARGET)

# Rule to build the target executable
$(TARGET): $(OBJS) | $(BIN_DIR)
	$(CC) $(OBJS) -o $@ $(LDFLAGS)

# Rule to compile source files into object files
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c | $(OBJ_DIR)
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

# Create directories if they don't exist
$(BIN_DIR) $(OBJ_DIR):
	mkdir -p $@

# Clean rule
clean:
	rm -rf $(OBJ_DIR) $(BIN_DIR)

.PHONY: all clean
