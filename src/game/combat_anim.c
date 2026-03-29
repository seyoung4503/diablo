#include "game/combat_anim.h"
#include <string.h>

void combat_anim_init(CombatAnimSystem *cas)
{
    memset(cas, 0, sizeof(*cas));
}

void combat_anim_update(CombatAnimSystem *cas, float dt)
{
    /* Update player attack phase timers */
    AttackAnimation *atk = &cas->player_attack;
    if (atk->active) {
        atk->phase_timer += dt;
        switch (atk->phase) {
        case ATTACK_PHASE_WINDUP:
            if (atk->phase_timer >= atk->timing.windup) {
                atk->phase = ATTACK_PHASE_SWING;
                atk->phase_timer = 0.0f;
            }
            break;
        case ATTACK_PHASE_SWING:
            if (atk->phase_timer >= atk->timing.swing) {
                atk->phase = ATTACK_PHASE_RECOVERY;
                atk->phase_timer = 0.0f;
            }
            break;
        case ATTACK_PHASE_RECOVERY:
            if (atk->phase_timer >= atk->timing.recovery) {
                atk->phase = ATTACK_PHASE_NONE;
                atk->active = false;
                atk->phase_timer = 0.0f;
            }
            break;
        default:
            atk->active = false;
            break;
        }
    }

    /* Update floating combat texts */
    for (int i = 0; i < MAX_COMBAT_TEXTS; i++) {
        CombatText *ct = &cas->texts[i];
        if (!ct->active)
            continue;
        ct->y += ct->vy * dt;
        ct->life -= dt;
        if (ct->life <= 0.0f)
            ct->active = false;
    }

    /* Update hit reactions */
    for (int i = 0; i < MAX_ENEMIES_ANIM; i++) {
        HitReaction *hr = &cas->reactions[i];
        if (!hr->active)
            continue;
        hr->timer -= dt;
        if (hr->timer <= 0.0f)
            hr->active = false;
    }

    /* Update death animations */
    for (int i = 0; i < MAX_ENEMIES_ANIM; i++) {
        DeathAnim *da = &cas->deaths[i];
        if (!da->active)
            continue;
        da->timer += dt;
        if (da->timer >= da->duration)
            da->active = false;
    }
}

bool combat_anim_start_attack(CombatAnimSystem *cas, uint32_t target_id,
                              AttackTiming timing)
{
    if (cas->player_attack.active)
        return false;

    cas->player_attack.active = true;
    cas->player_attack.phase = ATTACK_PHASE_WINDUP;
    cas->player_attack.phase_timer = 0.0f;
    cas->player_attack.damage_resolved = false;
    cas->player_attack.target_enemy_id = target_id;
    cas->player_attack.timing = timing;
    return true;
}

bool combat_anim_can_act(const CombatAnimSystem *cas)
{
    return !cas->player_attack.active;
}

bool combat_anim_should_resolve_damage(CombatAnimSystem *cas)
{
    AttackAnimation *atk = &cas->player_attack;
    if (atk->phase == ATTACK_PHASE_SWING && !atk->damage_resolved) {
        atk->damage_resolved = true;
        return true;
    }
    return false;
}

void combat_anim_spawn_text(CombatAnimSystem *cas, float x, float y,
                            const char *text, SDL_Color color)
{
    for (int i = 0; i < MAX_COMBAT_TEXTS; i++) {
        CombatText *ct = &cas->texts[i];
        if (ct->active)
            continue;
        ct->x = x;
        ct->y = y;
        ct->vy = -60.0f;
        ct->life = 1.0f;
        ct->max_life = 1.0f;
        strncpy(ct->text, text, sizeof(ct->text) - 1);
        ct->text[sizeof(ct->text) - 1] = '\0';
        ct->color = color;
        ct->active = true;
        return;
    }
}

void combat_anim_hit_reaction(CombatAnimSystem *cas, int enemy_idx,
                              float dx, float dy)
{
    if (enemy_idx < 0 || enemy_idx >= MAX_ENEMIES_ANIM)
        return;
    HitReaction *hr = &cas->reactions[enemy_idx];
    hr->active = true;
    hr->timer = 0.15f;
    hr->duration = 0.15f;
    hr->knockback_dx = dx;
    hr->knockback_dy = dy;
}

void combat_anim_start_death(CombatAnimSystem *cas, int enemy_idx,
                             uint32_t enemy_id, float x, float y,
                             int tile_x, int tile_y)
{
    if (enemy_idx < 0 || enemy_idx >= MAX_ENEMIES_ANIM)
        return;
    DeathAnim *da = &cas->deaths[enemy_idx];
    da->active = true;
    da->timer = 0.0f;
    da->duration = 1.0f;
    da->enemy_id = enemy_id;
    da->x = x;
    da->y = y;
    da->tile_x = tile_x;
    da->tile_y = tile_y;
}

