#include "world/dungeon.h"

/* Theme-specific texture path prefixes */
static const char *theme_prefixes[THEME_COUNT] = {
    [THEME_CATHEDRAL] = "assets/tiles/cathedral",
    [THEME_CATACOMBS] = "assets/tiles/catacombs",
    [THEME_CAVES]     = "assets/tiles/caves",
    [THEME_HELL]      = "assets/tiles/hell",
};

/* Base texture names per tile type */
static const char *tile_base_names[TILE_TYPE_COUNT] = {
    [TILE_NONE]        = "none",
    [TILE_GRASS]       = "grass",
    [TILE_DIRT]        = "dirt",
    [TILE_STONE_FLOOR] = "stone_floor",
    [TILE_WALL]        = "wall",
    [TILE_WATER]       = "water",
    [TILE_DOOR]        = "door",
    [TILE_STAIRS_UP]   = "stairs_up",
    [TILE_STAIRS_DOWN] = "stairs_down",
    [TILE_TREE]        = "tree",
};

/* Full path buffer (static, not thread-safe — fine for single-threaded game) */
static char path_buf[256];

const char *tile_get_texture_path(TileType type, int theme)
{
    if (type < 0 || type >= TILE_TYPE_COUNT)
        return "assets/tiles/none.png";

    const char *base = tile_base_names[type];
    if (!base) return "assets/tiles/none.png";

    /* Tiles that vary by dungeon theme */
    if (theme >= 0 && theme < THEME_COUNT &&
        (type == TILE_STONE_FLOOR || type == TILE_WALL ||
         type == TILE_DOOR || type == TILE_STAIRS_UP || type == TILE_STAIRS_DOWN)) {
        snprintf(path_buf, sizeof(path_buf), "%s/%s.png",
                 theme_prefixes[theme], base);
    } else {
        /* Generic / non-themed tiles */
        snprintf(path_buf, sizeof(path_buf), "assets/tiles/%s.png", base);
    }

    return path_buf;
}
