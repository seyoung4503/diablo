#ifndef DIABLO_NPC_MEMORY_H
#define DIABLO_NPC_MEMORY_H

#include "npc/npc.h"

#define MAX_MEMORIES_PER_NPC 32

typedef enum {
    MEM_PLAYER_HELPED,
    MEM_PLAYER_BETRAYED,
    MEM_PLAYER_IGNORED,
    MEM_NPC_DIED,
    MEM_NPC_INSULTED,
    MEM_NPC_PRAISED,
    MEM_QUEST_COMPLETED,
    MEM_QUEST_FAILED,
    MEM_HEARD_GOOD_NEWS,
    MEM_HEARD_BAD_NEWS,
    MEM_WITNESSED_COMBAT,
    MEM_RECEIVED_GIFT,
    MEM_WAS_THREATENED,
    MEM_EVENT_COUNT
} MemoryEventType;

typedef struct NPCMemory {
    MemoryEventType event_type;
    int subject_id;         /* who did it (-1 = player) */
    int object_id;          /* who it happened to */
    int game_day;           /* when */
    float emotional_weight; /* -1.0 to 1.0 (how much they care) */
    float salience;         /* 0.0 to 1.0 (how vivid, decays over time) */
} NPCMemory;

typedef struct MemoryBank {
    NPCMemory memories[MAX_MEMORIES_PER_NPC];
    int count;
    int write_index; /* ring buffer index */
} MemoryBank;

/* One memory bank per NPC */
typedef struct MemorySystem {
    MemoryBank banks[MAX_TOWN_NPCS];
} MemorySystem;

void memory_system_init(MemorySystem *sys);
void memory_add(MemorySystem *sys, int npc_id, MemoryEventType type,
                int subject_id, int object_id, int game_day,
                float emotional_weight);
void memory_decay(MemorySystem *sys, int game_day); /* decay salience for all NPCs */
int memory_count_about(const MemorySystem *sys, int npc_id, int subject_id);
float memory_sentiment_about(const MemorySystem *sys, int npc_id, int subject_id);

#endif /* DIABLO_NPC_MEMORY_H */
