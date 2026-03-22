#include "enemy/enemy_ai.h"
#include "world/map.h"
#include "world/pathfinding.h"
#include "game/player.h"

/* Distance in tiles (Chebyshev / king-move distance) */
static int tile_distance(int ax, int ay, int bx, int by)
{
    int dx = abs(ax - bx);
    int dy = abs(ay - by);
    return MAX(dx, dy);
}

/* Start walking the next step on the enemy's path */
static void begin_enemy_step(Enemy *enemy, const struct TileMap *map)
{
    int nx, ny;
    if (!path_next(&enemy->path, &nx, &ny)) {
        enemy->moving = false;
        return;
    }

    if (!tilemap_is_walkable(map, nx, ny)) {
        enemy->moving = false;
        path_clear(&enemy->path);
        return;
    }

    enemy->move_from_x = enemy->tile_x;
    enemy->move_from_y = enemy->tile_y;
    enemy->move_to_x = nx;
    enemy->move_to_y = ny;
    enemy->move_timer = 0.0f;
    enemy->moving = true;
    enemy->facing = player_calc_direction(
        enemy->move_from_x, enemy->move_from_y,
        enemy->move_to_x, enemy->move_to_y);
}

/* Pathfind toward the player */
static void chase_player(Enemy *enemy, int player_x, int player_y,
                         const struct TileMap *map)
{
    if (enemy->moving)
        return;

    if (pathfind(map, enemy->tile_x, enemy->tile_y,
                 player_x, player_y, &enemy->path)) {
        /* Skip first point if it's our current position */
        if (enemy->path.length > 0 &&
            enemy->path.points[0].x == enemy->tile_x &&
            enemy->path.points[0].y == enemy->tile_y) {
            enemy->path.current_step = 1;
        }
        begin_enemy_step(enemy, map);
    }
}

/* Pathfind away from the player (flee) */
static void flee_from_player(Enemy *enemy, int player_x, int player_y,
                             const struct TileMap *map)
{
    if (enemy->moving)
        return;

    /* Pick a point in the opposite direction from the player */
    int dx = enemy->tile_x - player_x;
    int dy = enemy->tile_y - player_y;

    /* Normalize to a small offset and scale */
    int flee_x = enemy->tile_x + (dx > 0 ? 4 : (dx < 0 ? -4 : 0));
    int flee_y = enemy->tile_y + (dy > 0 ? 4 : (dy < 0 ? -4 : 0));

    /* Clamp to map bounds */
    flee_x = CLAMP(flee_x, 1, MAP_WIDTH - 2);
    flee_y = CLAMP(flee_y, 1, MAP_HEIGHT - 2);

    if (pathfind(map, enemy->tile_x, enemy->tile_y,
                 flee_x, flee_y, &enemy->path)) {
        if (enemy->path.length > 0 &&
            enemy->path.points[0].x == enemy->tile_x &&
            enemy->path.points[0].y == enemy->tile_y) {
            enemy->path.current_step = 1;
        }
        begin_enemy_step(enemy, map);
    }
}

/* Wander randomly (patrol) */
static void patrol_wander(Enemy *enemy, const struct TileMap *map)
{
    if (enemy->moving)
        return;

    /* Pick a random nearby tile to walk toward */
    int dx = (rand() % 5) - 2;  /* -2 to +2 */
    int dy = (rand() % 5) - 2;
    int target_x = enemy->tile_x + dx;
    int target_y = enemy->tile_y + dy;

    target_x = CLAMP(target_x, 1, MAP_WIDTH - 2);
    target_y = CLAMP(target_y, 1, MAP_HEIGHT - 2);

    if (tilemap_is_walkable(map, target_x, target_y)) {
        if (pathfind(map, enemy->tile_x, enemy->tile_y,
                     target_x, target_y, &enemy->path)) {
            if (enemy->path.length > 0 &&
                enemy->path.points[0].x == enemy->tile_x &&
                enemy->path.points[0].y == enemy->tile_y) {
                enemy->path.current_step = 1;
            }
            begin_enemy_step(enemy, map);
        }
    }
}

void enemy_ai_update(Enemy *enemy, float dt, int player_x, int player_y,
                     const struct TileMap *map)
{
    if (!enemy->alive || enemy->state == ENEMY_STATE_DEAD)
        return;

    int dist = tile_distance(enemy->tile_x, enemy->tile_y,
                             player_x, player_y);

    /* Tick attack cooldown */
    if (enemy->attack_timer > 0.0f)
        enemy->attack_timer -= dt;

    switch (enemy->state) {
    case ENEMY_STATE_IDLE:
        /* Hidden enemies only detect at very close range */
        if (enemy->type == ENEMY_HIDDEN) {
            if (dist <= 3)
                enemy->state = ENEMY_STATE_CHASE;
        } else if (dist <= enemy->detection_range) {
            enemy->state = ENEMY_STATE_CHASE;
        }
        break;

    case ENEMY_STATE_PATROL:
        if (dist <= enemy->detection_range) {
            enemy->state = ENEMY_STATE_CHASE;
            path_clear(&enemy->path);
            enemy->moving = false;
        } else {
            patrol_wander(enemy, map);
        }
        break;

    case ENEMY_STATE_CHASE:
        /* Scavenger flees when low HP */
        if (enemy->type == ENEMY_SCAVENGER &&
            enemy->max_hp > 0 &&
            enemy->current_hp * 100 / enemy->max_hp < 30) {
            enemy->state = ENEMY_STATE_FLEE;
            path_clear(&enemy->path);
            enemy->moving = false;
            break;
        }

        if (dist <= enemy->attack_range) {
            enemy->state = ENEMY_STATE_ATTACK;
            enemy->moving = false;
            path_clear(&enemy->path);
            /* Face the player */
            enemy->facing = player_calc_direction(
                enemy->tile_x, enemy->tile_y,
                player_x, player_y);
        } else if (dist > enemy->detection_range + 3) {
            /* Lost the player */
            enemy->state = ENEMY_STATE_IDLE;
            enemy->moving = false;
            path_clear(&enemy->path);
        } else {
            chase_player(enemy, player_x, player_y, map);
        }
        break;

    case ENEMY_STATE_ATTACK:
        /* Scavenger flees when low HP, even mid-attack */
        if (enemy->type == ENEMY_SCAVENGER &&
            enemy->max_hp > 0 &&
            enemy->current_hp * 100 / enemy->max_hp < 30) {
            enemy->state = ENEMY_STATE_FLEE;
            path_clear(&enemy->path);
            enemy->moving = false;
            break;
        }
        if (dist > enemy->attack_range) {
            enemy->state = ENEMY_STATE_CHASE;
            break;
        }
        /* Face the player */
        enemy->facing = player_calc_direction(
            enemy->tile_x, enemy->tile_y,
            player_x, player_y);
        /* Attack timer handled in combat_update */
        break;

    case ENEMY_STATE_FLEE:
        if (dist > enemy->detection_range + 5) {
            enemy->state = ENEMY_STATE_IDLE;
            enemy->moving = false;
            path_clear(&enemy->path);
        } else {
            flee_from_player(enemy, player_x, player_y, map);
        }
        break;

    case ENEMY_STATE_DEAD:
        break;
    }
}
