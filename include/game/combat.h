#ifndef DIABLO_GAME_COMBAT_H
#define DIABLO_GAME_COMBAT_H

#include "common.h"

/* Forward declarations */
struct Player;
struct Enemy;
struct EnemyManager;
struct Inventory;

typedef struct CombatResult {
    bool hit;
    int damage;
    bool killed;
    int xp_gained;
} CombatResult;

/* Player attacks enemy */
CombatResult combat_player_attack(struct Player *player, struct Enemy *enemy,
                                  const struct Inventory *inv);

/* Enemy attacks player (inv needed for equipped armor calculation) */
CombatResult combat_enemy_attack(struct Enemy *enemy, struct Player *player,
                                 const struct Inventory *inv);

/* Check if two positions are within attack range */
bool combat_in_range(int ax, int ay, int bx, int by, int range);

/* Process all combat for this frame (enemies attacking player) */
void combat_update(struct EnemyManager *enemies, struct Player *player,
                   const struct Inventory *inv, float dt);

#endif /* DIABLO_GAME_COMBAT_H */
