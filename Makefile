.PHONY: build clean run

TARGET := klondike

BIN_DIR := bin
OBJ_DIR := obj
SRC_DIR := src
SOURCE_FILES := $(wildcard $(SRC_DIR)/*.c)
HEADER_FILES := $(wildcard $(SRC_DIR)/*.h)

OBJ_FILES := $(patsubst $(SRC_DIR)/%.c,$(OBJ_DIR)/%.o,$(SOURCE_FILES))

CFLAGS := -O3


build: $(BIN_DIR)/$(TARGET)

clean:
	rm -rf $(BIN_DIR) $(OBJ_DIR)

run: build
	$(BIN_DIR)/$(TARGET)


$(BIN_DIR):
	mkdir $(BIN_DIR)

$(OBJ_DIR):
	mkdir $(OBJ_DIR)

$(BIN_DIR)/$(TARGET): $(OBJ_FILES) | $(BIN_DIR) $(OBJ_DIR)
	$(LINK.o) $^ $(LDLIBS) -o $@

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c $(HEADER_FILES) | $(OBJ_DIR)
	$(COMPILE.c) $< -o $@
