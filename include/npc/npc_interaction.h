#ifndef DIABLO_NPC_INTERACTION_H
#define DIABLO_NPC_INTERACTION_H

#include "npc/npc.h"

/* Forward declarations */
struct RelationshipGraph;
struct MemorySystem;
struct EventQueue;
struct NPCManager;

typedef enum {
    INTERACT_CHAT,
    INTERACT_GOSSIP,
    INTERACT_ARGUE,
    INTERACT_COMFORT,
    INTERACT_TRADE,
    INTERACT_COUNT
} InteractionType;

typedef struct NPCInteraction {
    int npc_a;
    int npc_b;
    InteractionType type;
    float outcome;  /* positive or negative */
} NPCInteraction;

/* Check if two NPCs at the same location should interact */
bool npc_should_interact(const NPC *a, const NPC *b,
                         const struct RelationshipGraph *graph);

/* Resolve an interaction between two NPCs */
NPCInteraction npc_resolve_interaction(NPC *a, NPC *b,
                                       struct RelationshipGraph *graph,
                                       struct MemorySystem *memory,
                                       struct EventQueue *events, int game_day);

/* Process all possible interactions this tick (check co-located NPCs) */
void npc_process_interactions(struct NPCManager *mgr,
                              struct RelationshipGraph *graph,
                              struct MemorySystem *memory,
                              struct EventQueue *events, int game_day);

#endif /* DIABLO_NPC_INTERACTION_H */
