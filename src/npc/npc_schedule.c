#include "npc/npc_schedule.h"
#include "world/town.h"

/* Global schedule table */
static NPCSchedule g_schedules[MAX_SCHEDULES];
static int g_schedule_count = 0;

/* Helper: add a schedule entry */
static void sched_add(NPCSchedule *s, int h_start, int h_end,
                      TownLocation loc, NPCAction action)
{
    if (s->entry_count >= MAX_SCHEDULE_ENTRIES)
        return;
    ScheduleEntry *e = &s->entries[s->entry_count++];
    e->hour_start = h_start;
    e->hour_end   = h_end;
    e->location   = (NPCLocation)loc;
    e->action     = action;
}

void npc_schedules_init(void)
{
    memset(g_schedules, 0, sizeof(g_schedules));
    g_schedule_count = 0;

    NPCSchedule *s;

    /* Schedule 0: Griswold — work 7-18, tavern 18-20, home 20-7 */
    s = &g_schedules[g_schedule_count++];
    sched_add(s, 7,  18, LOC_BLACKSMITH, NPC_ACTION_WORK);
    sched_add(s, 18, 20, LOC_TAVERN,     NPC_ACTION_DRINK);
    sched_add(s, 20, 7,  LOC_BLACKSMITH, NPC_ACTION_SLEEP);

    /* Schedule 1: Pepin — work 6-20, home 20-6 */
    s = &g_schedules[g_schedule_count++];
    sched_add(s, 6,  20, LOC_HEALER,  NPC_ACTION_WORK);
    sched_add(s, 20, 6,  LOC_HEALER,  NPC_ACTION_SLEEP);

    /* Schedule 2: Ogden — work 6-23, sleep 23-6 */
    s = &g_schedules[g_schedule_count++];
    sched_add(s, 6,  23, LOC_TAVERN, NPC_ACTION_WORK);
    sched_add(s, 23, 6,  LOC_TAVERN, NPC_ACTION_SLEEP);

    /* Schedule 3: Gillian — work 10-23, home 23-10 */
    s = &g_schedules[g_schedule_count++];
    sched_add(s, 10, 23, LOC_TAVERN, NPC_ACTION_WORK);
    sched_add(s, 23, 10, LOC_TAVERN, NPC_ACTION_SLEEP);

    /* Schedule 4: Deckard Cain — home 6-9, square 9-12, cathedral 12-15,
       home 15-18, tavern 18-21, home 21-6 */
    s = &g_schedules[g_schedule_count++];
    sched_add(s, 6,  9,  LOC_HOME_CAIN,    NPC_ACTION_IDLE);
    sched_add(s, 9,  12, LOC_TOWN_SQUARE,  NPC_ACTION_SOCIALIZE);
    sched_add(s, 12, 15, LOC_CATHEDRAL,     NPC_ACTION_WORK);
    sched_add(s, 15, 18, LOC_HOME_CAIN,    NPC_ACTION_IDLE);
    sched_add(s, 18, 21, LOC_TAVERN,        NPC_ACTION_DRINK);
    sched_add(s, 21, 6,  LOC_HOME_CAIN,    NPC_ACTION_SLEEP);

    /* Schedule 5: Adria — home all day, night visit to square 22-1 */
    s = &g_schedules[g_schedule_count++];
    sched_add(s, 1,  22, LOC_ADRIA_HUT,    NPC_ACTION_WORK);
    sched_add(s, 22, 1,  LOC_TOWN_SQUARE,  NPC_ACTION_WANDER);

    /* Schedule 6: Farnham — tavern 10-2, wander 2-10 */
    s = &g_schedules[g_schedule_count++];
    sched_add(s, 10, 2,  LOC_TAVERN,       NPC_ACTION_DRINK);
    sched_add(s, 2,  10, LOC_TOWN_SQUARE,  NPC_ACTION_WANDER);

    /* Schedule 7: Wirt — wirt_corner 8-20, wander 20-8 */
    s = &g_schedules[g_schedule_count++];
    sched_add(s, 8,  20, LOC_WIRT_CORNER,  NPC_ACTION_WORK);
    sched_add(s, 20, 8,  LOC_TOWN_SQUARE,  NPC_ACTION_WANDER);

    /* Schedule 8: Lachdanan — patrol 6-18, tavern 18-20, home 20-6 */
    s = &g_schedules[g_schedule_count++];
    sched_add(s, 6,  18, LOC_TOWN_SQUARE,  NPC_ACTION_PATROL);
    sched_add(s, 18, 20, LOC_TAVERN,        NPC_ACTION_DRINK);
    sched_add(s, 20, 6,  LOC_TOWN_SQUARE,  NPC_ACTION_SLEEP);

    /* Schedule 9: Mariel — cathedral 6-18, square 18-20, cathedral 20-6 */
    s = &g_schedules[g_schedule_count++];
    sched_add(s, 6,  18, LOC_CATHEDRAL,     NPC_ACTION_PRAY);
    sched_add(s, 18, 20, LOC_TOWN_SQUARE,  NPC_ACTION_SOCIALIZE);
    sched_add(s, 20, 6,  LOC_CATHEDRAL,     NPC_ACTION_PRAY);

    /* Schedule 10: Tremain — home 5-7, square 7-12, home 12-17,
       tavern 17-19, home 19-5 */
    s = &g_schedules[g_schedule_count++];
    sched_add(s, 5,  7,  LOC_HOME_TREMAIN, NPC_ACTION_EAT);
    sched_add(s, 7,  12, LOC_TOWN_SQUARE,  NPC_ACTION_WORK);
    sched_add(s, 12, 17, LOC_HOME_TREMAIN, NPC_ACTION_WORK);
    sched_add(s, 17, 19, LOC_TAVERN,        NPC_ACTION_DRINK);
    sched_add(s, 19, 5,  LOC_HOME_TREMAIN, NPC_ACTION_SLEEP);

    /* Schedule 11: Hilda — home 5-12, square 12-14, home 14-19, sleep 19-5 */
    s = &g_schedules[g_schedule_count++];
    sched_add(s, 5,  12, LOC_HOME_TREMAIN, NPC_ACTION_WORK);
    sched_add(s, 12, 14, LOC_TOWN_SQUARE,  NPC_ACTION_SOCIALIZE);
    sched_add(s, 14, 19, LOC_HOME_TREMAIN, NPC_ACTION_WORK);
    sched_add(s, 19, 5,  LOC_HOME_TREMAIN, NPC_ACTION_SLEEP);

    /* Schedule 12: Theo — home 6-8, square 8-17, home 17-20, sleep 20-6 */
    s = &g_schedules[g_schedule_count++];
    sched_add(s, 6,  8,  LOC_HOME_TREMAIN, NPC_ACTION_EAT);
    sched_add(s, 8,  17, LOC_TOWN_SQUARE,  NPC_ACTION_WANDER);
    sched_add(s, 17, 20, LOC_HOME_TREMAIN, NPC_ACTION_EAT);
    sched_add(s, 20, 6,  LOC_HOME_TREMAIN, NPC_ACTION_SLEEP);

    /* Schedule 13: Old Mag — graveyard 6-10, square 10-14,
       graveyard 14-22, home 22-6 */
    s = &g_schedules[g_schedule_count++];
    sched_add(s, 6,  10, LOC_GRAVEYARD,    NPC_ACTION_WORK);
    sched_add(s, 10, 14, LOC_TOWN_SQUARE,  NPC_ACTION_SOCIALIZE);
    sched_add(s, 14, 22, LOC_GRAVEYARD,    NPC_ACTION_WORK);
    sched_add(s, 22, 6,  LOC_GRAVEYARD,    NPC_ACTION_SLEEP);

    /* Schedule 14: Sarnac — square 8-18, tavern 18-22, wander 22-8 */
    s = &g_schedules[g_schedule_count++];
    sched_add(s, 8,  18, LOC_TOWN_SQUARE,  NPC_ACTION_WORK);
    sched_add(s, 18, 22, LOC_TAVERN,        NPC_ACTION_DRINK);
    sched_add(s, 22, 8,  LOC_TOWN_SQUARE,  NPC_ACTION_WANDER);

    /* Schedule 15: Malchus — cathedral 5-12, healer 12-17,
       cathedral 17-22, sleep 22-5 */
    s = &g_schedules[g_schedule_count++];
    sched_add(s, 5,  12, LOC_CATHEDRAL,     NPC_ACTION_PRAY);
    sched_add(s, 12, 17, LOC_HEALER,        NPC_ACTION_WORK);
    sched_add(s, 17, 22, LOC_CATHEDRAL,     NPC_ACTION_PRAY);
    sched_add(s, 22, 5,  LOC_CATHEDRAL,     NPC_ACTION_SLEEP);

    /* Schedule 16: Lysa — blacksmith 7-18, tavern 18-21, home 21-7 */
    s = &g_schedules[g_schedule_count++];
    sched_add(s, 7,  18, LOC_BLACKSMITH,   NPC_ACTION_WORK);
    sched_add(s, 18, 21, LOC_TAVERN,        NPC_ACTION_SOCIALIZE);
    sched_add(s, 21, 7,  LOC_BLACKSMITH,   NPC_ACTION_SLEEP);

    /* Schedule 17: Gharbad — wander all day, square 12-14 */
    s = &g_schedules[g_schedule_count++];
    sched_add(s, 14, 12, LOC_TOWN_SQUARE,  NPC_ACTION_WANDER);
    sched_add(s, 12, 14, LOC_TOWN_SQUARE,  NPC_ACTION_IDLE);

    /* Schedule 18: Lazarus' Shadow — placeholder (not in town) */
    s = &g_schedules[g_schedule_count++];
    sched_add(s, 0,  24, LOC_CATHEDRAL,     NPC_ACTION_IDLE);

    /* Schedule 19: The Stranger — placeholder (not in town) */
    s = &g_schedules[g_schedule_count++];
    sched_add(s, 0,  24, LOC_TOWN_SQUARE,  NPC_ACTION_IDLE);
}

const NPCSchedule *npc_schedule_get(int schedule_id)
{
    if (schedule_id < 0 || schedule_id >= g_schedule_count)
        return NULL;
    return &g_schedules[schedule_id];
}

const ScheduleEntry *npc_schedule_current(int schedule_id, int hour)
{
    const NPCSchedule *sched = npc_schedule_get(schedule_id);
    if (!sched)
        return NULL;

    hour = hour % 24;

    for (int i = 0; i < sched->entry_count; i++) {
        const ScheduleEntry *e = &sched->entries[i];
        if (e->hour_start < e->hour_end) {
            /* Normal range: e.g. 6-18 */
            if (hour >= e->hour_start && hour < e->hour_end)
                return e;
        } else {
            /* Wrapping range: e.g. 20-7 means 20..23 + 0..6 */
            if (hour >= e->hour_start || hour < e->hour_end)
                return e;
        }
    }

    /* Fallback: return first entry */
    if (sched->entry_count > 0)
        return &sched->entries[0];

    return NULL;
}
