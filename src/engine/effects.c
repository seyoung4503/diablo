#include "engine/effects.h"

/* ── helpers ─────────────────────────────────────────────────────────── */

static float randf(float lo, float hi)
{
    return lo + (float)rand() / (float)RAND_MAX * (hi - lo);
}

static Particle *find_free_particle(ParticleSystem *ps)
{
    for (int i = 0; i < MAX_PARTICLES; i++) {
        if (!ps->particles[i].active)
            return &ps->particles[i];
    }
    return NULL;
}

static void spawn_particle(ParticleSystem *ps,
                            float x, float y,
                            float vx, float vy,
                            float life, SDL_Color color, int size)
{
    Particle *p = find_free_particle(ps);
    if (!p) return;
    p->x = x;
    p->y = y;
    p->vx = vx;
    p->vy = vy;
    p->life = life;
    p->max_life = life;
    p->color = color;
    p->size = size;
    p->active = true;
}

/* ── init / cleanup ──────────────────────────────────────────────────── */

void effects_init(EffectsSystem *fx)
{
    memset(fx, 0, sizeof(*fx));
    fx->light_radius = 5.0f;
    fx->fog_enabled = false;
    fx->fog_texture = NULL;
    fx->day_night_tint = 1.0f;
    fx->flash_timer = 0.0f;
    fx->flash_duration = 0.0f;
    fx->shake_timer = 0.0f;
    fx->shake_duration = 0.0f;
    fx->shake_intensity = 0.0f;
    memset(fx->fog_visited, 0, sizeof(fx->fog_visited));
}

void effects_cleanup(EffectsSystem *fx)
{
    if (fx->fog_texture) {
        SDL_DestroyTexture(fx->fog_texture);
        fx->fog_texture = NULL;
    }
}

/* ── update ──────────────────────────────────────────────────────────── */

void effects_update(EffectsSystem *fx, float dt, int game_hour)
{
    /* Update particles */
    for (int i = 0; i < MAX_PARTICLES; i++) {
        Particle *p = &fx->particles.particles[i];
        if (!p->active) continue;
        p->x += p->vx * dt;
        p->y += p->vy * dt;
        p->life -= dt;
        if (p->life <= 0.0f)
            p->active = false;
    }

    /* Update screen shake */
    if (fx->shake_timer > 0.0f)
        fx->shake_timer -= dt;

    /* Update screen flash */
    if (fx->flash_timer > 0.0f) {
        fx->flash_timer -= dt;
        if (fx->flash_timer < 0.0f)
            fx->flash_timer = 0.0f;
    }

    /* Update day/night tint based on game hour (0-23) */
    if (game_hour >= 8 && game_hour < 18) {
        /* Daytime — fully bright */
        fx->day_night_tint = 1.0f;
    } else if (game_hour >= 6 && game_hour < 8) {
        /* Sunrise: ramp from 0.4 to 1.0 */
        fx->day_night_tint = 0.4f + 0.6f * ((float)(game_hour - 6) / 2.0f);
    } else if (game_hour >= 18 && game_hour < 20) {
        /* Sunset: ramp from 1.0 to 0.4 */
        fx->day_night_tint = 1.0f - 0.6f * ((float)(game_hour - 18) / 2.0f);
    } else {
        /* Night */
        fx->day_night_tint = 0.3f;
    }
}

/* ── particle spawners ───────────────────────────────────────────────── */

void effects_spawn_blood(EffectsSystem *fx, int screen_x, int screen_y)
{
    int count = 8 + rand() % 5;  /* 8-12 particles */
    for (int i = 0; i < count; i++) {
        SDL_Color c = { (Uint8)(150 + rand() % 80), 0, 0, 255 };
        spawn_particle(&fx->particles,
                       (float)screen_x, (float)screen_y,
                       randf(-60.0f, 60.0f),   /* spread outward */
                       randf(-80.0f, 20.0f),   /* mostly upward */
                       randf(0.3f, 0.7f),
                       c, 2 + rand() % 2);
    }
}

