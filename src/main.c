#include "common.h"
#include "engine/engine.h"
#include "engine/renderer.h"
#include "engine/input.h"
#include "engine/ui.h"
#include "engine/audio.h"
#include "engine/effects.h"
#include "world/camera.h"
#include "world/map.h"
#include "world/town.h"
#include "world/dungeon.h"
#include "game/game.h"
#include "game/player.h"
#include "game/item.h"
#include "game/inventory.h"
#include "game/combat.h"
#include "game/save.h"
#include "game/stats.h"
#include "enemy/enemy.h"
#include "enemy/enemy_ai.h"
#include "npc/npc.h"
#include "npc/npc_schedule.h"
#include "npc/npc_relationship.h"
#include "npc/npc_memory.h"
#include "npc/world_event.h"
#include "npc/npc_ai.h"
#include "npc/npc_interaction.h"
#include "npc/npc_dialogue.h"
#include "story/dialogue_data.h"
#include "story/quest.h"
#include "story/consequence.h"
#include "story/story_arc.h"
#include "data/npc_defs.h"
#include <string.h>

/* Camera scroll speed in pixels per second */
#define CAMERA_SPEED 200

/* HUD bar dimensions */
#define BAR_WIDTH    150
#define BAR_HEIGHT   18
#define BAR_MARGIN   20
#define BAR_Y_OFFSET 40

/* Debug panel dimensions */
#define DEBUG_PANEL_W 200

/* Dialogue panel dimensions */
#define DLG_LINE_H    16
#define DLG_PAD        12

/* Tile texture indices */
static SDL_Texture *tile_textures[TILE_TYPE_COUNT];

/* Hovered tile under mouse */
static int hover_tile_x = -1;
static int hover_tile_y = -1;

/* Character screen toggle */
static bool show_character_screen = false;

/* NPC system state */
static NPCManager npc_mgr;
static RelationshipGraph rel_graph;
static MemorySystem mem_system;
static EventQueue event_queue;
static bool show_debug = false;
static int prev_game_hour = -1;

/* Dialogue and quest system state */
static DialogueSystem dialogue;
static QuestLog quest_log;
static StoryArcSystem story_arcs;
static int game_flags[64];
static int game_flag_count = 64;
static bool show_quest_log = false;

/* Combat and item system state */
static ItemDatabase item_db;
static EnemyManager enemy_mgr;
static Inventory inventory;
static bool show_inventory = false;
static bool player_dead = false;

/* Scene/level transition state */
typedef enum {
    SCENE_TITLE,
    SCENE_TOWN,
    SCENE_DUNGEON,
    SCENE_DEATH,
    SCENE_PAUSED
} SceneType;

static SceneType current_scene = SCENE_TITLE;
static SceneType scene_before_pause = SCENE_TOWN;
static DungeonLevel current_dungeon;
static int current_dungeon_level = 0;  /* 0 = town */

/* Audio & Effects */
static AudioSystem audio;
static EffectsSystem effects;

/* Title screen menu */
static int title_selection = 0;

/* Pause menu */
static int pause_selection = 0;

/* Death menu */
static int death_selection = 0;

/* Ground items (loot drops) */
#define MAX_GROUND_ITEMS 64

typedef struct GroundItem {
    Item item;
    int tile_x, tile_y;
    bool active;
} GroundItem;

static GroundItem ground_items[MAX_GROUND_ITEMS];
static int ground_item_count = 0;

/* Drop a loot item on the ground at a tile */
static void drop_ground_item(const Item *item, int tile_x, int tile_y)
{
    if (ground_item_count >= MAX_GROUND_ITEMS)
        return;
    ground_items[ground_item_count].item = *item;
    ground_items[ground_item_count].tile_x = tile_x;
    ground_items[ground_item_count].tile_y = tile_y;
    ground_items[ground_item_count].active = true;
    ground_item_count++;
}

/* Teleport player to a tile instantly (for level transitions) */
static void player_warp(Player *p, int tx, int ty)
{
    p->tile_x = tx;
    p->tile_y = ty;
    int sx, sy;
    iso_to_screen(tx, ty, &sx, &sy);
    p->world_x = (float)sx;
    p->world_y = (float)sy;
    p->move_from_x = tx;
    p->move_from_y = ty;
    p->move_to_x = tx;
    p->move_to_y = ty;
    p->move_timer = 0.0f;
    p->moving = false;
    path_clear(&p->path);
}

/* NPC sprite textures */
static SDL_Texture *tex_griswold;
static SDL_Texture *tex_pepin;
static SDL_Texture *tex_ogden;
static SDL_Texture *tex_cain;
static SDL_Texture *tex_npc_fallback;

/* Get NPC sprite texture by NPC ID */
static SDL_Texture *get_npc_texture(int npc_id)
{
    switch (npc_id) {
    case 0: return tex_griswold;
    case 1: return tex_pepin;
    case 2: return tex_ogden;
    case 4: return tex_cain;
    default: return tex_npc_fallback;
    }
}

/* NPC action name for debug display */
static const char *action_name(NPCAction a)
{
    static const char *names[] = {
        "Idle", "Work", "Social", "Eat", "Sleep",
        "Pray", "Drink", "Patrol", "Wander"
    };
    if (a >= 0 && a < NPC_ACTION_COUNT) return names[a];
    return "???";
}

/* Town location name for debug display */
static const char *location_name(NPCLocation loc)
{
    static const char *names[] = {
        "Square", "Smith", "Healer", "Tavern", "Church",
        "Well", "Grave", "Adria", "Tremain", "Cain", "Wirt"
    };
    if (loc >= 0 && loc < LOC_COUNT) return names[loc];
    return "???";
}

/* Mood color: green=happy, yellow=neutral, red=bad */
static SDL_Color mood_color(float mood)
{
    if (mood > 0.3f) return (SDL_Color){ 100, 220, 100, 255 };
    if (mood > -0.3f) return (SDL_Color){ 220, 220, 100, 255 };
    return (SDL_Color){ 220, 80, 80, 255 };
}

/* Trait name for debug display */
static const char *trait_name(int idx)
{
    static const char *names[] = { "OPN", "CON", "EXT", "AGR", "NEU" };
    if (idx >= 0 && idx < TRAIT_COUNT) return names[idx];
    return "?";
}

/* Memory event type name for debug display */
static const char *memory_type_name(MemoryEventType t)
{
    static const char *names[] = {
        "Helped", "Betrayed", "Ignored", "NPC Died", "Insulted",
        "Praised", "Quest OK", "Quest Fail", "Good News", "Bad News",
        "Combat", "Gift", "Threat"
    };
    if (t >= 0 && t < MEM_EVENT_COUNT) return names[t];
    return "???";
}

/* Load a texture, returning NULL (not crashing) on failure */
static SDL_Texture *load_tex(ResourceManager *rm, const char *path)
{
    int idx = resource_load_texture(rm, path);
    return resource_get_texture(rm, idx);
}

/* Check if a tile is visible in the current viewport */
static bool tile_visible(int tx, int ty, int cam_x, int cam_y)
{
    int sx, sy;
    iso_to_screen(tx, ty, &sx, &sy);
    sx -= cam_x;
    sy -= cam_y;
    sx += SCREEN_WIDTH / 2;
    /* Conservative check with generous margin */
    return sx > -TILE_WIDTH * 2 && sx < SCREEN_WIDTH + TILE_WIDTH * 2 &&
           sy > -TILE_HEIGHT * 4 && sy < VIEWPORT_HEIGHT + TILE_HEIGHT * 4;
}

/* Draw a filled bar with border (HP or MP) */
static void draw_bar(SDL_Renderer *renderer, int x, int y, int w, int h,
                     int current, int max, SDL_Color fill_color)
{
    /* Background (dark) */
    SDL_Rect bg = { x, y, w, h };
    SDL_SetRenderDrawColor(renderer, 20, 15, 10, 255);
    SDL_RenderFillRect(renderer, &bg);

    /* Fill proportional to current/max */
    if (max > 0 && current > 0) {
        int fill_w = (int)((float)current / max * w);
        if (fill_w > w) fill_w = w;
        SDL_Rect fill = { x, y, fill_w, h };
        SDL_SetRenderDrawColor(renderer, fill_color.r, fill_color.g, fill_color.b, 255);
        SDL_RenderFillRect(renderer, &fill);
    }

    /* Border */
    SDL_SetRenderDrawColor(renderer, 120, 100, 80, 255);
    SDL_RenderDrawRect(renderer, &bg);
}

/* Draw the enhanced HUD with HP/MP bars and level */
static void draw_hud(UI *ui, SDL_Renderer *renderer, const PlayerStats *stats,
                     int viewport_h, int panel_h, int screen_w)
{
    (void)panel_h;
    int panel_y = viewport_h;

    /* HP bar — left side */
    int hp_x = BAR_MARGIN;
    int hp_y = panel_y + BAR_Y_OFFSET;
    draw_bar(renderer, hp_x, hp_y, BAR_WIDTH, BAR_HEIGHT,
             stats->current_hp, stats->max_hp,
             (SDL_Color){ 180, 30, 30, 255 });

    /* HP text */
    char hp_text[32];
    snprintf(hp_text, sizeof(hp_text), "HP: %d/%d", stats->current_hp, stats->max_hp);
    ui_draw_text(ui, hp_text, hp_x + 4, hp_y + 2, (SDL_Color){ 255, 255, 255, 255 });

    /* MP bar — right side */
    int mp_x = screen_w - BAR_WIDTH - BAR_MARGIN;
    int mp_y = panel_y + BAR_Y_OFFSET;
    draw_bar(renderer, mp_x, mp_y, BAR_WIDTH, BAR_HEIGHT,
             stats->current_mana, stats->max_mana,
             (SDL_Color){ 30, 60, 180, 255 });

    /* MP text */
    char mp_text[32];
    snprintf(mp_text, sizeof(mp_text), "MP: %d/%d", stats->current_mana, stats->max_mana);
    ui_draw_text(ui, mp_text, mp_x + 4, mp_y + 2, (SDL_Color){ 255, 255, 255, 255 });

    /* Level — center */
    char lvl_text[32];
    snprintf(lvl_text, sizeof(lvl_text), "Level %d", stats->level);
    int lvl_x = screen_w / 2 - 30;
    int lvl_y = panel_y + BAR_Y_OFFSET;
    ui_draw_text(ui, lvl_text, lvl_x, lvl_y + 2, (SDL_Color){ 220, 200, 140, 255 });
}

