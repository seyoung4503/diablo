#include "engine/renderer.h"

void iso_renderer_init(IsoRenderer *r, SDL_Renderer *sdl_renderer, int screen_width)
{
    r->sdl_renderer = sdl_renderer;
    r->camera_offset_x = screen_width / 2;
}

/*
 * Draw a single isometric tile texture at the given tile coordinates.
 * The texture is scaled to TILE_WIDTH x TILE_HEIGHT and positioned so
 * the top of the diamond aligns with iso_to_screen output.
 */
void iso_draw_tile(IsoRenderer *r, SDL_Texture *tex,
                   int tile_x, int tile_y, int cam_x, int cam_y)
{
    if (!tex) return;

    int sx, sy;
    iso_to_screen(tile_x, tile_y, &sx, &sy);

    SDL_Rect dst = {
        .x = sx - TILE_HALF_W - cam_x + r->camera_offset_x,
        .y = sy - cam_y,
        .w = TILE_WIDTH,
        .h = TILE_HEIGHT,
    };

    SDL_RenderCopy(r->sdl_renderer, tex, NULL, &dst);
}

/*
 * Draw a wall block: floor-height tile plus a raised portion to give
 * the illusion of a 3D wall block sitting on the grid.
 */
void iso_draw_wall(IsoRenderer *r, SDL_Texture *tex,
                   int tile_x, int tile_y, int cam_x, int cam_y,
                   int extra_height)
{
    if (!tex) return;

    int sx, sy;
    iso_to_screen(tile_x, tile_y, &sx, &sy);

    SDL_Rect dst = {
        .x = sx - TILE_HALF_W - cam_x + r->camera_offset_x,
        .y = sy - extra_height - cam_y,
        .w = TILE_WIDTH,
        .h = TILE_HEIGHT + extra_height,
    };

    SDL_RenderCopy(r->sdl_renderer, tex, NULL, &dst);
}

/*
 * Draw a sprite centered on a tile.
 * sprite_w and sprite_h control the destination size.
 */
void iso_draw_sprite(IsoRenderer *r, SDL_Texture *tex,
                     int tile_x, int tile_y, int cam_x, int cam_y,
                     int sprite_w, int sprite_h)
{
    if (!tex) return;

    int sx, sy;
    iso_to_screen(tile_x, tile_y, &sx, &sy);

    /* Center sprite on the tile's center point */
    SDL_Rect dst = {
        .x = sx - sprite_w / 2 - cam_x + r->camera_offset_x,
        .y = sy + TILE_HALF_H - sprite_h + 8 - cam_y, /* feet at tile center */
        .w = sprite_w,
        .h = sprite_h,
    };

    SDL_RenderCopy(r->sdl_renderer, tex, NULL, &dst);
}

/*
 * Draw a filled isometric diamond outline for tile highlighting.
 * Draws a semi-transparent colored diamond at the given tile position.
 */
void iso_draw_tile_highlight(IsoRenderer *r,
                             int tile_x, int tile_y,
                             int cam_x, int cam_y, SDL_Color color)
{
    int sx, sy;
    iso_to_screen(tile_x, tile_y, &sx, &sy);

    /* Apply camera and center horizontally in viewport */
    int cx = sx - cam_x + r->camera_offset_x;
    int cy = sy - cam_y;

    /* Diamond vertices: top, right, bottom, left */
    SDL_Point pts[5] = {
        { cx,                cy },                    /* top */
        { cx + TILE_HALF_W,  cy + TILE_HALF_H },     /* right */
        { cx,                cy + TILE_HEIGHT },      /* bottom */
        { cx - TILE_HALF_W,  cy + TILE_HALF_H },     /* left */
        { cx,                cy },                    /* close */
    };

    /* Fill with semi-transparent overlay using scanlines */
    SDL_SetRenderDrawBlendMode(r->sdl_renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(r->sdl_renderer, color.r, color.g, color.b, color.a);

    for (int row = 0; row < TILE_HEIGHT; row++) {
        float t;
        int half_w;
        if (row <= TILE_HALF_H) {
            t = (float)row / TILE_HALF_H;
        } else {
            t = (float)(TILE_HEIGHT - row) / TILE_HALF_H;
        }
        half_w = (int)(t * TILE_HALF_W);
        SDL_RenderDrawLine(r->sdl_renderer, cx - half_w, cy + row,
                           cx + half_w, cy + row);
    }

    /* Draw diamond border */
    SDL_SetRenderDrawColor(r->sdl_renderer, color.r, color.g, color.b, 200);
    SDL_RenderDrawLines(r->sdl_renderer, pts, 5);
}
