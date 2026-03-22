#ifndef DIABLO_GAME_INVENTORY_H
#define DIABLO_GAME_INVENTORY_H

#include "common.h"
#include "game/item.h"

/* Forward declare PlayerStats */
struct PlayerStats;

#define INVENTORY_SIZE   40  /* 8x5 grid */
#define EQUIP_SLOT_COUNT 7

typedef enum {
    EQUIP_WEAPON = 0,
    EQUIP_SHIELD,
    EQUIP_HELM,
    EQUIP_ARMOR,
    EQUIP_RING1,
    EQUIP_RING2,
    EQUIP_AMULET
} EquipSlot;

typedef struct Inventory {
    Item slots[INVENTORY_SIZE];
    Item equipped[EQUIP_SLOT_COUNT];
    int gold;
} Inventory;

/* Initialize an empty inventory */
void inventory_init(Inventory *inv);

/* Add item to first empty slot. Returns true on success */
bool inventory_add_item(Inventory *inv, const Item *item);

/* Remove item from inventory slot. Returns true on success */
bool inventory_remove_item(Inventory *inv, int slot_index);

/* Move item from inventory slot to appropriate equip slot */
bool inventory_equip(Inventory *inv, int slot_index);

/* Move item from equip slot back to inventory */
bool inventory_unequip(Inventory *inv, EquipSlot slot);

/* Count non-empty inventory slots */
int inventory_count_items(const Inventory *inv);

/* Find item by id. Returns slot index or -1 if not found */
int inventory_find_item(const Inventory *inv, int item_id);

/* Calculate total stat bonuses from all equipped items */
void inventory_calc_bonuses(const Inventory *inv, int *str, int *dex, int *mag, int *vit,
                            int *hp, int *mana, int *armor, int *dmg_min, int *dmg_max);

/* Use a consumable item (potion). Returns true if consumed */
bool inventory_use_item(Inventory *inv, int slot_index, struct PlayerStats *stats);

#endif /* DIABLO_GAME_INVENTORY_H */
