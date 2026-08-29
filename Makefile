.PHONY: build clean run test

TARGET := klondike

BIN_DIR := bin
OBJ_DIR := obj
SRC_DIR := src
CONFIG_FILE := $(SRC_DIR)/config.h
SOURCE_FILES := $(wildcard $(SRC_DIR)/*.c)
HEADER_FILES := $(wildcard $(SRC_DIR)/*.h) $(CONFIG_FILE)

OBJ_FILES := $(patsubst $(SRC_DIR)/%.c,$(OBJ_DIR)/%.o,$(SOURCE_FILES))

CFLAGS := -Wall -Wextra -Werror -Wno-unused-parameter -O3 --std=c2x -D_DEFAULT_SOURCE


build: $(BIN_DIR)/$(TARGET)

clean:
	rm -rf $(BIN_DIR) $(OBJ_DIR)

run: build
	$(BIN_DIR)/$(TARGET)

test: build
	./test.sh

$(CONFIG_FILE): configure | $(OBJ_DIR)
	./configure

$(BIN_DIR):
	mkdir $(BIN_DIR)

$(OBJ_DIR):
	mkdir $(OBJ_DIR)

$(BIN_DIR)/$(TARGET): $(OBJ_FILES) | $(BIN_DIR) $(OBJ_DIR)
	$(LINK.o) $^ $(LDLIBS) -o $@

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c $(HEADER_FILES) | $(OBJ_DIR)
	$(COMPILE.c) $< -o $@
