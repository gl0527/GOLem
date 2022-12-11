CC=gcc
CFLAGS=-Wall -Wextra -pedantic -pedantic-errors
SDL2_CFLAGS=$(shell echo $$(sdl2-config --cflags))
SDL2_LIBS=$(shell echo $$(sdl2-config --libs)) -lSDL2_image

.PHONY: all clean

all: gol

gol: gol.c
	$(CC) gol.c $(CFLAGS) $(SDL2_CFLAGS) $(SDL2_LIBS) -o gol

clean:
	rm gol
