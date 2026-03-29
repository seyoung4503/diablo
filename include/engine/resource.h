#ifndef DIABLO_RESOURCE_H
#define DIABLO_RESOURCE_H

#include "common.h"

#define MAX_TEXTURES 128
#define MAX_FONTS    8

typedef struct ResourceManager {
    SDL_Renderer *renderer;
    SDL_Texture *textures[MAX_TEXTURES];
    char *texture_names[MAX_TEXTURES];
    int texture_count;
    TTF_Font *fonts[MAX_FONTS];
    int font_count;
} ResourceManager;

/* Initialize the resource manager. Must be called before loading any resources. */
void resource_init(ResourceManager *rm, SDL_Renderer *renderer);

/* Free all loaded resources. */
void resource_shutdown(ResourceManager *rm);

/* Load a texture from disk. Returns the texture index, or -1 on failure. */
int resource_load_texture(ResourceManager *rm, const char *path);

/* Load a tile texture with diamond masking (transparent outside diamond). */
int resource_load_tile_texture(ResourceManager *rm, const char *path, int w, int h);

/* Load a sprite texture with color-key background removal. */
int resource_load_sprite_texture(ResourceManager *rm, const char *path);

/* Get a texture by index. Returns NULL if the index is invalid. */
SDL_Texture *resource_get_texture(ResourceManager *rm, int index);

/* Load a font from disk at the given point size. Returns font index, or -1 on failure. */
int resource_load_font(ResourceManager *rm, const char *path, int size);

/* Get a font by index. Returns NULL if the index is invalid. */
TTF_Font *resource_get_font(ResourceManager *rm, int index);

#endif /* DIABLO_RESOURCE_H */
