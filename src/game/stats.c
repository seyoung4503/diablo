#include "game/stats.h"

void stats_init(PlayerStats *stats)
{
    stats->level = 1;
    stats->xp = 0;
    stats->stat_points = 0;

    /* Warrior starting stats (Diablo 1 inspired) */
    stats->strength  = 30;
    stats->dexterity = 20;
    stats->magic     = 10;
    stats->vitality  = 25;

    /* Zero current values before recalculate (it clamps them against max) */
    stats->current_hp   = 0;
    stats->current_mana = 0;

    stats_recalculate(stats);

    /* Start at full HP and mana */
    stats->current_hp   = stats->max_hp;
    stats->current_mana = stats->max_mana;
}

void stats_recalculate(PlayerStats *stats)
{
    int lvl = stats->level;

    stats->max_hp      = 2 * stats->vitality + 18 + (lvl - 1) * 2;
    stats->max_mana    = stats->magic + 10 + (lvl - 1);
    stats->armor_class = stats->dexterity / 5;
    stats->to_hit_bonus = 50 + stats->dexterity;
    stats->min_damage  = stats->strength / 5 + 1;
    stats->max_damage  = stats->strength / 2 + 2;
    stats->xp_to_next  = lvl * 100;

    /* Clamp current values to new maximums */
    if (stats->current_hp > stats->max_hp)
        stats->current_hp = stats->max_hp;
    if (stats->current_mana > stats->max_mana)
        stats->current_mana = stats->max_mana;
}

bool stats_add_xp(PlayerStats *stats, int amount)
{
    if (amount <= 0)
        return false;

    stats->xp += amount;
    bool leveled = false;

    while (stats->xp >= stats->xp_to_next) {
        stats->xp -= stats->xp_to_next;
        stats_level_up(stats);
        leveled = true;
    }

    return leveled;
}

void stats_level_up(PlayerStats *stats)
{
    stats->level++;
    stats->stat_points += 5;
    stats_recalculate(stats);

    /* Restore HP and mana on level-up */
    stats->current_hp   = stats->max_hp;
    stats->current_mana = stats->max_mana;
}

void stats_allocate_point(PlayerStats *stats, int stat_index)
{
    if (stats->stat_points <= 0)
        return;

    switch (stat_index) {
    case STAT_STRENGTH:  stats->strength++;  break;
    case STAT_DEXTERITY: stats->dexterity++; break;
    case STAT_MAGIC:     stats->magic++;     break;
    case STAT_VITALITY:  stats->vitality++;  break;
    default: return;  /* invalid index, don't spend the point */
    }

    stats->stat_points--;
    stats_recalculate(stats);
}
