#include "world/town.h"

/* ---- Helper functions for building the town layout ---- */

static void fill_rect(TileMap *map, int x0, int y0, int w, int h, TileType type)
{
    for (int y = y0; y < y0 + h; y++)
        for (int x = x0; x < x0 + w; x++)
            tilemap_set_tile(map, x, y, type);
}

/* Draw a hollow rectangle of walls with a stone floor interior */
static void build_room(TileMap *map, int x0, int y0, int w, int h)
{
    /* Walls around perimeter */
    fill_rect(map, x0, y0, w, h, TILE_WALL);
    /* Stone floor interior */
    if (w > 2 && h > 2)
        fill_rect(map, x0 + 1, y0 + 1, w - 2, h - 2, TILE_STONE_FLOOR);
}

/* Place a door tile at a specific position */
static void place_door(TileMap *map, int x, int y)
{
    tilemap_set_tile(map, x, y, TILE_DOOR);
}

/* Draw a dirt path between two points (L-shaped) */
static void draw_path(TileMap *map, int x0, int y0, int x1, int y1)
{
    /* Horizontal segment */
    int sx = (x0 < x1) ? x0 : x1;
    int ex = (x0 < x1) ? x1 : x0;
    for (int x = sx; x <= ex; x++) {
        if (tilemap_get_type(map, x, y0) == TILE_GRASS)
            tilemap_set_tile(map, x, y0, TILE_DIRT);
    }
    /* Vertical segment */
    int sy = (y0 < y1) ? y0 : y1;
    int ey = (y0 < y1) ? y1 : y0;
    for (int y = sy; y <= ey; y++) {
        if (tilemap_get_type(map, x1, y) == TILE_GRASS)
            tilemap_set_tile(map, x1, y, TILE_DIRT);
    }
}

/* Widen a path by adding dirt to adjacent grass tiles */
static void widen_path(TileMap *map, int x0, int y0, int x1, int y1)
{
    /* Horizontal segment - add row above and below */
    int sx = (x0 < x1) ? x0 : x1;
    int ex = (x0 < x1) ? x1 : x0;
    for (int x = sx; x <= ex; x++) {
        if (tilemap_get_type(map, x, y0 - 1) == TILE_GRASS)
            tilemap_set_tile(map, x, y0 - 1, TILE_DIRT);
        if (tilemap_get_type(map, x, y0 + 1) == TILE_GRASS)
            tilemap_set_tile(map, x, y0 + 1, TILE_DIRT);
    }
    /* Vertical segment - add column left and right */
    int sy = (y0 < y1) ? y0 : y1;
    int ey = (y0 < y1) ? y1 : y0;
    for (int y = sy; y <= ey; y++) {
        if (tilemap_get_type(map, x1 - 1, y) == TILE_GRASS)
            tilemap_set_tile(map, x1 - 1, y, TILE_DIRT);
        if (tilemap_get_type(map, x1 + 1, y) == TILE_GRASS)
            tilemap_set_tile(map, x1 + 1, y, TILE_DIRT);
    }
}

/* ---- Location definitions ---- */

static void init_locations(Town *town)
{
    LocationInfo locs[LOC_COUNT] = {
        { LOC_TOWN_SQUARE, "Town Square",       19, 14 },
        { LOC_BLACKSMITH,  "Griswold's Smithy",  8, 14 },
        { LOC_HEALER,      "Pepin's Clinic",     30, 14 },
        { LOC_TAVERN,      "Ogden's Tavern",     19,  8 },
        { LOC_CATHEDRAL,   "Tristram Cathedral", 32,  6 },
        { LOC_WELL,        "Town Well",          19, 16 },
        { LOC_GRAVEYARD,   "Graveyard",          20, 29 },
        { LOC_ADRIA_HUT,   "Adria's Hut",        33, 22 },
        { LOC_HOME_TREMAIN,"Tremain's Farm",     26, 21 },
        { LOC_HOME_CAIN,   "Cain's House",       13, 21 },
        { LOC_WIRT_CORNER, "Wirt's Corner",       6, 29 },
    };
    memcpy(town->locations, locs, sizeof(locs));
}

/* ---- Town layout generation ---- */

