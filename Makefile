CC := gcc

CFLAGS := -Wall -Wextra -pedantic -pedantic-errors
SDL2_CFLAGS := $(shell echo $$(sdl2-config --cflags))
SDL2_LIBS := $(shell echo $$(sdl2-config --libs)) -lSDL2_image
OPENMP_FLAGS := -fopenmp

DEBUG_FLAGS := $(CFLAGS) $(SDL2_CFLAGS) $(OPENMP_FLAGS) -O0 -g
RELEASE_FLAGS := $(CFLAGS) $(SDL2_CFLAGS) $(OPENMP_FLAGS) -O2 -DNDEBUG

BUILD_DIR := ./build
BIN_DIR := ./bin

.PHONY: all debug release clean

all: debug release

debug: $(BIN_DIR)/gold $(BUILD_DIR)/gold.o $(BUILD_DIR)/gold.s

release: $(BIN_DIR)/gol $(BUILD_DIR)/gol.o $(BUILD_DIR)/gol.s

$(BIN_DIR)/%: $(BUILD_DIR)/%.o
	@mkdir -p $(@D)
	@$(CC) $^ $(SDL2_LIBS) $(OPENMP_FLAGS) -o $@

$(BUILD_DIR)/%d.o: %.c
	@mkdir -p $(@D)
	@$(CC) -o $@ $^ $(DEBUG_FLAGS) -c

$(BUILD_DIR)/%.o: %.c
	@mkdir -p $(@D)
	@$(CC) -o $@ $^ $(RELEASE_FLAGS) -c

$(BUILD_DIR)/%d.s: %.c
	@mkdir -p $(@D)
	@$(CC) -o $@ $^ $(DEBUG_FLAGS) -S -fverbose-asm

$(BUILD_DIR)/%.s: %.c
	@mkdir -p $(@D)
	@$(CC) -o $@ $^ $(RELEASE_FLAGS) -S -fverbose-asm

clean:
	@rm -rf $(BUILD_DIR)
	@rm -rf $(BIN_DIR)