void effects_spawn_heal(EffectsSystem *fx, int screen_x, int screen_y)
{
    int count = 6 + rand() % 3;  /* 6-8 particles */
    for (int i = 0; i < count; i++) {
        SDL_Color c = { 50, (Uint8)(180 + rand() % 60), 50, 255 };
        spawn_particle(&fx->particles,
                       (float)screen_x + randf(-10.0f, 10.0f),
                       (float)screen_y,
                       randf(-15.0f, 15.0f),   /* slight horizontal drift */
                       randf(-80.0f, -40.0f),  /* float upward */
                       randf(0.5f, 1.0f),
                       c, 3);
    }
}

void effects_spawn_levelup(EffectsSystem *fx, int screen_x, int screen_y)
{
    int count = 20;
    for (int i = 0; i < count; i++) {
        /* Burst outward in a circle */
        float angle = (float)i / (float)count * 2.0f * (float)M_PI;
        float speed = randf(60.0f, 120.0f);
        SDL_Color c = { (Uint8)(200 + rand() % 55), (Uint8)(180 + rand() % 55), 30, 255 };
        spawn_particle(&fx->particles,
                       (float)screen_x, (float)screen_y,
                       cosf(angle) * speed,
                       sinf(angle) * speed,
                       randf(0.6f, 1.2f),
                       c, 3);
    }
}

void effects_spawn_magic(EffectsSystem *fx, int screen_x, int screen_y)
{
    int count = 10;
    for (int i = 0; i < count; i++) {
        /* Spiral pattern: offset angle + upward drift */
        float angle = (float)i / (float)count * 2.0f * (float)M_PI;
        float speed = randf(30.0f, 60.0f);
        SDL_Color c;
        if (rand() % 2) {
            c = (SDL_Color){ 80, 80, (Uint8)(200 + rand() % 55), 255 };  /* blue */
        } else {
            c = (SDL_Color){ (Uint8)(160 + rand() % 60), 50, (Uint8)(200 + rand() % 55), 255 };  /* purple */
        }
        spawn_particle(&fx->particles,
                       (float)screen_x + cosf(angle) * 8.0f,
                       (float)screen_y + sinf(angle) * 8.0f,
                       cosf(angle) * speed,
                       sinf(angle) * speed - 30.0f,  /* upward bias */
                       randf(0.4f, 0.9f),
                       c, 2 + rand() % 2);
    }
}

void effects_spawn_impact(EffectsSystem *fx, int screen_x, int screen_y, SDL_Color color)
{
    /* 12 particles in a radial burst pattern */
    for (int i = 0; i < 12; i++) {
        Particle *p = find_free_particle(&fx->particles);
        if (!p) break;
        float angle = (float)i / 12.0f * 6.2832f;  /* 2*PI */
        float speed = 40.0f + randf(0, 30.0f);
        p->active = true;
        p->x = (float)screen_x;
        p->y = (float)screen_y;
        p->vx = cosf(angle) * speed;
        p->vy = sinf(angle) * speed;
        p->life = 0.2f + randf(0, 0.15f);
        p->max_life = p->life;
        p->color = color;
        p->size = 2;
    }
}

/* ── render particles ────────────────────────────────────────────────── */

void effects_render_particles(EffectsSystem *fx, SDL_Renderer *renderer)
{
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);

    for (int i = 0; i < MAX_PARTICLES; i++) {
        Particle *p = &fx->particles.particles[i];
        if (!p->active) continue;

        /* Fade alpha based on remaining life */
        float alpha_frac = p->life / p->max_life;
        Uint8 alpha = (Uint8)(alpha_frac * 255.0f);

        SDL_SetRenderDrawColor(renderer, p->color.r, p->color.g, p->color.b, alpha);
        SDL_Rect r = {
            (int)p->x - p->size / 2,
            (int)p->y - p->size / 2,
            p->size, p->size
        };
        SDL_RenderFillRect(renderer, &r);
    }
}

/* ── fog of war ──────────────────────────────────────────────────────── */

