#include "engine/debug.h"
#include "game/player.h"
#include "game/game.h"
#include "enemy/enemy.h"
#include <sys/stat.h>
#include <time.h>
#include <errno.h>

/* Ensure debug output directory exists */
static bool ensure_debug_dir(void)
{
    if (mkdir(DEBUG_OUTPUT_DIR, 0755) == -1 && errno != EEXIST) {
        fprintf(stderr, "Failed to create %s directory\n", DEBUG_OUTPUT_DIR);
        return false;
    }
    return true;
}

/* Generate timestamp string YYYYMMDD_HHMMSS */
static void timestamp_str(char *buf, size_t len)
{
    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    strftime(buf, len, "%Y%m%d_%H%M%S", t);
}

/* ---- Screenshot ---- */

bool debug_screenshot_to(SDL_Renderer *renderer, int width, int height,
                         const char *path)
{
    SDL_Surface *surface = SDL_CreateRGBSurfaceWithFormat(
        0, width, height, 32, SDL_PIXELFORMAT_RGBA32);
    if (!surface) {
        fprintf(stderr, "debug_screenshot: surface creation failed: %s\n",
                SDL_GetError());
        return false;
    }

    if (SDL_RenderReadPixels(renderer, NULL, SDL_PIXELFORMAT_RGBA32,
                             surface->pixels, surface->pitch) != 0) {
        fprintf(stderr, "debug_screenshot: read pixels failed: %s\n",
                SDL_GetError());
        SDL_FreeSurface(surface);
        return false;
    }

    int result = IMG_SavePNG(surface, path);
    SDL_FreeSurface(surface);

    if (result != 0) {
        fprintf(stderr, "debug_screenshot: save PNG failed: %s\n",
                IMG_GetError());
        return false;
    }

    fprintf(stderr, "[DEBUG] Screenshot saved: %s\n", path);
    return true;
}

bool debug_screenshot(SDL_Renderer *renderer, int width, int height)
{
    if (!ensure_debug_dir()) return false;

    char ts[32];
    timestamp_str(ts, sizeof(ts));

    char path[256];
    snprintf(path, sizeof(path), "%s/screenshot_%s.png", DEBUG_OUTPUT_DIR, ts);

    return debug_screenshot_to(renderer, width, height, path);
}

/* ---- State Dump ---- */

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

static const char *variant_names[] = {
    "NORMAL", "CHAMPION", "ELITE"
};

bool debug_state_dump(const struct Player *player,
                      const struct Game *game,
                      const struct EnemyManager *enemies,
                      const SpriteSheetManager *sprites,
                      const ResourceManager *resources,
                      int scene_type, int dungeon_level)
{
    if (!ensure_debug_dir()) return false;

    char ts[32];
    timestamp_str(ts, sizeof(ts));

    char path[256];
    snprintf(path, sizeof(path), "%s/state_%s.txt", DEBUG_OUTPUT_DIR, ts);

    FILE *f = fopen(path, "w");
    if (!f) {
        fprintf(stderr, "debug_state_dump: cannot open %s\n", path);
        return false;
    }

    fprintf(f, "=== GAME STATE DUMP ===\n");
    fprintf(f, "Timestamp: %s\n", ts);
    fprintf(f, "Scene: %s (dungeon level: %d)\n",
            (scene_type >= 0 && scene_type < 5) ? scene_names[scene_type] : "?",
            dungeon_level);
    fprintf(f, "Game Day: %d  Hour: %d\n", game->game_day, game->game_hour);

    /* Player */
    fprintf(f, "\n--- Player ---\n");
    fprintf(f, "Position: tile(%d, %d) world(%.1f, %.1f)\n",
            player->tile_x, player->tile_y,
            player->world_x, player->world_y);
    fprintf(f, "Facing: %s (%d)\n",
            (player->facing < DIR_COUNT) ? dir_names[player->facing] : "?",
            player->facing);
    fprintf(f, "Moving: %s\n", player->moving ? "yes" : "no");

    int astate = player->anim_state;
    fprintf(f, "Anim State: %s (%d)\n",
            (astate >= 0 && astate < 5) ? anim_state_names[astate] : "?", astate);
    fprintf(f, "Anim Controller: frame %d, dir %d, finished=%s, sheet=%s\n",
            player->anim.current_frame,
            player->anim.current_dir,
            player->anim.finished ? "yes" : "no",
            player->anim.sheet ? "LOADED" : "NULL");

    fprintf(f, "HP: %d/%d  MP: %d/%d\n",
            player->stats.current_hp, player->stats.max_hp,
            player->stats.current_mana, player->stats.max_mana);
    fprintf(f, "Level: %d  XP: %d/%d\n",
            player->stats.level, player->stats.xp, player->stats.xp_to_next);
    fprintf(f, "Stats: STR %d  DEX %d  MAG %d  VIT %d\n",
            player->stats.strength, player->stats.dexterity,
            player->stats.magic, player->stats.vitality);

    /* Enemies */
    int alive = 0;
    for (int i = 0; i < enemies->count; i++)
        if (enemies->enemies[i].alive) alive++;

    fprintf(f, "\n--- Enemies (%d alive / %d total) ---\n", alive, enemies->count);
    for (int i = 0; i < enemies->count; i++) {
        const Enemy *e = &enemies->enemies[i];
        if (!e->alive) continue;
        fprintf(f, "[%d] %s (%s %s) at (%d,%d) state=%s hp=%d/%d facing=%s sheet=%d\n",
                i, e->name,
                (e->variant < 3) ? variant_names[e->variant] : "?",
                (e->type < ENEMY_TYPE_COUNT) ? enemy_type_names[e->type] : "?",
                e->tile_x, e->tile_y,
                (e->state <= 5) ? enemy_state_names[e->state] : "?",
                e->current_hp, e->max_hp,
                (e->facing < DIR_COUNT) ? dir_names[e->facing] : "?",
                e->sprite_sheet_id);
    }

    /* Sprite Sheets */
    fprintf(f, "\n--- Sprite Sheets (%d loaded) ---\n", sprites->count);
    for (int i = 0; i < sprites->count; i++) {
        const SpriteSheet *s = &sprites->sheets[i];
        if (!s->loaded) continue;
        fprintf(f, "[%d] %s (%dx%d frames, sheet %dx%d)\n",
                i, s->name, s->frame_width, s->frame_height,
                s->sheet_width, s->sheet_height);
    }

    /* Textures */
    fprintf(f, "\n--- Textures (%d loaded / %d max) ---\n",
            resources->texture_count, MAX_TEXTURES);
    for (int i = 0; i < resources->texture_count; i++) {
        fprintf(f, "[%d] %s%s\n", i,
                resources->texture_names[i] ? resources->texture_names[i] : "(null)",
                resources->textures[i] ? "" : " (FAILED)");
    }

    /* Missing assets warnings */
    fprintf(f, "\n--- Missing Asset Warnings ---\n");
    if (!player->anim.sheet)
        fprintf(f, "WARNING: Player sprite sheet not loaded (using static fallback)\n");
    if (sprites->count == 0)
        fprintf(f, "WARNING: No sprite sheets loaded at all\n");

    bool any_enemy_sheet = false;
    for (int i = 0; i < enemies->count; i++) {
        if (enemies->enemies[i].sprite_sheet_id >= 0) {
            any_enemy_sheet = true;
            break;
        }
    }
    if (!any_enemy_sheet && enemies->count > 0)
        fprintf(f, "WARNING: No enemy sprite sheets loaded (all using static textures)\n");

    fclose(f);
    fprintf(stderr, "[DEBUG] State dump saved: %s\n", path);
    return true;
}

