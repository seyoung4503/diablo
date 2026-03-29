#ifndef DIABLO_ENGINE_RENDERER_H
#define DIABLO_ENGINE_RENDERER_H

#include "common.h"
#include "world/map.h"

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

/* Procedural tile drawing (no textures needed) */
void iso_draw_tile_filled(IsoRenderer *r, int tile_x, int tile_y,
                          int cam_x, int cam_y,
                          SDL_Color base_color, SDL_Color shade_color);

void iso_draw_wall_filled(IsoRenderer *r, int tile_x, int tile_y,
                          int cam_x, int cam_y, int height,
                          SDL_Color top_color, SDL_Color front_color,
                          SDL_Color side_color);

/* Draw the correct visual for a given tile type */
void iso_draw_tile_by_type(IsoRenderer *r, TileType type,
                           int tile_x, int tile_y,
                           int cam_x, int cam_y);

/* Draw a simple humanoid character placeholder */
void iso_draw_character(IsoRenderer *r, int tile_x, int tile_y,
                        int cam_x, int cam_y,
                        SDL_Color body_color, SDL_Color head_color);

/* Draw an animated sprite frame at a world position */
void iso_draw_animated_sprite(IsoRenderer *r, SDL_Texture *tex,
                              SDL_Rect src_rect,
                              int world_x, int world_y,
                              int cam_x, int cam_y,
                              int dest_w, int dest_h);

#endif /* DIABLO_ENGINE_RENDERER_H */
