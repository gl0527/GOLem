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
    uint32_t initial_dead;
    uint32_t computed_dead;
} Colors;

typedef enum {
    QUIT,
    BUSY,
    IDLE,
} AppState;

typedef struct {
    SDL_Window *window;
    SDL_Renderer *renderer;
    AppState state;
} App;

typedef struct {
    SDL_Event event;
    uint32_t delay_ms;
    SDL_Rect rect;
    int const rect_start_w;
    int const rect_start_h;
    SDL_Point click_offset;
    bool in_rect;
    bool left_mouse_button_down;
} Context;

static bool CreateApp(char const *title, int width, int height, App *outApp)
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

    outApp->state = IDLE;

    return true;
}

static void DestroyApp(App *app)
{
    SDL_DestroyRenderer(app->renderer);
    SDL_DestroyWindow(app->window);
    IMG_Quit();
    SDL_Quit();
}

static inline uint8_t* GetPixelAddress(SDL_Surface const *const image, int x, int y)
{
    return (uint8_t*)(image->pixels) + y * image->pitch + x * image->format->BytesPerPixel;
}

static inline uint8_t IsAlive(SDL_Surface *const image, int x, int y)
{
    // Return the most significant bit of the pixel value.
    return ((*GetPixelAddress(image, x, y)) & BIT(7)) >> 7;
}

static uint8_t GetAliveNeighborCount(SDL_Surface *const image, int x, int y)
{
    int const w = image->w;
    int const h = image->h;

    int const left = (x - 1 + w) % w;
    int const right = (x + 1) % w;
    int const top = (y - 1 + h) % h;
    int const bottom = (y + 1) % h;

    return IsAlive(image, left, top) + IsAlive(image, x, top) + IsAlive(image, right, top) +
        IsAlive(image, left, y) + IsAlive(image, right, y) +
        IsAlive(image, left, bottom) + IsAlive(image, x, bottom) + IsAlive(image, right, bottom);
}

static void SetSurfacePixel(SDL_Surface *const image, int x, int y, uint32_t color)
{
    uint8_t const bytes_per_pixel = image->format->BytesPerPixel;

    uint8_t const r = (color & 0xFF000000) >> 24;
    uint8_t const g = (color & 0x00FF0000) >> 16;
    uint8_t const b = (color & 0x0000FF00) >> 8;
    uint8_t const a = (color & 0x000000FF);

    SDL_memset4(GetPixelAddress(image, x, y), SDL_MapRGBA(image->format, r, g, b, a), bytes_per_pixel / 4);
}

static bool Step(SDL_Surface *const src, SDL_Surface *const dst, Colors const *const colors, Rules const *const rules)
{
    int const src_height = src->h;
    int const src_width = src->w;

    if (src_width != dst->w || src_height != dst->h) {
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
    shared(src, dst, colors, rules, src_height, src_width)
    for (int y = 0; y < src_height; ++y) {
        for (int x = 0; x < src_width; ++x) {
            uint8_t const alive_neighbors = GetAliveNeighborCount(src, x, y);
            uint8_t const is_alive = IsAlive(src, x, y);

            if (is_alive == 1 && (alive_neighbors < rules->survive_min || alive_neighbors > rules->survive_max)) {
                SetSurfacePixel(dst, x, y, colors->computed_dead);
            } else if (is_alive == 0 && (alive_neighbors >= rules->reproduction_min && alive_neighbors <= rules->reproduction_max)) {
                SetSurfacePixel(dst, x, y, colors->alive);
            }
        }
    }

    return true;
}

static void Binarize(SDL_Surface *const image, Colors const *const colors)
{
    for (int y = 0; y < image->h; ++y) {
        for (int x = 0; x < image->w; ++x) {
            if (IsAlive(image, x, y) == 1) {
                SetSurfacePixel(image, x, y, colors->alive);
            } else {
                SetSurfacePixel(image, x, y, colors->initial_dead);
            }
        }
    }
}

static void HandleInputs(Context *const ctx, AppState *const app_state)
{
    while(SDL_PollEvent(&(ctx->event)) != 0) {
        switch(ctx->event.type) {
            case SDL_QUIT:
                *app_state = QUIT;
                break;
            case SDL_KEYUP:
                switch(ctx->event.key.keysym.sym) {
                    case SDLK_ESCAPE:
                        *app_state = QUIT;
                        break;
                    case SDLK_SPACE:
                        switch(*app_state)
                        {
                            case BUSY: *app_state = IDLE; break;
                            case IDLE: *app_state = BUSY; break;
                            case QUIT: break;
                        }
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
                    if (ctx->rect.w < 50 * ctx->rect_start_w) {
                        ctx->rect.w += ctx->rect_start_w;
                        ctx->rect.h += ctx->rect_start_h;
                    }
                } else {
                    if (ctx->rect.w > ctx->rect_start_w) {
                        ctx->rect.w -= ctx->rect_start_w;
                        ctx->rect.h -= ctx->rect_start_h;
                    }
                }
                break;
        }
    }
}

static void Draw(SDL_Texture *const texture, SDL_Surface const *const surface, SDL_Renderer *const renderer, SDL_Rect const *const rect)
{
    SDL_RenderClear(renderer);
    SDL_UpdateTexture(texture, NULL, surface->pixels, surface->pitch);
    SDL_RenderCopy(renderer, texture, NULL, rect);
    SDL_RenderPresent(renderer);
}

static void Swap(SDL_Surface **src, SDL_Surface **dst)
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

    Colors colors = { .alive = 0xFFFF00FF, .initial_dead = 0x400040FF, .computed_dead = 0x006060FF };

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
        .delay_ms = 20,
        .rect = {0, 0, src->w, src->h},
        .rect_start_w = src->w,
        .rect_start_h = src->h,
        .click_offset = { 0, 0 },
        .in_rect = false,
        .left_mouse_button_down = false,
    };

    while(app.state != QUIT) {
        HandleInputs(&ctx, &(app.state));
        Draw(texture, src, app.renderer, &(ctx.rect));
        if (app.state == BUSY) {
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
