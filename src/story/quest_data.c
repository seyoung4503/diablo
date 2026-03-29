#include "story/quest.h"
#include "npc/world_event.h"
#include "world/town.h"
#include <string.h>

/*
 * NPC IDs (from src/data/npc_defs.c):
 *   0=Griswold  1=Pepin    2=Ogden    3=Gillian  4=Cain
 *   5=Adria     6=Farnham  7=Wirt     8=Lachdanan 9=Mariel
 *  10=Tremain  11=Hilda   12=Theo    13=Old Mag  14=Sarnac
 *  15=Malchus  16=Lysa    17=Gharbad 18=Lazarus' Shadow  19=The Stranger
 */

/* Helper to set up an objective */
static void set_objective(QuestObjective *obj, ObjectiveType type,
                          int p1, int p2, const char *desc)
{
    obj->type = type;
    obj->param1 = p1;
    obj->param2 = p2;
    strncpy(obj->description, desc, sizeof(obj->description) - 1);
    obj->description[sizeof(obj->description) - 1] = '\0';
    obj->completed = false;
}

/* Helper to set up an outcome */
static void set_outcome(QuestOutcome *out, const char *desc,
                         int flag, int xp, int gold)
{
    memset(out, 0, sizeof(*out));
    strncpy(out->description, desc, sizeof(out->description) - 1);
    out->description[sizeof(out->description) - 1] = '\0';
    out->triggered_flag = flag;
    out->xp_reward = xp;
    out->gold_reward = gold;
}

/* Helper to add a world event to an outcome */
static void outcome_add_event(QuestOutcome *out, int evt_type,
                               int target, float magnitude)
{
    if (out->event_count >= 4) return;
    int i = out->event_count++;
    out->world_events[i] = evt_type;
    out->event_targets[i] = target;
    out->event_magnitudes[i] = magnitude;
}

/* Helper to initialize a quest shell */
static Quest *add_quest(QuestLog *ql, int id, const char *name,
                         const char *desc, int giver_npc_id, int deadline)
{
    if (ql->count >= MAX_QUESTS) return NULL;
    Quest *q = &ql->quests[ql->count++];
    memset(q, 0, sizeof(*q));
    q->id = id;
    strncpy(q->name, name, MAX_QUEST_NAME - 1);
    strncpy(q->description, desc, MAX_QUEST_DESC - 1);
    q->state = QUEST_AVAILABLE;
    q->giver_npc_id = giver_npc_id;
    q->chosen_outcome = -1;
    q->deadline_day = deadline;
    return q;
}

