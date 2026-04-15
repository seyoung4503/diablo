#include "engine/debug_cmd.h"
#include "engine/debug.h"
#include "game/player.h"
#include "game/game.h"
#include "game/inventory.h"
#include "game/item.h"
#include "enemy/enemy.h"
#include "npc/npc.h"
#include "world/map.h"
#include "world/camera.h"
#include "engine/animation.h"
#include "engine/resource.h"

#include <stdarg.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <errno.h>

/* ---- Name tables ---- */

static const char *scene_names[] = {
    "TITLE", "TOWN", "DUNGEON", "DEATH", "PAUSED"
};
static const char *anim_state_names[] = {
    "IDLE", "WALKING", "ATTACKING", "HIT", "DEATH"
};
static const char *dir_names[] = {
    "S", "SW", "W", "NW", "N", "NE", "E", "SE"
};
static const char *enemy_type_names[] = {
    "NONE", "FALLEN", "SKELETON", "ZOMBIE", "SCAVENGER",
    "HIDDEN", "GOAT_MAN", "ACID_DOG", "MAGE", "KNIGHT", "BALROG"
};
static const char *enemy_state_names[] = {
    "IDLE", "PATROL", "CHASE", "ATTACK", "FLEE", "DEAD"
};
static const char *tile_type_names[] = {
    "NONE", "GRASS", "DIRT", "STONE", "WALL", "WATER",
    "DOOR", "STAIRS_UP", "STAIRS_DOWN", "TREE"
};
static const char *item_type_names[] = {
    "NONE", "WEAPON", "ARMOR", "HELM", "SHIELD", "RING", "AMULET",
    "HP_POTION", "MP_POTION", "SCROLL", "GOLD", "QUEST"
};

/* Direction deltas in tile space (matching iso_to_screen convention) */
static const int dir_dx[8] = {  1,  0, -1, -1, -1,  0,  1,  1 };
static const int dir_dy[8] = {  1,  1,  1,  0, -1, -1, -1,  0 };

static int tile_dist(int x1, int y1, int x2, int y2)
{
    int dx = abs(x1 - x2);
    int dy = abs(y1 - y2);
    return (dx > dy) ? dx : dy;
}

/* ---- Response buffer helpers ---- */

static int resp_len;
static char *resp_buf;

static void resp_init(DebugCmdAdapter *a)
{
    resp_buf = a->response_text;
    resp_len = 0;
    resp_buf[0] = '\0';
}

static void resp_append(const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    int avail = 8192 - resp_len;
    if (avail > 0) {
        int n = vsnprintf(resp_buf + resp_len, (size_t)avail, fmt, ap);
        if (n > 0) resp_len += (n < avail) ? n : avail - 1;
    }
    va_end(ap);
}

static void resp_player(const DebugCmdContext *ctx)
{
    const struct Player *p = ctx->player;
    resp_append("\n[PLAYER]\n");
    resp_append("pos=(%d,%d) hp=%d/%d mp=%d/%d lv=%d xp=%d/%d\n",
                p->tile_x, p->tile_y,
                p->stats.current_hp, p->stats.max_hp,
                p->stats.current_mana, p->stats.max_mana,
                p->stats.level, p->stats.xp, p->stats.xp_to_next);
    resp_append("facing=%s moving=%s anim=%s\n",
                (p->facing < DIR_COUNT) ? dir_names[p->facing] : "?",
                p->moving ? "yes" : "no",
                (p->anim_state >= 0 && p->anim_state < 5) ? anim_state_names[p->anim_state] : "?");
    resp_append("str=%d dex=%d mag=%d vit=%d\n",
                p->stats.strength, p->stats.dexterity,
                p->stats.magic, p->stats.vitality);

    /* Equipped weapon */
    const Item *weap = &ctx->inventory->equipped[0]; /* EQUIP_WEAPON */
    if (weap->id > 0)
        resp_append("weapon=%s (dmg %d-%d)\n", weap->name, weap->damage_min, weap->damage_max);
}

