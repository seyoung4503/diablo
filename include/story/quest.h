#ifndef DIABLO_STORY_QUEST_H
#define DIABLO_STORY_QUEST_H

#include <stdbool.h>

/* Forward declarations */
struct EventQueue;

#define MAX_QUESTS 32
#define MAX_OBJECTIVES 4
#define MAX_OUTCOMES 3
#define MAX_QUEST_NAME 128
#define MAX_QUEST_DESC 512

typedef enum {
    QUEST_UNKNOWN,
    QUEST_AVAILABLE,
    QUEST_ACTIVE,
    QUEST_COMPLETED,
    QUEST_FAILED
} QuestState;

typedef enum {
    OBJ_KILL_ENEMIES,    /* param1=enemy_type, param2=count */
    OBJ_TALK_TO_NPC,     /* param1=npc_id */
    OBJ_REACH_LOCATION,  /* param1=location_id */
    OBJ_COLLECT_ITEM,    /* param1=item_id, param2=count */
    OBJ_SURVIVE_DAYS,    /* param1=day_count */
    OBJ_CHOICE_MADE,     /* param1=choice_flag */
} ObjectiveType;

typedef struct QuestObjective {
    ObjectiveType type;
    int param1, param2;
    char description[128];
    bool completed;
} QuestObjective;

typedef struct QuestOutcome {
    char description[256];
    int triggered_flag;         /* flag to set when this outcome chosen */
    int world_events[4];        /* WorldEventType indices to fire */
    int event_targets[4];       /* target NPC IDs (-1 = town-wide) */
    float event_magnitudes[4];  /* magnitudes */
    int event_count;
    int xp_reward;
    int gold_reward;
} QuestOutcome;

typedef struct Quest {
    int id;
    char name[MAX_QUEST_NAME];
    char description[MAX_QUEST_DESC];
    QuestState state;
    int giver_npc_id;

    QuestObjective objectives[MAX_OBJECTIVES];
    int objective_count;

    QuestOutcome outcomes[MAX_OUTCOMES];
    int outcome_count;
    int chosen_outcome;   /* -1 = not yet chosen */

    int deadline_day;     /* -1 = no deadline */
} Quest;

typedef struct QuestLog {
    Quest quests[MAX_QUESTS];
    int count;
} QuestLog;

void quest_log_init(QuestLog *ql);
Quest *quest_get(QuestLog *ql, int quest_id);
void quest_start(QuestLog *ql, int quest_id);
void quest_complete(QuestLog *ql, int quest_id, int outcome_index,
                    struct EventQueue *events, int game_day);
void quest_fail(QuestLog *ql, int quest_id);
void quest_update_objectives(QuestLog *ql);
int quest_count_active(const QuestLog *ql);
int quest_count_completed(const QuestLog *ql);

/* Quest data initialization (defines all quests) */
void quest_data_init(QuestLog *ql);

#endif /* DIABLO_STORY_QUEST_H */
