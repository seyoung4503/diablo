#ifndef DIABLO_STORY_CONSEQUENCE_H
#define DIABLO_STORY_CONSEQUENCE_H

#include "story/quest.h"

/* Forward declarations */
struct NPCManager;
struct RelationshipGraph;
struct MemorySystem;
struct EventQueue;

/* Apply quest outcome consequences to the world:
 * - Fires world events from the outcome
 * - Updates NPC trust/mood based on outcome effects
 * - Records memories for affected NPCs */
void consequence_apply_quest_outcome(const QuestOutcome *outcome,
                                     struct NPCManager *npcs,
                                     struct RelationshipGraph *graph,
                                     struct MemorySystem *memory,
                                     struct EventQueue *events,
                                     int game_day);

#endif /* DIABLO_STORY_CONSEQUENCE_H */
