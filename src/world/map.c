#include "world/map.h"

void tilemap_init(TileMap *map, int width, int height)
{
    map->width = MIN(width, MAP_WIDTH);
    map->height = MIN(height, MAP_HEIGHT);
    memset(map->tiles, 0, sizeof(map->tiles));
}

void tilemap_set_tile(TileMap *map, int x, int y, TileType type)
{
    if (!tilemap_in_bounds(map, x, y))
        return;

    Tile *t = &map->tiles[y][x];
    t->type = type;

    switch (type) {
    case TILE_GRASS:
    case TILE_DIRT:
    case TILE_STONE_FLOOR:
    case TILE_DOOR:
    case TILE_STAIRS_UP:
    case TILE_STAIRS_DOWN:
        t->walkable = true;
        t->blocks_sight = false;
        break;
    case TILE_WALL:
    case TILE_TREE:
        t->walkable = false;
        t->blocks_sight = true;
        break;
    case TILE_WATER:
        t->walkable = false;
        t->blocks_sight = false;
        break;
    case TILE_NONE:
    default:
        t->walkable = false;
        t->blocks_sight = false;
        break;
    }
}

Tile *tilemap_get_tile(TileMap *map, int x, int y)
{
    if (!tilemap_in_bounds(map, x, y))
        return NULL;
    return &map->tiles[y][x];
}

bool tilemap_is_walkable(const TileMap *map, int x, int y)
{
    if (!tilemap_in_bounds(map, x, y))
        return false;
    return map->tiles[y][x].walkable;
}

bool tilemap_in_bounds(const TileMap *map, int x, int y)
{
    return x >= 0 && x < map->width && y >= 0 && y < map->height;
}

TileType tilemap_get_type(const TileMap *map, int x, int y)
{
    if (!tilemap_in_bounds(map, x, y))
        return TILE_NONE;
    return map->tiles[y][x].type;
}
