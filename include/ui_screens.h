#ifndef DIABLO_UI_SCREENS_H
#define DIABLO_UI_SCREENS_H

#include "common.h"
#include "engine/ui.h"
#include "game/stats.h"
#include "game/inventory.h"
#include "game/save.h"
#include "world/town.h"
#include "npc/npc.h"
#include "npc/npc_relationship.h"
#include "npc/npc_memory.h"
#include "npc/npc_dialogue.h"
#include "story/quest.h"

/* Helper functions for debug display */
const char *action_name(NPCAction a);
const char *location_name(NPCLocation loc);
SDL_Color mood_color(float mood);
const char *trait_name(int idx);
const char *memory_type_name(MemoryEventType t);

/* Draw a filled bar with border (HP or MP) */
void draw_bar(SDL_Renderer *renderer, int x, int y, int w, int h,
              int current, int max, SDL_Color fill_color);

/* Draw the enhanced HUD with HP/MP bars and level */
void draw_hud(UI *ui, SDL_Renderer *renderer, const PlayerStats *stats,
              int viewport_h, int panel_h, int screen_w);

/* Draw character screen overlay */
void draw_character_screen(UI *ui, SDL_Renderer *renderer, const PlayerStats *stats);

/* Draw NPC debug overlay on the right side of viewport */
void draw_debug_overlay(UI *ui, SDL_Renderer *renderer,
                        const NPCManager *mgr,
                        const RelationshipGraph *graph,
                        const MemorySystem *memsys,
                        int hover_tx, int hover_ty);

/* Draw dialogue panel over the HUD area when a conversation is active */
void draw_dialogue_panel(UI *ui, SDL_Renderer *renderer,
                         const DialogueSystem *ds,
                         const NPCManager *mgr);

/* Draw quest log overlay on the viewport */
void draw_quest_log(UI *ui, SDL_Renderer *renderer,
                    const QuestLog *ql);

/* Draw a small HP bar above an enemy */
void draw_enemy_hp_bar(SDL_Renderer *renderer, int sx, int sy,
                       int current_hp, int max_hp);

/* Draw inventory screen overlay */
void draw_inventory_screen(UI *ui, SDL_Renderer *renderer,
                           const Inventory *inv);

/* Draw title screen */
void draw_title_screen(UI *ui, SDL_Renderer *renderer, int selection);

/* Draw pause menu overlay */
void draw_pause_menu(UI *ui, SDL_Renderer *renderer, int selection);

/* Draw death screen overlay with menu options */
void draw_death_screen(UI *ui, SDL_Renderer *renderer, int selection);

#endif /* DIABLO_UI_SCREENS_H */