bool combat_anim_is_dying(const CombatAnimSystem *cas, int enemy_idx)
{
    if (enemy_idx < 0 || enemy_idx >= MAX_ENEMIES_ANIM)
        return false;
    return cas->deaths[enemy_idx].active;
}

bool combat_anim_get_hit_offset(const CombatAnimSystem *cas, int enemy_idx,
                                float *dx, float *dy)
{
    if (enemy_idx < 0 || enemy_idx >= MAX_ENEMIES_ANIM)
        return false;
    const HitReaction *hr = &cas->reactions[enemy_idx];
    if (!hr->active)
        return false;
    /* Knockback fades over duration */
    float fade = hr->timer / hr->duration;
    *dx = hr->knockback_dx * fade;
    *dy = hr->knockback_dy * fade;
    return true;
}

void combat_anim_render_texts(const CombatAnimSystem *cas, SDL_Renderer *renderer,
                              TTF_Font *font)
{
    for (int i = 0; i < MAX_COMBAT_TEXTS; i++) {
        const CombatText *ct = &cas->texts[i];
        if (!ct->active)
            continue;

        float alpha_f = (ct->life / ct->max_life) * 255.0f;
        Uint8 alpha = (Uint8)(alpha_f > 255.0f ? 255 : alpha_f);

        int tx = (int)ct->x;
        int ty = (int)ct->y;

        if (font) {
            /* Render with TTF: shadow first, then colored text */
            SDL_Surface *shadow_surf = TTF_RenderText_Blended(
                font, ct->text, (SDL_Color){0, 0, 0, alpha});
            if (shadow_surf) {
                SDL_Texture *shadow_tex = SDL_CreateTextureFromSurface(renderer, shadow_surf);
                if (shadow_tex) {
                    SDL_SetTextureAlphaMod(shadow_tex, alpha);
                    SDL_Rect dst = { tx + 1, ty + 1, shadow_surf->w, shadow_surf->h };
                    SDL_RenderCopy(renderer, shadow_tex, NULL, &dst);
                    SDL_DestroyTexture(shadow_tex);
                }
                SDL_FreeSurface(shadow_surf);
            }

            SDL_Color col = ct->color;
            col.a = alpha;
            SDL_Surface *text_surf = TTF_RenderText_Blended(font, ct->text, col);
            if (text_surf) {
                SDL_Texture *text_tex = SDL_CreateTextureFromSurface(renderer, text_surf);
                if (text_tex) {
                    SDL_SetTextureAlphaMod(text_tex, alpha);
                    SDL_Rect dst = { tx, ty, text_surf->w, text_surf->h };
                    SDL_RenderCopy(renderer, text_tex, NULL, &dst);
                    SDL_DestroyTexture(text_tex);
                }
                SDL_FreeSurface(text_surf);
            }
        } else {
            /* Fallback: draw colored rectangles */
            int w = (int)strlen(ct->text) * 8;
            int h = 14;
            /* Shadow */
            SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
            SDL_SetRenderDrawColor(renderer, 0, 0, 0, alpha);
            SDL_Rect shadow = { tx + 1, ty + 1, w, h };
            SDL_RenderFillRect(renderer, &shadow);
            /* Text rectangle */
            SDL_SetRenderDrawColor(renderer, ct->color.r, ct->color.g,
                                   ct->color.b, alpha);
            SDL_Rect rect = { tx, ty, w, h };
            SDL_RenderFillRect(renderer, &rect);
        }
    }
}

void combat_anim_render_deaths(const CombatAnimSystem *cas, SDL_Renderer *renderer)
{
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    for (int i = 0; i < MAX_ENEMIES_ANIM; i++) {
        const DeathAnim *da = &cas->deaths[i];
        if (!da->active)
            continue;
        /* Fading dark-red rectangle as placeholder for sprite fade */
        float progress = da->timer / da->duration;
        Uint8 alpha = (Uint8)(255.0f * (1.0f - progress));
        SDL_SetRenderDrawColor(renderer, 120, 20, 20, alpha);
        SDL_Rect rect = {
            (int)da->x - SPRITE_SIZE / 2,
            (int)da->y - SPRITE_SIZE + TILE_HALF_H,
            SPRITE_SIZE, SPRITE_SIZE
        };
        SDL_RenderFillRect(renderer, &rect);
    }
}