/* ---- Debug Overlay ---- */

void debug_draw_overlay(UI *ui, SDL_Renderer *renderer,
                        const struct Player *player,
                        const struct EnemyManager *enemies,
                        int fps, int scene_type)
{
    /* Semi-transparent dark panel on the left */
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    SDL_Rect panel = { 4, 4, 200, 180 };
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 180);
    SDL_RenderFillRect(renderer, &panel);
    SDL_SetRenderDrawColor(renderer, 180, 160, 80, 255);
    SDL_RenderDrawRect(renderer, &panel);

    SDL_Color white = { 255, 255, 255, 255 };
    SDL_Color yellow = { 255, 220, 100, 255 };
    SDL_Color red = { 255, 80, 80, 255 };

    int x = 10, y = 10;
    int line_h = 16;

    /* Header */
    ui_draw_text(ui, "-- DEBUG --", x, y, yellow);
    y += line_h;

    /* FPS */
    char buf[128];
    snprintf(buf, sizeof(buf), "FPS: %d", fps);
    ui_draw_text(ui, buf, x, y, white);
    y += line_h;

    /* Scene */
    snprintf(buf, sizeof(buf), "Scene: %s",
             (scene_type >= 0 && scene_type < 5) ? scene_names[scene_type] : "?");
    ui_draw_text(ui, buf, x, y, white);
    y += line_h;

    /* Player position */
    snprintf(buf, sizeof(buf), "Pos: (%d,%d)", player->tile_x, player->tile_y);
    ui_draw_text(ui, buf, x, y, white);
    y += line_h;

    /* Player animation state */
    int astate = player->anim_state;
    snprintf(buf, sizeof(buf), "Anim: %s f:%d d:%s",
             (astate >= 0 && astate < 5) ? anim_state_names[astate] : "?",
             player->anim.current_frame,
             (player->facing < DIR_COUNT) ? dir_names[player->facing] : "?");
    ui_draw_text(ui, buf, x, y, white);
    y += line_h;

    /* Sprite sheet status */
    snprintf(buf, sizeof(buf), "Sheet: %s",
             player->anim.sheet ? "LOADED" : "NOT LOADED");
    ui_draw_text(ui, buf, x, y, player->anim.sheet ? white : red);
    y += line_h;

    /* Enemy count */
    int alive = 0;
    for (int i = 0; i < enemies->count; i++)
        if (enemies->enemies[i].alive) alive++;
    snprintf(buf, sizeof(buf), "Enemies: %d/%d alive", alive, enemies->count);
    ui_draw_text(ui, buf, x, y, white);
    y += line_h;

    /* Missing asset warnings */
    if (!player->anim.sheet) {
        ui_draw_text(ui, "[!] No player sheet", x, y, red);
        y += line_h;
    }

    bool any_enemy_sheet = false;
    for (int i = 0; i < enemies->count; i++) {
        if (enemies->enemies[i].sprite_sheet_id >= 0) {
            any_enemy_sheet = true;
            break;
        }
    }
    if (!any_enemy_sheet && enemies->count > 0) {
        ui_draw_text(ui, "[!] No enemy sheets", x, y, red);
        y += line_h;
    }

    /* Hotkey hints */
    y = panel.y + panel.h - line_h - 4;
    ui_draw_text(ui, "F9:Shot F10:Dump F11:Anim", x, y, yellow);
}
