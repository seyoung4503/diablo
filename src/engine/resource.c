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
                /* Inside diamond — smooth edge blending near boundary */
                float edge_dist = 1.0f - (dx + dy);
                if (edge_dist < 0.05f) {
                    /* Near edge: partial transparency for anti-aliasing */
                    Uint8 pr, pg, pb, pa;
                    SDL_GetRGBA(pixels[y * pitch + x], work->format,
                                &pr, &pg, &pb, &pa);
                    Uint8 alpha = (Uint8)(edge_dist / 0.05f * 255.0f);
                    pixels[y * pitch + x] = SDL_MapRGBA(work->format, pr, pg, pb, alpha);
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
 * Load a sprite texture. For PNG files with alpha, load directly.
 * For JPG files, load without color-key (JPG compression makes color-key unreliable).
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

    /* Check if loaded file is PNG (has alpha) — if so, use directly */
    const char *ext = strrchr(path, '.');
    bool is_png = false;

    /* Check if we actually loaded a .png version */
    if (ext && strcmp(ext, ".jpg") == 0) {
        char png_path[256];
        size_t base_len = (size_t)(ext - path);
        if (base_len < sizeof(png_path) - 5) {
            memcpy(png_path, path, base_len);
            strcpy(png_path + base_len, ".png");
            FILE *f = fopen(png_path, "r");
            if (f) { fclose(f); is_png = true; }
        }
    } else if (ext && strcmp(ext, ".png") == 0) {
        is_png = true;
    }

    SDL_Texture *tex;
    if (is_png && src->format->Amask != 0) {
        /* PNG with alpha — use directly, no color-key needed */
        tex = SDL_CreateTextureFromSurface(rm->renderer, src);
    } else {
        /* JPG or no alpha — just load as-is without destructive color-key */
        tex = SDL_CreateTextureFromSurface(rm->renderer, src);
    }
    SDL_FreeSurface(src);

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
