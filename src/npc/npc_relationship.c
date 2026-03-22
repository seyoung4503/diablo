#include "npc/npc_relationship.h"
#include <string.h>
#include <math.h>

void relationship_init(RelationshipGraph *graph)
{
    memset(graph, 0, sizeof(*graph));
    /* All relationships default to REL_STRANGER with zeroed values */
}

void relationship_set(RelationshipGraph *graph, int from, int to,
                      float affection, float trust, float respect,
                      RelationType type)
{
    if (from < 0 || from >= MAX_TOWN_NPCS || to < 0 || to >= MAX_TOWN_NPCS)
        return;

    Relationship *rel = &graph->matrix[from][to];
    rel->affection = CLAMP(affection, -1.0f, 1.0f);
    rel->trust     = CLAMP(trust, 0.0f, 1.0f);
    rel->respect   = CLAMP(respect, -1.0f, 1.0f);
    rel->type      = type;
}

Relationship *relationship_get(RelationshipGraph *graph, int from, int to)
{
    if (from < 0 || from >= MAX_TOWN_NPCS || to < 0 || to >= MAX_TOWN_NPCS)
        return NULL;
    return &graph->matrix[from][to];
}

void relationship_modify(RelationshipGraph *graph, int from, int to,
                         float d_affection, float d_trust, float d_respect)
{
    if (from < 0 || from >= MAX_TOWN_NPCS || to < 0 || to >= MAX_TOWN_NPCS)
        return;

    Relationship *rel = &graph->matrix[from][to];
    rel->affection = CLAMP(rel->affection + d_affection, -1.0f, 1.0f);
    rel->trust     = CLAMP(rel->trust + d_trust, 0.0f, 1.0f);
    rel->respect   = CLAMP(rel->respect + d_respect, -1.0f, 1.0f);
    rel->shared_history++;
}

float relationship_strength(const Relationship *rel)
{
    if (!rel) return 0.0f;
    /* Combined metric: weighted average of absolute affection, trust, and respect */
    return (fabsf(rel->affection) * 0.4f +
            rel->trust * 0.35f +
            fabsf(rel->respect) * 0.25f);
}

/* Helper to set a bidirectional relationship */
static void set_bidirectional(RelationshipGraph *graph, int a, int b,
                              float aff, float trust, float resp,
                              RelationType type)
{
    relationship_set(graph, a, b, aff, trust, resp, type);
    relationship_set(graph, b, a, aff, trust, resp, type);
}

void relationship_init_defaults(RelationshipGraph *graph)
{
    /* NPC IDs:
     *  0=Griswold, 1=Pepin, 2=Ogden, 3=Gillian, 4=Cain, 5=Adria,
     *  6=Farnham, 7=Wirt, 8=Lachdanan, 9=Mariel, 10=Tremain, 11=Hilda,
     *  12=Theo, 13=Old Mag, 14=Lester, 15=Malchus, 16=Lysa, 17=Gharbad
     */

    /* Griswold(0) <-> Ogden(2): friends */
    set_bidirectional(graph, 0, 2, 0.6f, 0.8f, 0.5f, REL_FRIEND);

    /* Griswold(0) -> Lysa(16): mentor */
    relationship_set(graph, 0, 16, 0.4f, 0.6f, 0.7f, REL_ACQUAINTANCE);

    /* Ogden(2) -> Gillian(3): uncle (family) */
    relationship_set(graph, 2, 3, 0.9f, 0.8f, 0.4f, REL_FAMILY);

    /* Pepin(1) <-> Cain(4): colleagues */
    set_bidirectional(graph, 1, 4, 0.5f, 0.7f, 0.9f, REL_FRIEND);

    /* Pepin(1) -> Malchus(15): mentor */
    relationship_set(graph, 1, 15, 0.4f, 0.6f, 0.5f, REL_ACQUAINTANCE);

    /* Tremain(10) <-> Hilda(11): married */
    set_bidirectional(graph, 10, 11, 0.8f, 0.9f, 0.7f, REL_FAMILY);

    /* Tremain(10) -> Theo(12): parent */
    relationship_set(graph, 10, 12, 1.0f, 0.8f, 0.3f, REL_FAMILY);
    /* Hilda(11) -> Theo(12): parent */
    relationship_set(graph, 11, 12, 1.0f, 0.8f, 0.3f, REL_FAMILY);

    /* Wirt(7) -> Griswold(0): resentment */
    relationship_set(graph, 7, 0, -0.3f, 0.2f, 0.4f, REL_RIVAL);

    /* Lachdanan(8) -> Mariel(9): respectful */
    relationship_set(graph, 8, 9, 0.3f, 0.6f, 0.7f, REL_ACQUAINTANCE);

    /* Farnham(6) -> Ogden(2): dependency */
    relationship_set(graph, 6, 2, 0.3f, 0.5f, 0.3f, REL_ACQUAINTANCE);

    /* Adria(5) -> all: distant */
    for (int i = 0; i < MAX_TOWN_NPCS; i++) {
        if (i == 5) continue;
        relationship_set(graph, 5, i, 0.0f, 0.1f, 0.2f, REL_STRANGER);
    }

    /* Cain(4) -> Adria(5): wary */
    relationship_set(graph, 4, 5, 0.0f, 0.2f, 0.5f, REL_ACQUAINTANCE);

    /* Gharbad(17) -> most: negative */
    for (int i = 0; i < MAX_TOWN_NPCS; i++) {
        if (i == 17) continue;
        relationship_set(graph, 17, i, -0.2f, 0.1f, 0.1f, REL_STRANGER);
    }

    /* Pepin(1) -> Gharbad(17): compassion */
    relationship_set(graph, 1, 17, 0.3f, 0.3f, 0.2f, REL_ACQUAINTANCE);

    /* Old Mag(13) -> Adria(5): rivalry */
    relationship_set(graph, 13, 5, -0.2f, -0.1f, 0.4f, REL_RIVAL);
}