void town_init(Town *town)
{
    TileMap *map = &town->map;
    tilemap_init(map, MAP_WIDTH, MAP_HEIGHT);

    /* Step 1: Fill everything with grass */
    fill_rect(map, 0, 0, MAP_WIDTH, MAP_HEIGHT, TILE_GRASS);

    /* Step 2: Natural borders - trees and water along edges */

    /* Northern tree line (rows 0-3) */
    fill_rect(map, 0, 0, MAP_WIDTH, 4, TILE_TREE);
    /* Southern tree line (rows 36-39) */
    fill_rect(map, 0, 36, MAP_WIDTH, 4, TILE_TREE);
    /* Western tree line (cols 0-2) */
    fill_rect(map, 0, 0, 3, MAP_HEIGHT, TILE_TREE);
    /* Eastern tree line (cols 37-39) */
    fill_rect(map, 37, 0, 3, MAP_HEIGHT, TILE_TREE);

    /* Small pond in the northwest */
    fill_rect(map, 4, 4, 4, 3, TILE_WATER);
    tilemap_set_tile(map, 5, 7, TILE_WATER);
    tilemap_set_tile(map, 6, 7, TILE_WATER);

    /* Scattered trees for organic feel */
    tilemap_set_tile(map, 10, 5, TILE_TREE);
    tilemap_set_tile(map, 11, 4, TILE_TREE);
    tilemap_set_tile(map, 14, 5, TILE_TREE);
    tilemap_set_tile(map, 25, 5, TILE_TREE);
    tilemap_set_tile(map, 36, 8, TILE_TREE);
    tilemap_set_tile(map, 36, 12, TILE_TREE);
    tilemap_set_tile(map, 4, 33, TILE_TREE);
    tilemap_set_tile(map, 5, 34, TILE_TREE);
    tilemap_set_tile(map, 35, 33, TILE_TREE);
    tilemap_set_tile(map, 34, 34, TILE_TREE);
    tilemap_set_tile(map, 12, 33, TILE_TREE);
    tilemap_set_tile(map, 28, 34, TILE_TREE);
    tilemap_set_tile(map, 4, 18, TILE_TREE);
    tilemap_set_tile(map, 3, 25, TILE_TREE);
    tilemap_set_tile(map, 36, 25, TILE_TREE);
    tilemap_set_tile(map, 35, 19, TILE_TREE);

    /* Step 3: Buildings */

    /* Cathedral (northeast, 7x6) - the largest structure */
    build_room(map, 29, 4, 7, 6);
    /* Stairs down to dungeon inside cathedral */
    tilemap_set_tile(map, 32, 7, TILE_STAIRS_DOWN);
    /* Cathedral entrance (south wall) */
    place_door(map, 32, 9);

    /* Ogden's Tavern (north-center, 6x5) */
    build_room(map, 17, 6, 6, 5);
    /* Tavern entrance (south wall) */
    place_door(map, 19, 10);

    /* Griswold's Blacksmith (west, 5x5) */
    build_room(map, 6, 12, 5, 5);
    /* Blacksmith entrance (east wall) */
    place_door(map, 10, 14);

    /* Pepin's Healing (east, 5x5) */
    build_room(map, 28, 12, 5, 5);
    /* Healer entrance (west wall) */
    place_door(map, 28, 14);

    /* Cain's House (center-west, 4x4) */
    build_room(map, 11, 19, 4, 5);
    /* Cain entrance (east wall) */
    place_door(map, 14, 21);

    /* Tremain's Farm (center-east, 5x4) */
    build_room(map, 24, 19, 5, 5);
    /* Tremain entrance (west wall) */
    place_door(map, 24, 21);

    /* Adria's Hut (far east, 3x3 small hut) */
    build_room(map, 32, 21, 3, 3);
    /* Adria entrance (west wall) */
    place_door(map, 32, 22);

    /* Step 4: Town square - open dirt area in center */
    fill_rect(map, 16, 12, 8, 7, TILE_DIRT);

    /* Town well (stone floor around the well marker) */
    fill_rect(map, 18, 15, 3, 3, TILE_STONE_FLOOR);

    /* Step 5: Paths connecting all areas */

    /* Main east-west road through town center (row 14) */
    draw_path(map, 4, 14, 36, 14);
    widen_path(map, 4, 14, 36, 14);

    /* Main north-south road through center (col 19) */
    draw_path(map, 19, 5, 19, 35);
    widen_path(map, 19, 5, 19, 35);

    /* Path from tavern to town square */
    draw_path(map, 19, 10, 19, 12);

    /* Path from cathedral south to main road */
    draw_path(map, 32, 9, 32, 14);

    /* Path to Cain's house */
    draw_path(map, 14, 21, 16, 21);
    draw_path(map, 16, 14, 16, 21);

    /* Path to Tremain's farm */
    draw_path(map, 24, 21, 24, 14);

    /* Path to Adria's hut */
    draw_path(map, 32, 22, 32, 14);

    /* Path to Wirt's corner (southwest) */
    draw_path(map, 6, 29, 19, 29);
    draw_path(map, 6, 14, 6, 29);

    /* Path to graveyard (south-center) */
    draw_path(map, 19, 18, 19, 29);
    draw_path(map, 17, 29, 23, 29);

    /* Graveyard area - stone floor with wall border */
    fill_rect(map, 17, 27, 7, 5, TILE_STONE_FLOOR);
    /* Graveyard wall segments (partial, open entrance north) */
    fill_rect(map, 17, 27, 1, 5, TILE_WALL);
    fill_rect(map, 23, 27, 1, 5, TILE_WALL);
    fill_rect(map, 17, 31, 7, 1, TILE_WALL);
    /* Entrance at north side */
    tilemap_set_tile(map, 20, 27, TILE_DOOR);

    /* Wirt's corner - small open area with a few stones */
    fill_rect(map, 5, 28, 3, 3, TILE_DIRT);
    tilemap_set_tile(map, 5, 28, TILE_STONE_FLOOR);

    /* Step 6: Player start at town square center */
    town->player_start_x = 19;
    town->player_start_y = 14;

    /* Step 7: Initialize location data */
    init_locations(town);
}

const LocationInfo *town_get_location(const Town *town, TownLocation loc)
{
    if (loc < 0 || loc >= LOC_COUNT)
        return NULL;
    return &town->locations[loc];
}
