#include <stdbool.h>
#include <stdio.h>

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>

typedef struct {
    SDL_Window *window;
    SDL_Renderer *renderer;
} App;

bool CreateApp(char const *title, int width, int height, App *outApp)
{
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        fprintf(stderr, "%s(%d):\tError @ SDL2 initialization: %s.\n", __FILE__, __LINE__, SDL_GetError());
        return false;
    }

    if (0 == IMG_Init(IMG_INIT_JPG)) {
        fprintf(stderr, "%s(%d):\tError @ SDL2_Image initialization: %s.\n", __FILE__, __LINE__, IMG_GetError());
        return false;
    }

    outApp->window = SDL_CreateWindow(title, SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, width, height, 0);
    if (NULL == outApp->window) {
        fprintf(stderr, "%s(%d):\tError @ window creation: %s.\n", __FILE__, __LINE__, SDL_GetError());
        return false;
    }

    outApp->renderer = SDL_CreateRenderer(outApp->window, -1, 0);
    if (NULL == outApp->renderer) {
        fprintf(stderr, "%s(%d):\tError @ renderer creation: %s.\n", __FILE__, __LINE__, SDL_GetError());
        return false;
    }

    return true;
}

void DestroyApp(App *app)
{
    SDL_DestroyRenderer(app->renderer);
    SDL_DestroyWindow(app->window);
    IMG_Quit();
    SDL_Quit();
}

bool IsAlive(SDL_Surface *const image, int x, int y)
{
    if (x < 0 || x > image->w) {
        fprintf(stderr, "%s(%d):\tHorizontal index is out of range. Valid range is [0; %d].\n", __FILE__, __LINE__, image->w);
        return false;
    }
    if (y < 0 || y > image->h) {
        fprintf(stderr, "%s(%d):\tVertical index is out of range. Valid range is [0; %d].\n", __FILE__, __LINE__, image->h);
        return false;
    }

    uint8_t const bytes_per_pixel = image->format->BytesPerPixel;
    uint8_t *const pixel_address = (uint8_t*)(image->pixels) + image->pitch * y + x * bytes_per_pixel;

    return *pixel_address > 127;
}

uint8_t GetAliveNeighborCount(SDL_Surface *const image, int x, int y)
{
    int const w = image->w;
    int const h = image->h;

    int const y_min = (y - 1 + h) % h;
    int const y_max = (y + 1) % h;
    int const x_min = (x - 1 + w) % w;
    int const x_max = (x + 1) % w;

    uint8_t sum = 0;

    for (int r = y_min; r <= y_max; ++r) {
        for (int c = x_min; c <= x_max; ++c) {
            if (c == x && r == y) {
                continue;
            }
            if (IsAlive(image, c, r)) {
                sum += 1;
            }
        }
    }

    return sum;
}

bool SetSurfacePixel(SDL_Surface *const image, int x, int y, int color)
{
    if (x < 0 || x > image->w) {
        fprintf(stderr, "%s(%d):\tHorizontal index is out of range. Valid range is [0; %d].\n", __FILE__, __LINE__, image->w);
        return false;
    }
    if (y < 0 || y > image->h) {
        fprintf(stderr, "%s(%d):\tVertical index is out of range. Valid range is [0; %d].\n", __FILE__, __LINE__, image->h);
        return false;
    }

    uint8_t const bytes_per_pixel = image->format->BytesPerPixel;
    uint8_t *const pixel_address = (uint8_t*)(image->pixels) + image->pitch * y + x * bytes_per_pixel;

    SDL_LockSurface(image);
    SDL_memset(pixel_address, color, bytes_per_pixel);
    SDL_UnlockSurface(image);

    return true;
}

int main(int argc, char **argv)
{
    if (argc != 2) {
        fprintf(stderr, "Wrong number of arguments provided! 1 argument is needed for the file path.\n");
        return 1;
    }

    App app;

    if (!CreateApp("Game of Life", 640, 480, &app)) {
        return 1;
    }

    SDL_Surface *image = IMG_Load(argv[1]);
    if (NULL == image) {
        fprintf(stderr, "%s(%d):\tError @ loading image: %s.\n", __FILE__, __LINE__, IMG_GetError());
        return 1;
    }

    SDL_Texture *texture = SDL_CreateTextureFromSurface(app.renderer, image);
    if (NULL == texture) {
        fprintf(stderr, "%s(%d):\tError @ creating texture: %s.\n", __FILE__, __LINE__, SDL_GetError());
        return 1;
    }

    bool quit = false;
    SDL_Event event;
    SDL_Rect rect = {0, 0, image->w, image->h};

    while(!quit) {
        SDL_RenderClear(app.renderer);

        while(SDL_PollEvent(&event) != 0) {
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
        }

        SDL_RenderCopy(app.renderer, texture, NULL, &rect);
        SDL_RenderPresent(app.renderer);

        SDL_Delay(20);
    }

    SDL_DestroyTexture(texture);
    SDL_FreeSurface(image);
    DestroyApp(&app);

    return 0;
}
