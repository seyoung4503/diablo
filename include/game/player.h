#ifndef DIABLO_GAME_PLAYER_H
#define DIABLO_GAME_PLAYER_H

#include "common.h"
#include "game/stats.h"
#include "world/pathfinding.h"

/* Forward declare TileMap to avoid circular includes */
struct TileMap;

typedef enum {
    ANIM_IDLE,
    ANIM_WALKING,
    ANIM_ATTACKING,
    ANIM_HIT,
    ANIM_DEATH
} AnimState;

typedef struct Player {
    /* Position */
    int tile_x, tile_y;             /* current tile (grid) */
    float world_x, world_y;        /* interpolated pixel position */
    int move_from_x, move_from_y;  /* tile moving from */
    int move_to_x, move_to_y;      /* tile moving to */
    float move_timer;               /* 0-1 interpolation progress */
    bool moving;

    /* Appearance */
    Direction facing;
    AnimState anim_state;

    /* Pathfinding */
    Path path;

    /* Stats */
    PlayerStats stats;

    /* Movement speed (tiles per second) */
    float move_speed;
} Player;

/* Initialize player at starting tile position */
void player_init(Player *player, int start_x, int start_y);

/* Update player movement and animation each frame */
void player_update(Player *player, float dt, const struct TileMap *map);

/* Pathfind and start moving to target tile */
void player_move_to(Player *player, const struct TileMap *map,
                    int target_x, int target_y);

/* Get screen position for rendering (top-left of sprite) */
void player_get_screen_pos(const Player *player, int *sx, int *sy);

/* Calculate facing direction from movement vector */
Direction player_calc_direction(int from_x, int from_y, int to_x, int to_y);

#endif /* DIABLO_GAME_PLAYER_H */
