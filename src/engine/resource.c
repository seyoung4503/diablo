#include "engine/resource.h"

void resource_init(ResourceManager *rm, SDL_Renderer *renderer)
{
    rm->renderer = renderer;
    rm->texture_count = 0;
    rm->font_count = 0;
    memset(rm->textures, 0, sizeof(rm->textures));
    memset(rm->texture_names, 0, sizeof(rm->texture_names));
    memset(rm->fonts, 0, sizeof(rm->fonts));
}

void resource_shutdown(ResourceManager *rm)
{
    for (int i = 0; i < rm->texture_count; i++) {
        if (rm->textures[i])
            SDL_DestroyTexture(rm->textures[i]);
        if (rm->texture_names[i])
            free(rm->texture_names[i]);
    }
    rm->texture_count = 0;

    for (int i = 0; i < rm->font_count; i++) {
        if (rm->fonts[i])
            TTF_CloseFont(rm->fonts[i]);
    }
    rm->font_count = 0;
}

int resource_load_texture(ResourceManager *rm, const char *path)
{
    if (rm->texture_count >= MAX_TEXTURES) {
        fprintf(stderr, "ResourceManager: texture limit reached (%d)\n", MAX_TEXTURES);
        return -1;
    }

    SDL_Texture *tex = IMG_LoadTexture(rm->renderer, path);
    if (!tex) {
        fprintf(stderr, "Failed to load texture '%s': %s\n", path, IMG_GetError());
        return -1;
    }

    int idx = rm->texture_count;
    rm->textures[idx] = tex;
    rm->texture_names[idx] = strdup(path);
    rm->texture_count++;
    return idx;
}

/* Try loading PNG version first, fall back to original path */
static SDL_Surface *load_image_prefer_png(const char *path)
{
    char png_path[256];
    const char *dot = strrchr(path, '.');
    if (dot && strcmp(dot, ".jpg") == 0) {
        size_t base_len = (size_t)(dot - path);
        if (base_len < sizeof(png_path) - 5) {
            memcpy(png_path, path, base_len);
            strcpy(png_path + base_len, ".png");
            SDL_Surface *s = IMG_Load(png_path);
            if (s) return s;
        }
    }
    return IMG_Load(path);
}

/*
 * Apply a diamond mask to a surface: pixels outside the isometric diamond
 * shape are set to fully transparent. The diamond is defined by:
 *   |x - w/2| / (w/2) + |y - h/2| / (h/2) <= 1.0
 */
static SDL_Surface *apply_diamond_mask(SDL_Surface *src, int w, int h)
{
    /* Convert to 32-bit RGBA */
    SDL_Surface *rgba = SDL_ConvertSurfaceFormat(src, SDL_PIXELFORMAT_RGBA32, 0);
    if (!rgba) return NULL;

    /* Scale if source dimensions differ */
    SDL_Surface *work = rgba;
    if (work->w != w || work->h != h) {
        SDL_Surface *scaled = SDL_CreateRGBSurfaceWithFormat(0, w, h, 32,
                                                             SDL_PIXELFORMAT_RGBA32);
        if (scaled) {
            SDL_BlitScaled(work, NULL, scaled, NULL);
            SDL_FreeSurface(work);
            work = scaled;
        }
    }

    float half_w = w / 2.0f;
    float half_h = h / 2.0f;

    SDL_LockSurface(work);
    Uint32 *pixels = (Uint32 *)work->pixels;
    int pitch = work->pitch / 4;

    for (int y = 0; y < h; y++) {
        for (int x = 0; x < w; x++) {
            float dx = fabsf((float)x - half_w) / half_w;
            float dy = fabsf((float)y - half_h) / half_h;
            if (dx + dy > 1.0f) {
                /* Outside diamond — fully transparent */
                pixels[y * pitch + x] = 0;
            } else {
                /* Inside diamond — remove pink/magenta artifacts */
                Uint8 pr, pg, pb, pa;
                SDL_GetRGBA(pixels[y * pitch + x], work->format,
                            &pr, &pg, &pb, &pa);
                if (pr > 150 && pg < 120 && pb > 130) {
                    pixels[y * pitch + x] = 0;
                }
            }
        }
    }
    SDL_UnlockSurface(work);
    return work;
}

