#include "game/game.h"

void game_init(Game *game)
{
    game->state = GAME_STATE_PLAYING;
    game->game_day = 1;
    game->game_hour = 6;  /* start at dawn */
    game->game_time_acc = 0.0f;
}

void game_update(Game *game, float dt)
{
    /* Advance game time: 1 real second = 1 game minute
     * So 1 game hour = 60 real seconds, 1 game day = 24 real minutes */
    game->game_time_acc += dt;
    while (game->game_time_acc >= 60.0f) {
        game->game_time_acc -= 60.0f;
        game->game_hour++;
        if (game->game_hour >= 24) {
            game->game_hour = 0;
            game->game_day++;
        }
    }
}
