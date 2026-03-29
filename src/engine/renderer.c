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

/* Color palette for each tile type (Diablo 1 dark aesthetic) */
typedef struct TileColors {
    SDL_Color base;
    SDL_Color shade;
} TileColors;

static TileColors tile_colors[] = {
    [TILE_NONE]        = {{ 20, 15, 25, 255}, { 10,  8, 15, 255}},
    [TILE_GRASS]       = {{ 35, 55, 30, 255}, { 25, 40, 20, 255}},
    [TILE_DIRT]        = {{ 80, 60, 40, 255}, { 55, 42, 28, 255}},
    [TILE_STONE_FLOOR] = {{ 70, 65, 60, 255}, { 50, 47, 43, 255}},
    [TILE_WALL]        = {{ 90, 80, 70, 255}, { 60, 55, 48, 255}},
    [TILE_WATER]       = {{ 25, 35, 60, 255}, { 15, 25, 45, 255}},
    [TILE_DOOR]        = {{ 90, 60, 30, 255}, { 65, 42, 20, 255}},
    [TILE_STAIRS_UP]   = {{ 60, 55, 50, 255}, { 40, 37, 33, 255}},
    [TILE_STAIRS_DOWN] = {{ 50, 45, 55, 255}, { 30, 25, 35, 255}},
    [TILE_TREE]        = {{ 30, 45, 25, 255}, { 20, 32, 18, 255}},
};

/*
 * Draw a filled isometric diamond with two-tone shading.
 * Top half uses shade_color (darker), bottom half uses base_color (lighter).
 */
void iso_draw_tile_filled(IsoRenderer *r, int tile_x, int tile_y,
                          int cam_x, int cam_y,
                          SDL_Color base_color, SDL_Color shade_color)
{
    int sx, sy;
    iso_to_screen(tile_x, tile_y, &sx, &sy);

    int cx = sx - cam_x + r->camera_offset_x;
    int cy = sy - cam_y;

    SDL_SetRenderDrawBlendMode(r->sdl_renderer, SDL_BLENDMODE_NONE);

    for (int row = 0; row < TILE_HEIGHT; row++) {
        float t;
        if (row <= TILE_HALF_H) {
            t = (float)row / TILE_HALF_H;
        } else {
            t = (float)(TILE_HEIGHT - row) / TILE_HALF_H;
        }
        int half_w = (int)(t * TILE_HALF_W);

        /* Top half = shade (darker), bottom half = base (lighter) */
        if (row <= TILE_HALF_H) {
            SDL_SetRenderDrawColor(r->sdl_renderer,
                shade_color.r, shade_color.g, shade_color.b, 255);
        } else {
            SDL_SetRenderDrawColor(r->sdl_renderer,
                base_color.r, base_color.g, base_color.b, 255);
        }

        SDL_RenderDrawLine(r->sdl_renderer,
                           cx - half_w, cy + row,
                           cx + half_w, cy + row);
    }

    /* Border: darker than shade */
    SDL_Color border = {
        (Uint8)(shade_color.r * 2 / 3),
        (Uint8)(shade_color.g * 2 / 3),
        (Uint8)(shade_color.b * 2 / 3),
        255
    };
    SDL_SetRenderDrawColor(r->sdl_renderer, border.r, border.g, border.b, 255);
    SDL_Point pts[5] = {
        { cx,                cy },
        { cx + TILE_HALF_W,  cy + TILE_HALF_H },
        { cx,                cy + TILE_HEIGHT },
        { cx - TILE_HALF_W,  cy + TILE_HALF_H },
        { cx,                cy },
    };
    SDL_RenderDrawLines(r->sdl_renderer, pts, 5);
}

/*
 * Draw a 3D isometric wall block with three visible faces.
 */
