#include "data/npc_defs.h"
#include "npc/npc.h"
#include "npc/npc_schedule.h"
#include "world/town.h"

/* Helper: set Big Five personality traits */
static void set_traits(NPCPersonality *p,
                       float o, float c, float e, float a, float n)
{
    p->traits[TRAIT_OPENNESS]          = o;
    p->traits[TRAIT_CONSCIENTIOUSNESS] = c;
    p->traits[TRAIT_EXTRAVERSION]      = e;
    p->traits[TRAIT_AGREEABLENESS]     = a;
    p->traits[TRAIT_NEUROTICISM]       = n;
}

/* Helper: set personality values */
static void set_values(NPCPersonality *p,
                       float courage, float greed, float loyalty,
                       float piety, float pragmatism)
{
    p->values[VALUE_COURAGE]    = courage;
    p->values[VALUE_GREED]      = greed;
    p->values[VALUE_LOYALTY]    = loyalty;
    p->values[VALUE_PIETY]      = piety;
    p->values[VALUE_PRAGMATISM] = pragmatism;
}

/* Helper: add an NPC to the manager at a town location */
static NPC *add_npc(NPCManager *mgr, int id,
                    const char *name, const char *title,
                    const Town *town, TownLocation start_loc,
                    TownLocation home_loc, TownLocation work_loc,
                    int schedule_id)
{
    if (mgr->count >= MAX_TOWN_NPCS)
        return NULL;

    NPC *npc = &mgr->npcs[mgr->count];
    const LocationInfo *loc = town_get_location(town, start_loc);
    int sx = loc ? loc->center_x : 20;
    int sy = loc ? loc->center_y : 20;

    npc_init(npc, id, name, title, sx, sy);
    npc->home_location    = (NPCLocation)home_loc;
    npc->work_location    = (NPCLocation)work_loc;
    npc->current_location = (NPCLocation)start_loc;
    npc->schedule_id      = schedule_id;

    mgr->count++;
    return npc;
}

