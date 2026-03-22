#ifndef DIABLO_WORLD_EVENT_H
#define DIABLO_WORLD_EVENT_H

#include <stdbool.h>

/* Forward declarations */
struct NPCManager;
struct RelationshipGraph;
struct MemorySystem;

#define MAX_PENDING_EVENTS 64

typedef enum {
    EVT_PLAYER_HELPED_NPC,
    EVT_PLAYER_HARMED_NPC,
    EVT_PLAYER_COMPLETED_QUEST,
    EVT_PLAYER_FAILED_QUEST,
    EVT_NPC_DIED,
    EVT_NPC_LEFT_TOWN,
    EVT_MONSTER_KILLED,
    EVT_ITEM_STOLEN,
    EVT_GOSSIP_GOOD,
    EVT_GOSSIP_BAD,
    EVT_THREAT_INCREASED,
    EVT_THREAT_DECREASED,
    EVT_TYPE_COUNT
} WorldEventType;

typedef struct WorldEvent {
    WorldEventType type;
    int source_id;          /* who caused it (-1 = player) */
    int target_id;          /* who it directly affects (-1 = town-wide) */
    float magnitude;        /* 0.0 to 1.0 */
    int propagation_hops;   /* remaining hops through relationship graph */
    int day_occurred;
} WorldEvent;

typedef struct EventQueue {
    WorldEvent events[MAX_PENDING_EVENTS];
    int count;
} EventQueue;

void event_queue_init(EventQueue *eq);
void event_push(EventQueue *eq, WorldEventType type, int source, int target,
                float magnitude, int hops, int day);
bool event_pop(EventQueue *eq, WorldEvent *out);

/* Process all pending events: update NPC moods, memories, trust, generate secondary events */
void event_process_all(EventQueue *eq, struct NPCManager *npcs,
                       struct RelationshipGraph *graph,
                       struct MemorySystem *memory, int game_day);

#endif /* DIABLO_WORLD_EVENT_H */