/* Draw character screen overlay */
static void draw_character_screen(UI *ui, SDL_Renderer *renderer, const PlayerStats *stats)
{
    /* Semi-transparent dark overlay */
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    SDL_Rect overlay = { 0, 0, SCREEN_WIDTH, VIEWPORT_HEIGHT };
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 180);
    SDL_RenderFillRect(renderer, &overlay);

    /* Character panel background */
    int panel_w = 260;
    int panel_h = 300;
    int panel_x = (SCREEN_WIDTH - panel_w) / 2;
    int panel_y = (VIEWPORT_HEIGHT - panel_h) / 2;
    SDL_Rect panel = { panel_x, panel_y, panel_w, panel_h };
    SDL_SetRenderDrawColor(renderer, 30, 25, 20, 240);
    SDL_RenderFillRect(renderer, &panel);
    SDL_SetRenderDrawColor(renderer, 140, 120, 80, 255);
    SDL_RenderDrawRect(renderer, &panel);

    SDL_Color gold  = { 220, 200, 140, 255 };
    SDL_Color white = { 255, 255, 255, 255 };
    int x = panel_x + 20;
    int y = panel_y + 15;
    int line_h = 22;

    /* Title */
    ui_draw_text(ui, "- CHARACTER -", panel_x + panel_w / 2 - 50, y, gold);
    y += line_h + 8;

    /* Stats display */
    char buf[64];

    snprintf(buf, sizeof(buf), "Level: %d", stats->level);
    ui_draw_text(ui, buf, x, y, white); y += line_h;

    snprintf(buf, sizeof(buf), "XP: %d / %d", stats->xp, stats->xp_to_next);
    ui_draw_text(ui, buf, x, y, white); y += line_h + 6;

    snprintf(buf, sizeof(buf), "STR: %d", stats->strength);
    ui_draw_text(ui, buf, x, y, white); y += line_h;

    snprintf(buf, sizeof(buf), "DEX: %d", stats->dexterity);
    ui_draw_text(ui, buf, x, y, white); y += line_h;

    snprintf(buf, sizeof(buf), "MAG: %d", stats->magic);
    ui_draw_text(ui, buf, x, y, white); y += line_h;

    snprintf(buf, sizeof(buf), "VIT: %d", stats->vitality);
    ui_draw_text(ui, buf, x, y, white); y += line_h + 6;

    snprintf(buf, sizeof(buf), "HP: %d / %d", stats->current_hp, stats->max_hp);
    ui_draw_text(ui, buf, x, y, white); y += line_h;

    snprintf(buf, sizeof(buf), "MP: %d / %d", stats->current_mana, stats->max_mana);
    ui_draw_text(ui, buf, x, y, white); y += line_h + 6;

    snprintf(buf, sizeof(buf), "Armor: %d", stats->armor_class);
    ui_draw_text(ui, buf, x, y, white); y += line_h;

    snprintf(buf, sizeof(buf), "Damage: %d-%d", stats->min_damage, stats->max_damage);
    ui_draw_text(ui, buf, x, y, white);

    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
}

/* Draw NPC debug overlay on the right side of viewport */
static void draw_debug_overlay(UI *ui, SDL_Renderer *renderer,
                                const NPCManager *mgr,
                                const RelationshipGraph *graph,
                                const MemorySystem *memsys)
{
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);

    /* Semi-transparent panel */
    int px = SCREEN_WIDTH - DEBUG_PANEL_W;
    SDL_Rect panel = { px, 0, DEBUG_PANEL_W, VIEWPORT_HEIGHT };
    SDL_SetRenderDrawColor(renderer, 10, 8, 15, 200);
    SDL_RenderFillRect(renderer, &panel);
    SDL_SetRenderDrawColor(renderer, 100, 80, 60, 255);
    SDL_RenderDrawRect(renderer, &panel);

    SDL_Color gold  = { 220, 200, 140, 255 };
    SDL_Color white = { 200, 200, 200, 255 };
    SDL_Color dim   = { 140, 140, 140, 255 };
    char buf[128];
    int x = px + 6;
    int y = 4;
    int line_h = 14;

    ui_draw_text(ui, "-- NPC DEBUG --", x, y, gold);
    y += line_h + 4;

    /* Find hovered NPC for detailed view */
    int hovered_npc = -1;
    for (int i = 0; i < mgr->count; i++) {
        const NPC *npc = &mgr->npcs[i];
        if (npc->is_alive && !npc->has_left_town &&
            npc->tile_x == hover_tile_x && npc->tile_y == hover_tile_y) {
            hovered_npc = i;
            break;
        }
    }

    /* If hovering over an NPC, show detailed info */
    if (hovered_npc >= 0) {
        const NPC *npc = &mgr->npcs[hovered_npc];

        /* Name and title */
        snprintf(buf, sizeof(buf), "%s", npc->name);
        ui_draw_text(ui, buf, x, y, gold); y += line_h;
        snprintf(buf, sizeof(buf), "(%s)", npc->title);
        ui_draw_text(ui, buf, x, y, dim); y += line_h + 2;

        /* Action and mood */
        snprintf(buf, sizeof(buf), "Action: %s", action_name(npc->current_action));
        ui_draw_text(ui, buf, x, y, white); y += line_h;
        snprintf(buf, sizeof(buf), "Mood: %.2f", npc->mood);
        ui_draw_text(ui, buf, x, y, mood_color(npc->mood)); y += line_h;
        snprintf(buf, sizeof(buf), "Stress: %.2f", npc->stress);
        ui_draw_text(ui, buf, x, y, white); y += line_h;
        snprintf(buf, sizeof(buf), "Loc: %s", location_name(npc->current_location));
        ui_draw_text(ui, buf, x, y, white); y += line_h + 4;

        /* Personality traits */
        ui_draw_text(ui, "Traits:", x, y, gold); y += line_h;
        for (int t = 0; t < TRAIT_COUNT; t++) {
            snprintf(buf, sizeof(buf), " %s: %.1f", trait_name(t), npc->personality.traits[t]);
            ui_draw_text(ui, buf, x, y, dim); y += line_h;
        }
        y += 2;

        /* Player trust */
        snprintf(buf, sizeof(buf), "Trust(plr): %.2f", npc->trust_player);
        SDL_Color trust_c = npc->trust_player > 0 ? (SDL_Color){100,220,100,255}
                                                   : (SDL_Color){220,100,100,255};
        ui_draw_text(ui, buf, x, y, trust_c); y += line_h;

        /* Relationship with player (from graph) - use const cast-free approach */
        (void)graph; /* accessed via trust_player above */

        /* Needs */
        y += 2;
        ui_draw_text(ui, "Needs:", x, y, gold); y += line_h;
        snprintf(buf, sizeof(buf), " Safe:%.1f Soc:%.1f",
                 npc->need_safety, npc->need_social);
        ui_draw_text(ui, buf, x, y, dim); y += line_h;
        snprintf(buf, sizeof(buf), " Econ:%.1f Purp:%.1f",
                 npc->need_economic, npc->need_purpose);
        ui_draw_text(ui, buf, x, y, dim); y += line_h + 4;

        /* Recent memories (last 3) */
        ui_draw_text(ui, "Memories:", x, y, gold); y += line_h;
        const MemoryBank *bank = &memsys->banks[npc->id];
        int shown = 0;
        for (int m = bank->count - 1; m >= 0 && shown < 3; m--) {
            int idx = m % MAX_MEMORIES_PER_NPC;
            const NPCMemory *mem = &bank->memories[idx];
            if (mem->salience < 0.01f) continue;
            snprintf(buf, sizeof(buf), " d%d: %s (%.1f)",
                     mem->game_day, memory_type_name(mem->event_type),
                     mem->emotional_weight);
            ui_draw_text(ui, buf, x, y, dim); y += line_h;
            shown++;
        }
        if (shown == 0) {
            ui_draw_text(ui, " (none)", x, y, dim); y += line_h;
        }
    } else {
        /* No NPC hovered — show compact list of all NPCs */
        for (int i = 0; i < mgr->count; i++) {
            const NPC *npc = &mgr->npcs[i];
            if (!npc->is_alive || npc->has_left_town) continue;

            /* Name + action (compact) */
            snprintf(buf, sizeof(buf), "%s [%s]",
                     npc->name, action_name(npc->current_action));
            ui_draw_text(ui, buf, x, y, mood_color(npc->mood));
            y += line_h;

            /* Location */
            snprintf(buf, sizeof(buf), " @%s", location_name(npc->current_location));
            ui_draw_text(ui, buf, x, y, dim);
            y += line_h + 1;

            if (y > VIEWPORT_HEIGHT - line_h) break;
        }
    }

    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
}

