#ifndef DIABLO_ENGINE_EFFECTS_H
#define DIABLO_ENGINE_EFFECTS_H

#include "common.h"

#define MAX_PARTICLES 256

typedef struct Particle {
    float x, y;            /* screen position */
    float vx, vy;          /* velocity */
    float life;            /* remaining life in seconds */
    float max_life;
    SDL_Color color;
    int size;              /* pixel size */
    bool active;
} Particle;

typedef struct ParticleSystem {
    Particle particles[MAX_PARTICLES];
} ParticleSystem;

typedef struct EffectsSystem {
    ParticleSystem particles;

    /* Fog of war / lighting */
    float light_radius;         /* in tiles (e.g., 5.0 for dungeon) */
    bool fog_enabled;           /* true in dungeons */
    SDL_Texture *fog_texture;   /* pre-allocated overlay texture */

    /* Day/night */
    float day_night_tint;       /* 0.0 = midnight (dark), 1.0 = noon (bright) */

    /* Screen flash */
    float flash_timer;
    float flash_duration;
    SDL_Color flash_color;
} EffectsSystem;

void effects_init(EffectsSystem *fx);
void effects_cleanup(EffectsSystem *fx);
void effects_update(EffectsSystem *fx, float dt, int game_hour);

/* Particles */
void effects_spawn_blood(EffectsSystem *fx, int screen_x, int screen_y);
void effects_spawn_heal(EffectsSystem *fx, int screen_x, int screen_y);
void effects_spawn_levelup(EffectsSystem *fx, int screen_x, int screen_y);
void effects_spawn_magic(EffectsSystem *fx, int screen_x, int screen_y);
void effects_render_particles(EffectsSystem *fx, SDL_Renderer *renderer);

/* Fog of war — darken tiles far from player */
void effects_render_fog(EffectsSystem *fx, SDL_Renderer *renderer,
                        int player_screen_x, int player_screen_y,
                        int viewport_w, int viewport_h);

/* Day/night tint overlay on town */
void effects_render_day_night(EffectsSystem *fx, SDL_Renderer *renderer,
                              int viewport_w, int viewport_h);

/* Screen flash (damage taken, etc.) */
void effects_flash(EffectsSystem *fx, SDL_Color color, float duration);
void effects_render_flash(EffectsSystem *fx, SDL_Renderer *renderer,
                          int screen_w, int screen_h);

#endif /* DIABLO_ENGINE_EFFECTS_H */
