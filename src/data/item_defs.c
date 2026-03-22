#include "game/item.h"

/* Helper to add an item definition to the database */
static void add_item(ItemDatabase *db, int id, const char *name, ItemType type,
                     ItemRarity rarity, int dmg_min, int dmg_max, int armor,
                     int str, int dex, int mag, int vit, int hp, int mana,
                     int value, bool stackable, int req_level)
{
    if (db->count >= MAX_ITEM_DEFS)
        return;

    Item *it = &db->items[db->count++];
    memset(it, 0, sizeof(*it));
    it->id = id;
    strncpy(it->name, name, sizeof(it->name) - 1);
    it->type = type;
    it->rarity = rarity;
    it->damage_min = dmg_min;
    it->damage_max = dmg_max;
    it->armor_value = armor;
    it->str_bonus = str;
    it->dex_bonus = dex;
    it->mag_bonus = mag;
    it->vit_bonus = vit;
    it->hp_bonus = hp;
    it->mana_bonus = mana;
    it->value = value;
    it->stackable = stackable;
    it->stack_count = stackable ? 1 : 0;
    it->required_level = req_level;
}

void item_db_init(ItemDatabase *db)
{
    memset(db, 0, sizeof(*db));

    /* === Weapons (IDs 1-5) === */
    /*        db, id, name,             type,        rarity,         dmg,   arm, str,dex,mag,vit, hp,mana, val,  stack, lvl */
    add_item(db,  1, "Short Sword",     ITEM_WEAPON, RARITY_COMMON,  2,  6, 0,  0, 0, 0, 0,  0, 0,    50, false, 1);
    add_item(db,  2, "Long Sword",      ITEM_WEAPON, RARITY_COMMON,  4, 10, 0,  0, 0, 0, 0,  0, 0,   150, false, 3);
    add_item(db,  3, "War Hammer",      ITEM_WEAPON, RARITY_MAGIC,   6, 14, 0,  3, 0, 0, 0,  0, 0,   300, false, 5);
    add_item(db,  4, "Shadow Blade",    ITEM_WEAPON, RARITY_RARE,    8, 16, 0,  0, 4, 0, 0,  0, 0,   600, false, 8);
    add_item(db,  5, "Griswold's Edge", ITEM_WEAPON, RARITY_UNIQUE, 10, 20, 0,  5, 3, 0, 0,  0, 0,  1000, false, 10);

    /* === Armor (IDs 6-9) === */
    add_item(db,  6, "Leather Armor",   ITEM_ARMOR,  RARITY_COMMON,  0,  0,  5, 0, 0, 0, 0,  0, 0,    75, false, 1);
    add_item(db,  7, "Chain Mail",      ITEM_ARMOR,  RARITY_COMMON,  0,  0, 12, 0, 0, 0, 0,  0, 0,   250, false, 4);
    add_item(db,  8, "Plate Mail",      ITEM_ARMOR,  RARITY_MAGIC,   0,  0, 20, 0, 0, 0, 3,  0, 0,   500, false, 7);
    add_item(db,  9, "Godly Plate",     ITEM_ARMOR,  RARITY_UNIQUE,  0,  0, 30, 3, 0, 0, 5,  0, 0,   900, false, 10);

    /* === Helms (IDs 10-12) === */
    add_item(db, 10, "Cap",             ITEM_HELM,   RARITY_COMMON,  0,  0,  2, 0, 0, 0, 0,  0, 0,    30, false, 1);
    add_item(db, 11, "Helm",            ITEM_HELM,   RARITY_COMMON,  0,  0,  5, 0, 0, 0, 0,  0, 0,   100, false, 3);
    add_item(db, 12, "Crown",           ITEM_HELM,   RARITY_MAGIC,   0,  0, 10, 0, 0, 3, 0,  0, 0,   350, false, 6);

    /* === Shields (IDs 13-15) === */
    add_item(db, 13, "Buckler",         ITEM_SHIELD, RARITY_COMMON,  0,  0,  3, 0, 0, 0, 0,  0, 0,    40, false, 1);
    add_item(db, 14, "Large Shield",    ITEM_SHIELD, RARITY_COMMON,  0,  0,  8, 0, 0, 0, 0,  0, 0,   120, false, 4);
    add_item(db, 15, "Tower Shield",    ITEM_SHIELD, RARITY_MAGIC,   0,  0, 14, 0, 0, 0, 0,  0, 0,   280, false, 7);

    /* === Rings (IDs 16-18) === */
    add_item(db, 16, "Ring of Strength", ITEM_RING,  RARITY_MAGIC,   0,  0,  0, 3, 0, 0, 0,  0, 0,   200, false, 3);
    add_item(db, 17, "Ring of Magic",    ITEM_RING,  RARITY_MAGIC,   0,  0,  0, 0, 0, 3, 0,  0, 0,   200, false, 3);
    add_item(db, 18, "Ring of Health",   ITEM_RING,  RARITY_MAGIC,   0,  0,  0, 0, 0, 0, 0, 20, 0,   200, false, 3);

    /* === Amulets (IDs 19-20) === */
    add_item(db, 19, "Amulet of Vigor",  ITEM_AMULET, RARITY_MAGIC,  0,  0,  0, 0, 0, 0, 3,  0, 0,   250, false, 4);
    add_item(db, 20, "Amulet of Stars",  ITEM_AMULET, RARITY_RARE,   0,  0,  0, 2, 2, 2, 2,  0, 0,   500, false, 7);

    /* === Potions (IDs 21-22) — stackable === */
    add_item(db, 21, "Healing Potion",   ITEM_POTION_HP,   RARITY_COMMON, 0, 0, 0, 0, 0, 0, 0, 50, 0,  50, true, 1);
    add_item(db, 22, "Mana Potion",      ITEM_POTION_MANA, RARITY_COMMON, 0, 0, 0, 0, 0, 0, 0, 0, 30,  50, true, 1);
}
