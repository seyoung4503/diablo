#include "story/story_arc.h"
#include <string.h>

/* Helper to add a stage to an arc */
static void add_stage(StoryArc *arc, int quest_id, int outcome,
                      int day, const char *desc)
{
    if (arc->stage_count >= MAX_ARC_STAGES) return;
    StoryArcStage *s = &arc->stages[arc->stage_count++];
    s->required_quest_id = quest_id;
    s->required_outcome = outcome;
    s->required_day = day;
    strncpy(s->description, desc, sizeof(s->description) - 1);
    s->description[sizeof(s->description) - 1] = '\0';
}

/* Helper to add an arc to the system */
static StoryArc *add_arc(StoryArcSystem *sys, int npc_id)
{
    if (sys->count >= MAX_TOWN_NPCS) return NULL;
    StoryArc *arc = &sys->arcs[sys->count++];
    memset(arc, 0, sizeof(*arc));
    arc->npc_id = npc_id;
    return arc;
}

void story_arc_init(StoryArcSystem *sys)
{
    memset(sys, 0, sizeof(*sys));
    StoryArc *arc;

    /* ------------------------------------------------------------------ */
    /* Griswold (id=0): From worried blacksmith to master craftsman        */
    /* ------------------------------------------------------------------ */
    arc = add_arc(sys, 0);
    add_stage(arc, -1, -1, -1,
        "Griswold worries about the town's safety and pours himself "
        "into his work at the forge.");
    add_stage(arc, 2, 0, -1,
        "With the rare ore returned, Griswold forges his masterwork. "
        "His confidence soars and he begins mentoring Lysa.");
    add_stage(arc, 1, -1, -1,
        "As the cathedral is cleared, Griswold's weapons are in high "
        "demand. He becomes the backbone of Tristram's defense.");
    add_stage(arc, -1, -1, 15,
        "Griswold speaks of reopening trade routes now that the roads "
        "are safer. His shop thrives.");
    add_stage(arc, 2, 1, -1,
        "If betrayed, Griswold grows bitter and withdrawn. He works "
        "alone, refusing to train Lysa.");

    /* ------------------------------------------------------------------ */
    /* Farnham (id=6): From town drunk to redeemed soul                   */
    /* ------------------------------------------------------------------ */
    arc = add_arc(sys, 6);
    add_stage(arc, -1, -1, -1,
        "Farnham drinks away his sorrows at Ogden's tavern, mumbling "
        "about horrors he saw in the cathedral.");
    add_stage(arc, 3, 0, -1,
        "With his medallion restored, Farnham begins to sober up. "
        "He shares useful intel about the dungeon's layout.");
    add_stage(arc, 1, -1, -1,
        "Farnham volunteers to guard the cathedral entrance, finding "
        "new purpose in protecting others.");
    add_stage(arc, -1, -1, 20,
        "Farnham is fully sober. He trains with Lachdanan and becomes "
        "a dependable member of the town watch.");

    /* ------------------------------------------------------------------ */
    /* Theo (id=12): From reckless boy to young hero                      */
    /* ------------------------------------------------------------------ */
    arc = add_arc(sys, 12);
    add_stage(arc, -1, -1, -1,
        "Theo dreams of adventure, sneaking away from his parents' "
        "farm to explore near the cathedral.");
    add_stage(arc, 4, 0, -1,
        "After being scolded, Theo obeys his parents but his spirit "
        "is dimmed. He helps at the farm reluctantly.");
    add_stage(arc, 4, 1, -1,
        "Encouraged by the hero, Theo begins training in secret. "
        "He carries a wooden sword everywhere he goes.");
    add_stage(arc, 1, -1, -1,
        "Theo assists Lachdanan with patrols around the town perimeter, "
        "earning grudging respect from his father.");
    add_stage(arc, -1, -1, 25,
        "Theo has grown into a capable young scout. Even Tremain admits "
        "his son may have found his true calling.");

    /* ------------------------------------------------------------------ */
    /* Adria (id=5): From mysterious witch to revealed ally or threat      */
    /* ------------------------------------------------------------------ */
    arc = add_arc(sys, 5);
    add_stage(arc, -1, -1, -1,
        "Adria watches the cathedral from her hut, speaking in riddles "
        "about the darkness below.");
    add_stage(arc, 5, 0, -1,
        "The hero accepted her power. Adria grows bolder, offering "
        "more dangerous enchantments. Cain grows suspicious.");
    add_stage(arc, 5, 1, -1,
        "The hero rejected her offer. Adria respects this and begins "
        "sharing genuine knowledge about the cathedral's wards.");
    add_stage(arc, 1, -1, -1,
        "As the cathedral is cleansed, Adria reveals she was once a "
        "priestess who turned to darker arts out of desperation.");
    add_stage(arc, -1, -1, 30,
        "Adria's true nature is revealed: she seeks to contain the evil, "
        "not wield it. She aids the final assault.");

    /* ------------------------------------------------------------------ */
    /* Gharbad (id=17): From outcast to accepted or exiled                 */
    /* ------------------------------------------------------------------ */
    arc = add_arc(sys, 17);
    add_stage(arc, -1, -1, -1,
        "Gharbad lurks at the edge of town, shunned by most. He "
        "desperately wants to belong.");
    add_stage(arc, 7, 0, -1,
        "The townsfolk begin accepting Gharbad. He helps with heavy "
        "labor and shares knowledge of monster behavior.");
    add_stage(arc, 7, 1, -1,
        "Driven away, Gharbad vanishes. Rumors say he joined a band "
        "of creatures in the wilds.");
    add_stage(arc, 7, 2, -1,
        "Working with Pepin, Gharbad discovers a talent for herbalism. "
        "He supplies rare ingredients from the wilds.");
    add_stage(arc, -1, -1, 20,
        "Gharbad has found his place. He tends a small herb garden "
        "near Adria's hut and is finally at peace.");

    /* ------------------------------------------------------------------ */
    /* Chapter Progress (meta-arc, npc_id=-1): Overall game progression    */
    /* ------------------------------------------------------------------ */
    arc = add_arc(sys, -1);
    add_stage(arc, -1, -1, -1,
        "Tristram: The hero arrives in town. The cathedral looms in "
        "the distance, its evil not yet confronted.");
    add_stage(arc, 1, -1, -1,
        "Cathedral: The hero has entered the cathedral and begun "
        "clearing levels 1 through 4 of the undead.");
    add_stage(arc, 10, -1, -1,
        "Catacombs: The hero descends into the catacombs, navigating "
        "levels 5 through 8 and the Halls of the Blind.");
    add_stage(arc, 11, -1, -1,
        "Caves: The hero fights through the caves on levels 9 through "
        "12, seeking Arkaine's Valor and greater challenges.");
    add_stage(arc, 13, -1, -1,
        "Hell: The hero has reached the deepest levels, 13 through 16. "
        "The Lord of Terror's domain awaits.");
    add_stage(arc, -1, -1, -1,
        "Victory: The hero has reached level 16 and returned to "
        "Tristram. The Lord of Terror is defeated.");
}

