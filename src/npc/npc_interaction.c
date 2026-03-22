#include "npc/npc_interaction.h"
#include "npc/npc_relationship.h"
#include "npc/npc_memory.h"
#include "npc/world_event.h"

/* Track which NPCs have already interacted this tick (max 1 per hour) */
static bool interacted_this_tick[MAX_TOWN_NPCS];

/* Choose interaction type based on relationship and personality */
static InteractionType choose_interaction_type(const NPC *a, const NPC *b,
                                               const Relationship *rel)
{
    /* Comfort if affectionate and other is in bad mood */
    if (rel->affection > 0.5f && b->mood < -0.3f)
        return INTERACT_COMFORT;

    /* Argue if low affection */
    if (rel->affection < -0.3f)
        return INTERACT_ARGUE;

    /* Gossip if extraverted */
    if (a->personality.traits[TRAIT_EXTRAVERSION] > 0.6f)
        return INTERACT_GOSSIP;

    /* Trade if both have economic need */
    if (a->need_economic > 0.5f && b->need_economic > 0.5f)
        return INTERACT_TRADE;

    /* Default: friendly chat */
    return INTERACT_CHAT;
}

bool npc_should_interact(const NPC *a, const NPC *b,
                         const RelationshipGraph *graph)
{
    (void)graph;

    /* Both must be alive, in town, and at the same location */
    if (!a->is_alive || !b->is_alive)
        return false;
    if (a->has_left_town || b->has_left_town)
        return false;
    if (a->current_location != b->current_location)
        return false;

    /* Neither can be sleeping */
    if (a->current_action == NPC_ACTION_SLEEP || b->current_action == NPC_ACTION_SLEEP)
        return false;

    return true;
}

NPCInteraction npc_resolve_interaction(NPC *a, NPC *b,
                                       RelationshipGraph *graph,
                                       MemorySystem *memory,
                                       EventQueue *events, int game_day)
{
    NPCInteraction result;
    result.npc_a = a->id;
    result.npc_b = b->id;
    result.outcome = 0.0f;
    result.type = INTERACT_CHAT;

    Relationship *rel_ab = relationship_get(graph, a->id, b->id);
    Relationship *rel_ba = relationship_get(graph, b->id, a->id);

    /* Guard against NULL (out-of-range IDs) */
    if (!rel_ab || !rel_ba)
        return result;

    result.type = choose_interaction_type(a, b, rel_ab);

    switch (result.type) {
    case INTERACT_CHAT:
        /* Both mood +0.1, social need -0.2, affection +0.02 */
        a->mood = CLAMP(a->mood + 0.1f, -1.0f, 1.0f);
        b->mood = CLAMP(b->mood + 0.1f, -1.0f, 1.0f);
        a->need_social = CLAMP(a->need_social - 0.2f, 0.0f, 1.0f);
        b->need_social = CLAMP(b->need_social - 0.2f, 0.0f, 1.0f);
        relationship_modify(graph, a->id, b->id, 0.02f, 0.0f, 0.0f);
        relationship_modify(graph, b->id, a->id, 0.02f, 0.0f, 0.0f);
        result.outcome = 0.1f;
        break;

    case INTERACT_GOSSIP:
        /* Spreads memories, social need -0.3 */
        a->need_social = CLAMP(a->need_social - 0.3f, 0.0f, 1.0f);
        b->need_social = CLAMP(b->need_social - 0.3f, 0.0f, 1.0f);
        /* Push gossip event — memory is written by event_process_all
         * (avoids double-counting if we also wrote it here) */
        if (rand() % 100 < 50) {
            event_push(events, EVT_GOSSIP_GOOD, a->id, b->id, 0.3f, 1, game_day);
        } else {
            event_push(events, EVT_GOSSIP_BAD, a->id, b->id, 0.3f, 1, game_day);
        }
        result.outcome = 0.05f;
        break;

    case INTERACT_ARGUE:
        /* Both mood -0.1, stress +0.1, respect changes */
        a->mood = CLAMP(a->mood - 0.1f, -1.0f, 1.0f);
        b->mood = CLAMP(b->mood - 0.1f, -1.0f, 1.0f);
        a->stress = CLAMP(a->stress + 0.1f, 0.0f, 1.0f);
        b->stress = CLAMP(b->stress + 0.1f, 0.0f, 1.0f);
        /* Dominant personality gains respect, other loses */
        if (a->personality.values[VALUE_COURAGE] > b->personality.values[VALUE_COURAGE]) {
            relationship_modify(graph, b->id, a->id, 0.0f, 0.0f, 0.05f);
            relationship_modify(graph, a->id, b->id, 0.0f, 0.0f, -0.05f);
        } else {
            relationship_modify(graph, a->id, b->id, 0.0f, 0.0f, 0.05f);
            relationship_modify(graph, b->id, a->id, 0.0f, 0.0f, -0.05f);
        }
        memory_add(memory, a->id, MEM_NPC_INSULTED, b->id, a->id,
                   game_day, -0.3f);
        memory_add(memory, b->id, MEM_NPC_INSULTED, a->id, b->id,
                   game_day, -0.3f);
        result.outcome = -0.1f;
        break;

    case INTERACT_COMFORT:
        /* Target mood +0.2, actor social need -0.3, trust +0.05 */
        b->mood = CLAMP(b->mood + 0.2f, -1.0f, 1.0f);
        a->need_social = CLAMP(a->need_social - 0.3f, 0.0f, 1.0f);
        relationship_modify(graph, b->id, a->id, 0.03f, 0.05f, 0.0f);
        result.outcome = 0.2f;
        break;

    case INTERACT_TRADE:
        /* Economic need -0.3 for both */
        a->need_economic = CLAMP(a->need_economic - 0.3f, 0.0f, 1.0f);
        b->need_economic = CLAMP(b->need_economic - 0.3f, 0.0f, 1.0f);
        relationship_modify(graph, a->id, b->id, 0.01f, 0.02f, 0.0f);
        relationship_modify(graph, b->id, a->id, 0.01f, 0.02f, 0.0f);
        result.outcome = 0.15f;
        break;

    default:
        break;
    }

    /* shared_history is already incremented by relationship_modify() */

    return result;
}

void npc_process_interactions(NPCManager *mgr, RelationshipGraph *graph,
                              MemorySystem *memory, EventQueue *events,
                              int game_day)
{
    /* Reset interaction tracking */
    for (int i = 0; i < MAX_TOWN_NPCS; i++)
        interacted_this_tick[i] = false;

    /* Check all pairs of NPCs for possible interactions */
    for (int i = 0; i < mgr->count; i++) {
        if (interacted_this_tick[i])
            continue;

        NPC *a = &mgr->npcs[i];

        for (int j = i + 1; j < mgr->count; j++) {
            if (interacted_this_tick[j])
                continue;

            NPC *b = &mgr->npcs[j];

            if (!npc_should_interact(a, b, graph))
                continue;

            /* Resolve the interaction */
            npc_resolve_interaction(a, b, graph, memory, events, game_day);

            /* Mark both as having interacted (max 1 per hour) */
            interacted_this_tick[i] = true;
            interacted_this_tick[j] = true;
            break; /* NPC a is done for this tick */
        }
    }
}
