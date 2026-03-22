#ifndef DIABLO_COMMON_H
#define DIABLO_COMMON_H

#include <SDL.h>
#include <SDL_image.h>
#include <SDL_ttf.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

/* Display */
#define SCREEN_WIDTH   640
#define SCREEN_HEIGHT  480
#define PANEL_HEIGHT   128
#define VIEWPORT_HEIGHT (SCREEN_HEIGHT - PANEL_HEIGHT)

/* Isometric tile dimensions (2:1 diamond) */
#define TILE_WIDTH   64
#define TILE_HEIGHT  32
#define TILE_HALF_W  32
#define TILE_HALF_H  16

/* Sprite sizes */
#define SPRITE_SIZE       96
#define SPRITE_SIZE_LARGE 128

/* Map dimensions */
#define MAP_WIDTH  40
#define MAP_HEIGHT 40

/* Timing */
#define TARGET_FPS 60

/* Entity limits */
#define MAX_NPCS    32
#define MAX_ENEMIES 64
#define MAX_ITEMS   256

/* 8-direction system (clockwise from south) */
typedef enum {
    DIR_S = 0,
    DIR_SW,
    DIR_W,
    DIR_NW,
    DIR_N,
    DIR_NE,
    DIR_E,
    DIR_SE,
    DIR_COUNT
} Direction;

/* Utility macros */
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define CLAMP(x, lo, hi) (MIN(MAX((x), (lo)), (hi)))

/*
 * Convert isometric tile coordinates to screen pixel coordinates.
 * Output (sx, sy) is the top-center of the diamond for the tile.
 */
static inline void iso_to_screen(int tile_x, int tile_y, int *sx, int *sy)
{
    *sx = (tile_x - tile_y) * TILE_HALF_W;
    *sy = (tile_x + tile_y) * TILE_HALF_H;
}

/*
 * Convert screen pixel coordinates back to isometric tile coordinates.
 * Uses integer math with rounding for tile picking.
 */
static inline void screen_to_iso(int sx, int sy, int *tile_x, int *tile_y)
{
    /* Floating point for accuracy, then floor */
    float fx = ((float)sx / TILE_HALF_W + (float)sy / TILE_HALF_H) / 2.0f;
    float fy = ((float)sy / TILE_HALF_H - (float)sx / TILE_HALF_W) / 2.0f;
    *tile_x = (int)floorf(fx);
    *tile_y = (int)floorf(fy);
}

#endif /* DIABLO_COMMON_H */
