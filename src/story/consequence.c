#include "story/consequence.h"
#include "npc/npc.h"
#include "npc/npc_relationship.h"
#include "npc/npc_memory.h"
#include "npc/world_event.h"
#include <string.h>

void consequence_apply_quest_outcome(const QuestOutcome *outcome,
                                     NPCManager *npcs,
                                     RelationshipGraph *graph,
                                     MemorySystem *memory,
                                     EventQueue *events,
                                     int game_day)
{
    if (!outcome || !npcs || !events) return;

    int evt_count = outcome->event_count < 4 ? outcome->event_count : 4;
    for (int i = 0; i < evt_count; i++) {
        WorldEventType evt_type = (WorldEventType)outcome->world_events[i];
        int target = outcome->event_targets[i];
        float mag = outcome->event_magnitudes[i];

        /* Push the event into the queue for normal event processing */
        event_push(events, evt_type, -1, target, mag, 3, game_day);

        /* Apply direct NPC consequences based on event type */
        NPC *target_npc = (target >= 0) ? npc_manager_get(npcs, target) : NULL;

        switch (evt_type) {
        case EVT_PLAYER_HELPED_NPC:
            if (target_npc) {
                target_npc->trust_player += mag;
                if (target_npc->trust_player > 1.0f)
                    target_npc->trust_player = 1.0f;
                target_npc->mood += mag * 0.5f;
                if (target_npc->mood > 1.0f)
                    target_npc->mood = 1.0f;
            }
            if (memory && target >= 0) {
                memory_add(memory, target, MEM_PLAYER_HELPED,
                           -1, target, game_day, mag);
            }
            break;

        case EVT_PLAYER_HARMED_NPC:
            if (target_npc) {
                target_npc->trust_player -= mag;
                if (target_npc->trust_player < -1.0f)
                    target_npc->trust_player = -1.0f;
                target_npc->mood -= mag * 0.5f;
                if (target_npc->mood < -1.0f)
                    target_npc->mood = -1.0f;
            }
            if (memory && target >= 0) {
                memory_add(memory, target, MEM_PLAYER_BETRAYED,
                           -1, target, game_day, -mag);
            }
            break;

        case EVT_NPC_LEFT_TOWN:
            if (target_npc) {
                target_npc->has_left_town = true;
            }
            /* Other NPCs who were close to them feel sad */
            if (graph && target >= 0) {
                for (int n = 0; n < npcs->count; n++) {
                    if (npcs->npcs[n].id == target) continue;
                    Relationship *rel = relationship_get(graph,
                        npcs->npcs[n].id, target);
                    if (rel && rel->affection > 0.3f) {
                        npcs->npcs[n].mood -= 0.1f;
                        if (npcs->npcs[n].mood < -1.0f)
                            npcs->npcs[n].mood = -1.0f;
                    }
                }
            }
            break;

        case EVT_THREAT_INCREASED:
            /* Town-wide stress increase */
            for (int n = 0; n < npcs->count; n++) {
                npcs->npcs[n].stress += mag * 0.3f;
                if (npcs->npcs[n].stress > 1.0f)
                    npcs->npcs[n].stress = 1.0f;
                npcs->npcs[n].mood -= mag * 0.2f;
                if (npcs->npcs[n].mood < -1.0f)
                    npcs->npcs[n].mood = -1.0f;
            }
            break;

        case EVT_THREAT_DECREASED:
            /* Town-wide relief */
            for (int n = 0; n < npcs->count; n++) {
                npcs->npcs[n].stress -= mag * 0.2f;
                if (npcs->npcs[n].stress < 0.0f)
                    npcs->npcs[n].stress = 0.0f;
                npcs->npcs[n].mood += mag * 0.1f;
                if (npcs->npcs[n].mood > 1.0f)
                    npcs->npcs[n].mood = 1.0f;
            }
            break;

        case EVT_GOSSIP_GOOD:
            /* Positive gossip: small trust boost from everyone */
            for (int n = 0; n < npcs->count; n++) {
                if (target >= 0 && npcs->npcs[n].id != target) continue;
                npcs->npcs[n].trust_player += mag * 0.1f;
                if (npcs->npcs[n].trust_player > 1.0f)
                    npcs->npcs[n].trust_player = 1.0f;
            }
            if (memory) {
                for (int n = 0; n < npcs->count; n++) {
                    memory_add(memory, npcs->npcs[n].id, MEM_HEARD_GOOD_NEWS,
                               -1, target, game_day, mag * 0.5f);
                }
            }
            break;

        case EVT_GOSSIP_BAD:
            /* Negative gossip: small trust decrease */
            for (int n = 0; n < npcs->count; n++) {
                if (target >= 0 && npcs->npcs[n].id != target) continue;
                npcs->npcs[n].trust_player -= mag * 0.1f;
                if (npcs->npcs[n].trust_player < -1.0f)
                    npcs->npcs[n].trust_player = -1.0f;
            }
            if (memory) {
                for (int n = 0; n < npcs->count; n++) {
                    memory_add(memory, npcs->npcs[n].id, MEM_HEARD_BAD_NEWS,
                               -1, target, game_day, -mag * 0.5f);
                }
            }
            break;

        case EVT_PLAYER_COMPLETED_QUEST:
            if (memory) {
                for (int n = 0; n < npcs->count; n++) {
                    memory_add(memory, npcs->npcs[n].id, MEM_QUEST_COMPLETED,
                               -1, -1, game_day, mag);
                }
            }
            break;

        default:
            break;
        }
    }
}