/* Draw dialogue panel over the HUD area when a conversation is active */
static void draw_dialogue_panel(UI *ui, SDL_Renderer *renderer,
                                const DialogueSystem *ds,
                                const NPCManager *mgr)
{
    if (!dialogue_is_active(ds))
        return;

    const DialogueNode *node = dialogue_current_node(ds);
    if (!node)
        return;

    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);

    /* Dark atmospheric panel covering the HUD area */
    int py = VIEWPORT_HEIGHT;
    SDL_Rect panel_bg = { 0, py, SCREEN_WIDTH, PANEL_HEIGHT };
    SDL_SetRenderDrawColor(renderer, 10, 8, 5, 235);
    SDL_RenderFillRect(renderer, &panel_bg);

    /* Gold border at top */
    SDL_SetRenderDrawColor(renderer, 160, 130, 50, 255);
    SDL_Rect border_top = { 0, py, SCREEN_WIDTH, 2 };
    SDL_RenderFillRect(renderer, &border_top);

    SDL_Color gold   = { 220, 190, 80, 255 };
    SDL_Color white  = { 210, 210, 200, 255 };
    SDL_Color yellow = { 255, 255, 100, 255 };
    SDL_Color dim    = { 150, 145, 130, 255 };

    /* Find NPC name */
    const NPC *npc = NULL;
    for (int i = 0; i < mgr->count; i++) {
        if (mgr->npcs[i].id == node->npc_id) {
            npc = &mgr->npcs[i];
            break;
        }
    }

    int x = DLG_PAD;
    int y = py + DLG_PAD;

    /* NPC name in gold */
    if (npc)
        ui_draw_text(ui, npc->name, x, y, gold);
    y += DLG_LINE_H + 4;

    /* Dialogue text with typewriter effect — word-wrapped */
    int text_len = (int)strlen(node->text);
    int show_chars = ds->chars_shown < text_len ? ds->chars_shown : text_len;
    char display[MAX_DIALOGUE_TEXT];
    memcpy(display, node->text, (size_t)show_chars);
    display[show_chars] = '\0';

    int chars_per_line = 72;
    int pos = 0;
    while (pos < show_chars) {
        int end = pos + chars_per_line;
        if (end > show_chars) end = show_chars;
        /* Break at last space to avoid splitting words */
        if (end < show_chars && end > pos) {
            int last_sp = end;
            while (last_sp > pos && display[last_sp] != ' ')
                last_sp--;
            if (last_sp > pos) end = last_sp + 1;
        }
        char line[MAX_DIALOGUE_TEXT];
        int line_len = end - pos;
        memcpy(line, &display[pos], (size_t)line_len);
        line[line_len] = '\0';
        ui_draw_text(ui, line, x, y, white);
        y += DLG_LINE_H;
        pos = end;
    }

    /* Show choices once text is fully revealed */
    bool text_done = ds->chars_shown >= text_len;
    if (text_done && node->choice_count > 0) {
        y += 4;
        for (int i = 0; i < node->choice_count; i++) {
            bool selected = (i == ds->selected_choice);
            SDL_Color color = selected ? yellow : dim;
            char choice_buf[MAX_CHOICE_TEXT + 4];
            snprintf(choice_buf, sizeof(choice_buf), "%s %s",
                     selected ? ">" : " ", node->choices[i].text);
            ui_draw_text(ui, choice_buf, x + 8, y, color);
            y += DLG_LINE_H;
        }
    }

    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
}

/* Draw quest log overlay on the viewport */
static void draw_quest_log(UI *ui, SDL_Renderer *renderer,
                           const QuestLog *ql)
{
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);

    /* Semi-transparent dark overlay */
    SDL_Rect overlay = { 0, 0, SCREEN_WIDTH, VIEWPORT_HEIGHT };
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 180);
    SDL_RenderFillRect(renderer, &overlay);

    /* Quest log panel */
    int pw = 360;
    int ph = 290;
    int px = (SCREEN_WIDTH - pw) / 2;
    int panel_y = (VIEWPORT_HEIGHT - ph) / 2;
    SDL_Rect panel = { px, panel_y, pw, ph };
    SDL_SetRenderDrawColor(renderer, 25, 20, 15, 240);
    SDL_RenderFillRect(renderer, &panel);
    SDL_SetRenderDrawColor(renderer, 160, 130, 50, 255);
    SDL_RenderDrawRect(renderer, &panel);

    SDL_Color gold  = { 220, 190, 80, 255 };
    SDL_Color white = { 210, 210, 200, 255 };
    SDL_Color dim   = { 120, 118, 110, 255 };
    char buf[256];
    int x = px + 16;
    int y = panel_y + 14;
    int line_h = 18;

    /* Title */
    ui_draw_text(ui, "- QUEST LOG -", px + pw / 2 - 52, y, gold);
    y += line_h + 8;

    /* Active quests */
    bool has_active = false;
    for (int i = 0; i < ql->count; i++) {
        const Quest *q = &ql->quests[i];
        if (q->state != QUEST_ACTIVE) continue;
        has_active = true;
        snprintf(buf, sizeof(buf), "* %s", q->name);
        ui_draw_text(ui, buf, x, y, white);
        y += line_h;
        /* Truncated description */
        if (strlen(q->description) > 55)
            snprintf(buf, sizeof(buf), "%.52s...", q->description);
        else
            snprintf(buf, sizeof(buf), "%s", q->description);
        ui_draw_text(ui, buf, x + 10, y, dim);
        y += line_h + 2;
        if (y > panel_y + ph - line_h * 4) break;
    }
    if (!has_active) {
        ui_draw_text(ui, "No active quests.", x, y, dim);
        y += line_h;
    }

    /* Completed quests */
    y += 6;
    ui_draw_text(ui, "Completed:", x, y, gold);
    y += line_h;
    bool has_completed = false;
    for (int i = 0; i < ql->count; i++) {
        const Quest *q = &ql->quests[i];
        if (q->state != QUEST_COMPLETED) continue;
        has_completed = true;
        snprintf(buf, sizeof(buf), "  %s", q->name);
        ui_draw_text(ui, buf, x, y, dim);
        y += line_h;
        if (y > panel_y + ph - line_h) break;
    }
    if (!has_completed) {
        ui_draw_text(ui, "  (none)", x, y, dim);
    }

    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
}

/* Draw a small HP bar above an enemy */
static void draw_enemy_hp_bar(SDL_Renderer *renderer, int sx, int sy,
                               int current_hp, int max_hp)
{
    int bar_w = 40;
    int bar_h = 4;
    int bx = sx + SPRITE_SIZE / 2 - bar_w / 2;
    int by = sy - 6;

    /* Background */
    SDL_Rect bg = { bx, by, bar_w, bar_h };
    SDL_SetRenderDrawColor(renderer, 20, 15, 10, 255);
    SDL_RenderFillRect(renderer, &bg);

    /* Fill */
    if (max_hp > 0 && current_hp > 0) {
        int fill_w = current_hp * bar_w / max_hp;
        if (fill_w > bar_w) fill_w = bar_w;
        SDL_Rect fill = { bx, by, fill_w, bar_h };
        SDL_SetRenderDrawColor(renderer, 200, 30, 30, 255);
        SDL_RenderFillRect(renderer, &fill);
    }

    /* Border */
    SDL_SetRenderDrawColor(renderer, 100, 80, 60, 255);
    SDL_RenderDrawRect(renderer, &bg);
}

/* Draw inventory screen overlay */
static void draw_inventory_screen(UI *ui, SDL_Renderer *renderer,
                                   const Inventory *inv)
{
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);

    /* Semi-transparent dark overlay */
    SDL_Rect overlay = { 0, 0, SCREEN_WIDTH, VIEWPORT_HEIGHT };
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 180);
    SDL_RenderFillRect(renderer, &overlay);

    /* Inventory panel background */
    int panel_w = 320;
    int panel_h = 300;
    int panel_x = (SCREEN_WIDTH - panel_w) / 2;
    int panel_y = (VIEWPORT_HEIGHT - panel_h) / 2;
    SDL_Rect panel = { panel_x, panel_y, panel_w, panel_h };
    SDL_SetRenderDrawColor(renderer, 30, 25, 20, 240);
    SDL_RenderFillRect(renderer, &panel);
    SDL_SetRenderDrawColor(renderer, 140, 120, 80, 255);
    SDL_RenderDrawRect(renderer, &panel);

    SDL_Color gold  = { 220, 200, 140, 255 };
    SDL_Color white = { 255, 255, 255, 255 };
    SDL_Color dim   = { 160, 155, 140, 255 };
    SDL_Color green = { 100, 220, 100, 255 };

    int x = panel_x + 12;
    int y = panel_y + 10;
    int line_h = 16;

    /* Title */
    ui_draw_text(ui, "- INVENTORY -", panel_x + panel_w / 2 - 52, y, gold);
    y += line_h + 6;

    /* Equipped items */
    ui_draw_text(ui, "Equipped:", x, y, gold);
    y += line_h;
    static const char *equip_names[] = {
        "Weapon", "Shield", "Helm", "Armor", "Ring1", "Ring2", "Amulet"
    };
    for (int i = 0; i < EQUIP_SLOT_COUNT; i++) {
        char buf[96];
        if (inv->equipped[i].type != ITEM_NONE) {
            snprintf(buf, sizeof(buf), " %s: %s", equip_names[i], inv->equipped[i].name);
            ui_draw_text(ui, buf, x, y, green);
        } else {
            snprintf(buf, sizeof(buf), " %s: (empty)", equip_names[i]);
            ui_draw_text(ui, buf, x, y, dim);
        }
        y += line_h;
    }
    y += 4;

    /* Gold */
    {
        char buf[64];
        snprintf(buf, sizeof(buf), "Gold: %d", inv->gold);
        ui_draw_text(ui, buf, x, y, gold);
        y += line_h + 4;
    }

    /* Inventory grid (compact list of occupied slots) */
    ui_draw_text(ui, "Backpack:", x, y, gold);
    y += line_h;
    bool has_items = false;
    for (int i = 0; i < INVENTORY_SIZE; i++) {
        if (inv->slots[i].type == ITEM_NONE)
            continue;
        has_items = true;
        char buf[96];
        if (inv->slots[i].stackable && inv->slots[i].stack_count > 1)
            snprintf(buf, sizeof(buf), " [%d] %s x%d", i + 1,
                     inv->slots[i].name, inv->slots[i].stack_count);
        else
            snprintf(buf, sizeof(buf), " [%d] %s", i + 1, inv->slots[i].name);
        ui_draw_text(ui, buf, x, y, white);
        y += line_h;
        if (y > panel_y + panel_h - line_h * 2) break;
    }
    if (!has_items) {
        ui_draw_text(ui, " (empty)", x, y, dim);
    }

    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
}

