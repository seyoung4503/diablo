#ifndef DIABLO_GAME_COMBAT_ANIM_H
#define DIABLO_GAME_COMBAT_ANIM_H

#include "common.h"

#define MAX_COMBAT_TEXTS 32
#define MAX_ENEMIES_ANIM 32

/* Attack animation phases */
typedef enum {
    ATTACK_PHASE_NONE,
    ATTACK_PHASE_WINDUP,
    ATTACK_PHASE_SWING,
    ATTACK_PHASE_RECOVERY
} AttackPhase;

/* Weapon-driven attack timing */
typedef struct AttackTiming {
    float windup;
    float swing;
    float recovery;
} AttackTiming;

/* Player attack state */
typedef struct AttackAnimation {
    bool active;
    AttackPhase phase;
    float phase_timer;
    AttackTiming timing;
    bool damage_resolved;
    uint32_t target_enemy_id;
} AttackAnimation;

/* Floating combat text */
typedef struct CombatText {
    float x, y;
    float vy;
    float life, max_life;
    char text[16];
    SDL_Color color;
    bool active;
} CombatText;

/* Hit reaction on an enemy */
typedef struct HitReaction {
    bool active;
    float timer;
    float duration;
    float knockback_dx, knockback_dy;
} HitReaction;

/* Death animation for enemy */
typedef struct DeathAnim {
    bool active;
    float timer;
    float duration;
    uint32_t enemy_id;
    float x, y;            /* screen position at time of death */
    int tile_x, tile_y;    /* tile at death for loot */
} DeathAnim;

/* Full combat animation system */
typedef struct CombatAnimSystem {
    AttackAnimation player_attack;
    CombatText texts[MAX_COMBAT_TEXTS];
    HitReaction reactions[MAX_ENEMIES_ANIM];
    DeathAnim deaths[MAX_ENEMIES_ANIM];
} CombatAnimSystem;

/* Default melee timing */
static const AttackTiming MELEE_DEFAULT_TIMING = { 0.15f, 0.10f, 0.20f };

void combat_anim_init(CombatAnimSystem *cas);
void combat_anim_update(CombatAnimSystem *cas, float dt);

/* Start player attack. Returns false if already attacking. */
bool combat_anim_start_attack(CombatAnimSystem *cas, uint32_t target_id,
                              AttackTiming timing);

/* Check if player can act (move/attack) */
bool combat_anim_can_act(const CombatAnimSystem *cas);

/* Check if damage should be resolved this frame */
bool combat_anim_should_resolve_damage(CombatAnimSystem *cas);

/* Spawn floating text */
void combat_anim_spawn_text(CombatAnimSystem *cas, float x, float y,
                            const char *text, SDL_Color color);

/* Trigger hit reaction on enemy by index */
void combat_anim_hit_reaction(CombatAnimSystem *cas, int enemy_idx,
                              float dx, float dy);

/* Start death animation for an enemy */
void combat_anim_start_death(CombatAnimSystem *cas, int enemy_idx,
                             uint32_t enemy_id, float x, float y,
                             int tile_x, int tile_y);

/* Check if enemy index has active death anim (skip normal rendering) */
bool combat_anim_is_dying(const CombatAnimSystem *cas, int enemy_idx);

/* Get hit reaction offsets for rendering */
bool combat_anim_get_hit_offset(const CombatAnimSystem *cas, int enemy_idx,
                                float *dx, float *dy);

/* Render floating texts and death animations */
void combat_anim_render_texts(const CombatAnimSystem *cas, SDL_Renderer *renderer,
                              TTF_Font *font);

/* Render death fading sprites (call with enemy texture/position info) */
void combat_anim_render_deaths(const CombatAnimSystem *cas, SDL_Renderer *renderer);

#endif
