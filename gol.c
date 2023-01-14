#include <stdbool.h>
#include <stdio.h>

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>

#define MIN(i,j) (((i) < (j)) ? (i) : (j))
#define MAX(i,j) (((i) > (j)) ? (i) : (j))

typedef struct {
    uint8_t survive_min;
    uint8_t survive_max;
    uint8_t reproduction_min;
    uint8_t reproduction_max;
} Rules;

typedef struct {
    uint32_t alive;
    uint32_t dead;
} Colors;

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

    return *((uint8_t*)(image->pixels) + y * image->pitch + x * image->format->BytesPerPixel) > 127;
}

uint8_t GetAliveNeighborCount(SDL_Surface *const image, int x, int y)
{
    if (x < 0 || x > image->w) {
        fprintf(stderr, "%s(%d):\tHorizontal index is out of range. Valid range is [0; %d].\n", __FILE__, __LINE__, image->w);
        return false;
    }
    if (y < 0 || y > image->h) {
        fprintf(stderr, "%s(%d):\tVertical index is out of range. Valid range is [0; %d].\n", __FILE__, __LINE__, image->h);
        return false;
    }

    int const y_min = MAX(y - 1, 0);
    int const y_max = MIN(y + 1, image->h - 1);
    int const x_min = MAX(x - 1, 0);
    int const x_max = MIN(x + 1, image->w - 1);

    uint8_t sum = 0;

    for (int r = y_min; r <= y_max; ++r) {
        for (int c = x_min; c <= x_max; ++c) {
            if (IsAlive(image, c, r)) {
                ++sum;
            }
        }
    }

    return sum - IsAlive(image, x, y);
}

bool SetSurfacePixel(SDL_Surface *const image, int x, int y, uint32_t color)
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

    uint8_t const r = (color & 0xFF000000) >> 24;
    uint8_t const g = (color & 0x00FF0000) >> 16;
    uint8_t const b = (color & 0x0000FF00) >> 8;
    uint8_t const a = (color & 0x000000FF);

    uint32_t const real_color = SDL_MapRGBA(image->format, r, g, b, a);

    SDL_LockSurface(image);
    SDL_memset4(pixel_address, real_color, bytes_per_pixel / 4);
    SDL_UnlockSurface(image);

    return true;
}

bool Step(SDL_Surface *const src, SDL_Surface *const dst, Colors const *const colors, Rules const *const rules)
{
    if (src->w != dst->w || src->h != dst->h) {
        fprintf(stderr, "%s(%d):\tSurface dimensions do not match.\n", __FILE__, __LINE__);
        return false;
    }
    if (SDL_BlitSurface(src, NULL, dst, NULL) != 0) {
        fprintf(stderr, "%s(%d):\tError @ copying surface: %s.\n", __FILE__, __LINE__, SDL_GetError());
        return false;
    }

    for (int y = 0; y < src->h; ++y) {
        for (int x = 0; x < src->w; ++x) {
            uint8_t const alive_neighbors = GetAliveNeighborCount(src, x, y);
            bool const is_alive = IsAlive(src, x, y);

            if (is_alive && (alive_neighbors < rules->survive_min || alive_neighbors > rules->survive_max)) {
                SetSurfacePixel(dst, x, y, colors->dead);
            } else if (!is_alive && (alive_neighbors >= rules->reproduction_min && alive_neighbors <= rules->reproduction_max)) {
                SetSurfacePixel(dst, x, y, colors->alive);
            }
        }
    }

    return true;
}

void Binarize(SDL_Surface *const image, Colors const *const colors)
{
    for (int y = 0; y < image->h; ++y) {
        for (int x = 0; x < image->w; ++x) {
            if (IsAlive(image, x, y)) {
                SetSurfacePixel(image, x, y, colors->alive);
            } else {
                SetSurfacePixel(image, x, y, colors->dead);
            }
        }
    }
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

    SDL_Surface *src = IMG_Load(argv[1]);
    if (NULL == src) {
        fprintf(stderr, "%s(%d):\tError @ loading image: %s.\n", __FILE__, __LINE__, IMG_GetError());
        return 1;
    }

    uint8_t const bits_per_pixel = src->format->BitsPerPixel;
    if (bits_per_pixel != 32) {
        fprintf(stderr, "%s(%d):\tWrong image format: %d bits per pixel instead of 32.\n", __FILE__, __LINE__, bits_per_pixel);
        return 1;
    }

    Colors colors = { .alive = 0xFF00FFFF, .dead = 0x000032FF };

    Binarize(src, &colors);

#if SDL_BYTEORDER == SDL_BIG_ENDIAN
    uint32_t rmask = 0xFF000000;
    uint32_t gmask = 0x00FF0000;
    uint32_t bmask = 0x0000FF00;
    uint32_t amask = 0x000000FF;
#else
    uint32_t rmask = 0x000000FF;
    uint32_t gmask = 0x0000FF00;
    uint32_t bmask = 0x00FF0000;
    uint32_t amask = 0xFF000000;
#endif

    SDL_Surface *dst = SDL_CreateRGBSurface(SDL_SWSURFACE, src->w, src->h, src->format->BitsPerPixel, rmask, gmask, bmask, amask);
    if (NULL == dst) {
        fprintf(stderr, "%s(%d):\tError @ creating image: %s.\n", __FILE__, __LINE__, SDL_GetError());
        return 1;
    }

    SDL_Texture *texture = SDL_CreateTextureFromSurface(app.renderer, src);
    if (NULL == texture) {
        fprintf(stderr, "%s(%d):\tError @ creating texture: %s.\n", __FILE__, __LINE__, SDL_GetError());
        return 1;
    }

    bool quit = false;
    SDL_Event event;
    SDL_Rect rect = {0, 0, src->w, src->h};
    Rules rules = { .survive_min = 2, .survive_max = 3, .reproduction_min = 3, .reproduction_max = 3 };

    while(!quit) {
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

        if (!Step(src, dst, &colors, &rules)) {
            break;
        }

        SDL_UpdateTexture(texture, NULL, src->pixels, src->pitch);

        SDL_RenderCopy(app.renderer, texture, NULL, &rect);
        SDL_RenderPresent(app.renderer);
        SDL_RenderClear(app.renderer);

        SDL_Surface *tmp = src;
        src = dst;
        dst = tmp;

        SDL_Delay(20);
    }
    SDL_DestroyTexture(texture);

    SDL_FreeSurface(dst);
    SDL_FreeSurface(src);
    DestroyApp(&app);

    return 0;
}
