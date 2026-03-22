#ifndef DIABLO_GAME_H
#define DIABLO_GAME_H

#include "common.h"

typedef enum {
    GAME_STATE_PLAYING,
    GAME_STATE_MENU,
    GAME_STATE_INVENTORY,
} GameState;

typedef struct Game {
    GameState state;
    int game_day;
    int game_hour;
    float game_time_acc;  /* accumulator for game time progression */
} Game;

void game_init(Game *game);
void game_update(Game *game, float dt);

#endif /* DIABLO_GAME_H */