void iso_draw_wall_filled(IsoRenderer *r, int tile_x, int tile_y,
                          int cam_x, int cam_y, int height,
                          SDL_Color top_color, SDL_Color front_color,
                          SDL_Color side_color)
{
    int sx, sy;
    iso_to_screen(tile_x, tile_y, &sx, &sy);

    int cx = sx - cam_x + r->camera_offset_x;
    int cy = sy - cam_y;

    /* Diamond corners at ground level */
    int top_y    = cy;
    int right_x  = cx + TILE_HALF_W;
    int right_y  = cy + TILE_HALF_H;
    int bottom_y = cy + TILE_HEIGHT;
    int left_x   = cx - TILE_HALF_W;
    int left_y   = cy + TILE_HALF_H;

    /* Elevated diamond corners (shifted up by height) */
    int e_top_y    = top_y    - height;
    int e_right_y  = right_y  - height;
    int e_bottom_y = bottom_y - height;
    int e_left_y   = left_y   - height;

    SDL_SetRenderDrawBlendMode(r->sdl_renderer, SDL_BLENDMODE_NONE);

    /* Front face (south-east): parallelogram from bottom to right of diamond */
    SDL_SetRenderDrawColor(r->sdl_renderer,
        front_color.r, front_color.g, front_color.b, 255);
    for (int row = 0; row < height; row++) {
        /* Interpolate the bottom edge of the elevated diamond */
        int y_pos = e_bottom_y + row;
        /* The front face spans from center-bottom to right */
        float progress = (float)row / height;
        /* Left edge goes from cx (elevated bottom) down to cx (ground bottom) */
        /* Right edge goes from right_x (elevated right) down to right_x (ground right) */
        int x_left = cx;
        int x_right = right_x;
        (void)progress; /* both edges are vertical in isometric */
        SDL_RenderDrawLine(r->sdl_renderer, x_left, y_pos, x_right, y_pos);
    }

    /* Side face (south-west): parallelogram from bottom to left of diamond */
    SDL_SetRenderDrawColor(r->sdl_renderer,
        side_color.r, side_color.g, side_color.b, 255);
    for (int row = 0; row < height; row++) {
        int y_pos = e_bottom_y + row;
        int x_left = left_x;
        int x_right = cx;
        SDL_RenderDrawLine(r->sdl_renderer, x_left, y_pos, x_right, y_pos);
    }

    /* Top face: filled diamond at elevated position */
    for (int row = 0; row < TILE_HEIGHT; row++) {
        float t;
        if (row <= TILE_HALF_H) {
            t = (float)row / TILE_HALF_H;
        } else {
            t = (float)(TILE_HEIGHT - row) / TILE_HALF_H;
        }
        int half_w = (int)(t * TILE_HALF_W);
        SDL_SetRenderDrawColor(r->sdl_renderer,
            top_color.r, top_color.g, top_color.b, 255);
        SDL_RenderDrawLine(r->sdl_renderer,
                           cx - half_w, e_top_y + row,
                           cx + half_w, e_top_y + row);
    }

    /* Edges of the block */
    SDL_Color edge = {
        (Uint8)(side_color.r * 2 / 3),
        (Uint8)(side_color.g * 2 / 3),
        (Uint8)(side_color.b * 2 / 3),
        255
    };
    SDL_SetRenderDrawColor(r->sdl_renderer, edge.r, edge.g, edge.b, 255);

    /* Top diamond outline */
    SDL_Point top_pts[5] = {
        { cx,       e_top_y },
        { right_x,  e_right_y },
        { cx,       e_bottom_y },
        { left_x,   e_left_y },
        { cx,       e_top_y },
    };
    SDL_RenderDrawLines(r->sdl_renderer, top_pts, 5);

    /* Vertical edges */
    SDL_RenderDrawLine(r->sdl_renderer, cx, e_bottom_y, cx, bottom_y);
    SDL_RenderDrawLine(r->sdl_renderer, right_x, e_right_y, right_x, right_y);
    SDL_RenderDrawLine(r->sdl_renderer, left_x, e_left_y, left_x, left_y);

    /* Bottom visible edges */
    SDL_RenderDrawLine(r->sdl_renderer, left_x, left_y, cx, bottom_y);
    SDL_RenderDrawLine(r->sdl_renderer, cx, bottom_y, right_x, right_y);
}

/*
 * Draw the appropriate visual for a tile type using procedural rendering.
 */
