#include "game/inventory.h"
#include "game/stats.h"
#include <limits.h>

void inventory_init(Inventory *inv)
{
    memset(inv, 0, sizeof(*inv));
}

bool inventory_add_item(Inventory *inv, const Item *item)
{
    /* Gold goes directly to gold counter (clamp to prevent overflow) */
    if (item->type == ITEM_GOLD) {
        if (item->stack_count > 0 && inv->gold <= INT_MAX - item->stack_count)
            inv->gold += item->stack_count;
        else if (item->stack_count > 0)
            inv->gold = INT_MAX;
        return true;
    }

    /* Stackable items: try to merge with existing stack first */
    if (item->stackable) {
        for (int i = 0; i < INVENTORY_SIZE; i++) {
            if (inv->slots[i].id == item->id && inv->slots[i].type != ITEM_NONE) {
                int new_count = inv->slots[i].stack_count + item->stack_count;
                if (new_count < inv->slots[i].stack_count)
                    new_count = INT_MAX; /* overflow guard */
                inv->slots[i].stack_count = new_count;
                return true;
            }
        }
    }

    /* Find first empty slot */
    for (int i = 0; i < INVENTORY_SIZE; i++) {
        if (inv->slots[i].type == ITEM_NONE) {
            inv->slots[i] = *item;
            if (inv->slots[i].stack_count < 1)
                inv->slots[i].stack_count = 1;
            return true;
        }
    }

    return false; /* inventory full */
}

bool inventory_remove_item(Inventory *inv, int slot_index)
{
    if (slot_index < 0 || slot_index >= INVENTORY_SIZE)
        return false;
    if (inv->slots[slot_index].type == ITEM_NONE)
        return false;

    memset(&inv->slots[slot_index], 0, sizeof(Item));
    return true;
}

/* Map item type to the appropriate equip slot */
static int item_type_to_equip_slot(ItemType type)
{
    switch (type) {
    case ITEM_WEAPON:  return EQUIP_WEAPON;
    case ITEM_SHIELD:  return EQUIP_SHIELD;
    case ITEM_HELM:    return EQUIP_HELM;
    case ITEM_ARMOR:   return EQUIP_ARMOR;
    case ITEM_RING:    return EQUIP_RING1; /* default to ring1, handle ring2 in equip */
    case ITEM_AMULET:  return EQUIP_AMULET;
    default:           return -1;
    }
}

bool inventory_equip(Inventory *inv, int slot_index)
{
    if (slot_index < 0 || slot_index >= INVENTORY_SIZE)
        return false;

    Item *item = &inv->slots[slot_index];
    if (item->type == ITEM_NONE)
        return false;

    int slot = item_type_to_equip_slot(item->type);
    if (slot < 0)
        return false; /* not equippable */

    /* For rings: if ring1 is occupied, try ring2 */
    if (item->type == ITEM_RING && inv->equipped[EQUIP_RING1].type != ITEM_NONE) {
        slot = EQUIP_RING2;
    }

    /* Swap: if slot is occupied, move old item to inventory */
    Item old_item = inv->equipped[slot];
    inv->equipped[slot] = *item;
    memset(item, 0, sizeof(Item));

    if (old_item.type != ITEM_NONE) {
        /* Put old equipped item back into the now-free slot */
        inv->slots[slot_index] = old_item;
    }

    return true;
}

bool inventory_unequip(Inventory *inv, EquipSlot slot)
{
    if (slot < 0 || slot >= EQUIP_SLOT_COUNT)
        return false;
    if (inv->equipped[slot].type == ITEM_NONE)
        return false;

    /* Find empty inventory slot */
    for (int i = 0; i < INVENTORY_SIZE; i++) {
        if (inv->slots[i].type == ITEM_NONE) {
            inv->slots[i] = inv->equipped[slot];
            memset(&inv->equipped[slot], 0, sizeof(Item));
            return true;
        }
    }

    return false; /* no room in inventory */
}

int inventory_count_items(const Inventory *inv)
{
    int count = 0;
    for (int i = 0; i < INVENTORY_SIZE; i++) {
        if (inv->slots[i].type != ITEM_NONE)
            count++;
    }
    return count;
}

int inventory_find_item(const Inventory *inv, int item_id)
{
    for (int i = 0; i < INVENTORY_SIZE; i++) {
        if (inv->slots[i].id == item_id && inv->slots[i].type != ITEM_NONE)
            return i;
    }
    return -1;
}

void inventory_calc_bonuses(const Inventory *inv, int *str, int *dex, int *mag, int *vit,
                            int *hp, int *mana, int *armor, int *dmg_min, int *dmg_max)
{
    *str = *dex = *mag = *vit = 0;
    *hp = *mana = *armor = 0;
    *dmg_min = *dmg_max = 0;

    for (int i = 0; i < EQUIP_SLOT_COUNT; i++) {
        const Item *it = &inv->equipped[i];
        if (it->type == ITEM_NONE)
            continue;

        *str   += it->str_bonus;
        *dex   += it->dex_bonus;
        *mag   += it->mag_bonus;
        *vit   += it->vit_bonus;
        *hp    += it->hp_bonus;
        *mana  += it->mana_bonus;
        *armor += it->armor_value;

        if (it->type == ITEM_WEAPON) {
            *dmg_min += it->damage_min;
            *dmg_max += it->damage_max;
        }
    }
}

bool inventory_use_item(Inventory *inv, int slot_index, struct PlayerStats *stats)
{
    if (slot_index < 0 || slot_index >= INVENTORY_SIZE)
        return false;

    Item *item = &inv->slots[slot_index];

    switch (item->type) {
    case ITEM_POTION_HP:
        if (stats->current_hp >= stats->max_hp)
            return false; /* already full */
        stats->current_hp = MIN(stats->current_hp + 50, stats->max_hp);
        break;

    case ITEM_POTION_MANA:
        if (stats->current_mana >= stats->max_mana)
            return false; /* already full */
        stats->current_mana = MIN(stats->current_mana + 30, stats->max_mana);
        break;

    default:
        return false; /* not consumable */
    }

    /* Consume: reduce stack or remove */
    item->stack_count--;
    if (item->stack_count <= 0) {
        memset(item, 0, sizeof(Item));
    }

    return true;
}
