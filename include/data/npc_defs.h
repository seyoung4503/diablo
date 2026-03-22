#ifndef DIABLO_DATA_NPC_DEFS_H
#define DIABLO_DATA_NPC_DEFS_H

#include "npc/npc.h"

struct Town;

/* Load all 20 Tristram NPC definitions into the manager */
void npc_defs_load(NPCManager *mgr, const struct Town *town);

#endif /* DIABLO_DATA_NPC_DEFS_H */