void iso_draw_tile_by_type(IsoRenderer *r, TileType type,
                           int tile_x, int tile_y,
                           int cam_x, int cam_y)
{
    if (type < 0 || type >= TILE_TYPE_COUNT)
        type = TILE_NONE;

    TileColors *tc = &tile_colors[type];

    switch (type) {
    case TILE_WALL: {
        /* Draw grass underneath, then a 3D wall block */
        TileColors *grass = &tile_colors[TILE_STONE_FLOOR];
        iso_draw_tile_filled(r, tile_x, tile_y, cam_x, cam_y,
                             grass->base, grass->shade);
        /* Wall faces: top = lightest, front = medium, side = darkest */
        SDL_Color top   = tc->base;
        SDL_Color front = tc->shade;
        SDL_Color side  = { (Uint8)(tc->shade.r * 3 / 4),
                            (Uint8)(tc->shade.g * 3 / 4),
                            (Uint8)(tc->shade.b * 3 / 4), 255 };
        iso_draw_wall_filled(r, tile_x, tile_y, cam_x, cam_y,
                             TILE_HEIGHT, top, front, side);
        break;
    }
    case TILE_TREE: {
        /* Grass base + green trunk block */
        TileColors *grass = &tile_colors[TILE_GRASS];
        iso_draw_tile_filled(r, tile_x, tile_y, cam_x, cam_y,
                             grass->base, grass->shade);
        SDL_Color top   = { 40, 65, 35, 255 };
        SDL_Color front = tc->base;
        SDL_Color side  = tc->shade;
        iso_draw_wall_filled(r, tile_x, tile_y, cam_x, cam_y,
                             TILE_HEIGHT, top, front, side);
        break;
    }
    case TILE_STAIRS_DOWN: {
        /* Stone floor with a dark hole in center */
        TileColors *stone = &tile_colors[TILE_STONE_FLOOR];
        iso_draw_tile_filled(r, tile_x, tile_y, cam_x, cam_y,
                             stone->base, stone->shade);
        /* Draw dark rectangle in center for the hole */
        int sx, sy;
        iso_to_screen(tile_x, tile_y, &sx, &sy);
        int cx = sx - cam_x + r->camera_offset_x;
        int cy = sy - cam_y + TILE_HALF_H;
        SDL_SetRenderDrawColor(r->sdl_renderer, 5, 3, 8, 255);
        SDL_Rect hole = { cx - 8, cy - 4, 16, 8 };
        SDL_RenderFillRect(r->sdl_renderer, &hole);
        /* Hole border */
        SDL_SetRenderDrawColor(r->sdl_renderer, 30, 20, 35, 255);
        SDL_RenderDrawRect(r->sdl_renderer, &hole);
        break;
    }
    case TILE_WATER: {
        /* Animated shimmer using SDL_GetTicks */
        Uint32 ticks = SDL_GetTicks();
        float phase = (float)ticks / 1000.0f +
                      (float)(tile_x * 7 + tile_y * 13);
        float shimmer = sinf(phase) * 0.15f + 1.0f;
        SDL_Color base = {
            (Uint8)CLAMP((int)(tc->base.r * shimmer), 0, 255),
            (Uint8)CLAMP((int)(tc->base.g * shimmer), 0, 255),
            (Uint8)CLAMP((int)(tc->base.b * shimmer), 0, 255),
            255
        };
        SDL_Color shade = {
            (Uint8)CLAMP((int)(tc->shade.r * shimmer), 0, 255),
            (Uint8)CLAMP((int)(tc->shade.g * shimmer), 0, 255),
            (Uint8)CLAMP((int)(tc->shade.b * shimmer), 0, 255),
            255
        };
        iso_draw_tile_filled(r, tile_x, tile_y, cam_x, cam_y, base, shade);
        break;
    }
    default:
        /* All other floor types: simple filled diamond */
        iso_draw_tile_filled(r, tile_x, tile_y, cam_x, cam_y,
                             tc->base, tc->shade);
        break;
    }
}

/*
 * Draw a simple humanoid character placeholder on a tile.
 */
void iso_draw_character(IsoRenderer *r, int tile_x, int tile_y,
                        int cam_x, int cam_y,
                        SDL_Color body_color, SDL_Color head_color)
{
    int sx, sy;
    iso_to_screen(tile_x, tile_y, &sx, &sy);

    int cx = sx - cam_x + r->camera_offset_x;
    int cy = sy - cam_y + TILE_HALF_H; /* center of tile */

    SDL_SetRenderDrawBlendMode(r->sdl_renderer, SDL_BLENDMODE_NONE);

    /* Body: rectangle */
    SDL_SetRenderDrawColor(r->sdl_renderer,
        body_color.r, body_color.g, body_color.b, 255);
    SDL_Rect body = { cx - 6, cy - 20, 12, 16 };
    SDL_RenderFillRect(r->sdl_renderer, &body);

    /* Shoulders */
    SDL_Rect shoulders = { cx - 8, cy - 18, 16, 4 };
    SDL_RenderFillRect(r->sdl_renderer, &shoulders);

    /* Head: small filled circle approximation */
    SDL_SetRenderDrawColor(r->sdl_renderer,
        head_color.r, head_color.g, head_color.b, 255);
    for (int dy = -4; dy <= 4; dy++) {
        int hw = (int)sqrtf(16.0f - (float)(dy * dy));
        SDL_RenderDrawLine(r->sdl_renderer,
                           cx - hw, cy - 24 + dy,
                           cx + hw, cy - 24 + dy);
    }

    /* Legs */
    SDL_SetRenderDrawColor(r->sdl_renderer,
        (Uint8)(body_color.r * 2 / 3),
        (Uint8)(body_color.g * 2 / 3),
        (Uint8)(body_color.b * 2 / 3), 255);
    SDL_Rect left_leg  = { cx - 5, cy - 4, 4, 6 };
    SDL_Rect right_leg = { cx + 1, cy - 4, 4, 6 };
    SDL_RenderFillRect(r->sdl_renderer, &left_leg);
    SDL_RenderFillRect(r->sdl_renderer, &right_leg);
}
