#include "game/player.h"
#include "world/map.h"

/* Default movement speed: tiles per second */
#define DEFAULT_MOVE_SPEED 6.0f

void player_init(Player *player, int start_x, int start_y)
{
    player->tile_x = start_x;
    player->tile_y = start_y;

    /* Compute initial world position from tile coords */
    int sx, sy;
    iso_to_screen(start_x, start_y, &sx, &sy);
    player->world_x = (float)sx;
    player->world_y = (float)sy;

    player->move_from_x = start_x;
    player->move_from_y = start_y;
    player->move_to_x   = start_x;
    player->move_to_y   = start_y;
    player->move_timer   = 0.0f;
    player->moving       = false;

    player->facing     = DIR_S;
    player->anim_state = ANIM_IDLE;
    player->move_speed = DEFAULT_MOVE_SPEED;

    path_clear(&player->path);
    stats_init(&player->stats);

    memset(&player->anim, 0, sizeof(player->anim));
}

/* Start moving toward the next waypoint on the path */
static bool begin_next_step(Player *player)
{
    int nx, ny;
    if (!path_next(&player->path, &nx, &ny)) {
        player->moving = false;
        return false;
    }
    player->move_from_x = player->tile_x;
    player->move_from_y = player->tile_y;
    player->move_to_x   = nx;
    player->move_to_y   = ny;
    player->move_timer   = 0.0f;
    player->moving       = true;

    player->facing = player_calc_direction(
        player->move_from_x, player->move_from_y,
        player->move_to_x, player->move_to_y);

    return true;
}

void player_update(Player *player, float dt, const struct TileMap *map)
{
    (void)map;  /* available for future collision checks */

    if (!player->moving) {
        player->anim_state = ANIM_IDLE;
        anim_controller_set_state(&player->anim, player->anim_state, player->facing);
        anim_controller_update(&player->anim, dt);
        return;
    }

    player->anim_state = ANIM_WALKING;
    player->move_timer += dt * player->move_speed;

    if (player->move_timer >= 1.0f) {
        /* Arrived at waypoint — carry over leftover time */
        float leftover = player->move_timer - 1.0f;

        player->tile_x = player->move_to_x;
        player->tile_y = player->move_to_y;

        /* Try next step */
        if (!begin_next_step(player)) {
            int sx, sy;
            iso_to_screen(player->tile_x, player->tile_y, &sx, &sy);
            player->world_x = (float)sx;
            player->world_y = (float)sy;
            player->moving = false;
            player->anim_state = ANIM_IDLE;
        } else {
            /* Apply leftover for smooth multi-step movement */
            player->move_timer = leftover;
        }
    } else {
        /* Interpolate between tiles */
        int from_sx, from_sy, to_sx, to_sy;
        iso_to_screen(player->move_from_x, player->move_from_y, &from_sx, &from_sy);
        iso_to_screen(player->move_to_x, player->move_to_y, &to_sx, &to_sy);
        float t = player->move_timer;
        player->world_x = from_sx + (to_sx - from_sx) * t;
        player->world_y = from_sy + (to_sy - from_sy) * t;
    }

    anim_controller_set_state(&player->anim, player->anim_state, player->facing);
    anim_controller_update(&player->anim, dt);
}

void player_move_to(Player *player, const struct TileMap *map,
                    int target_x, int target_y)
{
    /* If mid-move, snap to nearest tile to avoid visual pop */
    if (player->moving) {
        if (player->move_timer >= 0.5f) {
            player->tile_x = player->move_to_x;
            player->tile_y = player->move_to_y;
        } else {
            player->tile_x = player->move_from_x;
            player->tile_y = player->move_from_y;
        }
        int sx, sy;
        iso_to_screen(player->tile_x, player->tile_y, &sx, &sy);
        player->world_x = (float)sx;
        player->world_y = (float)sy;
        player->moving = false;
    }

    if (!pathfind(map, player->tile_x, player->tile_y,
                  target_x, target_y, &player->path))
        return;

    /* Skip first point if it's our current position */
    if (player->path.length > 0 &&
        player->path.points[0].x == player->tile_x &&
        player->path.points[0].y == player->tile_y) {
        player->path.current_step = 1;
    }

    player->moving = false;
    player->move_timer = 0.0f;
    begin_next_step(player);
}

void player_get_screen_pos(const Player *player, int *sx, int *sy)
{
    *sx = (int)player->world_x;
    *sy = (int)player->world_y;
}

Direction player_calc_direction(int from_x, int from_y, int to_x, int to_y)
{
    int dx = to_x - from_x;
    int dy = to_y - from_y;

    if (dx == 0 && dy == 0)
        return DIR_S;

    /*
     * Map dx,dy to 8 isometric directions.
     * In iso coordinates: +x is SE, +y is SW, -x is NW, -y is NE.
     */
    if (dx > 0 && dy == 0) return DIR_SE;
    if (dx < 0 && dy == 0) return DIR_NW;
    if (dx == 0 && dy > 0) return DIR_SW;
    if (dx == 0 && dy < 0) return DIR_NE;
    if (dx > 0 && dy > 0)  return DIR_S;
    if (dx > 0 && dy < 0)  return DIR_E;
    if (dx < 0 && dy > 0)  return DIR_W;
    /* dx < 0 && dy < 0 */
    return DIR_N;
}
