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
