#ifndef DIABLO_STORY_ARC_H
#define DIABLO_STORY_ARC_H

#include "npc/npc.h"
#include "story/quest.h"

#define MAX_ARC_STAGES 5

typedef struct StoryArcStage {
    int required_quest_id;    /* quest that must be completed (or -1) */
    int required_outcome;     /* which outcome (-1 = any) */
    int required_day;         /* minimum game day (-1 = any) */
    char description[256];    /* what happens at this stage */
} StoryArcStage;

typedef struct StoryArc {
    int npc_id;
    StoryArcStage stages[MAX_ARC_STAGES];
    int stage_count;
} StoryArc;

typedef struct StoryArcSystem {
    StoryArc arcs[MAX_TOWN_NPCS];
    int count;
} StoryArcSystem;

void story_arc_init(StoryArcSystem *sys);
void story_arc_update(StoryArcSystem *sys, NPCManager *npcs,
                      const QuestLog *ql, int game_day);

#endif /* DIABLO_STORY_ARC_H */
