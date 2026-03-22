#include "story/quest.h"
#include "npc/world_event.h"
#include <string.h>

void quest_log_init(QuestLog *ql)
{
    memset(ql, 0, sizeof(*ql));
}

Quest *quest_get(QuestLog *ql, int quest_id)
{
    for (int i = 0; i < ql->count; i++) {
        if (ql->quests[i].id == quest_id)
            return &ql->quests[i];
    }
    return NULL;
}

void quest_start(QuestLog *ql, int quest_id)
{
    Quest *q = quest_get(ql, quest_id);
    if (!q) return;
    if (q->state != QUEST_AVAILABLE) return;

    q->state = QUEST_ACTIVE;
    q->chosen_outcome = -1;

    /* Reset all objectives */
    for (int i = 0; i < q->objective_count; i++) {
        q->objectives[i].completed = false;
    }
}

void quest_complete(QuestLog *ql, int quest_id, int outcome_index,
                    EventQueue *events, int game_day)
{
    Quest *q = quest_get(ql, quest_id);
    if (!q) return;
    if (q->state != QUEST_ACTIVE) return;
    if (outcome_index < 0 || outcome_index >= q->outcome_count) return;

    q->state = QUEST_COMPLETED;
    q->chosen_outcome = outcome_index;

    /* Fire world events defined in the chosen outcome */
    if (!events) return;
    const QuestOutcome *out = &q->outcomes[outcome_index];
    int evt_count = out->event_count < 4 ? out->event_count : 4;
    for (int i = 0; i < evt_count; i++) {
        event_push(events,
                   (WorldEventType)out->world_events[i],
                   -1,  /* source = player */
                   out->event_targets[i],
                   out->event_magnitudes[i],
                   3,   /* propagation hops */
                   game_day);
    }
}

void quest_fail(QuestLog *ql, int quest_id)
{
    Quest *q = quest_get(ql, quest_id);
    if (!q) return;
    if (q->state != QUEST_ACTIVE) return;

    q->state = QUEST_FAILED;
}

void quest_update_objectives(QuestLog *ql)
{
    for (int i = 0; i < ql->count; i++) {
        Quest *q = &ql->quests[i];
        if (q->state != QUEST_ACTIVE) continue;

        /* Check if all objectives are completed */
        bool all_done = true;
        for (int j = 0; j < q->objective_count; j++) {
            if (!q->objectives[j].completed) {
                all_done = false;
                break;
            }
        }

        /* If all objectives done, mark quest as ready for completion
         * (player still needs to choose an outcome) */
        (void)all_done;
    }
}

int quest_count_active(const QuestLog *ql)
{
    int count = 0;
    for (int i = 0; i < ql->count; i++) {
        if (ql->quests[i].state == QUEST_ACTIVE)
            count++;
    }
    return count;
}

int quest_count_completed(const QuestLog *ql)
{
    int count = 0;
    for (int i = 0; i < ql->count; i++) {
        if (ql->quests[i].state == QUEST_COMPLETED)
            count++;
    }
    return count;
}
