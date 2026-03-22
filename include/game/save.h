#ifndef DIABLO_GAME_SAVE_H
#define DIABLO_GAME_SAVE_H

#include <stdbool.h>
#include <stdint.h>

/* Forward declarations to avoid pulling in heavy headers */
struct Game;
struct Player;
struct Inventory;
struct NPCManager;
struct RelationshipGraph;
struct MemorySystem;
struct QuestLog;
struct PlayerStats;

#define SAVE_FILE_PATH "diablo_save.dat"
#define SAVE_MAGIC   0x44494142  /* "DIAB" */
#define SAVE_VERSION 1

#define SAVE_MAX_NPCS  20
#define SAVE_MAX_FLAGS 64
#define SAVE_INV_SLOTS  40
#define SAVE_EQUIP_SLOTS 7

typedef struct SavedItem {
    int id;
    char name[64];
    int type;       /* ItemType */
    int rarity;     /* ItemRarity */
    int damage_min, damage_max;
    int armor_value;
    int str_bonus, dex_bonus, mag_bonus, vit_bonus;
    int hp_bonus, mana_bonus;
    int value;
    bool stackable;
    int stack_count;
    int required_level;
} SavedItem;

typedef struct SaveHeader {
    uint32_t magic;
    uint32_t version;
    uint32_t checksum;  /* simple XOR checksum of SaveData bytes */
} SaveHeader;

/*
 * SaveData packs all mutable game state into a single flat struct
 * suitable for binary serialization.  Immutable data (NPC definitions,
 * item database, tile maps) is NOT saved — it is re-initialized on load.
 */
typedef struct SaveData {
    /* Game clock */
    int game_day;
    int game_hour;
    float game_time_acc;
    int current_scene;   /* 0 = town, 1 = dungeon */
    int dungeon_level;

    /* Player position & facing */
    int player_tile_x, player_tile_y;
    int player_facing;   /* Direction enum stored as int */

    /* Player stats  (embedded by value) */
    int stat_level, stat_xp, stat_xp_to_next, stat_points;
    int stat_strength, stat_dexterity, stat_magic, stat_vitality;
    int stat_max_hp, stat_current_hp;
    int stat_max_mana, stat_current_mana;
    int stat_armor_class, stat_to_hit_bonus;
    int stat_min_damage, stat_max_damage;

    /* Inventory — stored as raw bytes so the struct layout is captured */
    /* We include game/inventory.h indirectly via save.c; here we reserve
       enough space.  Actual packing is done in save_pack / save_unpack. */
    int inventory_gold;

    /* Inventory slots: INVENTORY_SIZE(40) + EQUIP_SLOT_COUNT(7) = 47 items max */
    SavedItem inv_slots[SAVE_INV_SLOTS];
    SavedItem inv_equipped[SAVE_EQUIP_SLOTS];

    /* NPC mutable state */
    int npc_count;
    struct {
        float mood;
        float stress;
        float trust_player;
        int tile_x, tile_y;
        int arc_stage;
        bool is_alive;
        bool has_left_town;
        float need_safety, need_social, need_economic, need_purpose;
    } npc_states[SAVE_MAX_NPCS];

    /* Relationship graph — stored as flat array */
    struct {
        float affection;
        float trust;
        float respect;
        int type;           /* RelationType */
        int shared_history;
    } relationships[SAVE_MAX_NPCS][SAVE_MAX_NPCS];

    /* NPC memories */
    struct {
        struct {
            int event_type;     /* MemoryEventType */
            int subject_id;
            int object_id;
            int game_day;
            float emotional_weight;
            float salience;
        } memories[32];  /* MAX_MEMORIES_PER_NPC */
        int count;
        int write_index;
    } memory_banks[SAVE_MAX_NPCS];

    /* Quest log */
    int quest_count;
    struct {
        int id;
        char name[128];     /* MAX_QUEST_NAME */
        char description[512]; /* MAX_QUEST_DESC */
        int state;          /* QuestState */
        int giver_npc_id;
        struct {
            int type;       /* ObjectiveType */
            int param1, param2;
            char description[128];
            bool completed;
        } objectives[4];    /* MAX_OBJECTIVES */
        int objective_count;
        struct {
            char description[256];
            int triggered_flag;
            int world_events[4];
            int event_targets[4];
            float event_magnitudes[4];
            int event_count;
            int xp_reward;
            int gold_reward;
        } outcomes[3];      /* MAX_OUTCOMES */
        int outcome_count;
        int chosen_outcome;
        int deadline_day;
    } quests[32];           /* MAX_QUESTS */

    /* Global flags */
    int flags[SAVE_MAX_FLAGS];
    int flag_count;
} SaveData;

/* File I/O */
bool save_game(const char *path, const SaveData *data);
bool load_game(const char *path, SaveData *data);
bool save_exists(const char *path);

/* Pack live game state into SaveData for writing */
void save_pack(SaveData *data, const struct Game *game,
               const struct Player *player, const struct Inventory *inv,
               const struct NPCManager *npcs,
               const struct RelationshipGraph *graph,
               const struct MemorySystem *memory,
               const struct QuestLog *quests,
               const int *flags, int flag_count,
               int scene, int dlevel);

/* Unpack SaveData back into live game structs after reading */
void save_unpack(const SaveData *data, struct Game *game,
                 struct Player *player, struct Inventory *inv,
                 struct NPCManager *npcs,
                 struct RelationshipGraph *graph,
                 struct MemorySystem *memory,
                 struct QuestLog *quests,
                 int *flags, int *flag_count,
                 int *scene, int *dlevel);

#endif /* DIABLO_GAME_SAVE_H */