void effects_render_fog(EffectsSystem *fx, SDL_Renderer *renderer,
                        int player_screen_x, int player_screen_y,
                        int viewport_w, int viewport_h)
{
    if (!fx->fog_enabled) return;

    /* Ensure fog texture exists */
    if (!fx->fog_texture) {
        fx->fog_texture = SDL_CreateTexture(renderer,
                                            SDL_PIXELFORMAT_RGBA8888,
                                            SDL_TEXTUREACCESS_TARGET,
                                            viewport_w, viewport_h);
        if (!fx->fog_texture) return;
        SDL_SetTextureBlendMode(fx->fog_texture, SDL_BLENDMODE_BLEND);
    }

    /* Render to fog texture */
    SDL_SetRenderTarget(renderer, fx->fog_texture);

    /* Fill with opaque black */
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);

    /* Punch a radial gradient hole at the player position */
    float radius_pixels = fx->light_radius * (float)TILE_WIDTH;
    int rings = 24;

    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);

    for (int ring = rings; ring >= 0; ring--) {
        float frac = (float)ring / (float)rings;
        float r = radius_pixels * frac;

        /* Inner 70%: fully transparent. Outer 30%: gradient to opaque */
        Uint8 alpha;
        if (frac < 0.7f) {
            alpha = 0;
        } else {
            float t = (frac - 0.7f) / 0.3f;
            alpha = (Uint8)(t * 255.0f);
        }

        SDL_SetRenderDrawColor(renderer, 0, 0, 0, alpha);

        /* Draw filled circle as horizontal scan lines */
        int ir = (int)r;
        for (int dy = -ir; dy <= ir; dy++) {
            int dx = (int)sqrtf((float)(ir * ir - dy * dy));
            SDL_Rect line = {
                player_screen_x - dx,
                player_screen_y + dy,
                dx * 2,
                1
            };
            SDL_RenderFillRect(renderer, &line);
        }
    }

    /* Restore render target */
    SDL_SetRenderTarget(renderer, NULL);

    /* Draw fog overlay onto screen */
    SDL_Rect dst = { 0, 0, viewport_w, viewport_h };
    SDL_RenderCopy(renderer, fx->fog_texture, NULL, &dst);
}

/* ── day/night overlay ───────────────────────────────────────────────── */

void effects_render_day_night(EffectsSystem *fx, SDL_Renderer *renderer,
                              int viewport_w, int viewport_h)
{
    /* No overlay needed during full daylight */
    if (fx->day_night_tint >= 0.95f) return;

    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);

    /* Night: dark blue overlay. Sunrise/sunset: warm orange overlay */
    Uint8 r, g, b;
    if (fx->day_night_tint < 0.4f) {
        /* Deep night — dark blue */
        r = 10; g = 10; b = 40;
    } else {
        /* Sunrise/sunset — warm orange */
        r = 40; g = 20; b = 5;
    }

    /* Alpha inversely proportional to tint (darker = more overlay) */
    Uint8 alpha = (Uint8)((1.0f - fx->day_night_tint) * 160.0f);

    SDL_SetRenderDrawColor(renderer, r, g, b, alpha);
    SDL_Rect overlay = { 0, 0, viewport_w, viewport_h };
    SDL_RenderFillRect(renderer, &overlay);
}

/* ── screen flash ────────────────────────────────────────────────────── */

void effects_flash(EffectsSystem *fx, SDL_Color color, float duration)
{
    fx->flash_color = color;
    fx->flash_timer = duration;
    fx->flash_duration = duration;
}

void effects_render_flash(EffectsSystem *fx, SDL_Renderer *renderer,
                          int screen_w, int screen_h)
{
    if (fx->flash_timer <= 0.0f) return;

    float alpha_frac = fx->flash_timer / fx->flash_duration;
    Uint8 alpha = (Uint8)(alpha_frac * 128.0f);

    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(renderer,
                           fx->flash_color.r, fx->flash_color.g,
                           fx->flash_color.b, alpha);
    SDL_Rect r = { 0, 0, screen_w, screen_h };
    SDL_RenderFillRect(renderer, &r);
}

