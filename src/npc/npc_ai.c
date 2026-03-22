#include "npc/npc_ai.h"
#include "npc/npc_schedule.h"
#include "npc/npc_relationship.h"
#include "npc/npc_memory.h"
#include "npc/world_event.h"
#include "world/town.h"

/* Small random perturbation in range [-0.5, +0.5] */
static float random_perturbation(void)
{
    return ((float)(rand() % 100) / 100.0f) - 0.5f;
}

/* Check if hour falls in night range (22-6) */
static bool is_night_hour(int hour)
{
    return hour >= 22 || hour < 6;
}

/* Check if hour is a meal hour */
static bool is_meal_hour(int hour)
{
    return hour == 7 || hour == 12 || hour == 18;
}

float npc_ai_score_action(const NPC *npc, NPCAction action, int game_hour,
                          const RelationshipGraph *graph,
                          const MemorySystem *memory)
{
    (void)graph;
    (void)memory;

    const NPCPersonality *p = &npc->personality;
    float score = 0.0f;

    switch (action) {
    case NPC_ACTION_WORK:
        score = 5.0f * p->traits[TRAIT_CONSCIENTIOUSNESS];
        break;
    case NPC_ACTION_SOCIALIZE:
        score = 5.0f * p->traits[TRAIT_EXTRAVERSION] * (1.0f + npc->need_social);
        break;
    case NPC_ACTION_PRAY:
        score = 3.0f * p->values[VALUE_PIETY];
        break;
    case NPC_ACTION_DRINK:
        score = 3.0f * (npc->stress + p->traits[TRAIT_NEUROTICISM]);
        break;
    case NPC_ACTION_PATROL:
        score = 4.0f * p->values[VALUE_COURAGE] * p->traits[TRAIT_CONSCIENTIOUSNESS];
        break;
    case NPC_ACTION_WANDER:
        score = 2.0f * p->traits[TRAIT_OPENNESS];
        break;
    case NPC_ACTION_EAT:
        score = 4.0f;
        if (is_meal_hour(game_hour))
            score += 3.0f;
        break;
    case NPC_ACTION_SLEEP:
        score = is_night_hour(game_hour) ? 6.0f : 0.0f;
        break;
    case NPC_ACTION_IDLE:
        score = 1.0f;
        break;
    default:
        score = 0.0f;
        break;
    }

    /* Schedule override: boost matching action */
    const ScheduleEntry *sched = npc_schedule_current(npc->schedule_id, game_hour);
    if (sched && sched->action == action)
        score += 3.0f;

    /* Stress > 0.7: boost DRINK and PRAY */
    if (npc->stress > 0.7f) {
        if (action == NPC_ACTION_DRINK || action == NPC_ACTION_PRAY)
            score += 2.0f;
    }

    /* Low mood: boost SOCIALIZE (seeking comfort) */
    if (npc->mood < -0.5f && action == NPC_ACTION_SOCIALIZE)
        score += 2.0f;

    /* Random perturbation for variety */
    score += random_perturbation();

    return score;
}

NPCAction npc_ai_decide(const NPC *npc, int game_hour,
                         const RelationshipGraph *graph,
                         const MemorySystem *memory)
{
    NPCAction best = NPC_ACTION_IDLE;
    float best_score = -999.0f;

    for (int a = 0; a < NPC_ACTION_COUNT; a++) {
        float s = npc_ai_score_action(npc, (NPCAction)a, game_hour, graph, memory);
        if (s > best_score) {
            best_score = s;
            best = (NPCAction)a;
        }
    }

    return best;
}

void npc_ai_update_all(NPCManager *mgr, int game_hour, int game_day,
                       const Town *town, RelationshipGraph *graph,
                       MemorySystem *memory, EventQueue *events)
{
    (void)game_day;
    (void)events;

    for (int i = 0; i < mgr->count; i++) {
        NPC *npc = &mgr->npcs[i];

        /* Skip dead or absent NPCs */
        if (!npc->is_alive || npc->has_left_town)
            continue;

        /* Get schedule entry for current hour */
        const ScheduleEntry *sched = npc_schedule_current(npc->schedule_id, game_hour);

        /* If NPC not at scheduled location, move there */
        if (sched && npc->current_location != sched->location) {
            npc_move_to_location(npc, &town->map, town, sched->location);
            npc->current_location = sched->location;
        }

        /* Decide action via utility AI */
        NPCAction action = npc_ai_decide(npc, game_hour, graph, memory);
        npc->current_action = action;

        /* Update needs based on chosen action */
        switch (action) {
        case NPC_ACTION_SOCIALIZE:
            npc->need_social = CLAMP(npc->need_social - 0.1f, 0.0f, 1.0f);
            break;
        case NPC_ACTION_EAT:
            /* Eating reduces stress slightly */
            npc->stress = CLAMP(npc->stress - 0.05f, 0.0f, 1.0f);
            break;
        case NPC_ACTION_SLEEP:
            npc->stress = CLAMP(npc->stress - 0.1f, 0.0f, 1.0f);
            npc->mood = CLAMP(npc->mood + 0.05f, -1.0f, 1.0f);
            break;
        case NPC_ACTION_WORK:
            npc->need_economic = CLAMP(npc->need_economic - 0.1f, 0.0f, 1.0f);
            npc->need_purpose = CLAMP(npc->need_purpose - 0.1f, 0.0f, 1.0f);
            break;
        case NPC_ACTION_PRAY:
            npc->stress = CLAMP(npc->stress - 0.1f, 0.0f, 1.0f);
            npc->mood = CLAMP(npc->mood + 0.1f, -1.0f, 1.0f);
            break;
        case NPC_ACTION_DRINK:
            npc->stress = CLAMP(npc->stress - 0.15f, 0.0f, 1.0f);
            npc->mood = CLAMP(npc->mood + 0.05f, -1.0f, 1.0f);
            break;
        case NPC_ACTION_PATROL:
            npc->need_purpose = CLAMP(npc->need_purpose - 0.1f, 0.0f, 1.0f);
            break;
        default:
            break;
        }

        /* Passive need drift: needs increase when not being addressed */
        if (action != NPC_ACTION_SOCIALIZE)
            npc->need_social = CLAMP(npc->need_social + 0.02f, 0.0f, 1.0f);
        if (action != NPC_ACTION_WORK && action != NPC_ACTION_PATROL)
            npc->need_purpose = CLAMP(npc->need_purpose + 0.02f, 0.0f, 1.0f);
        if (action != NPC_ACTION_WORK)
            npc->need_economic = CLAMP(npc->need_economic + 0.01f, 0.0f, 1.0f);
    }
}
