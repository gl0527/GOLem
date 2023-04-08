CC=gcc
CFLAGS=-Wall -Wextra -pedantic -pedantic-errors
SDL2_CFLAGS=$(shell echo $$(sdl2-config --cflags))
SDL2_LIBS=$(shell echo $$(sdl2-config --libs)) -lSDL2_image
OPENMP_FLAGS=-fopenmp

.PHONY: all clean

all: gol_dbg gol_rel

gol_dbg: gol.c
	$(CC) $^ $(CFLAGS) $(SDL2_CFLAGS) $(SDL2_LIBS) $(OPENMP_FLAGS) -O0 -g -o $@

gol_rel: gol.c
	$(CC) $^ $(CFLAGS) -DNDEBUG $(SDL2_CFLAGS) $(SDL2_LIBS) $(OPENMP_FLAGS) -O2 -o $@

clean:
	rm gol_dbg gol_rel
