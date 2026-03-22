#ifndef DIABLO_NPC_AI_H
#define DIABLO_NPC_AI_H

#include "npc/npc.h"

/* Forward declarations */
struct Town;
struct RelationshipGraph;
struct MemorySystem;
struct EventQueue;
struct NPCManager;

/* Score an action for an NPC based on personality, needs, relationships, time */
float npc_ai_score_action(const NPC *npc, NPCAction action, int game_hour,
                          const struct RelationshipGraph *graph,
                          const struct MemorySystem *memory);

/* Choose the best action for an NPC this hour */
NPCAction npc_ai_decide(const NPC *npc, int game_hour,
                         const struct RelationshipGraph *graph,
                         const struct MemorySystem *memory);

/* Update all NPCs' AI decisions and movements.
 * Called once per game hour (or when an event triggers re-evaluation). */
void npc_ai_update_all(struct NPCManager *mgr, int game_hour, int game_day,
                       const struct Town *town, struct RelationshipGraph *graph,
                       struct MemorySystem *memory, struct EventQueue *events);

#endif /* DIABLO_NPC_AI_H */