static void resp_scene(const DebugCmdContext *ctx)
{
    resp_append("\n[SCENE]\n");
    resp_append("type=%s day=%d hour=%d dungeon_level=%d\n",
                (ctx->scene_type >= 0 && ctx->scene_type < 5) ? scene_names[ctx->scene_type] : "?",
                ctx->game->game_day, ctx->game->game_hour,
                ctx->dungeon_level);
}

static void resp_enemies(const DebugCmdContext *ctx)
{
    const struct EnemyManager *em = ctx->enemies;
    int alive = 0;
    for (int i = 0; i < em->count; i++)
        if (em->enemies[i].alive) alive++;

    resp_append("\n[ENEMIES] %d alive\n", alive);

    /* Sort by distance for readability */
    int px = ctx->player->tile_x, py = ctx->player->tile_y;
    for (int i = 0; i < em->count; i++) {
        const Enemy *e = &em->enemies[i];
        if (!e->alive) continue;
        int d = tile_dist(px, py, e->tile_x, e->tile_y);
        resp_append("  %s at (%d,%d) hp=%d/%d dist=%d state=%s\n",
                    e->name, e->tile_x, e->tile_y,
                    e->current_hp, e->max_hp, d,
                    (e->state <= 5) ? enemy_state_names[e->state] : "?");
    }
}

static void resp_npcs(const DebugCmdContext *ctx)
{
    const struct NPCManager *nm = ctx->npcs;
    if (!nm || nm->count == 0) return;

    resp_append("\n[NPCS]\n");
    int px = ctx->player->tile_x, py = ctx->player->tile_y;
    for (int i = 0; i < nm->count; i++) {
        const NPC *n = &nm->npcs[i];
        if (!n->is_alive) continue;
        int d = tile_dist(px, py, n->tile_x, n->tile_y);
        resp_append("  %s (%s) at (%d,%d) dist=%d\n",
                    n->name, n->title, n->tile_x, n->tile_y, d);
    }
}

static void resp_inventory(const DebugCmdContext *ctx)
{
    const struct Inventory *inv = ctx->inventory;
    resp_append("\n[INVENTORY] gold=%d\n", inv->gold);

    /* Equipped */
    const char *slot_names[] = { "Weapon", "Shield", "Helm", "Armor", "Ring1", "Ring2", "Amulet" };
    for (int i = 0; i < EQUIP_SLOT_COUNT; i++) {
        if (inv->equipped[i].id > 0)
            resp_append("  [EQUIP %s] %s\n", slot_names[i], inv->equipped[i].name);
    }

    /* Bag */
    for (int i = 0; i < INVENTORY_SIZE; i++) {
        if (inv->slots[i].id > 0) {
            const Item *it = &inv->slots[i];
            if (it->type == ITEM_POTION_HP || it->type == ITEM_POTION_MANA)
                resp_append("  [%d] %s x%d\n", i, it->name, it->stack_count);
            else
                resp_append("  [%d] %s (%s)\n", i, it->name,
                            (it->type < ITEM_TYPE_COUNT) ? item_type_names[it->type] : "?");
        }
    }
}

static void resp_look(const DebugCmdContext *ctx)
{
    const struct Player *p = ctx->player;
    const struct TileMap *map = ctx->active_map;
    int px = p->tile_x, py = p->tile_y;

    resp_append("\n[SURROUNDINGS] from (%d,%d)\n", px, py);

    /* Adjacent tiles */
    resp_append("Adjacent:\n");
    for (int d = 0; d < 8; d++) {
        int tx = px + dir_dx[d];
        int ty = py + dir_dy[d];
        if (map && tilemap_in_bounds(map, tx, ty)) {
            TileType tt = tilemap_get_type(map, tx, ty);
            bool walk = tilemap_is_walkable(map, tx, ty);
            resp_append("  %s (%d,%d): %s%s\n", dir_names[d], tx, ty,
                        (tt < TILE_TYPE_COUNT) ? tile_type_names[tt] : "?",
                        walk ? "" : " [BLOCKED]");
        } else {
            resp_append("  %s: out of bounds\n", dir_names[d]);
        }
    }

    /* Standing on */
    if (map && tilemap_in_bounds(map, px, py)) {
        TileType here = tilemap_get_type(map, px, py);
        resp_append("Standing on: %s\n",
                    (here < TILE_TYPE_COUNT) ? tile_type_names[here] : "?");
    }

    /* Find stairs */
    if (map) {
        for (int ty = 0; ty < map->height; ty++) {
            for (int tx = 0; tx < map->width; tx++) {
                TileType tt = tilemap_get_type(map, tx, ty);
                if (tt == TILE_STAIRS_DOWN || tt == TILE_STAIRS_UP) {
                    int d = tile_dist(px, py, tx, ty);
                    resp_append("Stairs: %s at (%d,%d) dist=%d\n",
                                tt == TILE_STAIRS_DOWN ? "DOWN" : "UP", tx, ty, d);
                }
            }
        }
    }
}

