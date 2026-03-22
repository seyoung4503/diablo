#include "game/item.h"
#include <time.h>

static bool rand_seeded = false;

static void ensure_rand_seeded(void)
{
    if (!rand_seeded) {
        srand((unsigned)time(NULL));
        rand_seeded = true;
    }
}

const Item *item_db_get(const ItemDatabase *db, int id)
{
    for (int i = 0; i < db->count; i++) {
        if (db->items[i].id == id) {
            return &db->items[i];
        }
    }
    return NULL;
}

Item item_create(const ItemDatabase *db, int id)
{
    const Item *tmpl = item_db_get(db, id);
    if (tmpl) {
        Item item = *tmpl;
        item.stack_count = 1;
        return item;
    }
    /* Return empty item if not found */
    Item empty;
    memset(&empty, 0, sizeof(empty));
    return empty;
}

Item item_create_random(const ItemDatabase *db, int min_level, int max_level)
{
    ensure_rand_seeded();

    /* Collect candidates within level range */
    int candidates[MAX_ITEM_DEFS];
    int num_candidates = 0;

    for (int i = 0; i < db->count; i++) {
        const Item *it = &db->items[i];
        if (it->type == ITEM_NONE || it->type == ITEM_QUEST || it->type == ITEM_GOLD)
            continue;
        if (it->required_level >= min_level && it->required_level <= max_level) {
            candidates[num_candidates++] = i;
        }
    }

    if (num_candidates == 0) {
        /* Fallback: pick any non-special item */
        for (int i = 0; i < db->count; i++) {
            const Item *it = &db->items[i];
            if (it->type != ITEM_NONE && it->type != ITEM_QUEST && it->type != ITEM_GOLD) {
                candidates[num_candidates++] = i;
            }
        }
    }

    if (num_candidates == 0) {
        Item empty;
        memset(&empty, 0, sizeof(empty));
        return empty;
    }

    int pick = rand() % num_candidates;
    Item item = db->items[candidates[pick]];
    item.stack_count = 1;
    return item;
}
