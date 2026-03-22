#ifndef DIABLO_WORLD_MAP_H
#define DIABLO_WORLD_MAP_H

#include "common.h"

/* Tile types */
typedef enum {
    TILE_NONE = 0,
    TILE_GRASS,
    TILE_DIRT,
    TILE_STONE_FLOOR,
    TILE_WALL,
    TILE_WATER,
    TILE_DOOR,
    TILE_STAIRS_DOWN,
    TILE_TREE,
    TILE_TYPE_COUNT
} TileType;

/* Tile properties */
typedef struct Tile {
    TileType type;
    bool walkable;
    bool blocks_sight;
} Tile;

/* Tile map */
typedef struct TileMap {
    Tile tiles[MAP_HEIGHT][MAP_WIDTH];
    int width, height;
} TileMap;

void tilemap_init(TileMap *map, int width, int height);
void tilemap_set_tile(TileMap *map, int x, int y, TileType type);
Tile *tilemap_get_tile(TileMap *map, int x, int y);
bool tilemap_is_walkable(const TileMap *map, int x, int y);
bool tilemap_in_bounds(const TileMap *map, int x, int y);
TileType tilemap_get_type(const TileMap *map, int x, int y);

#endif /* DIABLO_WORLD_MAP_H */
