#include "npc/npc.h"
#include "npc/npc_schedule.h"
#include "world/town.h"
#include "world/pathfinding.h"

/* ------------------------------------------------------------------ */
/*  Individual NPC                                                     */
/* ------------------------------------------------------------------ */

void npc_init(NPC *npc, int id, const char *name, const char *title,
              int start_x, int start_y)
{
    memset(npc, 0, sizeof(*npc));
    npc->id = id;
    strncpy(npc->name, name, MAX_NPC_NAME - 1);
    strncpy(npc->title, title, MAX_NPC_TITLE - 1);

    npc->tile_x = start_x;
    npc->tile_y = start_y;

    /* World position matches tile center */
    npc->world_x = (float)start_x;
    npc->world_y = (float)start_y;

    npc->facing = DIR_S;
    npc->move_speed = 2.5f;  /* default, overridden by npc_defs */

    npc->mood = 0.0f;
    npc->stress = 0.0f;
    npc->trust_player = 0.0f;

    npc->current_action = NPC_ACTION_IDLE;
    npc->is_alive = true;
    npc->has_left_town = false;

    path_clear(&npc->path);
}

/* Compute facing direction from dx, dy deltas */
static Direction direction_from_delta(int dx, int dy)
{
    if (dx == 0 && dy > 0)  return DIR_S;
    if (dx < 0 && dy > 0)   return DIR_SW;
    if (dx < 0 && dy == 0)  return DIR_W;
    if (dx < 0 && dy < 0)   return DIR_NW;
    if (dx == 0 && dy < 0)  return DIR_N;
    if (dx > 0 && dy < 0)   return DIR_NE;
    if (dx > 0 && dy == 0)  return DIR_E;
    if (dx > 0 && dy > 0)   return DIR_SE;
    return DIR_S;
}

void npc_update(NPC *npc, float dt, const struct TileMap *map)
{
    (void)map;

    if (!npc->is_alive || npc->has_left_town)
        return;

    if (!npc->moving)
        return;

    /* Advance movement timer */
    float step_duration = 1.0f / npc->move_speed;
    npc->move_timer += dt;

    if (npc->move_timer >= step_duration) {
        /* Arrived at next tile */
        npc->move_timer = 0.0f;
        npc->tile_x = npc->move_to_x;
        npc->tile_y = npc->move_to_y;
        npc->world_x = (float)npc->tile_x;
        npc->world_y = (float)npc->tile_y;

        /* Try to advance to next path point */
        int next_x, next_y;
        if (path_next(&npc->path, &next_x, &next_y)) {
            npc->move_from_x = npc->tile_x;
            npc->move_from_y = npc->tile_y;
            npc->move_to_x = next_x;
            npc->move_to_y = next_y;

            int dx = next_x - npc->tile_x;
            int dy = next_y - npc->tile_y;
            npc->facing = direction_from_delta(dx, dy);
        } else {
            /* Path complete */
            npc->moving = false;
        }
    } else {
        /* Interpolate position between tiles */
        float t = npc->move_timer / step_duration;
        npc->world_x = (float)npc->move_from_x +
                        t * (float)(npc->move_to_x - npc->move_from_x);
        npc->world_y = (float)npc->move_from_y +
                        t * (float)(npc->move_to_y - npc->move_from_y);
    }
}

void npc_move_to_tile(NPC *npc, const struct TileMap *map,
                      int target_x, int target_y)
{
    /* Already there */
    if (npc->tile_x == target_x && npc->tile_y == target_y) {
        npc->moving = false;
        return;
    }

    if (!pathfind(map, npc->tile_x, npc->tile_y, target_x, target_y,
                  &npc->path)) {
        npc->moving = false;
        return;
    }

    /* Start moving to first path point */
    int next_x, next_y;
    if (path_next(&npc->path, &next_x, &next_y)) {
        npc->moving = true;
        npc->move_timer = 0.0f;
        npc->move_from_x = npc->tile_x;
        npc->move_from_y = npc->tile_y;
        npc->move_to_x = next_x;
        npc->move_to_y = next_y;

        int dx = next_x - npc->tile_x;
        int dy = next_y - npc->tile_y;
        npc->facing = direction_from_delta(dx, dy);
    }
}

void npc_move_to_location(NPC *npc, const struct TileMap *map,
                          const struct Town *town, NPCLocation loc)
{
    const LocationInfo *info = town_get_location(town, (TownLocation)loc);
    if (info) {
        npc_move_to_tile(npc, map, info->center_x, info->center_y);
    }
}

/* ------------------------------------------------------------------ */
/*  NPC Manager                                                        */
/* ------------------------------------------------------------------ */

void npc_manager_init(NPCManager *mgr)
{
    memset(mgr, 0, sizeof(*mgr));
}

void npc_manager_update(NPCManager *mgr, float dt, int game_hour,
                        const struct TileMap *map)
{
    for (int i = 0; i < mgr->count; i++) {
        NPC *npc = &mgr->npcs[i];

        if (!npc->is_alive || npc->has_left_town)
            continue;

        /* Check schedule: if not already moving, direct NPC to scheduled location */
        if (!npc->moving) {
            const ScheduleEntry *entry =
                npc_schedule_current(npc->schedule_id, game_hour);
            if (entry) {
                npc->current_action = entry->action;

                /* If NPC is not at scheduled location, move there */
                if (npc->current_location != entry->location) {
                    npc_move_to_tile(npc, map, 0, 0);
                    /* We'll need town pointer for proper location lookup;
                       for now, the schedule drives action state. Movement to
                       specific locations is triggered by npc_defs or game logic
                       calling npc_move_to_location(). */
                }
            }
        }

        npc_update(npc, dt, map);
    }
}

NPC *npc_manager_get(NPCManager *mgr, int id)
{
    for (int i = 0; i < mgr->count; i++) {
        if (mgr->npcs[i].id == id)
            return &mgr->npcs[i];
    }
    return NULL;
}

NPC *npc_manager_get_by_name(NPCManager *mgr, const char *name)
{
    for (int i = 0; i < mgr->count; i++) {
        if (strcmp(mgr->npcs[i].name, name) == 0)
            return &mgr->npcs[i];
    }
    return NULL;
}

NPC *npc_manager_at_tile(NPCManager *mgr, int x, int y)
{
    for (int i = 0; i < mgr->count; i++) {
        NPC *npc = &mgr->npcs[i];
        if (npc->is_alive && !npc->has_left_town &&
            npc->tile_x == x && npc->tile_y == y) {
            return npc;
        }
    }
    return NULL;
}
