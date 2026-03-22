#ifndef DIABLO_GAME_STATS_H
#define DIABLO_GAME_STATS_H

#include "common.h"

/* Stat indices for point allocation */
#define STAT_STRENGTH  0
#define STAT_DEXTERITY 1
#define STAT_MAGIC     2
#define STAT_VITALITY  3

typedef struct PlayerStats {
    int level;
    int xp;
    int xp_to_next;     /* XP needed for next level */
    int stat_points;    /* unallocated points from level-up */

    /* Base stats (player allocates on level-up) */
    int strength;       /* melee damage, carry weight */
    int dexterity;      /* hit chance, dodge, attack speed */
    int magic;          /* spell damage, mana pool */
    int vitality;       /* HP pool, HP regen */

    /* Derived stats (calculated from base) */
    int max_hp, current_hp;
    int max_mana, current_mana;
    int armor_class;
    int to_hit_bonus;
    int min_damage, max_damage;
} PlayerStats;

/* Initialize with Level 1 warrior defaults */
void stats_init(PlayerStats *stats);

/* Recalculate derived stats from base stats */
void stats_recalculate(PlayerStats *stats);

/* Add XP. Returns true if leveled up */
bool stats_add_xp(PlayerStats *stats, int amount);

/* Increment level and grant stat points */
void stats_level_up(PlayerStats *stats);

/* Allocate one stat point: 0=str, 1=dex, 2=mag, 3=vit */
void stats_allocate_point(PlayerStats *stats, int stat_index);

#endif /* DIABLO_GAME_STATS_H */
