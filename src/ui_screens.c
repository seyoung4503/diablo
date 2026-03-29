#include "ui_screens.h"

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

/* NPC action name for debug display */
const char *action_name(NPCAction a)
{
    static const char *names[] = {
        "Idle", "Work", "Social", "Eat", "Sleep",
        "Pray", "Drink", "Patrol", "Wander"
    };
    if (a >= 0 && a < NPC_ACTION_COUNT) return names[a];
    return "???";
}

/* Town location name for debug display */
const char *location_name(NPCLocation loc)
{
    static const char *names[] = {
        "Square", "Smith", "Healer", "Tavern", "Church",
        "Well", "Grave", "Adria", "Tremain", "Cain", "Wirt"
    };
    if (loc >= 0 && loc < LOC_COUNT) return names[loc];
    return "???";
}

/* Mood color: green=happy, yellow=neutral, red=bad */
SDL_Color mood_color(float mood)
{
    if (mood > 0.3f) return (SDL_Color){ 100, 220, 100, 255 };
    if (mood > -0.3f) return (SDL_Color){ 220, 220, 100, 255 };
    return (SDL_Color){ 220, 80, 80, 255 };
}

/* Trait name for debug display */
const char *trait_name(int idx)
{
    static const char *names[] = { "OPN", "CON", "EXT", "AGR", "NEU" };
    if (idx >= 0 && idx < TRAIT_COUNT) return names[idx];
    return "?";
}

/* Memory event type name for debug display */
const char *memory_type_name(MemoryEventType t)
{
    static const char *names[] = {
        "Helped", "Betrayed", "Ignored", "NPC Died", "Insulted",
        "Praised", "Quest OK", "Quest Fail", "Good News", "Bad News",
        "Combat", "Gift", "Threat"
    };
    if (t >= 0 && t < MEM_EVENT_COUNT) return names[t];
    return "???";
}

/* Draw a filled bar with border (HP or MP) */
void draw_bar(SDL_Renderer *renderer, int x, int y, int w, int h,
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
void draw_hud(UI *ui, SDL_Renderer *renderer, const PlayerStats *stats,
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
void draw_character_screen(UI *ui, SDL_Renderer *renderer, const PlayerStats *stats)
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
void draw_debug_overlay(UI *ui, SDL_Renderer *renderer,
                        const NPCManager *mgr,
                        const RelationshipGraph *graph,
                        const MemorySystem *memsys,
                        int hover_tx, int hover_ty)
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
            npc->tile_x == hover_tx && npc->tile_y == hover_ty) {
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
void draw_dialogue_panel(UI *ui, SDL_Renderer *renderer,
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
void draw_quest_log(UI *ui, SDL_Renderer *renderer,
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
void draw_enemy_hp_bar(SDL_Renderer *renderer, int sx, int sy,
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
void draw_inventory_screen(UI *ui, SDL_Renderer *renderer,
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
void draw_title_screen(UI *ui, SDL_Renderer *renderer, int selection)
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
        bool selected = (selection == item_idx);
        SDL_Color c = selected ? white : dim;
        char buf[32];
        snprintf(buf, sizeof(buf), "%s New Game", selected ? ">" : " ");
        ui_draw_text(ui, buf, SCREEN_WIDTH / 2 - 50, menu_y, c);
        menu_y += menu_spacing;
        item_idx++;
    }

    /* Continue (only if save exists) */
    if (has_save) {
        bool selected = (selection == item_idx);
        SDL_Color c = selected ? white : dim;
        char buf[32];
        snprintf(buf, sizeof(buf), "%s Continue", selected ? ">" : " ");
        ui_draw_text(ui, buf, SCREEN_WIDTH / 2 - 50, menu_y, c);
        menu_y += menu_spacing;
        item_idx++;
    }

    /* Quit */
    {
        bool selected = (selection == item_idx);
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
void draw_pause_menu(UI *ui, SDL_Renderer *renderer, int selection)
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
        bool selected = (selection == i);
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
void draw_death_screen(UI *ui, SDL_Renderer *renderer, int selection)
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
        bool selected = (selection == item_idx);
        SDL_Color c = selected ? white : dim;
        char buf[32];
        snprintf(buf, sizeof(buf), "%s Load Save", selected ? ">" : " ");
        ui_draw_text(ui, buf, SCREEN_WIDTH / 2 - 50, y, c);
        y += spacing;
        item_idx++;
    }

    {
        bool selected = (selection == item_idx);
        SDL_Color c = selected ? white : dim;
        char buf[32];
        snprintf(buf, sizeof(buf), "%s Quit to Title", selected ? ">" : " ");
        ui_draw_text(ui, buf, SCREEN_WIDTH / 2 - 60, y, c);
    }

    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
}
