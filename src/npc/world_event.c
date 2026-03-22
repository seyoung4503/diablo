#include "npc/world_event.h"
#include "npc/npc.h"
#include "npc/npc_relationship.h"
#include "npc/npc_memory.h"
#include <string.h>

void event_queue_init(EventQueue *eq)
{
    memset(eq, 0, sizeof(*eq));
}

void event_push(EventQueue *eq, WorldEventType type, int source, int target,
                float magnitude, int hops, int day)
{
    if (eq->count >= MAX_PENDING_EVENTS) return;

    WorldEvent *evt = &eq->events[eq->count++];
    evt->type             = type;
    evt->source_id        = source;
    evt->target_id        = target;
    evt->magnitude        = CLAMP(magnitude, 0.0f, 1.0f);
    evt->propagation_hops = hops;
    evt->day_occurred     = day;
}

bool event_pop(EventQueue *eq, WorldEvent *out)
{
    if (eq->count <= 0) return false;

    *out = eq->events[0];
    /* Shift remaining events forward */
    eq->count--;
    for (int i = 0; i < eq->count; i++)
        eq->events[i] = eq->events[i + 1];
    return true;
}

/* Map event types to memory event types and mood deltas */
static MemoryEventType event_to_memory(WorldEventType type)
{
    switch (type) {
    case EVT_PLAYER_HELPED_NPC:     return MEM_PLAYER_HELPED;
    case EVT_PLAYER_HARMED_NPC:     return MEM_PLAYER_BETRAYED;
    case EVT_PLAYER_COMPLETED_QUEST: return MEM_QUEST_COMPLETED;
    case EVT_PLAYER_FAILED_QUEST:   return MEM_QUEST_FAILED;
    case EVT_NPC_DIED:              return MEM_NPC_DIED;
    case EVT_NPC_LEFT_TOWN:         return MEM_HEARD_BAD_NEWS;
    case EVT_MONSTER_KILLED:        return MEM_WITNESSED_COMBAT;
    case EVT_ITEM_STOLEN:           return MEM_PLAYER_BETRAYED;
    case EVT_GOSSIP_GOOD:           return MEM_HEARD_GOOD_NEWS;
    case EVT_GOSSIP_BAD:            return MEM_HEARD_BAD_NEWS;
    case EVT_THREAT_INCREASED:      return MEM_WAS_THREATENED;
    case EVT_THREAT_DECREASED:      return MEM_HEARD_GOOD_NEWS;
    default:                        return MEM_HEARD_BAD_NEWS;
    }
}

/* Returns mood delta sign: positive events boost mood, negative ones lower it */
static float event_mood_sign(WorldEventType type)
{
    switch (type) {
    case EVT_PLAYER_HELPED_NPC:
    case EVT_PLAYER_COMPLETED_QUEST:
    case EVT_MONSTER_KILLED:
    case EVT_GOSSIP_GOOD:
    case EVT_THREAT_DECREASED:
        return 1.0f;
    case EVT_PLAYER_HARMED_NPC:
    case EVT_PLAYER_FAILED_QUEST:
    case EVT_NPC_DIED:
    case EVT_NPC_LEFT_TOWN:
    case EVT_ITEM_STOLEN:
    case EVT_GOSSIP_BAD:
    case EVT_THREAT_INCREASED:
        return -1.0f;
    default:
        return 0.0f;
    }
}

/* Process a single event for one NPC */
static void process_event_for_npc(NPC *npc, const WorldEvent *evt,
                                  RelationshipGraph *graph,
                                  MemorySystem *memory,
                                  EventQueue *eq, int game_day)
{
    if (!npc->is_alive || npc->has_left_town) return;

    int npc_id = npc->id;

    /* a. Update mood based on event type and magnitude */
    float mood_delta = event_mood_sign(evt->type) * evt->magnitude * 0.3f;
    npc->mood = CLAMP(npc->mood + mood_delta, -1.0f, 1.0f);

    /* b. Add memory */
    float emotional_weight = event_mood_sign(evt->type) * evt->magnitude;
    memory_add(memory, npc_id, event_to_memory(evt->type),
               evt->source_id, evt->target_id, game_day, emotional_weight);

    /* c. Update trust_player if source is player */
    if (evt->source_id == -1) {
        float trust_delta = event_mood_sign(evt->type) * evt->magnitude * 0.2f;
        npc->trust_player = CLAMP(npc->trust_player + trust_delta, -1.0f, 1.0f);
    }

    /* d. Gossip propagation: extraverted NPCs spread news */
    if (evt->propagation_hops > 0 &&
        npc->personality.traits[TRAIT_EXTRAVERSION] > 0.5f) {
        float new_magnitude = evt->magnitude * 0.6f;
        if (new_magnitude >= 0.05f) {
            /* Spread to NPCs this one trusts */
            for (int i = 0; i < MAX_TOWN_NPCS; i++) {
                if (i == npc_id) continue;
                Relationship *rel = relationship_get(graph, npc_id, i);
                if (rel && rel->trust > 0.3f) {
                    WorldEventType gossip_type =
                        (event_mood_sign(evt->type) >= 0.0f)
                            ? EVT_GOSSIP_GOOD : EVT_GOSSIP_BAD;
                    event_push(eq, gossip_type, npc_id, i,
                               new_magnitude, evt->propagation_hops - 1,
                               game_day);
                }
            }
        }
    }
}

void event_process_all(EventQueue *eq, struct NPCManager *npcs,
                       struct RelationshipGraph *graph,
                       struct MemorySystem *memory, int game_day)
{
    /* Process events iteratively; new gossip events may be appended during processing */
    WorldEvent evt;
    int safety = 0;
    const int max_iterations = MAX_PENDING_EVENTS * 4; /* prevent infinite loops */

    while (event_pop(eq, &evt) && safety++ < max_iterations) {
        if (evt.magnitude < 0.05f) continue;

        if (evt.target_id >= 0 && evt.target_id < npcs->count) {
            /* Targeted event: process for specific NPC */
            process_event_for_npc(&npcs->npcs[evt.target_id], &evt,
                                  (RelationshipGraph *)graph,
                                  (MemorySystem *)memory, eq, game_day);
        } else if (evt.target_id == -1) {
            /* Town-wide event: process for all NPCs */
            for (int i = 0; i < npcs->count; i++) {
                process_event_for_npc(&npcs->npcs[i], &evt,
                                      (RelationshipGraph *)graph,
                                      (MemorySystem *)memory, eq, game_day);
            }
        }
    }
}
