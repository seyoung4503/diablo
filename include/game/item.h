#ifndef DIABLO_GAME_ITEM_H
#define DIABLO_GAME_ITEM_H

#include "common.h"

typedef enum {
    ITEM_NONE = 0,
    ITEM_WEAPON,
    ITEM_ARMOR,
    ITEM_HELM,
    ITEM_SHIELD,
    ITEM_RING,
    ITEM_AMULET,
    ITEM_POTION_HP,
    ITEM_POTION_MANA,
    ITEM_SCROLL,
    ITEM_GOLD,
    ITEM_QUEST,
    ITEM_TYPE_COUNT
} ItemType;

typedef enum {
    RARITY_COMMON,
    RARITY_MAGIC,
    RARITY_RARE,
    RARITY_UNIQUE
} ItemRarity;

typedef struct Item {
    int id;
    char name[64];
    ItemType type;
    ItemRarity rarity;
    int damage_min, damage_max;   /* weapons */
    int armor_value;              /* armor/helm/shield */
    int str_bonus, dex_bonus, mag_bonus, vit_bonus;
    int hp_bonus, mana_bonus;
    int value;                    /* gold value */
    bool stackable;
    int stack_count;
    int required_level;
} Item;

#define MAX_ITEM_DEFS 64

typedef struct ItemDatabase {
    Item items[MAX_ITEM_DEFS];
    int count;
} ItemDatabase;

/* Load all item definitions into the database */
void item_db_init(ItemDatabase *db);

/* Get item template by id (returns NULL if not found) */
const Item *item_db_get(const ItemDatabase *db, int id);

/* Create an item instance from a template */
Item item_create(const ItemDatabase *db, int id);

/* Create a random item appropriate for the given level range */
Item item_create_random(const ItemDatabase *db, int min_level, int max_level);

#endif /* DIABLO_GAME_ITEM_H */