/* Draw title screen */
static void draw_title_screen(UI *ui, SDL_Renderer *renderer)
{
    bool has_save = save_exists(SAVE_FILE_PATH);

    /* Full black background */
    SDL_SetRenderDrawColor(renderer, 5, 3, 8, 255);
    SDL_Rect bg = { 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT };
    SDL_RenderFillRect(renderer, &bg);

    /* Atmospheric dark red vignette border */
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(renderer, 80, 10, 10, 40);
    for (int i = 0; i < 20; i++) {
        SDL_Rect border = { i, i, SCREEN_WIDTH - i * 2, SCREEN_HEIGHT - i * 2 };
        SDL_RenderDrawRect(renderer, &border);
    }

    /* Title text */
    SDL_Color gold = { 220, 180, 50, 255 };
    SDL_Color dim_gold = { 160, 130, 40, 255 };
    SDL_Color white = { 255, 255, 255, 255 };
    SDL_Color dim = { 140, 135, 120, 255 };
    SDL_Color red = { 180, 40, 40, 255 };

    /* Main title */
    ui_draw_text(ui, "DESCENT INTO DARKNESS", SCREEN_WIDTH / 2 - 88, 100, gold);

    /* Subtitle */
    ui_draw_text(ui, "A Diablo Chronicle", SCREEN_WIDTH / 2 - 70, 130, dim_gold);

    /* Decorative separator */
    SDL_SetRenderDrawColor(renderer, 120, 90, 30, 200);
    SDL_Rect sep = { SCREEN_WIDTH / 2 - 80, 155, 160, 1 };
    SDL_RenderFillRect(renderer, &sep);

    /* Menu items */
    int menu_y = 200;
    int menu_spacing = 30;
    int item_idx = 0;

    /* New Game */
    {
        bool selected = (title_selection == item_idx);
        SDL_Color c = selected ? white : dim;
        char buf[32];
        snprintf(buf, sizeof(buf), "%s New Game", selected ? ">" : " ");
        ui_draw_text(ui, buf, SCREEN_WIDTH / 2 - 50, menu_y, c);
        menu_y += menu_spacing;
        item_idx++;
    }

    /* Continue (only if save exists) */
    if (has_save) {
        bool selected = (title_selection == item_idx);
        SDL_Color c = selected ? white : dim;
        char buf[32];
        snprintf(buf, sizeof(buf), "%s Continue", selected ? ">" : " ");
        ui_draw_text(ui, buf, SCREEN_WIDTH / 2 - 50, menu_y, c);
        menu_y += menu_spacing;
        item_idx++;
    }

    /* Quit */
    {
        bool selected = (title_selection == item_idx);
        SDL_Color c = selected ? red : dim;
        char buf[32];
        snprintf(buf, sizeof(buf), "%s Quit", selected ? ">" : " ");
        ui_draw_text(ui, buf, SCREEN_WIDTH / 2 - 50, menu_y, c);
    }

    /* Controls hint */
    ui_draw_text(ui, "Arrow Keys to select, Enter to confirm",
                 SCREEN_WIDTH / 2 - 140, SCREEN_HEIGHT - 50, dim);

    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
}

/* Draw pause menu overlay */
static void draw_pause_menu(UI *ui, SDL_Renderer *renderer)
{
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);

    /* Darken the entire screen */
    SDL_Rect overlay = { 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT };
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 160);
    SDL_RenderFillRect(renderer, &overlay);

    /* Pause panel */
    int pw = 240;
    int ph = 180;
    int px = (SCREEN_WIDTH - pw) / 2;
    int py = (SCREEN_HEIGHT - ph) / 2;
    SDL_Rect panel = { px, py, pw, ph };
    SDL_SetRenderDrawColor(renderer, 20, 15, 10, 230);
    SDL_RenderFillRect(renderer, &panel);
    SDL_SetRenderDrawColor(renderer, 140, 120, 80, 255);
    SDL_RenderDrawRect(renderer, &panel);

    SDL_Color gold  = { 220, 200, 140, 255 };
    SDL_Color white = { 255, 255, 255, 255 };
    SDL_Color dim   = { 140, 135, 120, 255 };
    SDL_Color red   = { 180, 40, 40, 255 };

    /* Title */
    ui_draw_text(ui, "- PAUSED -", px + pw / 2 - 40, py + 20, gold);

    /* Menu items */
    int y = py + 60;
    int spacing = 28;
    const char *items[] = { "Resume", "Save Game", "Quit to Title" };
    int count = 3;

    for (int i = 0; i < count; i++) {
        bool selected = (pause_selection == i);
        SDL_Color c;
        if (i == 2)
            c = selected ? red : dim;
        else
            c = selected ? white : dim;
        char buf[48];
        snprintf(buf, sizeof(buf), "%s %s", selected ? ">" : " ", items[i]);
        ui_draw_text(ui, buf, px + 40, y, c);
        y += spacing;
    }

    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
}

/* Draw death screen overlay with menu options */
static void draw_death_screen(UI *ui, SDL_Renderer *renderer)
{
    bool has_save = save_exists(SAVE_FILE_PATH);

    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);

    SDL_Rect overlay = { 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT };
    SDL_SetRenderDrawColor(renderer, 60, 0, 0, 180);
    SDL_RenderFillRect(renderer, &overlay);

    /* Pulsing dark atmosphere */
    SDL_SetRenderDrawColor(renderer, 100, 0, 0, 30);
    for (int i = 0; i < 10; i++) {
        SDL_Rect ring = { SCREEN_WIDTH / 2 - 100 - i * 15,
                          SCREEN_HEIGHT / 2 - 80 - i * 12,
                          200 + i * 30, 160 + i * 24 };
        SDL_RenderDrawRect(renderer, &ring);
    }

    SDL_Color red   = { 255, 50, 50, 255 };
    SDL_Color white = { 255, 255, 255, 255 };
    SDL_Color dim   = { 180, 160, 140, 255 };

    /* Death title */
    ui_draw_text(ui, "YOU HAVE DIED", SCREEN_WIDTH / 2 - 55,
                 SCREEN_HEIGHT / 2 - 60, red);

    /* Skull-like decorative line */
    SDL_SetRenderDrawColor(renderer, 150, 30, 30, 200);
    SDL_Rect sep = { SCREEN_WIDTH / 2 - 60, SCREEN_HEIGHT / 2 - 35, 120, 1 };
    SDL_RenderFillRect(renderer, &sep);

    /* Menu items */
    int y = SCREEN_HEIGHT / 2 - 10;
    int spacing = 28;
    int item_idx = 0;

    if (has_save) {
        bool selected = (death_selection == item_idx);
        SDL_Color c = selected ? white : dim;
        char buf[32];
        snprintf(buf, sizeof(buf), "%s Load Save", selected ? ">" : " ");
        ui_draw_text(ui, buf, SCREEN_WIDTH / 2 - 50, y, c);
        y += spacing;
        item_idx++;
    }

    {
        bool selected = (death_selection == item_idx);
        SDL_Color c = selected ? white : dim;
        char buf[32];
        snprintf(buf, sizeof(buf), "%s Quit to Title", selected ? ">" : " ");
        ui_draw_text(ui, buf, SCREEN_WIDTH / 2 - 60, y, c);
    }

    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
}

/* Confirm the currently selected dialogue choice: apply consequences and advance */
static void dialogue_confirm_choice(DialogueSystem *ds, Game *game_state,
                                    NPCManager *npcs, EventQueue *events,
                                    RelationshipGraph *graph, int *flags)
{
    const DialogueNode *node = dialogue_current_node(ds);
    if (!node) return;

    int text_len = (int)strlen(node->text);
    if (ds->chars_shown < text_len) {
        /* Text still typing — skip to end */
        ds->chars_shown = text_len;
        ds->typewriter_timer = (float)text_len;
        return;
    }

    if (node->choice_count > 0) {
        const DialogueChoice *ch = &node->choices[ds->selected_choice];
        dialogue_apply_consequences(ch->consequences, ch->consequence_count,
                                    npcs, events, graph, flags,
                                    game_state->game_day);
        dialogue_select_choice(ds, ds->selected_choice);
    }
}

/* Get the MusicTrack for the current dungeon theme */
static MusicTrack dungeon_music_for_theme(int theme)
{
    switch (theme) {
    case THEME_CATHEDRAL: return MUSIC_DUNGEON_1;
    case THEME_CATACOMBS: return MUSIC_DUNGEON_2;
    case THEME_CAVES:     return MUSIC_DUNGEON_3;
    case THEME_HELL:      return MUSIC_DUNGEON_4;
    default:              return MUSIC_DUNGEON_1;
    }
}

/* Reset all mutable game state for a new game or before loading a save */
static void reset_game_state(Game *game, Player *player, Town *town,
                             Camera *camera)
{
    game_init(game);
    player_init(player, town->player_start_x, town->player_start_y);

    /* NPC systems */
    npc_defs_load(&npc_mgr, town);
    relationship_init(&rel_graph);
    relationship_init_defaults(&rel_graph);
    memory_system_init(&mem_system);
    event_queue_init(&event_queue);
    prev_game_hour = game->game_hour;

    /* Dialogue, quest, story */
    dialogue_init(&dialogue);
    dialogue_data_init(&dialogue);
    quest_log_init(&quest_log);
    quest_data_init(&quest_log);
    story_arc_init(&story_arcs);
    memset(game_flags, 0, sizeof(game_flags));
    game_flag_count = 64;

    /* Inventory and combat */
    inventory_init(&inventory);
    enemy_manager_init(&enemy_mgr);
    memset(ground_items, 0, sizeof(ground_items));
    ground_item_count = 0;

    /* Starting equipment */
    {
        Item start_sword = item_create(&item_db, 1);  /* Short Sword */
        inventory_add_item(&inventory, &start_sword);
        inventory_equip(&inventory, 0);

        Item hp_pot = item_create(&item_db, 21);  /* Healing Potion */
        hp_pot.stack_count = 3;
        inventory_add_item(&inventory, &hp_pot);
    }

    /* Spawn enemies near cathedral entrance */
    enemy_spawn_group(&enemy_mgr, ENEMY_FALLEN, 28, 8, 3, &town->map);
    enemy_spawn_group(&enemy_mgr, ENEMY_SKELETON, 30, 10, 2, &town->map);

    /* UI state */
    show_character_screen = false;
    show_quest_log = false;
    show_inventory = false;
    show_debug = false;
    player_dead = false;

    /* Camera */
    camera_center_on_tile(camera, player->tile_x, player->tile_y);
    camera->x = camera->target_x;
    camera->y = camera->target_y;
}

