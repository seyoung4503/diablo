#ifndef DIABLO_WORLD_TOWN_H
#define DIABLO_WORLD_TOWN_H

#include "world/map.h"

/* Named locations in Tristram */
typedef enum {
    LOC_TOWN_SQUARE,
    LOC_BLACKSMITH,
    LOC_HEALER,
    LOC_TAVERN,
    LOC_CATHEDRAL,
    LOC_WELL,
    LOC_GRAVEYARD,
    LOC_ADRIA_HUT,
    LOC_HOME_TREMAIN,
    LOC_HOME_CAIN,
    LOC_WIRT_CORNER,
    LOC_COUNT
} TownLocation;

typedef struct LocationInfo {
    TownLocation id;
    const char *name;
    int center_x, center_y;
} LocationInfo;

typedef struct Town {
    TileMap map;
    LocationInfo locations[LOC_COUNT];
    int player_start_x, player_start_y;
} Town;

void town_init(Town *town);
const LocationInfo *town_get_location(const Town *town, TownLocation loc);

#endif /* DIABLO_WORLD_TOWN_H */
