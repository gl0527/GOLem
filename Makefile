CC := gcc
CFLAGS := -Wall -Wextra -pedantic -pedantic-errors
SDL2_CFLAGS := $(shell echo $$(sdl2-config --cflags))
SDL2_LIBS := $(shell echo $$(sdl2-config --libs)) -lSDL2_image
OPENMP_FLAGS := -fopenmp
BUILD_DIR := ./build
BIN_DIR := ./bin

.PHONY: all debug release clean

all: debug release

debug: $(BIN_DIR)/gold $(BUILD_DIR)/gold.o

release: $(BIN_DIR)/gol $(BUILD_DIR)/gol.o

$(BIN_DIR)/%: $(BUILD_DIR)/%.o
	@mkdir -p $(@D)
	@$(CC) $^ $(SDL2_LIBS) $(OPENMP_FLAGS) -o $@

$(BUILD_DIR)/%d.o: %.c
	@mkdir -p $(@D)
	@$(CC) $^ $(CFLAGS) $(SDL2_CFLAGS) $(OPENMP_FLAGS) -c -O0 -g -o $@

$(BUILD_DIR)/%.o: %.c
	@mkdir -p $(@D)
	@$(CC) $^ $(CFLAGS) -DNDEBUG $(SDL2_CFLAGS) $(OPENMP_FLAGS) -c -O2 -o $@

clean:
	@rm -rf $(BUILD_DIR)
	@rm -rf $(BIN_DIR)
