CC=gcc
CFLAGS=-Wall -Wextra -pedantic -pedantic-errors
SDL2_CFLAGS=$(shell echo $$(sdl2-config --cflags))
SDL2_LIBS=$(shell echo $$(sdl2-config --libs)) -lSDL2_image

.PHONY: all clean

all: gol_dbg gol_rel

gol_dbg: gol.c
	$(CC) gol.c $(CFLAGS) $(SDL2_CFLAGS) $(SDL2_LIBS) -O0 -g -o gol_dbg

gol_rel: gol.c
	$(CC) gol.c $(CFLAGS) -DNDEBUG $(SDL2_CFLAGS) $(SDL2_LIBS) -O2 -o gol_rel

clean:
	rm gol_dbg gol_rel