/* ---- Direction parsing ---- */

static bool parse_dir(const char *s, Direction *dir)
{
    struct { const char *name; Direction d; } map[] = {
        {"s", DIR_S}, {"sw", DIR_SW}, {"w", DIR_W}, {"nw", DIR_NW},
        {"n", DIR_N}, {"ne", DIR_NE}, {"e", DIR_E}, {"se", DIR_SE},
    };
    for (int i = 0; i < 8; i++) {
        if (strcasecmp(s, map[i].name) == 0) {
            *dir = map[i].d;
            return true;
        }
    }
    return false;
}

/* ---- Command handlers ---- */

static void cmd_help(DebugCmdAdapter *a)
{
    (void)a;
    resp_append("[HELP]\n");
    resp_append("  move <dir>         - Step one tile (n/s/e/w/ne/nw/se/sw)\n");
    resp_append("  move_to <x> <y>    - Pathfind to tile\n");
    resp_append("  attack <x> <y>     - Melee attack enemy at tile\n");
    resp_append("  interact <x> <y>   - Talk to NPC at tile\n");
    resp_append("  wait [frames]      - Wait N frames (default 60)\n");
    resp_append("  screenshot         - Capture current frame\n");
    resp_append("  dump               - Full state dump\n");
    resp_append("  status             - Quick status\n");
    resp_append("  look               - Describe surroundings\n");
    resp_append("  inventory          - List inventory\n");
    resp_append("  use <slot>         - Use item from slot\n");
    resp_append("  equip <slot>       - Equip item from slot\n");
    resp_append("  spell <0-3>        - Select spell (0=none 1=fire 2=heal 3=lightning)\n");
    resp_append("  enter              - Go to stairs and enter\n");
    resp_append("  step [N]           - Advance N frames (default 1)\n");
    resp_append("  play               - Switch to real-time mode\n");
    resp_append("  pause              - Switch to step mode (paused)\n");
    resp_append("  help               - Show this help\n");
}

static void cmd_status(DebugCmdContext *ctx)
{
    resp_player(ctx);
    resp_scene(ctx);
    resp_enemies(ctx);
}

static void cmd_look(DebugCmdContext *ctx)
{
    resp_player(ctx);
    resp_scene(ctx);
    resp_look(ctx);
    resp_enemies(ctx);
    resp_npcs(ctx);
}

static void cmd_move(DebugCmdContext *ctx, const char *dir_str)
{
    Direction d;
    if (!parse_dir(dir_str, &d)) {
        resp_append("[ERROR] Invalid direction: %s\n", dir_str);
        return;
    }

    int tx = ctx->player->tile_x + dir_dx[d];
    int ty = ctx->player->tile_y + dir_dy[d];

    if (!ctx->active_map || !tilemap_in_bounds(ctx->active_map, tx, ty)) {
        resp_append("[ERROR] Out of bounds: (%d,%d)\n", tx, ty);
        return;
    }
    if (!tilemap_is_walkable(ctx->active_map, tx, ty)) {
        resp_append("[ERROR] Blocked: (%d,%d)\n", tx, ty);
        return;
    }

    player_move_to(ctx->player, ctx->active_map, tx, ty);
    resp_append("[OK] Moving %s to (%d,%d)\n", dir_names[d], tx, ty);
    resp_player(ctx);
}