/* ── screen shake ───────────────────────────────────────────────────── */

void effects_screen_shake(EffectsSystem *fx, float intensity, float duration)
{
    fx->shake_timer = duration;
    fx->shake_duration = duration;
    fx->shake_intensity = intensity;
}

int effects_get_shake_x(const EffectsSystem *fx)
{
    if (fx->shake_timer > 0.0f && fx->shake_duration > 0.0f)
        return (int)(sinf(fx->shake_timer * 40.0f) * fx->shake_intensity *
                     (fx->shake_timer / fx->shake_duration));
    return 0;
}

int effects_get_shake_y(const EffectsSystem *fx)
{
    if (fx->shake_timer > 0.0f && fx->shake_duration > 0.0f)
        return (int)(cosf(fx->shake_timer * 40.0f) * fx->shake_intensity *
                     (fx->shake_timer / fx->shake_duration));
    return 0;
}

/* ── projectiles ────────────────────────────────────────────────────── */

void effects_spawn_projectile(EffectsSystem *fx, float x, float y,
                              float tx, float ty, float speed,
                              int damage, int source_id, SDL_Color color)
{
    for (int i = 0; i < MAX_PROJECTILES; i++) {
        if (fx->projectiles[i].active) continue;
        float dx = tx - x;
        float dy = ty - y;
        float dist = sqrtf(dx * dx + dy * dy);
        if (dist < 0.001f) dist = 1.0f;
        fx->projectiles[i].x = x;
        fx->projectiles[i].y = y;
        fx->projectiles[i].vx = (dx / dist) * speed;
        fx->projectiles[i].vy = (dy / dist) * speed;
        fx->projectiles[i].life = 5.0f;
        fx->projectiles[i].damage = damage;
        fx->projectiles[i].source_id = source_id;
        fx->projectiles[i].color = color;
        fx->projectiles[i].active = true;
        return;
    }
}

void effects_update_projectiles(EffectsSystem *fx, float dt)
{
    for (int i = 0; i < MAX_PROJECTILES; i++) {
        if (!fx->projectiles[i].active) continue;
        fx->projectiles[i].x += fx->projectiles[i].vx * dt;
        fx->projectiles[i].y += fx->projectiles[i].vy * dt;
        fx->projectiles[i].life -= dt;
        if (fx->projectiles[i].life <= 0.0f)
            fx->projectiles[i].active = false;
    }
}

void effects_render_projectiles(EffectsSystem *fx, SDL_Renderer *renderer,
                                int cam_x, int cam_y, int screen_cx)
{
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    for (int i = 0; i < MAX_PROJECTILES; i++) {
        if (!fx->projectiles[i].active) continue;
        int sx = (int)fx->projectiles[i].x - cam_x + screen_cx;
        int sy = (int)fx->projectiles[i].y - cam_y;
        SDL_Color c = fx->projectiles[i].color;
        SDL_SetRenderDrawColor(renderer, c.r, c.g, c.b, c.a);
        /* Draw small filled circle (3px radius) */
        for (int dy = -3; dy <= 3; dy++) {
            int dx = (int)sqrtf((float)(9 - dy * dy));
            SDL_Rect line = { sx - dx, sy + dy, dx * 2, 1 };
            SDL_RenderFillRect(renderer, &line);
        }
    }
}

/* ── fog visited tiles ──────────────────────────────────────────────── */

void effects_mark_visited(EffectsSystem *fx, int tx, int ty)
{
    if (tx >= 0 && tx < MAP_WIDTH && ty >= 0 && ty < MAP_HEIGHT)
        fx->fog_visited[ty][tx] = true;
}

bool effects_is_visited(const EffectsSystem *fx, int tx, int ty)
{
    if (tx >= 0 && tx < MAP_WIDTH && ty >= 0 && ty < MAP_HEIGHT)
        return fx->fog_visited[ty][tx];
    return false;
}
