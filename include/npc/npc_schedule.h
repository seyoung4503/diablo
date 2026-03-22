#ifndef DIABLO_NPC_SCHEDULE_H
#define DIABLO_NPC_SCHEDULE_H

#include "npc/npc.h"

/* Forward declaration */
struct Town;

typedef struct ScheduleEntry {
    int hour_start;         /* 0-23 */
    int hour_end;           /* 0-23 (exclusive, wraps at 24) */
    NPCLocation location;  /* TownLocation value */
    NPCAction action;
} ScheduleEntry;

#define MAX_SCHEDULE_ENTRIES 8
#define MAX_SCHEDULES 20

typedef struct NPCSchedule {
    ScheduleEntry entries[MAX_SCHEDULE_ENTRIES];
    int entry_count;
} NPCSchedule;

/* Get the schedule for a given NPC schedule_id */
const NPCSchedule *npc_schedule_get(int schedule_id);

/* Get what an NPC should be doing at a given hour */
const ScheduleEntry *npc_schedule_current(int schedule_id, int hour);

/* Initialize all schedule data */
void npc_schedules_init(void);

#endif /* DIABLO_NPC_SCHEDULE_H */
