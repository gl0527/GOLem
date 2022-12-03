#include <cstdio>
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>

int main(int argc, char **argv)
{
    if (argc != 2) {
        printf("Wrong number of arguments provided! 1 argument is needed for the file path.\n");
        return 1;
    }

    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        printf("Error @ SDL2 initialization: %s.\n", SDL_GetError());
        return 1;
    }

    if (0 == IMG_Init(IMG_INIT_JPG)) {
        printf("Error @ SDL2_Image initialization.\n");
        return 1;
    }

    SDL_Window *window = SDL_CreateWindow("Empty Window", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 640, 480, 0);
    if (nullptr == window) {
        printf("Error @ window creation.\n");
        return 1;
    }

    SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, 0);
    if (nullptr == renderer) {
        printf("Error @ renderer creation.\n");
        return 1;
    }

    SDL_Surface *image = IMG_Load(argv[1]);
    if (nullptr == image) {
        printf("Error @ loading image: %s.\n", IMG_GetError());
        return 1;
    }

    SDL_Texture *texture = SDL_CreateTextureFromSurface(renderer, image);
    if (nullptr == texture) {
        printf("Error @ creating texture.\n");
        return 1;
    }

    bool quit = false;
    SDL_Event event;

    while(!quit) {
        SDL_RenderClear(renderer);

        SDL_WaitEvent(&event);

        switch(event.type) {
            case SDL_QUIT:
                quit = true;
                break;
        }

        SDL_RenderCopy(renderer, texture, nullptr, nullptr);
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