void quest_data_init(QuestLog *ql)
{
    quest_log_init(ql);
    Quest *q;

    /* ------------------------------------------------------------------ */
    /* Quest 0: "The Poisoned Well" (giver: Pepin, id=1)                  */
    /* ------------------------------------------------------------------ */
    q = add_quest(ql, 0, "The Poisoned Well",
        "The town well has been poisoned. Pepin asks you to investigate "
        "before more townsfolk fall ill.", 1, -1);

    q->objective_count = 2;
    set_objective(&q->objectives[0], OBJ_TALK_TO_NPC, 1, 0,
                  "Talk to Pepin about the illness");
    set_objective(&q->objectives[1], OBJ_REACH_LOCATION, LOC_TOWN_SQUARE, 0,
                  "Investigate the town well");

    q->outcome_count = 3;

    /* Outcome A: Blame the stranger */
    set_outcome(&q->outcomes[0],
        "You accuse the stranger of poisoning the well. The townsfolk "
        "drive him out. Pepin is grateful, but was it the right call?",
        1, 100, 50);
    outcome_add_event(&q->outcomes[0], EVT_NPC_LEFT_TOWN, 19, 0.8f);
    outcome_add_event(&q->outcomes[0], EVT_PLAYER_HELPED_NPC, 1, 0.2f);

    /* Outcome B: Find the real culprit */
    set_outcome(&q->outcomes[1],
        "You discover the source: tainted runoff from the cathedral depths. "
        "The town is grateful for your thorough investigation.",
        2, 200, 100);
    outcome_add_event(&q->outcomes[1], EVT_PLAYER_COMPLETED_QUEST, -1, 0.1f);
    outcome_add_event(&q->outcomes[1], EVT_GOSSIP_GOOD, -1, 0.3f);

    /* Outcome C: Ignore the problem */
    set_outcome(&q->outcomes[2],
        "You ignore the poisoned well. After a few days, the sickness "
        "spreads. Multiple NPCs fall ill and the town's morale plummets.",
        3, 0, 0);
    outcome_add_event(&q->outcomes[2], EVT_THREAT_INCREASED, -1, 0.5f);
    outcome_add_event(&q->outcomes[2], EVT_GOSSIP_BAD, -1, 0.4f);

    /* ------------------------------------------------------------------ */
    /* Quest 1: "Into the Cathedral" (giver: Cain, id=4)                  */
    /* ------------------------------------------------------------------ */
    q = add_quest(ql, 1, "Into the Cathedral",
        "Deckard Cain urges you to enter the cathedral and cleanse "
        "the evil that festers within its depths.", 4, -1);

    q->objective_count = 1;
    set_objective(&q->objectives[0], OBJ_REACH_LOCATION, LOC_CATHEDRAL, 0,
                  "Enter the cathedral");

    q->outcome_count = 1;
    set_outcome(&q->outcomes[0],
        "You descend into the cathedral and clear the first level of evil. "
        "The threat to Tristram lessens.",
        10, 200, 150);
    outcome_add_event(&q->outcomes[0], EVT_THREAT_DECREASED, -1, 0.3f);
    outcome_add_event(&q->outcomes[0], EVT_PLAYER_COMPLETED_QUEST, 4, 0.2f);

    /* ------------------------------------------------------------------ */
    /* Quest 2: "Griswold's Masterwork" (giver: Griswold, id=0)           */
    /* ------------------------------------------------------------------ */
    q = add_quest(ql, 2, "Griswold's Masterwork",
        "Griswold needs a rare ore from the cathedral depths to forge "
        "his finest weapon. Will you retrieve it for him?", 0, -1);

    q->objective_count = 1;
    set_objective(&q->objectives[0], OBJ_COLLECT_ITEM, 100, 1,
                  "Find the rare ore in the cathedral");

    q->outcome_count = 2;

    /* Outcome A: Give ore to Griswold */
    set_outcome(&q->outcomes[0],
        "You give the ore to Griswold. He forges a magnificent blade "
        "and presents you with a fine weapon as thanks.",
        20, 150, 0);
    outcome_add_event(&q->outcomes[0], EVT_PLAYER_HELPED_NPC, 0, 0.3f);
    outcome_add_event(&q->outcomes[0], EVT_GOSSIP_GOOD, 0, 0.2f);

    /* Outcome B: Keep the ore */
    set_outcome(&q->outcomes[1],
        "You keep the rare ore for yourself. Griswold is visibly "
        "disappointed and his trust in you wavers.",
        21, 50, 200);
    outcome_add_event(&q->outcomes[1], EVT_PLAYER_HARMED_NPC, 0, 0.2f);
    outcome_add_event(&q->outcomes[1], EVT_GOSSIP_BAD, 0, 0.15f);

    /* ------------------------------------------------------------------ */
    /* Quest 3: "Farnham's Redemption" (giver: Farnham, id=6)             */
    /*   Requires trust > 0.3 with Farnham to unlock                      */
    /* ------------------------------------------------------------------ */
    q = add_quest(ql, 3, "Farnham's Redemption",
        "Farnham opens up to you about his past. He lost a medallion "
        "that once kept him grounded. Help him find it.", 6, -1);

    q->objective_count = 2;
    set_objective(&q->objectives[0], OBJ_TALK_TO_NPC, 6, 0,
                  "Talk to Farnham about his past");
    set_objective(&q->objectives[1], OBJ_COLLECT_ITEM, 101, 1,
                  "Find Farnham's lost medallion");

    q->outcome_count = 1;
    set_outcome(&q->outcomes[0],
        "Farnham holds the medallion close, tears in his eyes. He vows "
        "to stop drinking and reveals a secret passage in the dungeons.",
        30, 100, 0);
    outcome_add_event(&q->outcomes[0], EVT_PLAYER_HELPED_NPC, 6, 0.5f);
    outcome_add_event(&q->outcomes[0], EVT_GOSSIP_GOOD, 6, 0.3f);

    /* ------------------------------------------------------------------ */
    /* Quest 4: "Theo's Adventure" (giver: Theo, id=12)                   */
    /* ------------------------------------------------------------------ */
    q = add_quest(ql, 4, "Theo's Adventure",
        "Young Theo has wandered dangerously close to the cathedral, "
        "dreaming of being an adventurer. Find him before it's too late.", 12, -1);

    q->objective_count = 1;
    set_objective(&q->objectives[0], OBJ_REACH_LOCATION, LOC_CATHEDRAL, 0,
                  "Find Theo near the cathedral");

    q->outcome_count = 2;

    /* Outcome A: Scold Theo, return to parents */
    set_outcome(&q->outcomes[0],
        "You scold Theo and bring him home. Tremain and Hilda are "
        "relieved, but Theo is heartbroken.",
        40, 100, 50);
    outcome_add_event(&q->outcomes[0], EVT_PLAYER_HELPED_NPC, 10, 0.2f);
    outcome_add_event(&q->outcomes[0], EVT_PLAYER_HELPED_NPC, 11, 0.2f);
    outcome_add_event(&q->outcomes[0], EVT_PLAYER_HARMED_NPC, 12, 0.3f);

    /* Outcome B: Encourage Theo's spirit */
    set_outcome(&q->outcomes[1],
        "You tell Theo he has the heart of a hero. His parents are "
        "worried, but Theo's loyalty to you is unshakable.",
        41, 100, 0);
    outcome_add_event(&q->outcomes[1], EVT_PLAYER_HARMED_NPC, 10, 0.1f);
    outcome_add_event(&q->outcomes[1], EVT_PLAYER_HARMED_NPC, 11, 0.1f);
    outcome_add_event(&q->outcomes[1], EVT_PLAYER_HELPED_NPC, 12, 0.5f);

    /* ------------------------------------------------------------------ */
    /* Quest 5: "The Witch's Bargain" (giver: Adria, id=5)                */
    /* ------------------------------------------------------------------ */
    q = add_quest(ql, 5, "The Witch's Bargain",
        "Adria offers you forbidden power drawn from the cathedral's "
        "dark energies. The price may be more than gold.", 5, -1);

    q->objective_count = 1;
    set_objective(&q->objectives[0], OBJ_CHOICE_MADE, 50, 0,
                  "Decide whether to accept Adria's offer");

    q->outcome_count = 2;

    /* Outcome A: Accept the dark power */
    set_outcome(&q->outcomes[0],
        "You accept Adria's gift. Dark energy courses through you. "
        "Cain watches with growing unease.",
        50, 250, 0);
    outcome_add_event(&q->outcomes[0], EVT_PLAYER_HELPED_NPC, 5, 0.3f);
    outcome_add_event(&q->outcomes[0], EVT_PLAYER_HARMED_NPC, 4, 0.2f);
    outcome_add_event(&q->outcomes[0], EVT_GOSSIP_BAD, 1, 0.15f);

    /* Outcome B: Reject the offer */
    set_outcome(&q->outcomes[1],
        "You refuse Adria's power. She respects your resolve. "
        "Cain nods approvingly from across the square.",
        51, 100, 50);
    outcome_add_event(&q->outcomes[1], EVT_PLAYER_HELPED_NPC, 5, 0.2f);
    outcome_add_event(&q->outcomes[1], EVT_PLAYER_HELPED_NPC, 4, 0.1f);

    /* ------------------------------------------------------------------ */
    /* Quest 6: "Missing Merchant" (giver: Ogden, id=2)                   */
    /* ------------------------------------------------------------------ */
    q = add_quest(ql, 6, "Missing Merchant",
        "Sarnac the traveling merchant hasn't been seen in days. "
        "Ogden is worried something happened on the road.", 2, -1);

    q->objective_count = 1;
    set_objective(&q->objectives[0], OBJ_REACH_LOCATION, LOC_TOWN_SQUARE, 0,
                  "Search for Sarnac along the trade road");

    q->outcome_count = 1;
    set_outcome(&q->outcomes[0],
        "You find Sarnac hiding from bandits near the road. He returns "
        "safely to Tristram, goods intact.",
        60, 100, 75);
    outcome_add_event(&q->outcomes[0], EVT_GOSSIP_GOOD, 14, 0.3f);
    outcome_add_event(&q->outcomes[0], EVT_PLAYER_HELPED_NPC, 2, 0.2f);
    outcome_add_event(&q->outcomes[0], EVT_PLAYER_HELPED_NPC, 14, 0.3f);

    /* ------------------------------------------------------------------ */
    /* Quest 7: "Gharbad's Plea" (giver: Gharbad, id=17)                  */
    /* ------------------------------------------------------------------ */
    q = add_quest(ql, 7, "Gharbad's Plea",
        "Gharbad the outcast begs you to help him find acceptance "
        "among the townspeople of Tristram.", 17, -1);

    q->objective_count = 1;
    set_objective(&q->objectives[0], OBJ_TALK_TO_NPC, 17, 0,
                  "Speak with Gharbad about his troubles");

    q->outcome_count = 3;

    /* Outcome A: Convince NPCs to accept him */
    set_outcome(&q->outcomes[0],
        "You speak on Gharbad's behalf. Slowly, the townsfolk begin "
        "to accept him. Gharbad is overjoyed.",
        70, 150, 0);
    outcome_add_event(&q->outcomes[0], EVT_PLAYER_HELPED_NPC, 17, 0.5f);
    outcome_add_event(&q->outcomes[0], EVT_GOSSIP_GOOD, 17, 0.3f);

    /* Outcome B: Tell Gharbad to leave */
    set_outcome(&q->outcomes[1],
        "You tell Gharbad he doesn't belong here. Crushed, he "
        "packs his meager belongings and vanishes into the wilds.",
        71, 50, 0);
    outcome_add_event(&q->outcomes[1], EVT_NPC_LEFT_TOWN, 17, 0.5f);
    outcome_add_event(&q->outcomes[1], EVT_PLAYER_HARMED_NPC, 17, 0.5f);

    /* Outcome C: Help him find his place */
    set_outcome(&q->outcomes[2],
        "You help Gharbad offer his herbal knowledge to Pepin. "
        "The healer grudgingly accepts, and Gharbad finds purpose.",
        72, 200, 50);
    outcome_add_event(&q->outcomes[2], EVT_PLAYER_HELPED_NPC, 17, 0.4f);
    outcome_add_event(&q->outcomes[2], EVT_PLAYER_HELPED_NPC, 1, 0.1f);
    outcome_add_event(&q->outcomes[2], EVT_GOSSIP_GOOD, -1, 0.1f);

    /* ------------------------------------------------------------------ */
    /* Quest 9: "The Butcher's Lair" (giver: Ogden, id=2)                 */
    /* ------------------------------------------------------------------ */
    q = add_quest(ql, 9, "The Butcher's Lair",
        "Ogden speaks of a terrible creature in the cathedral's first "
        "level. A demon called The Butcher has been terrorizing those "
        "who venture below.", 2, -1);

    q->objective_count = 1;
    set_objective(&q->objectives[0], OBJ_KILL_ENEMIES, -1, 10,
                  "Kill 10 enemies on dungeon level 2");

    q->outcome_count = 1;
    set_outcome(&q->outcomes[0],
        "The Butcher is slain. Ogden raises a toast in your honor, and "
        "the townsfolk breathe a little easier.",
        80, 300, 200);
    outcome_add_event(&q->outcomes[0], EVT_THREAT_DECREASED, -1, 0.4f);
    outcome_add_event(&q->outcomes[0], EVT_GOSSIP_GOOD, 2, 0.3f);
    outcome_add_event(&q->outcomes[0], EVT_PLAYER_COMPLETED_QUEST, -1, 0.2f);

    /* ------------------------------------------------------------------ */
    /* Quest 10: "Halls of the Blind" (giver: Cain, id=4)                 */
    /* ------------------------------------------------------------------ */
    q = add_quest(ql, 10, "Halls of the Blind",
        "Cain has uncovered a passage in the catacombs where the blind "
        "lead the blind. Ancient texts speak of a treasure hidden "
        "within.", 4, -1);

    q->objective_count = 1;
    set_objective(&q->objectives[0], OBJ_REACH_LOCATION, LOC_CATHEDRAL, 0,
                  "Find and reach the stairs on dungeon level 6");

    q->outcome_count = 1;
    set_outcome(&q->outcomes[0],
        "You navigate the Halls of the Blind and uncover the ancient "
        "treasure. Cain is fascinated by your discoveries.",
        81, 250, 300);
    outcome_add_event(&q->outcomes[0], EVT_PLAYER_COMPLETED_QUEST, 4, 0.2f);
    outcome_add_event(&q->outcomes[0], EVT_GOSSIP_GOOD, -1, 0.2f);

    /* ------------------------------------------------------------------ */
    /* Quest 11: "Valor" (giver: Griswold, id=0)                          */
    /* ------------------------------------------------------------------ */
    q = add_quest(ql, 11, "Valor",
        "Griswold tells of an enchanted suit of armor lost deep in the "
        "caves. It was worn by Arkaine, the great warrior of legend.", 0, -1);

    q->objective_count = 1;
    set_objective(&q->objectives[0], OBJ_KILL_ENEMIES, -1, 15,
                  "Kill 15 enemies in the caves (levels 9-12)");

    q->outcome_count = 2;

    /* Outcome A: Return the armor to Griswold */
    set_outcome(&q->outcomes[0],
        "You recover Arkaine's Valor. Griswold reverently repairs the "
        "legendary armor and presents it to you.",
        82, 400, 0);
    outcome_add_event(&q->outcomes[0], EVT_PLAYER_HELPED_NPC, 0, 0.3f);
    outcome_add_event(&q->outcomes[0], EVT_GOSSIP_GOOD, 0, 0.2f);
    outcome_add_event(&q->outcomes[0], EVT_PLAYER_COMPLETED_QUEST, -1, 0.2f);

    /* Outcome B: Keep it for yourself */
    set_outcome(&q->outcomes[1],
        "You don the armor yourself without telling Griswold. "
        "Its power is undeniable, but trust is harder to forge.",
        83, 300, 100);
    outcome_add_event(&q->outcomes[1], EVT_PLAYER_HARMED_NPC, 0, 0.2f);
    outcome_add_event(&q->outcomes[1], EVT_GOSSIP_BAD, 0, 0.15f);

    /* ------------------------------------------------------------------ */
    /* Quest 12: "The Chamber of Bone" (giver: Cain, id=4)                */
    /* ------------------------------------------------------------------ */
    q = add_quest(ql, 12, "The Chamber of Bone",
        "Beyond the halls of the blind lies a chamber of bone, where "
        "the undead mass in great numbers. Clear this abomination.", 4, -1);

    q->objective_count = 1;
    set_objective(&q->objectives[0], OBJ_KILL_ENEMIES, -1, 20,
                  "Kill 20 enemies on dungeon level 8");

    q->outcome_count = 1;
    set_outcome(&q->outcomes[0],
        "The Chamber of Bone is cleansed. Among the remains you find "
        "an ancient tome that Cain is eager to study.",
        84, 350, 250);
    outcome_add_event(&q->outcomes[0], EVT_THREAT_DECREASED, -1, 0.3f);
    outcome_add_event(&q->outcomes[0], EVT_PLAYER_COMPLETED_QUEST, 4, 0.2f);
    outcome_add_event(&q->outcomes[0], EVT_GOSSIP_GOOD, -1, 0.15f);

    /* ------------------------------------------------------------------ */
    /* Quest 13: "Diablo's Domain" (giver: Cain, id=4)                    */
    /* ------------------------------------------------------------------ */
    q = add_quest(ql, 13, "Diablo's Domain",
        "The Lord of Terror awaits in the deepest depths of Hell. "
        "You must descend through all 16 levels to face the ultimate "
        "evil.", 4, -1);

    q->objective_count = 1;
    set_objective(&q->objectives[0], OBJ_REACH_LOCATION, LOC_CATHEDRAL, 0,
                  "Reach dungeon level 16");

    q->outcome_count = 1;
    set_outcome(&q->outcomes[0],
        "You have reached the deepest level of Hell. The Lord of Terror "
        "awaits. Tristram's fate rests upon your shoulders.",
        85, 500, 500);
    outcome_add_event(&q->outcomes[0], EVT_THREAT_DECREASED, -1, 0.5f);
    outcome_add_event(&q->outcomes[0], EVT_PLAYER_COMPLETED_QUEST, -1, 0.3f);
    outcome_add_event(&q->outcomes[0], EVT_GOSSIP_GOOD, -1, 0.3f);

    /* ------------------------------------------------------------------ */
    /* Quest 14: "Wirt's Leg" (giver: Wirt, id=7)                         */
    /* ------------------------------------------------------------------ */
    q = add_quest(ql, 14, "Wirt's Leg",
        "Wirt has a peculiar request - find his lost wooden leg "
        "somewhere in the catacombs. He swears it contains hidden "
        "treasure.", 7, -1);

    q->objective_count = 1;
    set_objective(&q->objectives[0], OBJ_KILL_ENEMIES, -1, 5,
                  "Kill 5 enemies on dungeon level 5");

    q->outcome_count = 2;

    /* Outcome A: Return the leg to Wirt */
    set_outcome(&q->outcomes[0],
        "You find Wirt's wooden leg and the treasure hidden inside. "
        "Wirt is delighted to have it back.",
        86, 150, 100);
    outcome_add_event(&q->outcomes[0], EVT_PLAYER_HELPED_NPC, 7, 0.3f);
    outcome_add_event(&q->outcomes[0], EVT_GOSSIP_GOOD, 7, 0.2f);

    /* Outcome B: Keep the treasure */
    set_outcome(&q->outcomes[1],
        "You pocket the treasure from the wooden leg. Wirt won't be "
        "happy, but gold is gold.",
        87, 50, 300);
    outcome_add_event(&q->outcomes[1], EVT_PLAYER_HARMED_NPC, 7, 0.3f);
    outcome_add_event(&q->outcomes[1], EVT_GOSSIP_BAD, 7, 0.2f);
}
