#ifndef DIABLO_NPC_NPC_H
#define DIABLO_NPC_NPC_H

#include "common.h"
#include "world/pathfinding.h"

/* Forward declarations */
struct TileMap;
struct Town;

/* NPC name/title limits */
#define MAX_NPC_NAME  64
#define MAX_NPC_TITLE 64

/* Personality dimensions */
#define TRAIT_COUNT 5   /* Big Five */
#define VALUE_COUNT 5   /* courage, greed, loyalty, piety, pragmatism */

typedef enum {
    TRAIT_OPENNESS = 0,
    TRAIT_CONSCIENTIOUSNESS,
    TRAIT_EXTRAVERSION,
    TRAIT_AGREEABLENESS,
    TRAIT_NEUROTICISM
} PersonalityTrait;

typedef enum {
    VALUE_COURAGE = 0,
    VALUE_GREED,
    VALUE_LOYALTY,
    VALUE_PIETY,
    VALUE_PRAGMATISM
} PersonalityValue;

typedef struct NPCPersonality {
    float traits[TRAIT_COUNT];    /* 0.0 - 1.0 */
    float values[VALUE_COUNT];   /* 0.0 - 1.0 */
} NPCPersonality;

typedef enum {
    NPC_ACTION_IDLE,
    NPC_ACTION_WORK,
    NPC_ACTION_SOCIALIZE,
    NPC_ACTION_EAT,
    NPC_ACTION_SLEEP,
    NPC_ACTION_PRAY,
    NPC_ACTION_DRINK,
    NPC_ACTION_PATROL,
    NPC_ACTION_WANDER,
    NPC_ACTION_COUNT
} NPCAction;

/* TownLocation is defined in world/town.h; forward-declare the enum value range */
typedef int NPCLocation;  /* stores TownLocation values without pulling in town.h */

typedef struct NPC {
    int id;
    char name[MAX_NPC_NAME];
    char title[MAX_NPC_TITLE];
    NPCPersonality personality;

    /* Position */
    int tile_x, tile_y;
    float world_x, world_y;
    Direction facing;
    NPCLocation current_location;
    NPCLocation home_location;
    NPCLocation work_location;

    /* Movement */
    Path path;
    bool moving;
    int move_from_x, move_from_y;
    int move_to_x, move_to_y;
    float move_timer;
    float move_speed;   /* tiles per second (2-4, slower than player) */

    /* Needs (0.0 - 1.0, higher = more urgent) */
    float need_safety;
    float need_social;
    float need_economic;
    float need_purpose;

    /* Emotional state */
    float mood;           /* -1.0 to 1.0 */
    float stress;         /* 0.0 to 1.0 */
    float trust_player;   /* -1.0 to 1.0 */

    /* State */
    NPCAction current_action;
    int arc_stage;
    bool is_alive;
    bool has_left_town;

    /* Schedule (index into global schedule table) */
    int schedule_id;
} NPC;

#define MAX_TOWN_NPCS 20

typedef struct NPCManager {
    NPC npcs[MAX_TOWN_NPCS];
    int count;
} NPCManager;

/* Manager functions */
void npc_manager_init(NPCManager *mgr);
void npc_manager_update(NPCManager *mgr, float dt, int game_hour,
                        const struct TileMap *map);
NPC *npc_manager_get(NPCManager *mgr, int id);
NPC *npc_manager_get_by_name(NPCManager *mgr, const char *name);
NPC *npc_manager_at_tile(NPCManager *mgr, int x, int y);

/* Individual NPC functions */
void npc_init(NPC *npc, int id, const char *name, const char *title,
              int start_x, int start_y);
void npc_update(NPC *npc, float dt, const struct TileMap *map);
void npc_move_to_tile(NPC *npc, const struct TileMap *map,
                      int target_x, int target_y);
void npc_move_to_location(NPC *npc, const struct TileMap *map,
                          const struct Town *town, NPCLocation loc);

#endif /* DIABLO_NPC_NPC_H */
