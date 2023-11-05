#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <omp.h>
#include <stdbool.h>

#define BIT(n) (1ULL << (n))

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

typedef struct {
    bool quit;
    SDL_Event event;
    uint32_t delay_ms;
    SDL_Rect rect;
    int const rect_start_w;
    int const rect_start_h;
    SDL_Point click_offset;
    bool in_rect;
    bool left_mouse_button_down;
    float const zoom_factor;
    bool is_running;
} Context;

bool CreateApp(char const *title, int width, int height, App *outApp)
{
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        SDL_Log("%s(%d):\tError @ SDL2 initialization: %s.\n", __FILE__, __LINE__, SDL_GetError());
        return false;
    }

    if (0 == IMG_Init(IMG_INIT_PNG)) {
        SDL_Log("%s(%d):\tError @ SDL2_Image initialization: %s.\n", __FILE__, __LINE__, IMG_GetError());
        return false;
    }

    outApp->window = SDL_CreateWindow(title, SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, width, height, SDL_WINDOW_RESIZABLE);
    if (NULL == outApp->window) {
        SDL_Log("%s(%d):\tError @ window creation: %s.\n", __FILE__, __LINE__, SDL_GetError());
        return false;
    }

    outApp->renderer = SDL_CreateRenderer(outApp->window, -1, 0);
    if (NULL == outApp->renderer) {
        SDL_Log("%s(%d):\tError @ renderer creation: %s.\n", __FILE__, __LINE__, SDL_GetError());
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

uint8_t IsAlive(SDL_Surface *const image, int x, int y)
{
    // Return the most significant bit of the pixel value.
    return ((*((uint8_t*)(image->pixels) + y * image->pitch + x * image->format->BytesPerPixel)) & BIT(7)) >> 7;
}

uint8_t GetAliveNeighborCount(SDL_Surface *const image, int x, int y)
{
    uint8_t sum = 0;

    if (y - 1 >= 0 && x - 1 >= 0) {
        sum += IsAlive(image, x - 1, y - 1);
    }
    if (y - 1 >= 0) {
        sum += IsAlive(image, x, y - 1);
    }
    if (y - 1 >= 0 && x + 1 < image->w) {
        sum += IsAlive(image, x + 1, y - 1);
    }
    if (x - 1 >= 0) {
        sum += IsAlive(image, x - 1, y);
    }
    if (x + 1 < image->w) {
        sum += IsAlive(image, x + 1, y);
    }
    if (y + 1 < image->h && x - 1 >= 0) {
        sum += IsAlive(image, x - 1, y + 1);
    }
    if (y + 1 < image->h) {
        sum += IsAlive(image, x, y + 1);
    }
    if (y + 1 < image->h && x + 1 < image->w) {
        sum += IsAlive(image, x + 1, y + 1);
    }

    return sum;
}

void SetSurfacePixel(SDL_Surface *const image, int x, int y, uint32_t color)
{
    uint8_t const bytes_per_pixel = image->format->BytesPerPixel;
    uint8_t *const pixel_address = (uint8_t*)(image->pixels) + image->pitch * y + x * bytes_per_pixel;

    uint8_t const r = (color & 0xFF000000) >> 24;
    uint8_t const g = (color & 0x00FF0000) >> 16;
    uint8_t const b = (color & 0x0000FF00) >> 8;
    uint8_t const a = (color & 0x000000FF);

    SDL_memset4(pixel_address, SDL_MapRGBA(image->format, r, g, b, a), bytes_per_pixel / 4);
}

bool Step(SDL_Surface *const src, SDL_Surface *const dst, Colors const *const colors, Rules const *const rules)
{
    if (src->w != dst->w || src->h != dst->h) {
        SDL_Log("%s(%d):\tSurface dimensions do not match.\n", __FILE__, __LINE__);
        return false;
    }
    if (SDL_BlitSurface(src, NULL, dst, NULL) != 0) {
        SDL_Log("%s(%d):\tError @ copying surface: %s.\n", __FILE__, __LINE__, SDL_GetError());
        return false;
    }


#pragma omp parallel for \
    collapse(2) \
    default(none) \
    shared(src, dst, colors, rules)
    for (int y = 0; y < src->h; ++y) {
        for (int x = 0; x < src->w; ++x) {
            uint8_t const alive_neighbors = GetAliveNeighborCount(src, x, y);
            uint8_t const is_alive = IsAlive(src, x, y);

            if (is_alive == 1 && (alive_neighbors < rules->survive_min || alive_neighbors > rules->survive_max)) {
                SetSurfacePixel(dst, x, y, colors->dead);
            } else if (is_alive == 0 && (alive_neighbors >= rules->reproduction_min && alive_neighbors <= rules->reproduction_max)) {
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
            if (IsAlive(image, x, y) == 1) {
                SetSurfacePixel(image, x, y, colors->alive);
            } else {
                SetSurfacePixel(image, x, y, colors->dead);
            }
        }
    }
}

void HandleInputs(Context *const ctx)
{
    while(SDL_PollEvent(&(ctx->event)) != 0) {
        switch(ctx->event.type) {
            case SDL_QUIT:
                ctx->quit = true;
                break;
            case SDL_KEYUP:
                switch(ctx->event.key.keysym.sym) {
                    case SDLK_ESCAPE:
                        ctx->quit = true;
                        break;
                    case SDLK_SPACE:
                        ctx->is_running = !ctx->is_running;
                        break;
                }
                break;
            case SDL_MOUSEMOTION:
                if (ctx->left_mouse_button_down && ctx->in_rect) {
                    ctx->rect.x = ctx->event.motion.x - ctx->click_offset.x;
                    ctx->rect.y = ctx->event.motion.y - ctx->click_offset.y;
                }
                break;
            case SDL_MOUSEBUTTONUP:
                switch(ctx->event.button.button) {
                    case SDL_BUTTON_LEFT:
                        ctx->left_mouse_button_down = false;
                        ctx->in_rect = false;
                        break;
                    case SDL_BUTTON_MIDDLE:
                        ctx->rect.w = ctx->rect_start_w;
                        ctx->rect.h = ctx->rect_start_h;
                        break;
                }
                break;
            case SDL_MOUSEBUTTONDOWN:
                switch(ctx->event.button.button) {
                    case SDL_BUTTON_LEFT:
                        ctx->left_mouse_button_down = true;
                        SDL_Point mouse_pos = { ctx->event.motion.x, ctx->event.motion.y };
                        if (SDL_PointInRect(&mouse_pos, &(ctx->rect))) {
                            ctx->click_offset.x = mouse_pos.x - ctx->rect.x;
                            ctx->click_offset.y = mouse_pos.y - ctx->rect.y;
                            ctx->in_rect = true;
                        } else {
                            ctx->in_rect = false;
                        }
                        break;
                }
                break;
            case SDL_MOUSEWHEEL:
                if (ctx->event.wheel.y > 0) {
                    ctx->rect.w *= ctx->zoom_factor;
                    ctx->rect.h *= ctx->zoom_factor;
                } else {
                    ctx->rect.w /= ctx->zoom_factor;
                    ctx->rect.h /= ctx->zoom_factor;
                }
                break;
        }
    }
}

void Draw(SDL_Texture *const texture, SDL_Surface const *const surface, SDL_Renderer *const renderer, SDL_Rect const *const rect)
{
    SDL_RenderClear(renderer);
    SDL_UpdateTexture(texture, NULL, surface->pixels, surface->pitch);
    SDL_RenderCopy(renderer, texture, NULL, rect);
    SDL_RenderPresent(renderer);
}

void Swap(SDL_Surface **src, SDL_Surface **dst)
{
    SDL_Surface *tmp = *src;
    *src = *dst;
    *dst = tmp;
}

int main(int argc, char **argv)
{
    if (argc != 2) {
        SDL_Log("Wrong number of arguments provided! 1 argument is needed for the file path.\n");
        return 1;
    }

    App app;

    if (!CreateApp("Game of Life", 800, 600, &app)) {
        return 1;
    }

    SDL_Surface *src = IMG_Load(argv[1]);
    if (NULL == src) {
        SDL_Log("%s(%d):\tError @ loading image: %s.\n", __FILE__, __LINE__, IMG_GetError());
        return 1;
    }

    uint8_t const bits_per_pixel = src->format->BitsPerPixel;
    if (bits_per_pixel != 32) {
        SDL_Log("%s(%d):\tWrong image format: %d bits per pixel instead of 32.\n", __FILE__, __LINE__, bits_per_pixel);
        return 1;
    }

    Colors colors = { .alive = 0xFFFF00FF, .dead = 0x121212FF };

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
        SDL_Log("%s(%d):\tError @ creating image: %s.\n", __FILE__, __LINE__, SDL_GetError());
        return 1;
    }

    SDL_Texture *texture = SDL_CreateTextureFromSurface(app.renderer, src);
    if (NULL == texture) {
        SDL_Log("%s(%d):\tError @ creating texture: %s.\n", __FILE__, __LINE__, SDL_GetError());
        return 1;
    }

    Rules const rules = { .survive_min = 2, .survive_max = 3, .reproduction_min = 3, .reproduction_max = 3 };

    Context ctx = {
        .quit = false,
        .delay_ms = 20,
        .rect = {0, 0, src->w, src->h},
        .rect_start_w = src->w,
        .rect_start_h = src->h,
        .click_offset = { 0, 0 },
        .in_rect = false,
        .left_mouse_button_down = false,
        .zoom_factor = 1.1F,
        .is_running = false
    };

    while(!ctx.quit) {
        HandleInputs(&ctx);
        Draw(texture, src, app.renderer, &(ctx.rect));
        if (ctx.is_running) {
            Step(src, dst, &colors, &rules);
            Swap(&src, &dst);
        }
        SDL_Delay(ctx.delay_ms);
    }
    SDL_DestroyTexture(texture);

    SDL_FreeSurface(dst);
    SDL_FreeSurface(src);
    DestroyApp(&app);

    return 0;
}
