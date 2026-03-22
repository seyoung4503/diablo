#ifndef DIABLO_ENEMY_ENEMY_AI_H
#define DIABLO_ENEMY_ENEMY_AI_H

#include "enemy/enemy.h"

/* Forward declarations */
struct TileMap;

/* Update AI state machine for a single enemy */
void enemy_ai_update(Enemy *enemy, float dt, int player_x, int player_y,
                     const struct TileMap *map);

#endif /* DIABLO_ENEMY_ENEMY_AI_H */
