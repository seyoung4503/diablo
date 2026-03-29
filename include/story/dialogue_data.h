#ifndef DIABLO_STORY_DIALOGUE_DATA_H
#define DIABLO_STORY_DIALOGUE_DATA_H

struct DialogueSystem;

/* NPC IDs — must match NPC manager IDs */
#define NPC_ID_GRISWOLD  0
#define NPC_ID_PEPIN     1
#define NPC_ID_CAIN      2
#define NPC_ID_OGDEN     3
#define NPC_ID_FARNHAM   4
#define NPC_ID_ADRIA     5
#define NPC_ID_THEO      6
#define NPC_ID_GHARBAD   7
#define NPC_ID_WIRT      8

/* Flag IDs for dialogue state tracking */
#define FLAG_POISONED_WELL_STARTED  10
#define FLAG_WITCHS_BARGAIN_DONE    11
#define FLAG_THEOS_ADVENTURE        12
#define FLAG_GHARBADS_PLEA          13
#define FLAG_TALKED_GRISWOLD_FORGE  14
#define FLAG_TALKED_CAIN_LORE       15
#define FLAG_FARNHAM_TRUSTED        16
#define FLAG_BOUGHT_FARNHAM_DRINK   17
#define FLAG_BUTCHERS_LAIR          18
#define FLAG_HALLS_OF_BLIND         19
#define FLAG_VALOR_QUEST            20
#define FLAG_CHAMBER_OF_BONE        21
#define FLAG_DIABLOS_DOMAIN         22
#define FLAG_WIRTS_LEG              23

/* Populate all NPC dialogue nodes into the system */
void dialogue_data_init(struct DialogueSystem *ds);

#endif /* DIABLO_STORY_DIALOGUE_DATA_H */
