#ifndef DIABLO_ENGINE_RENDERER_H
#define DIABLO_ENGINE_RENDERER_H

#include "common.h"

typedef struct IsoRenderer {
    SDL_Renderer *sdl_renderer;
    int camera_offset_x;  /* screen_width / 2 centering */
} IsoRenderer;

void iso_renderer_init(IsoRenderer *r, SDL_Renderer *sdl_renderer, int screen_width);

/* Core isometric drawing functions */
void iso_draw_tile(IsoRenderer *r, SDL_Texture *tex,
                   int tile_x, int tile_y, int cam_x, int cam_y);

void iso_draw_wall(IsoRenderer *r, SDL_Texture *tex,
                   int tile_x, int tile_y, int cam_x, int cam_y,
                   int extra_height);

void iso_draw_sprite(IsoRenderer *r, SDL_Texture *tex,
                     int tile_x, int tile_y, int cam_x, int cam_y,
                     int sprite_w, int sprite_h);

void iso_draw_tile_highlight(IsoRenderer *r,
                             int tile_x, int tile_y,
                             int cam_x, int cam_y, SDL_Color color);

#endif /* DIABLO_ENGINE_RENDERER_H */
