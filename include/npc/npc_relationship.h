#ifndef DIABLO_NPC_RELATIONSHIP_H
#define DIABLO_NPC_RELATIONSHIP_H

#include "npc/npc.h"

typedef enum {
    REL_STRANGER,
    REL_ACQUAINTANCE,
    REL_FRIEND,
    REL_CLOSE_FRIEND,
    REL_FAMILY,
    REL_ROMANTIC,
    REL_RIVAL,
    REL_ENEMY
} RelationType;

typedef struct Relationship {
    float affection;    /* -1.0 (hatred) to 1.0 (love) */
    float trust;        /* 0.0 to 1.0 */
    float respect;      /* -1.0 (contempt) to 1.0 (reverence) */
    RelationType type;
    int shared_history; /* count of meaningful interactions */
} Relationship;

typedef struct RelationshipGraph {
    Relationship matrix[MAX_TOWN_NPCS][MAX_TOWN_NPCS]; /* directed: [from][to] */
} RelationshipGraph;

void relationship_init(RelationshipGraph *graph);
void relationship_set(RelationshipGraph *graph, int from, int to,
                      float affection, float trust, float respect,
                      RelationType type);
Relationship *relationship_get(RelationshipGraph *graph, int from, int to);
void relationship_modify(RelationshipGraph *graph, int from, int to,
                         float d_affection, float d_trust, float d_respect);
float relationship_strength(const Relationship *rel); /* combined metric */

/* Initialize all default Tristram relationships */
void relationship_init_defaults(RelationshipGraph *graph);

#endif /* DIABLO_NPC_RELATIONSHIP_H */