int resource_load_tile_texture(ResourceManager *rm, const char *path, int w, int h)
{
    if (rm->texture_count >= MAX_TEXTURES) {
        fprintf(stderr, "ResourceManager: texture limit reached (%d)\n", MAX_TEXTURES);
        return -1;
    }

    SDL_Surface *src = load_image_prefer_png(path);
    if (!src) {
        fprintf(stderr, "Failed to load tile '%s': %s\n", path, IMG_GetError());
        return -1;
    }

    SDL_Surface *masked = apply_diamond_mask(src, w, h);
    SDL_FreeSurface(src);
    if (!masked) {
        fprintf(stderr, "Failed to apply diamond mask for '%s'\n", path);
        return -1;
    }

    SDL_Texture *tex = SDL_CreateTextureFromSurface(rm->renderer, masked);
    SDL_FreeSurface(masked);
    if (!tex) {
        fprintf(stderr, "Failed to create texture for '%s': %s\n", path, SDL_GetError());
        return -1;
    }

    SDL_SetTextureBlendMode(tex, SDL_BLENDMODE_BLEND);

    int idx = rm->texture_count;
    rm->textures[idx] = tex;
    rm->texture_names[idx] = strdup(path);
    rm->texture_count++;
    return idx;
}

/*
 * Load a sprite with color-key transparency. Detects the background color
 * from the top-left corner pixel and makes all matching pixels transparent.
 */
int resource_load_sprite_texture(ResourceManager *rm, const char *path)
{
    if (rm->texture_count >= MAX_TEXTURES) {
        fprintf(stderr, "ResourceManager: texture limit reached (%d)\n", MAX_TEXTURES);
        return -1;
    }

    SDL_Surface *src = load_image_prefer_png(path);
    if (!src) {
        fprintf(stderr, "Failed to load sprite '%s': %s\n", path, IMG_GetError());
        return -1;
    }

    /* Convert to 32-bit RGBA for pixel manipulation */
    SDL_Surface *rgba = SDL_ConvertSurfaceFormat(src, SDL_PIXELFORMAT_RGBA32, 0);
    SDL_FreeSurface(src);
    if (!rgba) return -1;

    int sw = rgba->w;
    int sh = rgba->h;

    SDL_LockSurface(rgba);
    Uint32 *pixels = (Uint32 *)rgba->pixels;
    int pitch = rgba->pitch / 4;

    /* Sample 4 corner pixels to detect checkerboard background colors */
    Uint32 corner_raw[4] = {
        pixels[0],                              /* top-left */
        pixels[sw - 1],                         /* top-right */
        pixels[(sh - 1) * pitch],               /* bottom-left */
        pixels[(sh - 1) * pitch + (sw - 1)]     /* bottom-right */
    };
    Uint8 corner_r[4], corner_g[4], corner_b[4];
    for (int c = 0; c < 4; c++)
        SDL_GetRGB(corner_raw[c], rgba->format,
                   &corner_r[c], &corner_g[c], &corner_b[c]);

    /* Remove all pixels matching any corner color (with tolerance) */
    #define BG_TOLERANCE 30
    for (int y = 0; y < sh; y++) {
        for (int x = 0; x < sw; x++) {
            Uint8 pr, pg, pb;
            SDL_GetRGB(pixels[y * pitch + x], rgba->format, &pr, &pg, &pb);
            for (int c = 0; c < 4; c++) {
                if (abs((int)pr - corner_r[c]) < BG_TOLERANCE &&
                    abs((int)pg - corner_g[c]) < BG_TOLERANCE &&
                    abs((int)pb - corner_b[c]) < BG_TOLERANCE) {
                    pixels[y * pitch + x] = 0; /* fully transparent */
                    break;
                }
            }
        }
    }
    #undef BG_TOLERANCE

    SDL_UnlockSurface(rgba);

    SDL_Texture *tex = SDL_CreateTextureFromSurface(rm->renderer, rgba);
    SDL_FreeSurface(rgba);
    if (!tex) {
        fprintf(stderr, "Failed to create sprite texture '%s': %s\n", path, SDL_GetError());
        return -1;
    }

    SDL_SetTextureBlendMode(tex, SDL_BLENDMODE_BLEND);

    int idx = rm->texture_count;
    rm->textures[idx] = tex;
    rm->texture_names[idx] = strdup(path);
    rm->texture_count++;
    return idx;
}

SDL_Texture *resource_get_texture(ResourceManager *rm, int index)
{
    if (index < 0 || index >= rm->texture_count)
        return NULL;
    return rm->textures[index];
}

int resource_load_font(ResourceManager *rm, const char *path, int size)
{
    if (rm->font_count >= MAX_FONTS) {
        fprintf(stderr, "ResourceManager: font limit reached (%d)\n", MAX_FONTS);
        return -1;
    }

    TTF_Font *font = TTF_OpenFont(path, size);
    if (!font) {
        fprintf(stderr, "Failed to load font '%s': %s\n", path, TTF_GetError());
        return -1;
    }

    int idx = rm->font_count;
    rm->fonts[idx] = font;
    rm->font_count++;
    return idx;
}

TTF_Font *resource_get_font(ResourceManager *rm, int index)
{
    if (index < 0 || index >= rm->font_count)
        return NULL;
    return rm->fonts[index];
}
