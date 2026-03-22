#ifndef DIABLO_WORLD_DUNGEON_H
#define DIABLO_WORLD_DUNGEON_H

#include "common.h"
#include "world/map.h"
#include "enemy/enemy.h"

#define MAX_DUNGEON_LEVELS 16
#define DUNGEON_WIDTH      40
#define DUNGEON_HEIGHT     40
#define MAX_BSP_NODES      128
#define MIN_ROOM_SIZE      5
#define MAX_ROOM_SIZE      12
#define MAX_ROOMS          32

/* Dungeon themes */
#define THEME_CATHEDRAL  0
#define THEME_CATACOMBS  1
#define THEME_CAVES      2
#define THEME_HELL       3
#define THEME_COUNT      4

typedef struct Room {
    int x, y, w, h;
} Room;

typedef struct BSPNode {
    int x, y, w, h;        /* area bounds */
    Room room;              /* room within this area */
    bool has_room;
    int left, right;        /* child indices (-1 = leaf) */
} BSPNode;

typedef struct DungeonLevel {
    TileMap map;
    int level_num;          /* 1-16 */
    int stairs_up_x, stairs_up_y;
    int stairs_down_x, stairs_down_y;
    Room rooms[MAX_ROOMS];
    int room_count;
    int theme;              /* 0=cathedral, 1=catacombs, 2=caves, 3=hell */
} DungeonLevel;

typedef struct Dungeon {
    DungeonLevel levels[MAX_DUNGEON_LEVELS];
    int current_level;      /* 0-15 */
} Dungeon;

typedef struct DungeonTheme {
    int theme_id;
    const char *name;
    EnemyType enemy_types[3];   /* which enemies appear */
    int enemy_count_min;        /* per room */
    int enemy_count_max;
    int enemy_level_bonus;      /* added to base stats */
} DungeonTheme;

/* Dungeon generation */
void dungeon_init(Dungeon *dungeon);
void dungeon_generate_level(DungeonLevel *level, int level_num, unsigned int seed);
void dungeon_get_theme(int level_num, int *theme);

/* Theme system */
void dungeon_theme_init(void);
const DungeonTheme *dungeon_theme_get(int theme_id);
void dungeon_populate_enemies(DungeonLevel *level, EnemyManager *mgr);

/* Tile definitions */
const char *tile_get_texture_path(TileType type, int theme);

#endif /* DIABLO_WORLD_DUNGEON_H */
