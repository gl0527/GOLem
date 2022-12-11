#include <stdbool.h>
#include <stdio.h>

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>

int main(int argc, char **argv)
{
    if (argc != 2) {
        fprintf(stderr, "Wrong number of arguments provided! 1 argument is needed for the file path.\n");
        return 1;
    }

    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        fprintf(stderr, "Error @ SDL2 initialization: %s.\n", SDL_GetError());
        return 1;
    }

    if (0 == IMG_Init(IMG_INIT_JPG)) {
        fprintf(stderr, "Error @ SDL2_Image initialization: %s.\n", IMG_GetError());
        return 1;
    }

    SDL_Window *window = SDL_CreateWindow("Empty Window", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 640, 480, 0);
    if (NULL == window) {
        fprintf(stderr, "Error @ window creation: %s.\n", SDL_GetError());
        return 1;
    }

    SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, 0);
    if (NULL == renderer) {
        fprintf(stderr, "Error @ renderer creation: %s.\n", SDL_GetError());
        return 1;
    }

    SDL_Surface *image = IMG_Load(argv[1]);
    if (NULL == image) {
        fprintf(stderr, "Error @ loading image: %s.\n", IMG_GetError());
        return 1;
    }

    SDL_Texture *texture = SDL_CreateTextureFromSurface(renderer, image);
    if (NULL == texture) {
        fprintf(stderr, "Error @ creating texture: %s.\n", SDL_GetError());
        return 1;
    }

    bool quit = false;
    SDL_Event event;
    SDL_Rect rect = {0, 0, image->w, image->h};

    while(!quit) {
        SDL_RenderClear(renderer);

        SDL_WaitEvent(&event);

        switch(event.type) {
            case SDL_QUIT:
                quit = true;
                break;
            case SDL_KEYDOWN:
                switch(event.key.keysym.sym) {
                    case SDLK_ESCAPE:
                        quit = true;
                        break;
                    case SDLK_LEFT:
                        rect.x -= 10;
                        break;
                    case SDLK_RIGHT:
                        rect.x += 10;
                        break;
                    case SDLK_DOWN:
                        rect.y += 10;
                        break;
                    case SDLK_UP:
                        rect.y -= 10;
                        break;
                }
                break;
        }

        SDL_RenderCopy(renderer, texture, NULL, &rect);
        SDL_RenderPresent(renderer);
    }

    SDL_DestroyTexture(texture);
    SDL_FreeSurface(image);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    IMG_Quit();
    SDL_Quit();

    return 0;
}
