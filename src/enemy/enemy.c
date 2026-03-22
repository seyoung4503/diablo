#include "enemy/enemy.h"
#include "enemy/enemy_ai.h"
#include "game/player.h"
#include "world/map.h"

void enemy_manager_init(EnemyManager *mgr)
{
    memset(mgr->enemies, 0, sizeof(mgr->enemies));
    mgr->count = 0;
}

void enemy_manager_update(EnemyManager *mgr, float dt, const struct TileMap *map)
{
    for (int i = 0; i < mgr->count; i++) {
        Enemy *e = &mgr->enemies[i];
        if (!e->alive)
            continue;

        /* Movement interpolation (same pattern as player) */
        if (e->moving) {
            e->move_timer += dt * e->move_speed;

            if (e->move_timer >= 1.0f) {
                float leftover = e->move_timer - 1.0f;

                e->tile_x = e->move_to_x;
                e->tile_y = e->move_to_y;

                /* Try next path step */
                int nx, ny;
                if (path_next(&e->path, &nx, &ny) &&
                    tilemap_is_walkable(map, nx, ny)) {
                    e->move_from_x = e->tile_x;
                    e->move_from_y = e->tile_y;
                    e->move_to_x = nx;
                    e->move_to_y = ny;
                    e->move_timer = leftover;
                    e->facing = player_calc_direction(
                        e->move_from_x, e->move_from_y,
                        e->move_to_x, e->move_to_y);
                } else {
                    int sx, sy;
                    iso_to_screen(e->tile_x, e->tile_y, &sx, &sy);
                    e->world_x = (float)sx;
                    e->world_y = (float)sy;
                    e->moving = false;
                }
            } else {
                /* Interpolate between tiles */
                int from_sx, from_sy, to_sx, to_sy;
                iso_to_screen(e->move_from_x, e->move_from_y,
                              &from_sx, &from_sy);
                iso_to_screen(e->move_to_x, e->move_to_y,
                              &to_sx, &to_sy);
                float t = e->move_timer;
                e->world_x = from_sx + (to_sx - from_sx) * t;
                e->world_y = from_sy + (to_sy - from_sy) * t;
            }
        }
    }
}

void enemy_spawn(EnemyManager *mgr, EnemyType type, int x, int y)
{
    if (mgr->count >= MAX_ENEMIES_PER_LEVEL)
        return;

    Enemy *e = &mgr->enemies[mgr->count];
    memset(e, 0, sizeof(Enemy));

    e->id = mgr->count;
    e->type = type;
    e->state = ENEMY_STATE_IDLE;
    e->tile_x = x;
    e->tile_y = y;
    e->move_from_x = x;
    e->move_from_y = y;
    e->move_to_x = x;
    e->move_to_y = y;
    e->facing = DIR_S;
    e->alive = true;

    int sx, sy;
    iso_to_screen(x, y, &sx, &sy);
    e->world_x = (float)sx;
    e->world_y = (float)sy;

    path_clear(&e->path);

    /* Apply stats from enemy type template */
    mgr->count++;
    enemy_apply_template(e);
}

Enemy *enemy_at_tile(EnemyManager *mgr, int x, int y)
{
    for (int i = 0; i < mgr->count; i++) {
        Enemy *e = &mgr->enemies[i];
        if (e->alive && e->tile_x == x && e->tile_y == y)
            return e;
    }
    return NULL;
}

void enemy_take_damage(Enemy *enemy, int damage)
{
    if (!enemy || !enemy->alive)
        return;

    enemy->current_hp -= damage;
    if (enemy->current_hp <= 0) {
        enemy->current_hp = 0;
        enemy->alive = false;
        enemy->state = ENEMY_STATE_DEAD;
        enemy->moving = false;
    }
}

int enemy_count_alive(const EnemyManager *mgr)
{
    int count = 0;
    for (int i = 0; i < mgr->count; i++) {
        if (mgr->enemies[i].alive)
            count++;
    }
    return count;
}