void story_arc_update(StoryArcSystem *sys, NPCManager *npcs,
                      const QuestLog *ql, int game_day)
{
    if (!sys || !npcs || !ql) return;

    for (int a = 0; a < sys->count; a++) {
        StoryArc *arc = &sys->arcs[a];
        NPC *npc = npc_manager_get(npcs, arc->npc_id);
        if (!npc) continue;

        int current_stage = npc->arc_stage;

        /* Try to advance to the next matching stage */
        for (int s = current_stage + 1; s < arc->stage_count; s++) {
            StoryArcStage *stage = &arc->stages[s];
            bool can_advance = true;

            /* Check quest requirement */
            if (stage->required_quest_id >= 0) {
                const Quest *rq = NULL;
                for (int q = 0; q < ql->count; q++) {
                    if (ql->quests[q].id == stage->required_quest_id) {
                        rq = &ql->quests[q];
                        break;
                    }
                }
                if (!rq || rq->state != QUEST_COMPLETED) {
                    can_advance = false;
                }
                /* Check specific outcome requirement */
                if (can_advance && stage->required_outcome >= 0) {
                    if (!rq || rq->chosen_outcome != stage->required_outcome) {
                        can_advance = false;
                    }
                }
            }

            /* Check day requirement */
            if (stage->required_day >= 0 && game_day < stage->required_day) {
                can_advance = false;
            }

            if (can_advance) {
                npc->arc_stage = s;
                /* Only advance one stage per update */
                break;
            }
        }
    }
}
