#include "common.h"
#include "engine/animation.h"
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
#include "game/combat_anim.h"
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
#include "ui_screens.h"
#include <string.h>

/* Camera scroll speed in pixels per second */
#define CAMERA_SPEED 200

/* Tile textures (diamond-masked) */
static SDL_Texture *tile_textures[TILE_TYPE_COUNT];

/* Hovered tile under mouse */
static int hover_tile_x = -1;
static int hover_tile_y = -1;

/* Character screen toggle */
static bool show_character_screen = false;

/* Animation sprite sheet manager */
static SpriteSheetManager sprite_mgr;

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

/* Combat animation system */
static CombatAnimSystem combat_anim;

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

/* NPC sprite textures (color-keyed for transparency) */
static SDL_Texture *tex_griswold;
static SDL_Texture *tex_pepin;
static SDL_Texture *tex_ogden;
static SDL_Texture *tex_cain;
static SDL_Texture *tex_npc_fallback;

/* Player/enemy sprite texture */
static SDL_Texture *tex_warrior;

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

    /* Initialize combat animation system */
    combat_anim_init(&combat_anim);

    /* Initialize Tristram town */
    Town town;
    town_init(&town);

    /* Load tile textures with diamond masking */
    {
        int idx;
        idx = resource_load_tile_texture(&engine.resources,
                  "assets/tiles/town/grass.jpg", TILE_WIDTH, TILE_HEIGHT);
        SDL_Texture *tex_grass = resource_get_texture(&engine.resources, idx);

        idx = resource_load_tile_texture(&engine.resources,
                  "assets/tiles/town/dirt_path.jpg", TILE_WIDTH, TILE_HEIGHT);
        SDL_Texture *tex_dirt = resource_get_texture(&engine.resources, idx);

        idx = resource_load_tile_texture(&engine.resources,
                  "assets/tiles/floor_stone.jpg", TILE_WIDTH, TILE_HEIGHT);
        SDL_Texture *tex_stone = resource_get_texture(&engine.resources, idx);

        idx = resource_load_tile_texture(&engine.resources,
                  "assets/tiles/wall_stone.jpg", TILE_WIDTH, TILE_HEIGHT);
        SDL_Texture *tex_wall = resource_get_texture(&engine.resources, idx);

        idx = resource_load_tile_texture(&engine.resources,
                  "assets/tiles/town/water.jpg", TILE_WIDTH, TILE_HEIGHT);
        SDL_Texture *tex_water = resource_get_texture(&engine.resources, idx);

        idx = resource_load_tile_texture(&engine.resources,
                  "assets/tiles/town/tree.jpg", TILE_WIDTH, TILE_HEIGHT);
        SDL_Texture *tex_tree = resource_get_texture(&engine.resources, idx);

        tile_textures[TILE_NONE]        = tex_stone;
        tile_textures[TILE_GRASS]       = tex_grass;
        tile_textures[TILE_DIRT]        = tex_dirt;
        tile_textures[TILE_STONE_FLOOR] = tex_stone;
        tile_textures[TILE_WALL]        = tex_wall;
        tile_textures[TILE_WATER]       = tex_water;
        tile_textures[TILE_DOOR]        = tex_stone;
        tile_textures[TILE_STAIRS_UP]   = tex_stone;
        tile_textures[TILE_STAIRS_DOWN] = tex_stone;
        tile_textures[TILE_TREE]        = tex_tree ? tex_tree : tex_grass;
    }

    /* Load character sprites with color-key background removal */
    {
        int idx;
        idx = resource_load_sprite_texture(&engine.resources,
                  "assets/sprites/player/warrior_test.jpg");
        tex_warrior = resource_get_texture(&engine.resources, idx);

        idx = resource_load_sprite_texture(&engine.resources,
                  "assets/sprites/npcs/griswold.jpg");
        tex_griswold = resource_get_texture(&engine.resources, idx);

        idx = resource_load_sprite_texture(&engine.resources,
                  "assets/sprites/npcs/pepin.jpg");
        tex_pepin = resource_get_texture(&engine.resources, idx);

        idx = resource_load_sprite_texture(&engine.resources,
                  "assets/sprites/npcs/ogden.jpg");
        tex_ogden = resource_get_texture(&engine.resources, idx);

        idx = resource_load_sprite_texture(&engine.resources,
                  "assets/sprites/npcs/cain.jpg");
        tex_cain = resource_get_texture(&engine.resources, idx);

        tex_npc_fallback = tex_warrior;
    }

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
    spritesheet_manager_init(&sprite_mgr);

    /* Try to load sprite sheets (graceful — game works without them) */
    int warrior_sheet_id = spritesheet_load(&sprite_mgr, engine.renderer,
        "assets/sprites/player/warrior_sheet.png", 96, 96);
    if (warrior_sheet_id >= 0) {
        SpriteSheet *ws = (SpriteSheet *)spritesheet_get(&sprite_mgr, warrior_sheet_id);
        if (ws) spritesheet_apply_defaults(ws);
        anim_controller_init(&player.anim, spritesheet_get(&sprite_mgr, warrior_sheet_id));
    }

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
                                            hover_tile_x, hover_tile_y, 1) &&
                            combat_anim_can_act(&combat_anim)) {
                            /* Face the enemy */
                            int dx = target_enemy->tile_x - player.tile_x;
                            int dy = target_enemy->tile_y - player.tile_y;
                            if (dx > 0 && dy > 0) player.facing = DIR_SE;
                            else if (dx > 0 && dy < 0) player.facing = DIR_NE;
                            else if (dx < 0 && dy > 0) player.facing = DIR_SW;
                            else if (dx < 0 && dy < 0) player.facing = DIR_NW;
                            else if (dx > 0) player.facing = DIR_E;
                            else if (dx < 0) player.facing = DIR_W;
                            else if (dy > 0) player.facing = DIR_S;
                            else player.facing = DIR_N;

                            player.anim_state = ANIM_ATTACKING;
                            combat_anim_start_attack(&combat_anim, target_enemy->id,
                                                     MELEE_DEFAULT_TIMING);
                            audio_play_sfx(&audio, SFX_SWORD_SWING);
                        } else if (combat_anim_can_act(&combat_anim) && active_map && hover_tile_x >= 0 && hover_tile_y >= 0 &&
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

            /* Update player movement (blocked during attack animation) */
            if (combat_anim_can_act(&combat_anim))
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
                effects_screen_shake(&effects, 2.0f, 0.1f);
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

            /* Resolve player attack damage at swing phase */
            {
                int cam_x_upd = (int)camera.x;
                int cam_y_upd = (int)camera.y;
                if (combat_anim_should_resolve_damage(&combat_anim)) {
                    uint32_t tid = combat_anim.player_attack.target_enemy_id;
                    Enemy *target = NULL;
                    int target_idx = -1;
                    for (int i = 0; i < enemy_mgr.count; i++) {
                        if (enemy_mgr.enemies[i].id == tid && enemy_mgr.enemies[i].alive) {
                            target = &enemy_mgr.enemies[i];
                            target_idx = i;
                            break;
                        }
                    }
                    if (target) {
                        CombatResult cr = combat_player_attack(&player, target, &inventory);
                        int esx = (int)target->world_x - cam_x_upd + SCREEN_WIDTH / 2;
                        int esy = (int)target->world_y - cam_y_upd;
                        if (cr.hit) {
                            audio_play_sfx(&audio, SFX_HIT);
                            effects_spawn_blood(&effects, esx, esy);
                            if (cr.damage > target->max_hp / 3)
                                effects_screen_shake(&effects, 3.0f, 0.15f);
                            float kb_dx = (float)(target->tile_x - player.tile_x) * 2.0f;
                            float kb_dy = (float)(target->tile_y - player.tile_y) * 2.0f;
                            combat_anim_hit_reaction(&combat_anim, target_idx, kb_dx, kb_dy);
                            char dmg_str[16];
                            snprintf(dmg_str, sizeof(dmg_str), "%d", cr.damage);
                            combat_anim_spawn_text(&combat_anim, (float)esx, (float)esy - 30,
                                                   dmg_str, (SDL_Color){255, 220, 50, 255});
                        } else {
                            combat_anim_spawn_text(&combat_anim, (float)esx, (float)esy - 30,
                                                   "MISS", (SDL_Color){180, 180, 180, 255});
                        }
                        if (!target->alive) {
                            audio_play_sfx(&audio, SFX_ENEMY_DIE);
                            combat_anim_start_death(&combat_anim, target_idx, target->id,
                                                    (float)esx, (float)esy,
                                                    target->tile_x, target->tile_y);
                            if (target->gold_max > 0) {
                                int gold_amt = target->gold_min +
                                    rand() % (target->gold_max - target->gold_min + 1);
                                inventory.gold += gold_amt;
                            }
                            if (rand() % 100 < 30) {
                                Item loot = item_create_random(&item_db, 1, player.stats.level + 2);
                                if (loot.type != ITEM_NONE)
                                    drop_ground_item(&loot, target->tile_x, target->tile_y);
                            }
                        }
                    }
                }
            }

            /* Update combat animations */
            combat_anim_update(&combat_anim, dt);

            /* Reset to idle after attack finishes */
            if (!combat_anim.player_attack.active && player.anim_state == ANIM_ATTACKING) {
                player.anim_state = ANIM_IDLE;
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
            draw_title_screen(&ui, engine.renderer, title_selection);
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

        int cam_x = (int)camera.x + effects_get_shake_x(&effects);
        int cam_y = (int)camera.y + effects_get_shake_y(&effects);

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

                if (floor_tex) {
                    /* Texture-based rendering (diamond-masked) */
                    iso_draw_tile(&iso_renderer, floor_tex, tx, ty, cam_x, cam_y);

                    /* Walls and trees get an elevated block on top */
                    if (type == TILE_WALL && tile_textures[TILE_WALL]) {
                        iso_draw_wall(&iso_renderer, tile_textures[TILE_WALL],
                                      tx, ty, cam_x, cam_y, TILE_HEIGHT);
                    } else if (type == TILE_TREE && tile_textures[TILE_TREE]) {
                        SDL_Texture *tree_tex = tile_textures[TILE_TREE];
                        iso_draw_wall(&iso_renderer, tree_tex,
                                      tx, ty, cam_x, cam_y, TILE_HEIGHT);
                    }
                } else {
                    /* Procedural fallback when texture is missing */
                    iso_draw_tile_by_type(&iso_renderer, type, tx, ty, cam_x, cam_y);
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

        /* Player sprite — 3-tier: animated sheet > static texture > procedural */
        {
            int draw_sx, draw_sy;
            player_get_screen_pos(&player, &draw_sx, &draw_sy);
            draw_sx = draw_sx - cam_x + (SCREEN_WIDTH / 2) - SPRITE_SIZE / 2;
            draw_sy = draw_sy - cam_y - SPRITE_SIZE + TILE_HALF_H;

            if (player.anim.sheet) {
                SDL_Rect src = anim_controller_get_src_rect(&player.anim);
                iso_draw_animated_sprite(&iso_renderer, player.anim.sheet->texture,
                                         src, (int)player.world_x, (int)player.world_y,
                                         cam_x, cam_y, SPRITE_SIZE, SPRITE_SIZE);
            } else if (tex_warrior) {
                SDL_Rect dst = { draw_sx, draw_sy, SPRITE_SIZE, SPRITE_SIZE };
                SDL_RenderCopy(engine.renderer, tex_warrior, NULL, &dst);
            } else {
                SDL_Color player_body = { 60, 60, 120, 255 };
                SDL_Color player_head = { 200, 170, 140, 255 };
                iso_draw_character(&iso_renderer,
                                   player.tile_x, player.tile_y,
                                   cam_x, cam_y, player_body, player_head);
            }
        }

        /* Draw enemies */
        for (int i = 0; i < enemy_mgr.count; i++) {
            Enemy *e = &enemy_mgr.enemies[i];
            if (!e->alive)
                continue;
            if (combat_anim_is_dying(&combat_anim, i))
                continue;

            int e_sx = (int)e->world_x - cam_x + (SCREEN_WIDTH / 2) - SPRITE_SIZE / 2;
            int e_sy = (int)e->world_y - cam_y - SPRITE_SIZE + TILE_HALF_H;

            /* Apply hit reaction knockback offset */
            float hit_dx = 0, hit_dy = 0;
            combat_anim_get_hit_offset(&combat_anim, i, &hit_dx, &hit_dy);
            e_sx += (int)hit_dx;
            e_sy += (int)hit_dy;

            if (e->anim.sheet) {
                SDL_Rect src = anim_controller_get_src_rect(&e->anim);
                SDL_Texture *sheet_tex = e->anim.sheet->texture;
                SDL_SetTextureColorMod(sheet_tex, 255, 80, 80);
                iso_draw_animated_sprite(&iso_renderer, sheet_tex, src,
                                         (int)e->world_x, (int)e->world_y,
                                         cam_x, cam_y, SPRITE_SIZE, SPRITE_SIZE);
                SDL_SetTextureColorMod(sheet_tex, 255, 255, 255);
            } else if (tex_warrior) {
                SDL_SetTextureColorMod(tex_warrior, 255, 80, 80);
                SDL_Rect dst = { e_sx, e_sy, SPRITE_SIZE, SPRITE_SIZE };
                SDL_RenderCopy(engine.renderer, tex_warrior, NULL, &dst);
                SDL_SetTextureColorMod(tex_warrior, 255, 255, 255);
            } else {
                SDL_Color enemy_body = { 140, 40, 40, 255 };
                SDL_Color enemy_head = { 180, 80, 60, 255 };
                iso_draw_character(&iso_renderer,
                                   e->tile_x, e->tile_y,
                                   cam_x, cam_y, enemy_body, enemy_head);
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

            int npc_sx = (int)npc->world_x - cam_x + (SCREEN_WIDTH / 2) - SPRITE_SIZE / 2;
            int npc_sy = (int)npc->world_y - cam_y - SPRITE_SIZE + TILE_HALF_H;

            SDL_Texture *npc_tex = get_npc_texture(npc->id);
            if (npc_tex) {
                SDL_Rect dst = { npc_sx, npc_sy, SPRITE_SIZE, SPRITE_SIZE };
                SDL_RenderCopy(engine.renderer, npc_tex, NULL, &dst);
            } else {
                /* Procedural fallback */
                SDL_Color npc_body = { 80, 80, 90, 255 };
                SDL_Color npc_head = { 200, 170, 140, 255 };
                iso_draw_character(&iso_renderer,
                                   npc->tile_x, npc->tile_y,
                                   cam_x, cam_y, npc_body, npc_head);
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

        /* Combat floating texts and death animations */
        combat_anim_render_texts(&combat_anim, engine.renderer, ui.font);
        combat_anim_render_deaths(&combat_anim, engine.renderer);

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
            draw_debug_overlay(&ui, engine.renderer, &npc_mgr, &rel_graph, &mem_system, hover_tile_x, hover_tile_y);

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
            draw_pause_menu(&ui, engine.renderer, pause_selection);

        if (current_scene == SCENE_DEATH)
            draw_death_screen(&ui, engine.renderer, death_selection);

        engine_end_frame(&engine);
    }

    /* Cleanup */
    spritesheet_manager_shutdown(&sprite_mgr);
    effects_cleanup(&effects);
    audio_shutdown(&audio);
    engine_shutdown(&engine);
    return 0;
}