static void cmd_move_to(DebugCmdContext *ctx, int x, int y)
{
    if (!ctx->active_map || !tilemap_in_bounds(ctx->active_map, x, y)) {
        resp_append("[ERROR] Out of bounds: (%d,%d)\n", x, y);
        return;
    }
    if (!tilemap_is_walkable(ctx->active_map, x, y)) {
        resp_append("[ERROR] Not walkable: (%d,%d)\n", x, y);
        return;
    }

    player_move_to(ctx->player, ctx->active_map, x, y);
    int dist = tile_dist(ctx->player->tile_x, ctx->player->tile_y, x, y);
    resp_append("[OK] Pathfinding to (%d,%d) dist=%d\n", x, y, dist);
    resp_player(ctx);
}

static void cmd_enter(DebugCmdContext *ctx, DebugCmdOutput *output)
{
    /* Find nearest stairs */
    if (!ctx->active_map) {
        resp_append("[ERROR] No active map\n");
        return;
    }

    int px = ctx->player->tile_x, py = ctx->player->tile_y;
    int best_x = -1, best_y = -1, best_d = 9999;
    TileType best_type = TILE_NONE;

    for (int ty = 0; ty < ctx->active_map->height; ty++) {
        for (int tx = 0; tx < ctx->active_map->width; tx++) {
            TileType tt = tilemap_get_type(ctx->active_map, tx, ty);
            if (tt == TILE_STAIRS_DOWN || tt == TILE_STAIRS_UP) {
                int d = tile_dist(px, py, tx, ty);
                if (d < best_d) {
                    best_d = d; best_x = tx; best_y = ty; best_type = tt;
                }
            }
        }
    }

    if (best_x < 0) {
        resp_append("[ERROR] No stairs found on this map\n");
        return;
    }

    if (best_d <= 1) {
        /* Already on/adjacent to stairs — walk onto them (game auto-transitions) */
        player_move_to(ctx->player, ctx->active_map, best_x, best_y);
        resp_append("[OK] Entering stairs at (%d,%d) type=%s\n",
                    best_x, best_y,
                    best_type == TILE_STAIRS_DOWN ? "DOWN" : "UP");
        if (best_type == TILE_STAIRS_DOWN)
            output->enter_stairs = true;
        else
            output->exit_dungeon = true;
    } else {
        /* Path to stairs first */
        player_move_to(ctx->player, ctx->active_map, best_x, best_y);
        resp_append("[OK] Walking to stairs at (%d,%d) dist=%d\n", best_x, best_y, best_d);
    }
    resp_player(ctx);
}

/* ---- Process a single command line ---- */