int main(int argc, char *argv[])
{
    (void)argc;
    (void)argv;

    /* Core systems */
    Engine engine;
    Game game;
    Camera camera;
    IsoRenderer iso_renderer;
    InputState input;
    UI ui;

    /* Initialize engine (SDL, window, renderer, timer, resource manager) */
    if (!engine_init(&engine, "Descent into Darkness", SCREEN_WIDTH, SCREEN_HEIGHT))
        return 1;

    /* Initialize audio system (graceful — game works without audio) */
    audio_init(&audio);

    /* Initialize effects system */
    effects_init(&effects);

    /* Initialize Tristram town */
    Town town;
    town_init(&town);

    /* Load tile textures */
    SDL_Texture *tex_grass  = load_tex(&engine.resources, "assets/tiles/town/grass.jpg");
    SDL_Texture *tex_dirt   = load_tex(&engine.resources, "assets/tiles/town/dirt_path.jpg");
    SDL_Texture *tex_stone  = load_tex(&engine.resources, "assets/tiles/floor_stone.jpg");
    SDL_Texture *tex_wall   = load_tex(&engine.resources, "assets/tiles/wall_stone.jpg");
    SDL_Texture *tex_water  = load_tex(&engine.resources, "assets/tiles/town/water.jpg");
    SDL_Texture *tex_tree   = load_tex(&engine.resources, "assets/tiles/town/tree.jpg");

    tile_textures[TILE_NONE]        = tex_stone;   /* fallback */
    tile_textures[TILE_GRASS]       = tex_grass;
    tile_textures[TILE_DIRT]        = tex_dirt;
    tile_textures[TILE_STONE_FLOOR] = tex_stone;
    tile_textures[TILE_WALL]        = tex_wall;
    tile_textures[TILE_WATER]       = tex_water;
    tile_textures[TILE_DOOR]        = tex_stone;    /* door drawn as stone floor */
    tile_textures[TILE_STAIRS_UP]   = tex_stone;    /* stairs up drawn as stone floor */
    tile_textures[TILE_STAIRS_DOWN] = tex_stone;    /* stairs down drawn as stone floor */
    tile_textures[TILE_TREE]        = tex_grass;    /* grass base under trees */

    /* Player sprite */
    SDL_Texture *tex_warrior = load_tex(&engine.resources, "assets/sprites/player/warrior_test.jpg");

    /* NPC sprites */
    tex_griswold     = load_tex(&engine.resources, "assets/sprites/npcs/griswold.jpg");
    tex_pepin        = load_tex(&engine.resources, "assets/sprites/npcs/pepin.jpg");
    tex_ogden        = load_tex(&engine.resources, "assets/sprites/npcs/ogden.jpg");
    tex_cain         = load_tex(&engine.resources, "assets/sprites/npcs/cain.jpg");
    tex_npc_fallback = load_tex(&engine.resources, "assets/sprites/player/warrior_test.jpg");

    /* Load HUD font — try several system font paths */
    const char *font_paths[] = {
        "/System/Library/Fonts/Monaco.ttf",
        "/System/Library/Fonts/Geneva.ttf",
        "/Library/Fonts/Arial Unicode.ttf",
        "/System/Library/Fonts/Helvetica.ttc",
        NULL,
    };
    int font_idx = -1;
    for (int i = 0; font_paths[i] != NULL; i++) {
        font_idx = resource_load_font(&engine.resources, font_paths[i], 14);
        if (font_idx >= 0) break;
    }
    TTF_Font *hud_font = resource_get_font(&engine.resources, font_idx);
    if (!hud_font)
        fprintf(stderr, "Warning: No font loaded, HUD text disabled\n");

    /* Initialize subsystems */
    game_init(&game);
    camera_init(&camera, CAMERA_SPEED);
    iso_renderer_init(&iso_renderer, engine.renderer, SCREEN_WIDTH);
    input_init(&input);
    ui_init(&ui, engine.renderer, hud_font);

    /* Initialize one-time databases (survive across new game / load) */
    item_db_init(&item_db);
    enemy_defs_init();
    dungeon_theme_init();

    /* Initialize player and game state (will be re-initialized on New Game) */
    Player player;
    player_init(&player, town.player_start_x, town.player_start_y);
    npc_defs_load(&npc_mgr, &town);
    relationship_init(&rel_graph);
    relationship_init_defaults(&rel_graph);
    memory_system_init(&mem_system);
    event_queue_init(&event_queue);
    dialogue_init(&dialogue);
    quest_log_init(&quest_log);
    story_arc_init(&story_arcs);
    inventory_init(&inventory);
    enemy_manager_init(&enemy_mgr);

    /* Start at title screen */
    current_scene = SCENE_TITLE;
    title_selection = 0;
    audio_play_music(&audio, MUSIC_TITLE);

    /* ---- Game loop ---- */
    while (engine.running) {
        engine_begin_frame(&engine);
        float dt = engine.delta_time;

        bool in_gameplay = (current_scene == SCENE_TOWN || current_scene == SCENE_DUNGEON);
        const TileMap *active_map = NULL;
        if (in_gameplay)
            active_map = (current_scene == SCENE_TOWN) ? &town.map : &current_dungeon.map;

        bool in_dialogue = in_gameplay && dialogue_is_active(&dialogue);

        /* Snapshot previous input state before polling events */
        input_update(&input);

        /* Event handling */
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            switch (event.type) {
            case SDL_QUIT:
                engine.running = false;
                break;

            case SDL_KEYDOWN: {
                SDL_Keycode key = event.key.keysym.sym;

                /* ---- TITLE SCREEN ---- */
                if (current_scene == SCENE_TITLE) {
                    bool has_save = save_exists(SAVE_FILE_PATH);
                    int menu_count = has_save ? 3 : 2;  /* New Game, [Continue], Quit */

                    if (key == SDLK_UP) {
                        title_selection--;
                        if (title_selection < 0) title_selection = menu_count - 1;
                        audio_play_sfx(&audio, SFX_UI_CLICK);
                    }
                    if (key == SDLK_DOWN) {
                        title_selection++;
                        if (title_selection >= menu_count) title_selection = 0;
                        audio_play_sfx(&audio, SFX_UI_CLICK);
                    }
                    if (key == SDLK_RETURN) {
                        audio_play_sfx(&audio, SFX_UI_CLICK);
                        int action = title_selection;
                        /* If no save, "Quit" is at index 1 instead of 2 */
                        if (!has_save && action >= 1) action++;

                        if (action == 0) {
                            /* New Game */
                            reset_game_state(&game, &player, &town, &camera);
                            current_scene = SCENE_TOWN;
                            current_dungeon_level = 0;
                            audio_play_music(&audio, MUSIC_TOWN);
                            effects.fog_enabled = false;
                        } else if (action == 1) {
                            /* Continue (load save) */
                            reset_game_state(&game, &player, &town, &camera);
                            SaveData save_data;
                            if (load_game(SAVE_FILE_PATH, &save_data)) {
                                int loaded_scene = 0, loaded_dlevel = 0;
                                save_unpack(&save_data, &game, &player, &inventory,
                                            &npc_mgr, &rel_graph, &mem_system,
                                            &quest_log, game_flags, &game_flag_count,
                                            &loaded_scene, &loaded_dlevel);
                                prev_game_hour = game.game_hour;
                                if (loaded_scene == 1 && loaded_dlevel > 0) {
                                    /* Restore to dungeon */
                                    current_scene = SCENE_DUNGEON;
                                    current_dungeon_level = loaded_dlevel;
                                    dungeon_generate_level(&current_dungeon, loaded_dlevel,
                                                           (unsigned)(game.game_day * 1000 + loaded_dlevel));
                                    enemy_manager_init(&enemy_mgr);
                                    dungeon_populate_enemies(&current_dungeon, &enemy_mgr);
                                    effects.fog_enabled = true;
                                    audio_play_music(&audio, dungeon_music_for_theme(current_dungeon.theme));
                                } else {
                                    /* Restore to town */
                                    current_scene = SCENE_TOWN;
                                    current_dungeon_level = 0;
                                    enemy_manager_init(&enemy_mgr);
                                    enemy_spawn_group(&enemy_mgr, ENEMY_FALLEN, 28, 8, 3, &town.map);
                                    enemy_spawn_group(&enemy_mgr, ENEMY_SKELETON, 30, 10, 2, &town.map);
                                    effects.fog_enabled = false;
                                    audio_play_music(&audio, MUSIC_TOWN);
                                }
                                camera_center_on_tile(&camera, player.tile_x, player.tile_y);
                                camera.x = camera.target_x;
                                camera.y = camera.target_y;
                            }
                        } else {
                            /* Quit */
                            engine.running = false;
                        }
                    }
                    if (key == SDLK_ESCAPE) {
                        engine.running = false;
                    }
                    break;
                }

                /* ---- DEATH SCREEN ---- */
                if (current_scene == SCENE_DEATH) {
                    bool has_save = save_exists(SAVE_FILE_PATH);
                    int menu_count = has_save ? 2 : 1;

                    if (key == SDLK_UP) {
                        death_selection--;
                        if (death_selection < 0) death_selection = menu_count - 1;
                        audio_play_sfx(&audio, SFX_UI_CLICK);
                    }
                    if (key == SDLK_DOWN) {
                        death_selection++;
                        if (death_selection >= menu_count) death_selection = 0;
                        audio_play_sfx(&audio, SFX_UI_CLICK);
                    }
                    if (key == SDLK_RETURN) {
                        audio_play_sfx(&audio, SFX_UI_CLICK);
                        int action = death_selection;
                        if (!has_save) action++;  /* skip "Load Save" */

                        if (action == 0) {
                            /* Load Save */
                            reset_game_state(&game, &player, &town, &camera);
                            SaveData save_data;
                            if (load_game(SAVE_FILE_PATH, &save_data)) {
                                int loaded_scene = 0, loaded_dlevel = 0;
                                save_unpack(&save_data, &game, &player, &inventory,
                                            &npc_mgr, &rel_graph, &mem_system,
                                            &quest_log, game_flags, &game_flag_count,
                                            &loaded_scene, &loaded_dlevel);
                                prev_game_hour = game.game_hour;
                                if (loaded_scene == 1 && loaded_dlevel > 0) {
                                    current_scene = SCENE_DUNGEON;
                                    current_dungeon_level = loaded_dlevel;
                                    dungeon_generate_level(&current_dungeon, loaded_dlevel,
                                                           (unsigned)(game.game_day * 1000 + loaded_dlevel));
                                    enemy_manager_init(&enemy_mgr);
                                    dungeon_populate_enemies(&current_dungeon, &enemy_mgr);
                                    effects.fog_enabled = true;
                                    audio_play_music(&audio, dungeon_music_for_theme(current_dungeon.theme));
                                } else {
                                    current_scene = SCENE_TOWN;
                                    current_dungeon_level = 0;
                                    enemy_manager_init(&enemy_mgr);
                                    enemy_spawn_group(&enemy_mgr, ENEMY_FALLEN, 28, 8, 3, &town.map);
                                    enemy_spawn_group(&enemy_mgr, ENEMY_SKELETON, 30, 10, 2, &town.map);
                                    effects.fog_enabled = false;
                                    audio_play_music(&audio, MUSIC_TOWN);
                                }
                                camera_center_on_tile(&camera, player.tile_x, player.tile_y);
                                camera.x = camera.target_x;
                                camera.y = camera.target_y;
                            }
                        } else {
                            /* Quit to Title */
                            current_scene = SCENE_TITLE;
                            title_selection = 0;
                            audio_play_music(&audio, MUSIC_TITLE);
                        }
                    }
                    break;
                }

                /* ---- PAUSE MENU ---- */
                if (current_scene == SCENE_PAUSED) {
                    if (key == SDLK_UP) {
                        pause_selection--;
                        if (pause_selection < 0) pause_selection = 2;
                        audio_play_sfx(&audio, SFX_UI_CLICK);
                    }
                    if (key == SDLK_DOWN) {
                        pause_selection++;
                        if (pause_selection > 2) pause_selection = 0;
                        audio_play_sfx(&audio, SFX_UI_CLICK);
                    }
                    if (key == SDLK_RETURN) {
                        audio_play_sfx(&audio, SFX_UI_CLICK);
                        if (pause_selection == 0) {
                            /* Resume */
                            current_scene = scene_before_pause;
                        } else if (pause_selection == 1) {
                            /* Save Game */
                            int save_scene = (scene_before_pause == SCENE_DUNGEON) ? 1 : 0;
                            SaveData save_data;
                            save_pack(&save_data, &game, &player, &inventory,
                                      &npc_mgr, &rel_graph, &mem_system,
                                      &quest_log, game_flags, game_flag_count,
                                      save_scene, current_dungeon_level);
                            save_game(SAVE_FILE_PATH, &save_data);
                        } else {
                            /* Quit to Title */
                            current_scene = SCENE_TITLE;
                            title_selection = 0;
                            audio_play_music(&audio, MUSIC_TITLE);
                        }
                    }
                    if (key == SDLK_ESCAPE) {
                        /* Escape also resumes */
                        current_scene = scene_before_pause;
                    }
                    break;
                }

                /* ---- GAMEPLAY (TOWN / DUNGEON) ---- */

                /* Escape: close overlays in priority order, or pause */
                if (key == SDLK_ESCAPE) {
                    if (in_dialogue) {
                        /* Force-close dialogue */
                        dialogue.active = false;
                        dialogue.current_node_id = -1;
                        dialogue.talking_to_npc_id = -1;
                        in_dialogue = false;
                    } else if (show_inventory) {
                        show_inventory = false;
                    } else if (show_quest_log) {
                        show_quest_log = false;
                    } else if (show_character_screen) {
                        show_character_screen = false;
                    } else {
                        /* Open pause menu */
                        scene_before_pause = current_scene;
                        current_scene = SCENE_PAUSED;
                        pause_selection = 0;
                    }
                    break;
                }

                /* During active dialogue: navigate choices or confirm */
                if (in_dialogue) {
                    const DialogueNode *cnode = dialogue_current_node(&dialogue);
                    if (cnode) {
                        int tlen = (int)strlen(cnode->text);
                        bool text_done = dialogue.chars_shown >= tlen;

                        if (text_done && cnode->choice_count > 0) {
                            if (key == SDLK_UP || key == SDLK_w) {
                                dialogue.selected_choice--;
                                if (dialogue.selected_choice < 0)
                                    dialogue.selected_choice = cnode->choice_count - 1;
                            }
                            if (key == SDLK_DOWN || key == SDLK_s) {
                                dialogue.selected_choice++;
                                if (dialogue.selected_choice >= cnode->choice_count)
                                    dialogue.selected_choice = 0;
                            }
                        }

                        if (key == SDLK_RETURN) {
                            dialogue_confirm_choice(&dialogue, &game, &npc_mgr,
                                                    &event_queue, &rel_graph,
                                                    game_flags);
                        }
                    }
                    break;  /* Consume all keys during dialogue */
                }

                /* Normal key bindings (not in dialogue) */
                if (key == SDLK_q)
                    show_quest_log = !show_quest_log;
                if (key == SDLK_c)
                    show_character_screen = !show_character_screen;
                if (key == SDLK_i)
                    show_inventory = !show_inventory;
                if (key == SDLK_F1)
                    show_debug = !show_debug;

                /* Number keys 1-4: quick-use potions from inventory */
                if (key >= SDLK_1 && key <= SDLK_4) {
                    int belt_slot = key - SDLK_1;
                    /* Find nth consumable in inventory */
                    int found = 0;
                    for (int s = 0; s < INVENTORY_SIZE; s++) {
                        if (inventory.slots[s].type == ITEM_POTION_HP ||
                            inventory.slots[s].type == ITEM_POTION_MANA) {
                            if (found == belt_slot) {
                                int hp_before = player.stats.current_hp;
                                int mana_before = player.stats.current_mana;
                                inventory_use_item(&inventory, s, &player.stats);
                                audio_play_sfx(&audio, SFX_POTION);
                                /* Heal particle effect */
                                if (player.stats.current_hp > hp_before ||
                                    player.stats.current_mana > mana_before) {
                                    int psx, psy;
                                    player_get_screen_pos(&player, &psx, &psy);
                                    psx = psx - (int)camera.x + SCREEN_WIDTH / 2;
                                    psy = psy - (int)camera.y;
                                    effects_spawn_heal(&effects, psx, psy);
                                }
                                break;
                            }
                            found++;
                        }
                    }
                }
                break;
            }

            case SDL_MOUSEBUTTONDOWN:
                if (!in_gameplay) break;

                /* Right-click: talk to NPC */
                if (event.button.button == SDL_BUTTON_RIGHT && !in_dialogue &&
                    current_scene == SCENE_TOWN) {
                    NPC *clicked_npc = npc_manager_at_tile(&npc_mgr,
                                                           hover_tile_x,
                                                           hover_tile_y);
                    if (clicked_npc) {
                        dialogue_start(&dialogue, clicked_npc->id);
                        in_dialogue = dialogue_is_active(&dialogue);
                    }
                }

                /* Left-click */
                if (event.button.button == SDL_BUTTON_LEFT && !player_dead) {
                    if (in_dialogue) {
                        /* Confirm choice or skip typewriter */
                        dialogue_confirm_choice(&dialogue, &game, &npc_mgr,
                                                &event_queue, &rel_graph,
                                                game_flags);
                    } else if (!show_character_screen && !show_quest_log && !show_inventory) {
                        /* Check if clicking on an enemy tile (melee attack) */
                        Enemy *target_enemy = enemy_at_tile(&enemy_mgr, hover_tile_x, hover_tile_y);
                        if (target_enemy && target_enemy->alive &&
                            combat_in_range(player.tile_x, player.tile_y,
                                            hover_tile_x, hover_tile_y, 1)) {
                            /* Player attacks the enemy */
                            audio_play_sfx(&audio, SFX_SWORD_SWING);
                            CombatResult cr = combat_player_attack(&player, target_enemy, &inventory);

                            if (cr.hit) {
                                audio_play_sfx(&audio, SFX_HIT);
                                /* Blood at enemy position */
                                int esx = (int)target_enemy->world_x - (int)camera.x + SCREEN_WIDTH / 2;
                                int esy = (int)target_enemy->world_y - (int)camera.y;
                                effects_spawn_blood(&effects, esx, esy);
                            }

                            /* Check for level up from XP gained */
                            if (cr.xp_gained > 0 && cr.killed) {
                                /* Level up check — compare level before/after */
                                /* stats_add_xp was already called in combat_player_attack */
                            }

                            /* Drop loot if killed */
                            if (!target_enemy->alive) {
                                audio_play_sfx(&audio, SFX_ENEMY_DIE);
                                /* Drop gold */
                                if (target_enemy->gold_max > 0) {
                                    int gold_amt = target_enemy->gold_min +
                                        rand() % (target_enemy->gold_max - target_enemy->gold_min + 1);
                                    inventory.gold += gold_amt;
                                }
                                /* Chance to drop a random item (30%) */
                                if (rand() % 100 < 30) {
                                    Item loot = item_create_random(&item_db, 1, player.stats.level + 2);
                                    if (loot.type != ITEM_NONE)
                                        drop_ground_item(&loot, target_enemy->tile_x, target_enemy->tile_y);
                                }
                            }
                        } else if (active_map && hover_tile_x >= 0 && hover_tile_y >= 0 &&
                                   tilemap_in_bounds(active_map, hover_tile_x, hover_tile_y) &&
                                   tilemap_is_walkable(active_map, hover_tile_x, hover_tile_y)) {
                            /* Click-to-move: pathfind to hovered tile */
                            player_move_to(&player, active_map, hover_tile_x, hover_tile_y);
                        }
                    }
                }
                break;
            }
        }

        /* ---- UPDATE (gameplay scenes only) ---- */
        if (in_gameplay && !player_dead) {
            /* Camera scroll from keyboard input (disabled during dialogue) */
            if (!in_dialogue) {
                float scroll_speed = camera.speed * dt;
                if (input_key_held(&input, SDL_SCANCODE_LEFT) || input_key_held(&input, SDL_SCANCODE_A))
                    camera_scroll(&camera, -scroll_speed, 0);
                if (input_key_held(&input, SDL_SCANCODE_RIGHT) || input_key_held(&input, SDL_SCANCODE_D))
                    camera_scroll(&camera, scroll_speed, 0);
                if (input_key_held(&input, SDL_SCANCODE_UP) || input_key_held(&input, SDL_SCANCODE_W))
                    camera_scroll(&camera, 0, -scroll_speed);
                if (input_key_held(&input, SDL_SCANCODE_DOWN) || input_key_held(&input, SDL_SCANCODE_S))
                    camera_scroll(&camera, 0, scroll_speed);
            }

            /* Update dialogue typewriter */
            if (dialogue_is_active(&dialogue))
                dialogue_update(&dialogue, dt);

            /* Update player movement */
            player_update(&player, dt, active_map);

            /* Update enemies (AI + movement) */
            for (int i = 0; i < enemy_mgr.count; i++) {
                if (enemy_mgr.enemies[i].alive) {
                    enemy_ai_update(&enemy_mgr.enemies[i], dt,
                                    player.tile_x, player.tile_y, active_map);
                }
            }
            enemy_manager_update(&enemy_mgr, dt, active_map);

            /* Combat: enemies attack player — track HP for effects */
            int hp_before_combat = player.stats.current_hp;
            int level_before = player.stats.level;
            combat_update(&enemy_mgr, &player, &inventory, dt);

            /* Player took damage from enemies */
            if (player.stats.current_hp < hp_before_combat) {
                audio_play_sfx(&audio, SFX_PLAYER_HIT);
                effects_flash(&effects, (SDL_Color){ 200, 30, 30, 255 }, 0.3f);
                int psx, psy;
                player_get_screen_pos(&player, &psx, &psy);
                psx = psx - (int)camera.x + SCREEN_WIDTH / 2;
                psy = psy - (int)camera.y;
                effects_spawn_blood(&effects, psx, psy);
            }

            /* Level up detection */
            if (player.stats.level > level_before) {
                audio_play_sfx(&audio, SFX_LEVEL_UP);
                int psx, psy;
                player_get_screen_pos(&player, &psx, &psy);
                psx = psx - (int)camera.x + SCREEN_WIDTH / 2;
                psy = psy - (int)camera.y;
                effects_spawn_levelup(&effects, psx, psy);
            }

            /* Check player death */
            if (player.stats.current_hp <= 0) {
                player_dead = true;
                audio_play_sfx(&audio, SFX_PLAYER_DIE);
                audio_stop_music(&audio);
                current_scene = SCENE_DEATH;
                death_selection = 0;
            }

            /* Loot pickup: player walks over ground items */
            for (int i = 0; i < ground_item_count; i++) {
                if (ground_items[i].active &&
                    ground_items[i].tile_x == player.tile_x &&
                    ground_items[i].tile_y == player.tile_y) {
                    if (inventory_add_item(&inventory, &ground_items[i].item)) {
                        ground_items[i].active = false;
                        audio_play_sfx(&audio, SFX_PICKUP);
                    }
                }
            }

            /* Level transitions — check when player stops on stairs */
            if (!player.moving && active_map) {
                TileType standing_on = tilemap_get_type(active_map, player.tile_x, player.tile_y);
                if (standing_on == TILE_STAIRS_DOWN) {
                    audio_play_sfx(&audio, SFX_DOOR_OPEN);
                    if (current_scene == SCENE_TOWN) {
                        /* Enter dungeon level 1 */
                        current_scene = SCENE_DUNGEON;
                        current_dungeon_level = 1;
                        dungeon_generate_level(&current_dungeon, 1,
                                               (unsigned)(game.game_day * 1000 + 1));
                        enemy_manager_init(&enemy_mgr);
                        dungeon_populate_enemies(&current_dungeon, &enemy_mgr);
                        player_warp(&player, current_dungeon.stairs_up_x,
                                    current_dungeon.stairs_up_y);
                        camera_center_on_tile(&camera, player.tile_x, player.tile_y);
                        camera.x = camera.target_x;
                        camera.y = camera.target_y;
                        ground_item_count = 0;
                        effects.fog_enabled = true;
                        audio_play_music(&audio, dungeon_music_for_theme(current_dungeon.theme));
                    } else {
                        /* Go deeper in dungeon */
                        current_dungeon_level++;
                        dungeon_generate_level(&current_dungeon, current_dungeon_level,
                                               (unsigned)(game.game_day * 1000 + current_dungeon_level));
                        enemy_manager_init(&enemy_mgr);
                        dungeon_populate_enemies(&current_dungeon, &enemy_mgr);
                        player_warp(&player, current_dungeon.stairs_up_x,
                                    current_dungeon.stairs_up_y);
                        camera_center_on_tile(&camera, player.tile_x, player.tile_y);
                        camera.x = camera.target_x;
                        camera.y = camera.target_y;
                        ground_item_count = 0;
                        audio_play_music(&audio, dungeon_music_for_theme(current_dungeon.theme));
                    }
                } else if (standing_on == TILE_STAIRS_UP && current_scene == SCENE_DUNGEON) {
                    audio_play_sfx(&audio, SFX_DOOR_OPEN);
                    if (current_dungeon_level <= 1) {
                        /* Return to town */
                        current_scene = SCENE_TOWN;
                        current_dungeon_level = 0;
                        enemy_manager_init(&enemy_mgr);
                        enemy_spawn_group(&enemy_mgr, ENEMY_FALLEN, 28, 8, 3, &town.map);
                        enemy_spawn_group(&enemy_mgr, ENEMY_SKELETON, 30, 10, 2, &town.map);
                        player_warp(&player, 32, 9);  /* cathedral entrance */
                        camera_center_on_tile(&camera, player.tile_x, player.tile_y);
                        camera.x = camera.target_x;
                        camera.y = camera.target_y;
                        ground_item_count = 0;
                        effects.fog_enabled = false;
                        audio_play_music(&audio, MUSIC_TOWN);
                    } else {
                        /* Go up one dungeon level */
                        current_dungeon_level--;
                        dungeon_generate_level(&current_dungeon, current_dungeon_level,
                                               (unsigned)(game.game_day * 1000 + current_dungeon_level));
                        enemy_manager_init(&enemy_mgr);
                        dungeon_populate_enemies(&current_dungeon, &enemy_mgr);
                        player_warp(&player, current_dungeon.stairs_down_x,
                                    current_dungeon.stairs_down_y);
                        camera_center_on_tile(&camera, player.tile_x, player.tile_y);
                        camera.x = camera.target_x;
                        camera.y = camera.target_y;
                        ground_item_count = 0;
                        audio_play_music(&audio, dungeon_music_for_theme(current_dungeon.theme));
                    }
                }
            }

            /* Update NPC movement — town only */
            if (current_scene == SCENE_TOWN) {
                npc_manager_update(&npc_mgr, dt, game.game_hour, &town.map);

                /* Hourly NPC AI update */
                if (game.game_hour != prev_game_hour) {
                    npc_ai_update_all(&npc_mgr, game.game_hour, game.game_day,
                                      &town, &rel_graph, &mem_system, &event_queue);
                    npc_process_interactions(&npc_mgr, &rel_graph, &mem_system,
                                            &event_queue, game.game_day);
                    event_process_all(&event_queue, &npc_mgr, &rel_graph,
                                      &mem_system, game.game_day);
                    memory_decay(&mem_system, game.game_day);
                    story_arc_update(&story_arcs, &npc_mgr, &quest_log, game.game_day);
                    prev_game_hour = game.game_hour;
                }
            }

            /* Camera follows player (smooth) */
            if (!input_key_held(&input, SDL_SCANCODE_LEFT) &&
                !input_key_held(&input, SDL_SCANCODE_RIGHT) &&
                !input_key_held(&input, SDL_SCANCODE_UP) &&
                !input_key_held(&input, SDL_SCANCODE_DOWN) &&
                !input_key_held(&input, SDL_SCANCODE_A) &&
                !input_key_held(&input, SDL_SCANCODE_D) &&
                !input_key_held(&input, SDL_SCANCODE_W) &&
                !input_key_held(&input, SDL_SCANCODE_S)) {
                camera_center_on_tile(&camera, player.tile_x, player.tile_y);
            }

            camera_update(&camera, dt);
            game_update(&game, dt);

            /* Update active_map pointer after possible scene transitions */
            active_map = (current_scene == SCENE_TOWN) ? &town.map : &current_dungeon.map;
        }

        /* Update effects (always, for particle fade-out even on menus) */
        effects_update(&effects, dt, in_gameplay ? game.game_hour : 12);

        /* Mouse tile picking (gameplay only) */
        if (in_gameplay && input.mouse_y < VIEWPORT_HEIGHT) {
            int world_x, world_y;
            camera_screen_to_world(&camera, input.mouse_x, input.mouse_y,
                                   SCREEN_WIDTH / 2, &world_x, &world_y);
            screen_to_iso(world_x, world_y, &hover_tile_x, &hover_tile_y);
        } else {
            hover_tile_x = -1;
            hover_tile_y = -1;
        }

        /* ---- Render ---- */

        if (current_scene == SCENE_TITLE) {
            /* Title screen */
            draw_title_screen(&ui, engine.renderer);
            engine_end_frame(&engine);
            continue;
        }

        /* For gameplay scenes (town, dungeon) and overlays (paused, death) */
        /* Determine which map to render */
        const TileMap *render_map = NULL;
        SceneType render_scene = current_scene;
        if (current_scene == SCENE_PAUSED)
            render_scene = scene_before_pause;
        if (current_scene == SCENE_DEATH)
            render_scene = player_dead ? scene_before_pause : current_scene;

        if (render_scene == SCENE_TOWN)
            render_map = &town.map;
        else if (render_scene == SCENE_DUNGEON)
            render_map = &current_dungeon.map;

        if (!render_map) {
            engine_end_frame(&engine);
            continue;
        }

        int cam_x = (int)camera.x;
        int cam_y = (int)camera.y;

        /* Set clip rect to viewport (above the UI panel) */
        SDL_Rect viewport_clip = { 0, 0, SCREEN_WIDTH, VIEWPORT_HEIGHT };
        SDL_RenderSetClipRect(engine.renderer, &viewport_clip);

        /* Dark viewport background */
        SDL_SetRenderDrawColor(engine.renderer, 15, 12, 20, 255);
        SDL_RenderFillRect(engine.renderer, &viewport_clip);

        /* Draw tiles back-to-front (painter's algorithm) with frustum culling */
        for (int ty = 0; ty < render_map->height; ty++) {
            for (int tx = 0; tx < render_map->width; tx++) {
                if (!tile_visible(tx, ty, cam_x, cam_y))
                    continue;

                TileType type = tilemap_get_type(render_map, tx, ty);
                SDL_Texture *floor_tex = tile_textures[type];

                /* Always draw the floor layer */
                if (floor_tex)
                    iso_draw_tile(&iso_renderer, floor_tex, tx, ty, cam_x, cam_y);

                /* Elevated tiles: walls and trees get a wall block on top */
                if (type == TILE_WALL) {
                    if (tex_wall)
                        iso_draw_wall(&iso_renderer, tex_wall, tx, ty, cam_x, cam_y, TILE_HEIGHT);
                } else if (type == TILE_TREE) {
                    /* Draw tree sprite on top of grass base */
                    SDL_Texture *tree_overlay = tex_tree ? tex_tree : tex_wall;
                    if (tree_overlay)
                        iso_draw_wall(&iso_renderer, tree_overlay, tx, ty, cam_x, cam_y, TILE_HEIGHT);
                }
            }
        }

        /* Path visualization: highlight planned path tiles */
        if (!path_is_complete(&player.path)) {
            SDL_Color path_color = { 100, 180, 255, 50 };
            for (int i = player.path.current_step; i < player.path.length; i++) {
                int px = player.path.points[i].x;
                int py = player.path.points[i].y;
                if (tile_visible(px, py, cam_x, cam_y))
                    iso_draw_tile_highlight(&iso_renderer, px, py, cam_x, cam_y, path_color);
            }
        }

        /* Tile highlight under mouse cursor */
        if (hover_tile_x >= 0 && hover_tile_y >= 0 &&
            tilemap_in_bounds(render_map, hover_tile_x, hover_tile_y)) {
            SDL_Color highlight;
            if (tilemap_is_walkable(render_map, hover_tile_x, hover_tile_y))
                highlight = (SDL_Color){ 255, 200, 50, 80 };
            else
                highlight = (SDL_Color){ 255, 60, 60, 80 };
            iso_draw_tile_highlight(&iso_renderer, hover_tile_x, hover_tile_y,
                                    cam_x, cam_y, highlight);
        }

        /* Player sprite — use interpolated world position */
        {
            int draw_sx, draw_sy;
            player_get_screen_pos(&player, &draw_sx, &draw_sy);
            draw_sx = draw_sx - cam_x + (SCREEN_WIDTH / 2) - SPRITE_SIZE / 2;
            draw_sy = draw_sy - cam_y - SPRITE_SIZE + TILE_HALF_H;
            if (tex_warrior) {
                SDL_Rect dst = { draw_sx, draw_sy, SPRITE_SIZE, SPRITE_SIZE };
                SDL_RenderCopy(engine.renderer, tex_warrior, NULL, &dst);
            }
        }

        /* Draw enemies */
        for (int i = 0; i < enemy_mgr.count; i++) {
            Enemy *e = &enemy_mgr.enemies[i];
            if (!e->alive)
                continue;

            int e_sx = (int)e->world_x - cam_x + (SCREEN_WIDTH / 2) - SPRITE_SIZE / 2;
            int e_sy = (int)e->world_y - cam_y - SPRITE_SIZE + TILE_HALF_H;

            if (tex_warrior) {
                /* Tint enemies red */
                SDL_SetTextureColorMod(tex_warrior, 255, 80, 80);
                SDL_Rect dst = { e_sx, e_sy, SPRITE_SIZE, SPRITE_SIZE };
                SDL_RenderCopy(engine.renderer, tex_warrior, NULL, &dst);
                /* Reset tint */
                SDL_SetTextureColorMod(tex_warrior, 255, 255, 255);
            }

            /* HP bar above enemy */
            draw_enemy_hp_bar(engine.renderer, e_sx, e_sy, e->current_hp, e->max_hp);

            /* Show enemy name on hover */
            if (hover_tile_x == e->tile_x && hover_tile_y == e->tile_y) {
                char label[48];
                snprintf(label, sizeof(label), "%s (HP: %d/%d)",
                         e->name, e->current_hp, e->max_hp);
                int label_x = e_sx + SPRITE_SIZE / 2 - 50;
                int label_y = e_sy - 14;
                ui_draw_text(&ui, label, label_x, label_y,
                             (SDL_Color){ 255, 100, 100, 255 });
            }
        }

        /* Draw ground items (loot) */
        for (int i = 0; i < ground_item_count; i++) {
            if (!ground_items[i].active)
                continue;
            int gx = ground_items[i].tile_x;
            int gy = ground_items[i].tile_y;
            if (!tile_visible(gx, gy, cam_x, cam_y))
                continue;
            /* Draw a small gold highlight on the tile */
            SDL_Color loot_color = { 255, 215, 0, 100 };
            iso_draw_tile_highlight(&iso_renderer, gx, gy, cam_x, cam_y, loot_color);
        }

        /* Draw NPC sprites — town only */
        if (render_scene == SCENE_TOWN)
        for (int i = 0; i < npc_mgr.count; i++) {
            NPC *npc = &npc_mgr.npcs[i];
            if (!npc->is_alive || npc->has_left_town)
                continue;

            /* NPC world_x/world_y are isometric screen coords, same as player */
            int npc_sx = (int)npc->world_x - cam_x + (SCREEN_WIDTH / 2) - SPRITE_SIZE / 2;
            int npc_sy = (int)npc->world_y - cam_y - SPRITE_SIZE + TILE_HALF_H;

            SDL_Texture *npc_tex = get_npc_texture(npc->id);
            if (npc_tex) {
                SDL_Rect dst = { npc_sx, npc_sy, SPRITE_SIZE, SPRITE_SIZE };
                SDL_RenderCopy(engine.renderer, npc_tex, NULL, &dst);
            }

            /* Show NPC name/title when hovering over their tile */
            if (hover_tile_x == npc->tile_x && hover_tile_y == npc->tile_y) {
                char npc_label[MAX_NPC_NAME + MAX_NPC_TITLE + 8];
                snprintf(npc_label, sizeof(npc_label), "%s - %s", npc->name, npc->title);
                int label_x = npc_sx + SPRITE_SIZE / 2 - 40;
                int label_y = npc_sy - 14;
                ui_draw_text(&ui, npc_label, label_x, label_y,
                             (SDL_Color){ 255, 220, 100, 255 });
            }
        }

        /* ---- Effects rendering (after sprites, before overlays) ---- */

        /* Fog of war — dungeon only */
        if (render_scene == SCENE_DUNGEON) {
            int psx, psy;
            player_get_screen_pos(&player, &psx, &psy);
            psx = psx - cam_x + SCREEN_WIDTH / 2;
            psy = psy - cam_y;
            effects_render_fog(&effects, engine.renderer, psx, psy,
                               SCREEN_WIDTH, VIEWPORT_HEIGHT);
        }

        /* Day/night tint — town only */
        if (render_scene == SCENE_TOWN)
            effects_render_day_night(&effects, engine.renderer, SCREEN_WIDTH, VIEWPORT_HEIGHT);

        /* Particles (blood, heal, level up) */
        effects_render_particles(&effects, engine.renderer);

        /* Screen flash (damage) */
        effects_render_flash(&effects, engine.renderer, SCREEN_WIDTH, VIEWPORT_HEIGHT);

        /* Character screen overlay (drawn on viewport before releasing clip) */
        if (show_character_screen)
            draw_character_screen(&ui, engine.renderer, &player.stats);

        /* Quest log overlay (drawn on viewport) */
        if (show_quest_log)
            draw_quest_log(&ui, engine.renderer, &quest_log);

        /* Inventory overlay */
        if (show_inventory)
            draw_inventory_screen(&ui, engine.renderer, &inventory);

        /* F1 Debug overlay — NPC debug is town only */
        if (show_debug && render_scene == SCENE_TOWN)
            draw_debug_overlay(&ui, engine.renderer, &npc_mgr, &rel_graph, &mem_system);

        SDL_RenderSetClipRect(engine.renderer, NULL);

        /* ---- HUD panel (always last) ---- */
        ui_draw_panel(&ui, VIEWPORT_HEIGHT, PANEL_HEIGHT, SCREEN_WIDTH);
        ui_draw_fps(&ui, engine.fps);

        /* Enhanced HUD: HP/MP bars and level */
        draw_hud(&ui, engine.renderer, &player.stats,
                 VIEWPORT_HEIGHT, PANEL_HEIGHT, SCREEN_WIDTH);

        /* HUD info: context-dependent */
        {
            char info[256];
            if (render_scene == SCENE_TOWN) {
                snprintf(info, sizeof(info),
                         "Tile: (%d,%d)  Player: (%d,%d)  Day %d %02d:00  Enemies: %d  Gold: %d",
                         hover_tile_x, hover_tile_y,
                         player.tile_x, player.tile_y,
                         game.game_day, game.game_hour,
                         enemy_count_alive(&enemy_mgr), inventory.gold);
            } else {
                const DungeonTheme *dtheme = dungeon_theme_get(current_dungeon.theme);
                snprintf(info, sizeof(info),
                         "Level %d - %s  Enemies: %d  Gold: %d",
                         current_dungeon_level,
                         dtheme ? dtheme->name : "Unknown",
                         enemy_count_alive(&enemy_mgr), inventory.gold);
            }
            ui_draw_panel_info(&ui, VIEWPORT_HEIGHT, info);
        }

        /* Dialogue panel overlays the HUD when a conversation is active (town only) */
        if (render_scene == SCENE_TOWN)
            draw_dialogue_panel(&ui, engine.renderer, &dialogue, &npc_mgr);

        /* ---- Scene overlays (drawn on top of everything) ---- */
        if (current_scene == SCENE_PAUSED)
            draw_pause_menu(&ui, engine.renderer);

        if (current_scene == SCENE_DEATH)
            draw_death_screen(&ui, engine.renderer);

        engine_end_frame(&engine);
    }

    /* Cleanup */
    effects_cleanup(&effects);
    audio_shutdown(&audio);
    engine_shutdown(&engine);
    return 0;
}
