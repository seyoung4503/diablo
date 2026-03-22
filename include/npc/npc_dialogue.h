#ifndef DIABLO_NPC_DIALOGUE_H
#define DIABLO_NPC_DIALOGUE_H

#include <stdbool.h>

/* Forward declarations */
struct Game;
struct NPCManager;
struct EventQueue;
struct RelationshipGraph;

#define MAX_DIALOGUE_NODES   512
#define MAX_CHOICES_PER_NODE 4
#define MAX_CONDITIONS       4
#define MAX_CONSEQUENCES     4
#define MAX_DIALOGUE_TEXT    256
#define MAX_CHOICE_TEXT      128

/* Typewriter speed (characters per second) */
#define DIALOGUE_CHARS_PER_SEC 30.0f

typedef enum {
    COND_NONE,
    COND_QUEST_STATE,      /* param1=quest_id, param2=required_state */
    COND_NPC_TRUST_ABOVE,  /* fparam=threshold */
    COND_NPC_TRUST_BELOW,
    COND_NPC_ALIVE,        /* param1=npc_id */
    COND_FLAG_SET,         /* param1=flag_id */
    COND_GAME_DAY_AFTER,   /* param1=day */
    COND_PLAYER_LEVEL,     /* param1=min_level */
} ConditionType;

typedef struct DialogueCondition {
    ConditionType type;
    int param1, param2;
    float fparam;
} DialogueCondition;

typedef enum {
    CONSEQ_NONE,
    CONSEQ_SET_FLAG,       /* param1=flag_id */
    CONSEQ_START_QUEST,    /* param1=quest_id */
    CONSEQ_WORLD_EVENT,    /* param1=event_type, param2=target_npc, fparam=magnitude */
    CONSEQ_MODIFY_TRUST,   /* param1=npc_id, fparam=delta */
    CONSEQ_MODIFY_MOOD,    /* param1=npc_id, fparam=delta */
    CONSEQ_GIVE_XP,        /* param1=amount */
    CONSEQ_GIVE_GOLD,      /* param1=amount */
} ConsequenceType;

typedef struct DialogueConsequence {
    ConsequenceType type;
    int param1, param2;
    float fparam;
} DialogueConsequence;

typedef struct DialogueChoice {
    char text[MAX_CHOICE_TEXT];
    int next_node_id;  /* -1 = end conversation */
    DialogueConsequence consequences[MAX_CONSEQUENCES];
    int consequence_count;
    DialogueCondition conditions[MAX_CONDITIONS];  /* conditions to show this choice */
    int condition_count;
} DialogueChoice;

typedef struct DialogueNode {
    int id;
    int npc_id;  /* which NPC says this */
    char text[MAX_DIALOGUE_TEXT];
    DialogueChoice choices[MAX_CHOICES_PER_NODE];
    int choice_count;
    DialogueCondition conditions[MAX_CONDITIONS];  /* conditions for this node to be available */
    int condition_count;
} DialogueNode;

typedef struct DialogueSystem {
    DialogueNode nodes[MAX_DIALOGUE_NODES];
    int node_count;

    /* Active conversation state */
    bool active;
    int current_node_id;
    int talking_to_npc_id;
    float typewriter_timer;
    int chars_shown;
    int selected_choice;  /* keyboard selection index */
} DialogueSystem;

void dialogue_init(DialogueSystem *ds);
void dialogue_start(DialogueSystem *ds, int npc_id);
void dialogue_select_choice(DialogueSystem *ds, int choice_index);
void dialogue_update(DialogueSystem *ds, float dt);
bool dialogue_is_active(const DialogueSystem *ds);
const DialogueNode *dialogue_current_node(const DialogueSystem *ds);

/* Check if a condition is met */
bool dialogue_check_condition(const DialogueCondition *cond, const struct Game *game,
                              const struct NPCManager *npcs, const int *flags, int flag_count);

/* Apply consequences of a choice */
void dialogue_apply_consequences(const DialogueConsequence *conseqs, int count,
                                 struct NPCManager *npcs, struct EventQueue *events,
                                 struct RelationshipGraph *graph, int *flags, int game_day);

#endif /* DIABLO_NPC_DIALOGUE_H */