static void process_line(DebugCmdAdapter *adapter, const char *line,
                         DebugCmdContext *ctx, DebugCmdOutput *output)
{
    /* Skip empty lines */
    while (*line == ' ' || *line == '\t') line++;
    if (*line == '\0' || *line == '#') return;

    char cmd[32] = {0}, a1[32] = {0}, a2[32] = {0};
    sscanf(line, "%31s %31s %31s", cmd, a1, a2);

    resp_init(adapter);
    resp_append("[CMD] %s\n", line);

    if (strcasecmp(cmd, "help") == 0) {
        cmd_help(adapter);
    }
    else if (strcasecmp(cmd, "status") == 0) {
        resp_append("[OK]\n");
        cmd_status(ctx);
    }
    else if (strcasecmp(cmd, "look") == 0) {
        resp_append("[OK]\n");
        cmd_look(ctx);
    }
    else if (strcasecmp(cmd, "move") == 0) {
        cmd_move(ctx, a1);
    }
    else if (strcasecmp(cmd, "move_to") == 0) {
        int x = atoi(a1), y = atoi(a2);
        cmd_move_to(ctx, x, y);
    }
    else if (strcasecmp(cmd, "attack") == 0) {
        int x = atoi(a1), y = atoi(a2);
        output->attack_requested = true;
        output->attack_x = x;
        output->attack_y = y;
        resp_append("[OK] Attack requested at (%d,%d)\n", x, y);
        resp_player(ctx);
    }
    else if (strcasecmp(cmd, "interact") == 0) {
        int x = atoi(a1), y = atoi(a2);
        output->interact_requested = true;
        output->interact_x = x;
        output->interact_y = y;
        resp_append("[OK] Interact at (%d,%d)\n", x, y);
    }
    else if (strcasecmp(cmd, "wait") == 0) {
        int frames = a1[0] ? atoi(a1) : 60;
        if (frames < 1) frames = 1;
        if (frames > 3600) frames = 3600;
        if (adapter->step_frames >= 0) {
            /* Step mode: wait = step (advance N frames) */
            adapter->step_frames = frames;
            resp_append("[OK] Stepping %d frames...\n", frames);
        } else {
            /* Real-time mode: just wait */
            adapter->wait_frames = frames;
            resp_append("[OK] Waiting %d frames...\n", frames);
        }
        return; /* response after stepping/waiting finishes */
    }
    else if (strcasecmp(cmd, "screenshot") == 0) {
        output->screenshot_requested = true;
        resp_append("[OK] Screenshot requested\n");
    }
    else if (strcasecmp(cmd, "dump") == 0) {
        output->dump_requested = true;
        resp_append("[OK] State dump requested\n");
    }
    else if (strcasecmp(cmd, "inventory") == 0) {
        resp_append("[OK]\n");
        resp_player(ctx);
        resp_inventory(ctx);
    }
    else if (strcasecmp(cmd, "use") == 0) {
        int slot = atoi(a1);
        output->use_item_requested = true;
        output->use_item_slot = slot;
        resp_append("[OK] Use item at slot %d\n", slot);
        resp_inventory(ctx);
    }
    else if (strcasecmp(cmd, "equip") == 0) {
        int slot = atoi(a1);
        output->use_item_requested = true;
        output->use_item_slot = -(slot + 1); /* negative = equip */
        resp_append("[OK] Equip item at slot %d\n", slot);
    }
    else if (strcasecmp(cmd, "spell") == 0) {
        int id = atoi(a1);
        output->select_spell = true;
        output->spell_id = id;
        resp_append("[OK] Spell %d selected\n", id);
    }
    else if (strcasecmp(cmd, "enter") == 0) {
        cmd_enter(ctx, output);
    }
    else if (strcasecmp(cmd, "step") == 0) {
        int n = a1[0] ? atoi(a1) : 1;
        if (n < 1) n = 1;
        if (n > 3600) n = 3600;
        adapter->step_frames = n;
        resp_append("[OK] Stepping %d frames...\n", n);
        /* Response comes after stepping finishes */
        return;
    }
    else if (strcasecmp(cmd, "play") == 0) {
        adapter->step_frames = -1;
        resp_append("[OK] Switched to real-time mode\n");
        resp_player(ctx);
    }
    else if (strcasecmp(cmd, "pause") == 0) {
        adapter->step_frames = 0;
        resp_append("[OK] Paused (step mode). Use 'step N' to advance.\n");
        resp_player(ctx);
    }
    else {
        resp_append("[ERROR] Unknown command: %s\n", cmd);
        resp_append("Type 'help' for available commands.\n");
    }

    adapter->response_pending = true;
}

/* ---- FIFO management ---- */

void debug_cmd_init(DebugCmdAdapter *adapter, bool enabled)
{
    memset(adapter, 0, sizeof(*adapter));
    adapter->pipe_fd = -1;
    adapter->active = enabled;

    if (!enabled) return;

    /* Ensure debug/ directory exists */
    if (mkdir("debug", 0755) == -1 && errno != EEXIST) {
        fprintf(stderr, "[CMD] Failed to create debug/ directory\n");
        adapter->active = false;
        return;
    }

    /* Remove stale pipe and create fresh */
    unlink(CMD_PIPE_PATH);
    if (mkfifo(CMD_PIPE_PATH, 0666) == -1) {
        fprintf(stderr, "[CMD] mkfifo failed: %s\n", strerror(errno));
        adapter->active = false;
        return;
    }

    /* Start paused in step mode */
    adapter->step_frames = 0;

    /* Open RDWR + NONBLOCK: keeps pipe open, never blocks, never EOF */
    adapter->pipe_fd = open(CMD_PIPE_PATH, O_RDWR | O_NONBLOCK);
    if (adapter->pipe_fd == -1) {
        fprintf(stderr, "[CMD] Failed to open pipe: %s\n", strerror(errno));
        adapter->active = false;
        return;
    }

    fprintf(stderr, "[CMD] Remote control active on %s\n", CMD_PIPE_PATH);
    fprintf(stderr, "[CMD] Usage: echo \"help\" > %s\n", CMD_PIPE_PATH);
}