void npc_defs_load(NPCManager *mgr, const Town *town)
{
    npc_manager_init(mgr);
    npc_schedules_init();

    NPC *npc;

    /* 0: Griswold — Blacksmith */
    npc = add_npc(mgr, 0, "Griswold", "Blacksmith", town,
                  LOC_BLACKSMITH, LOC_BLACKSMITH, LOC_BLACKSMITH, 0);
    if (npc) {
        set_traits(&npc->personality, 0.3f, 0.8f, 0.4f, 0.5f, 0.3f);
        set_values(&npc->personality, 0.8f, 0.3f, 0.9f, 0.4f, 0.7f);
        npc->mood = 0.2f;
        npc->move_speed = 2.5f;
    }

    /* 1: Pepin — Healer */
    npc = add_npc(mgr, 1, "Pepin", "Healer", town,
                  LOC_HEALER, LOC_HEALER, LOC_HEALER, 1);
    if (npc) {
        set_traits(&npc->personality, 0.7f, 0.9f, 0.3f, 0.9f, 0.7f);
        set_values(&npc->personality, 0.3f, 0.1f, 0.7f, 0.8f, 0.5f);
        npc->mood = 0.1f;
        npc->move_speed = 2.0f;
    }

    /* 2: Ogden — Tavern Keeper */
    npc = add_npc(mgr, 2, "Ogden", "Tavern Keeper", town,
                  LOC_TAVERN, LOC_TAVERN, LOC_TAVERN, 2);
    if (npc) {
        set_traits(&npc->personality, 0.5f, 0.6f, 0.9f, 0.7f, 0.3f);
        set_values(&npc->personality, 0.4f, 0.5f, 0.6f, 0.3f, 0.8f);
        npc->mood = 0.3f;
        npc->move_speed = 2.5f;
    }

    /* 3: Gillian — Barmaid */
    npc = add_npc(mgr, 3, "Gillian", "Barmaid", town,
                  LOC_TAVERN, LOC_TAVERN, LOC_TAVERN, 3);
    if (npc) {
        set_traits(&npc->personality, 0.6f, 0.5f, 0.8f, 0.8f, 0.4f);
        set_values(&npc->personality, 0.3f, 0.2f, 0.7f, 0.5f, 0.6f);
        npc->mood = 0.2f;
        npc->move_speed = 2.5f;
    }

    /* 4: Deckard Cain — Scholar */
    npc = add_npc(mgr, 4, "Deckard Cain", "Scholar", town,
                  LOC_HOME_CAIN, LOC_HOME_CAIN, LOC_HOME_CAIN, 4);
    if (npc) {
        set_traits(&npc->personality, 0.9f, 0.8f, 0.4f, 0.6f, 0.5f);
        set_values(&npc->personality, 0.6f, 0.1f, 0.8f, 0.7f, 0.4f);
        npc->mood = 0.1f;
        npc->move_speed = 2.0f;
    }

    /* 5: Adria — Witch */
    npc = add_npc(mgr, 5, "Adria", "Witch", town,
                  LOC_ADRIA_HUT, LOC_ADRIA_HUT, LOC_ADRIA_HUT, 5);
    if (npc) {
        set_traits(&npc->personality, 0.9f, 0.7f, 0.2f, 0.3f, 0.6f);
        set_values(&npc->personality, 0.7f, 0.4f, 0.3f, 0.2f, 0.5f);
        npc->mood = 0.0f;
        npc->move_speed = 2.0f;
    }

    /* 6: Farnham — Drunk */
    npc = add_npc(mgr, 6, "Farnham", "Drunk", town,
                  LOC_TAVERN, LOC_TAVERN, LOC_TAVERN, 6);
    if (npc) {
        set_traits(&npc->personality, 0.3f, 0.2f, 0.6f, 0.5f, 0.9f);
        set_values(&npc->personality, 0.2f, 0.3f, 0.4f, 0.2f, 0.3f);
        npc->mood = -0.3f;
        npc->stress = 0.7f;
        npc->move_speed = 2.0f;
    }

    /* 7: Wirt — Young Merchant */
    npc = add_npc(mgr, 7, "Wirt", "Young Merchant", town,
                  LOC_WIRT_CORNER, LOC_WIRT_CORNER, LOC_WIRT_CORNER, 7);
    if (npc) {
        set_traits(&npc->personality, 0.5f, 0.4f, 0.5f, 0.3f, 0.7f);
        set_values(&npc->personality, 0.4f, 0.8f, 0.2f, 0.1f, 0.9f);
        npc->mood = -0.1f;
        npc->move_speed = 3.0f;
    }

    /* 8: Lachdanan — Guard Captain */
    npc = add_npc(mgr, 8, "Lachdanan", "Guard Captain", town,
                  LOC_TOWN_SQUARE, LOC_TOWN_SQUARE, LOC_TOWN_SQUARE, 8);
    if (npc) {
        set_traits(&npc->personality, 0.4f, 0.9f, 0.5f, 0.6f, 0.3f);
        set_values(&npc->personality, 0.9f, 0.1f, 0.9f, 0.6f, 0.7f);
        npc->mood = 0.1f;
        npc->move_speed = 3.0f;
    }

    /* 9: Mariel — Priestess */
    npc = add_npc(mgr, 9, "Mariel", "Priestess", town,
                  LOC_CATHEDRAL, LOC_CATHEDRAL, LOC_CATHEDRAL, 9);
    if (npc) {
        set_traits(&npc->personality, 0.6f, 0.8f, 0.4f, 0.8f, 0.4f);
        set_values(&npc->personality, 0.5f, 0.1f, 0.7f, 0.9f, 0.3f);
        npc->mood = 0.2f;
        npc->move_speed = 2.0f;
    }

    /* 10: Tremain — Farmer */
    npc = add_npc(mgr, 10, "Tremain", "Farmer", town,
                  LOC_HOME_TREMAIN, LOC_HOME_TREMAIN, LOC_TOWN_SQUARE, 10);
    if (npc) {
        set_traits(&npc->personality, 0.3f, 0.7f, 0.5f, 0.6f, 0.4f);
        set_values(&npc->personality, 0.5f, 0.4f, 0.7f, 0.5f, 0.8f);
        npc->mood = 0.1f;
        npc->move_speed = 2.5f;
    }

    /* 11: Hilda — Farmer's Wife */
    npc = add_npc(mgr, 11, "Hilda", "Farmer's Wife", town,
                  LOC_HOME_TREMAIN, LOC_HOME_TREMAIN, LOC_HOME_TREMAIN, 11);
    if (npc) {
        set_traits(&npc->personality, 0.4f, 0.8f, 0.5f, 0.7f, 0.4f);
        set_values(&npc->personality, 0.4f, 0.3f, 0.8f, 0.6f, 0.9f);
        npc->mood = 0.1f;
        npc->move_speed = 2.5f;
    }

    /* 12: Theo — Farmer's Son */
    npc = add_npc(mgr, 12, "Theo", "Farmer's Son", town,
                  LOC_HOME_TREMAIN, LOC_HOME_TREMAIN, LOC_TOWN_SQUARE, 12);
    if (npc) {
        set_traits(&npc->personality, 0.8f, 0.3f, 0.8f, 0.7f, 0.3f);
        set_values(&npc->personality, 0.6f, 0.2f, 0.6f, 0.3f, 0.4f);
        npc->mood = 0.3f;
        npc->move_speed = 3.0f;
    }

    /* 13: Old Mag — Herbalist */
    npc = add_npc(mgr, 13, "Old Mag", "Herbalist", town,
                  LOC_GRAVEYARD, LOC_GRAVEYARD, LOC_GRAVEYARD, 13);
    if (npc) {
        set_traits(&npc->personality, 0.8f, 0.4f, 0.3f, 0.4f, 0.6f);
        set_values(&npc->personality, 0.5f, 0.2f, 0.4f, 0.3f, 0.7f);
        npc->mood = 0.0f;
        npc->move_speed = 2.0f;
    }

    /* 14: Sarnac — Traveling Merchant */
    npc = add_npc(mgr, 14, "Sarnac", "Traveling Merchant", town,
                  LOC_TOWN_SQUARE, LOC_TOWN_SQUARE, LOC_TOWN_SQUARE, 14);
    if (npc) {
        set_traits(&npc->personality, 0.7f, 0.5f, 0.8f, 0.6f, 0.3f);
        set_values(&npc->personality, 0.5f, 0.7f, 0.4f, 0.2f, 0.8f);
        npc->mood = 0.2f;
        npc->move_speed = 2.5f;
    }

    /* 15: Malchus — Monk */
    npc = add_npc(mgr, 15, "Malchus", "Monk", town,
                  LOC_CATHEDRAL, LOC_CATHEDRAL, LOC_CATHEDRAL, 15);
    if (npc) {
        set_traits(&npc->personality, 0.6f, 0.9f, 0.2f, 0.7f, 0.3f);
        set_values(&npc->personality, 0.6f, 0.1f, 0.6f, 0.9f, 0.4f);
        npc->mood = 0.2f;
        npc->move_speed = 2.0f;
    }

    /* 16: Lysa — Blacksmith Apprentice */
    npc = add_npc(mgr, 16, "Lysa", "Blacksmith Apprentice", town,
                  LOC_BLACKSMITH, LOC_BLACKSMITH, LOC_BLACKSMITH, 16);
    if (npc) {
        set_traits(&npc->personality, 0.6f, 0.7f, 0.6f, 0.5f, 0.4f);
        set_values(&npc->personality, 0.6f, 0.5f, 0.5f, 0.3f, 0.7f);
        npc->mood = 0.2f;
        npc->move_speed = 2.5f;
    }

    /* 17: Gharbad — Outcast */
    npc = add_npc(mgr, 17, "Gharbad", "Outcast", town,
                  LOC_TOWN_SQUARE, LOC_TOWN_SQUARE, LOC_TOWN_SQUARE, 17);
    if (npc) {
        set_traits(&npc->personality, 0.4f, 0.3f, 0.3f, 0.4f, 0.8f);
        set_values(&npc->personality, 0.3f, 0.6f, 0.3f, 0.2f, 0.5f);
        npc->mood = -0.2f;
        npc->stress = 0.5f;
        npc->move_speed = 2.0f;
    }

    /* 18: Lazarus' Shadow — not yet in town */
    npc = add_npc(mgr, 18, "Lazarus' Shadow", "Unknown", town,
                  LOC_CATHEDRAL, LOC_CATHEDRAL, LOC_CATHEDRAL, 18);
    if (npc) {
        set_traits(&npc->personality, 0.8f, 0.7f, 0.3f, 0.2f, 0.5f);
        set_values(&npc->personality, 0.4f, 0.7f, 0.2f, 0.8f, 0.6f);
        npc->is_alive = true;
        npc->has_left_town = true;
        npc->mood = 0.0f;
        npc->move_speed = 2.5f;
    }

    /* 19: The Stranger — not yet in town */
    npc = add_npc(mgr, 19, "The Stranger", "Unknown", town,
                  LOC_TOWN_SQUARE, LOC_TOWN_SQUARE, LOC_TOWN_SQUARE, 19);
    if (npc) {
        set_traits(&npc->personality, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f);
        set_values(&npc->personality, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f);
        npc->is_alive = true;
        npc->has_left_town = true;
        npc->mood = 0.0f;
        npc->move_speed = 2.5f;
    }
}
