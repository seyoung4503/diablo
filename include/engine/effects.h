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

    /* Screen shake */
    float shake_timer;
    float shake_duration;
    float shake_intensity;

    /* Projectile system */
#define MAX_PROJECTILES 16
    struct {
        float x, y;            /* world position */
        float vx, vy;          /* velocity pixels/sec */
        float life;
        int damage;
        int source_id;          /* -1 = player, >= 0 = enemy index */
        SDL_Color color;
        bool active;
    } projectiles[MAX_PROJECTILES];

    /* Visited tiles for fog (explored but not in light radius) */
    bool fog_visited[MAP_HEIGHT][MAP_WIDTH];
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

/* Screen shake */
void effects_screen_shake(EffectsSystem *fx, float intensity, float duration);
int effects_get_shake_x(const EffectsSystem *fx);
int effects_get_shake_y(const EffectsSystem *fx);

/* Projectiles */
void effects_spawn_projectile(EffectsSystem *fx, float x, float y,
                              float tx, float ty, float speed,
                              int damage, int source_id, SDL_Color color);
void effects_update_projectiles(EffectsSystem *fx, float dt);
void effects_render_projectiles(EffectsSystem *fx, SDL_Renderer *renderer,
                                int cam_x, int cam_y, int screen_cx);

/* Fog visited tiles */
void effects_mark_visited(EffectsSystem *fx, int tx, int ty);
bool effects_is_visited(const EffectsSystem *fx, int tx, int ty);

#endif /* DIABLO_ENGINE_EFFECTS_H */
