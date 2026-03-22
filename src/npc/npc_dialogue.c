#include "npc/npc_dialogue.h"
#include "npc/npc.h"
#include "npc/world_event.h"
#include "npc/npc_relationship.h"
#include "game/game.h"
#include <string.h>
#include <stdio.h>

void dialogue_init(DialogueSystem *ds)
{
    memset(ds, 0, sizeof(*ds));
    ds->active = false;
    ds->current_node_id = -1;
    ds->talking_to_npc_id = -1;
    ds->selected_choice = 0;
}

/* Find the best starting node for an NPC (first node with matching npc_id whose conditions pass) */
void dialogue_start(DialogueSystem *ds, int npc_id)
{
    ds->current_node_id = -1;

    for (int i = 0; i < ds->node_count; i++) {
        if (ds->nodes[i].npc_id != npc_id)
            continue;

        /* Use the first node for this NPC as the greeting/entry point.
         * Condition checking requires Game/NPCManager context which we don't
         * have here — dialogue_start is a lightweight activation.
         * For conditional entry points, use dialogue_start_ex(). */
        ds->current_node_id = ds->nodes[i].id;
        break;
    }

    if (ds->current_node_id < 0)
        return;  /* no dialogue for this NPC */

    ds->active = true;
    ds->talking_to_npc_id = npc_id;
    ds->typewriter_timer = 0.0f;
    ds->chars_shown = 0;
    ds->selected_choice = 0;
}

static const DialogueNode *find_node(const DialogueSystem *ds, int node_id)
{
    for (int i = 0; i < ds->node_count; i++) {
        if (ds->nodes[i].id == node_id)
            return &ds->nodes[i];
    }
    return NULL;
}

void dialogue_select_choice(DialogueSystem *ds, int choice_index)
{
    if (!ds->active)
        return;

    const DialogueNode *node = find_node(ds, ds->current_node_id);
    if (!node || choice_index < 0 || choice_index >= node->choice_count)
        return;

    const DialogueChoice *choice = &node->choices[choice_index];
    int next = choice->next_node_id;

    if (next < 0) {
        /* End conversation */
        ds->active = false;
        ds->current_node_id = -1;
        ds->talking_to_npc_id = -1;
        return;
    }

    ds->current_node_id = next;
    ds->typewriter_timer = 0.0f;
    ds->chars_shown = 0;
    ds->selected_choice = 0;
}

void dialogue_update(DialogueSystem *ds, float dt)
{
    if (!ds->active)
        return;

    const DialogueNode *node = find_node(ds, ds->current_node_id);
    if (!node)
        return;

    int text_len = (int)strlen(node->text);
    if (ds->chars_shown < text_len) {
        ds->typewriter_timer += dt * DIALOGUE_CHARS_PER_SEC;
        ds->chars_shown = (int)ds->typewriter_timer;
        if (ds->chars_shown > text_len)
            ds->chars_shown = text_len;
    }
}

bool dialogue_is_active(const DialogueSystem *ds)
{
    return ds->active;
}

const DialogueNode *dialogue_current_node(const DialogueSystem *ds)
{
    if (!ds->active)
        return NULL;
    return find_node(ds, ds->current_node_id);
}

bool dialogue_check_condition(const DialogueCondition *cond, const Game *game,
                              const NPCManager *npcs, const int *flags, int flag_count)
{
    switch (cond->type) {
    case COND_NONE:
        return true;

    case COND_QUEST_STATE:
        /* Quest system integration — check flag as proxy for now */
        if (cond->param1 >= 0 && cond->param1 < flag_count)
            return flags[cond->param1] >= cond->param2;
        return false;

    case COND_NPC_TRUST_ABOVE: {
        const NPC *npc = NULL;
        for (int i = 0; i < npcs->count; i++) {
            if (npcs->npcs[i].id == cond->param1) {
                npc = &npcs->npcs[i];
                break;
            }
        }
        return npc && npc->trust_player > cond->fparam;
    }

    case COND_NPC_TRUST_BELOW: {
        const NPC *npc = NULL;
        for (int i = 0; i < npcs->count; i++) {
            if (npcs->npcs[i].id == cond->param1) {
                npc = &npcs->npcs[i];
                break;
            }
        }
        return npc && npc->trust_player < cond->fparam;
    }

    case COND_NPC_ALIVE: {
        const NPC *npc = NULL;
        for (int i = 0; i < npcs->count; i++) {
            if (npcs->npcs[i].id == cond->param1) {
                npc = &npcs->npcs[i];
                break;
            }
        }
        return npc && npc->is_alive;
    }

    case COND_FLAG_SET:
        if (cond->param1 >= 0 && cond->param1 < flag_count)
            return flags[cond->param1] != 0;
        return false;

    case COND_GAME_DAY_AFTER:
        return game->game_day > cond->param1;

    case COND_PLAYER_LEVEL:
        /* Player level not in Game struct yet — always pass for now */
        return true;

    default:
        return false;
    }
}

void dialogue_apply_consequences(const DialogueConsequence *conseqs, int count,
                                 NPCManager *npcs, EventQueue *events,
                                 RelationshipGraph *graph __attribute__((unused)),
                                 int *flags, int game_day)
{
    for (int i = 0; i < count; i++) {
        const DialogueConsequence *c = &conseqs[i];
        switch (c->type) {
        case CONSEQ_NONE:
            break;

        case CONSEQ_SET_FLAG:
            if (flags && c->param1 >= 0)
                flags[c->param1] = 1;
            break;

        case CONSEQ_START_QUEST:
            /* Mark quest flag as started (state=1) */
            if (flags && c->param1 >= 0)
                flags[c->param1] = 1;
            break;

        case CONSEQ_WORLD_EVENT:
            if (events)
                event_push(events, (WorldEventType)c->param1,
                           -1, c->param2, c->fparam, 2, game_day);
            break;

        case CONSEQ_MODIFY_TRUST: {
            NPC *npc = npc_manager_get(npcs, c->param1);
            if (npc) {
                npc->trust_player += c->fparam;
                if (npc->trust_player > 1.0f) npc->trust_player = 1.0f;
                if (npc->trust_player < -1.0f) npc->trust_player = -1.0f;
            }
            break;
        }

        case CONSEQ_MODIFY_MOOD: {
            NPC *npc = npc_manager_get(npcs, c->param1);
            if (npc) {
                npc->mood += c->fparam;
                if (npc->mood > 1.0f) npc->mood = 1.0f;
                if (npc->mood < -1.0f) npc->mood = -1.0f;
            }
            break;
        }

        case CONSEQ_GIVE_XP:
            /* XP system integration — generate world event for now */
            if (events)
                event_push(events, EVT_PLAYER_COMPLETED_QUEST,
                           -1, -1, (float)c->param1 / 100.0f, 0, game_day);
            break;

        case CONSEQ_GIVE_GOLD:
            /* Gold system integration — generate world event for now */
            if (events)
                event_push(events, EVT_PLAYER_COMPLETED_QUEST,
                           -1, -1, (float)c->param1 / 1000.0f, 0, game_day);
            break;

        default:
            break;
        }
    }
}