void debug_cmd_poll(DebugCmdAdapter *adapter, DebugCmdContext *ctx,
                    DebugCmdOutput *output)
{
    if (!adapter->active) return;

    /* Clear output flags */
    memset(output, 0, sizeof(*output));

    /* Don't process new commands while stepping or waiting */
    if (adapter->step_frames > 0) return;

    /* Handle wait countdown (real-time mode only) */
    if (adapter->wait_frames > 0) {
        adapter->wait_frames--;
        if (adapter->wait_frames == 0) {
            resp_append("...wait complete.\n");
            resp_player(ctx);
            resp_scene(ctx);
            resp_enemies(ctx);
            adapter->response_pending = true;
        }
        return;
    }

    /* Read from pipe (non-blocking) */
    char tmp[512];
    ssize_t n = read(adapter->pipe_fd, tmp, sizeof(tmp) - 1);
    if (n <= 0) return; /* EAGAIN or error */
    tmp[n] = '\0';

    /* Append to line buffer and process complete lines */
    for (int i = 0; i < n; i++) {
        char c = tmp[i];
        if (c == '\n' || c == '\r') {
            adapter->line_buf[adapter->line_len] = '\0';
            if (adapter->line_len > 0) {
                process_line(adapter, adapter->line_buf, ctx, output);
            }
            adapter->line_len = 0;

            /* If wait was set, stop processing further lines */
            if (adapter->wait_frames > 0) return;
        } else if (adapter->line_len < (int)sizeof(adapter->line_buf) - 1) {
            adapter->line_buf[adapter->line_len++] = c;
        }
    }
}

void debug_cmd_flush_response(DebugCmdAdapter *adapter, DebugCmdContext *ctx)
{
    if (!adapter->active || !adapter->response_pending) return;
    adapter->response_pending = false;

    /* Write response text to file */
    FILE *f = fopen(CMD_RESPONSE_PATH, "w");
    if (f) {
        fputs(adapter->response_text, f);
        fclose(f);
    }

    /* Auto-screenshot */
    debug_screenshot_to(ctx->renderer, SCREEN_WIDTH, SCREEN_HEIGHT,
                        CMD_SCREENSHOT_PATH);

    fprintf(stderr, "[CMD] Response written to %s\n", CMD_RESPONSE_PATH);
}

bool debug_cmd_should_step(const DebugCmdAdapter *adapter)
{
    if (!adapter->active) return true; /* no adapter = normal gameplay */
    return adapter->step_frames != 0;  /* -1 (realtime) or >0 (stepping) */
}

void debug_cmd_frame_done(DebugCmdAdapter *adapter, DebugCmdContext *ctx)
{
    if (!adapter->active) return;
    if (adapter->step_frames <= 0) return; /* realtime or already paused */

    adapter->step_frames--;
    if (adapter->step_frames == 0) {
        /* Step sequence complete — build state response */
        resp_append("...step complete.\n");
        resp_player(ctx);
        resp_scene(ctx);
        resp_enemies(ctx);
        adapter->response_pending = true;
    }
}

void debug_cmd_shutdown(DebugCmdAdapter *adapter)
{
    if (!adapter->active) return;
    if (adapter->pipe_fd >= 0)
        close(adapter->pipe_fd);
    unlink(CMD_PIPE_PATH);
    adapter->active = false;
    fprintf(stderr, "[CMD] Remote control shutdown\n");
}
