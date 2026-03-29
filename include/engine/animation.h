#ifndef DIABLO_ENGINE_ANIMATION_H
#define DIABLO_ENGINE_ANIMATION_H

#include "common.h"

#define MAX_ANIM_FRAMES   16
#define MAX_SPRITE_SHEETS 32
#define ANIM_STATE_COUNT  5
#define DIR_COUNT_ANIM    8

/* A single frame within a sprite sheet */
typedef struct SpriteFrame {
    SDL_Rect src_rect;
    float duration;
    int offset_x, offset_y;
} SpriteFrame;

/* An animation sequence (one state + one direction) */
typedef struct AnimSequence {
    SpriteFrame frames[MAX_ANIM_FRAMES];
    int frame_count;
    bool looping;
    float total_duration;
} AnimSequence;

/* Complete sprite sheet with all animation data */
typedef struct SpriteSheet {
    SDL_Texture *texture;
    int sheet_width, sheet_height;
    int frame_width, frame_height;
    int pivot_x, pivot_y;           /* foot pivot within frame */
    /* Data-driven metadata: sequences[anim_state][direction] */
    AnimSequence sequences[ANIM_STATE_COUNT][DIR_COUNT_ANIM];
    char name[64];
    bool loaded;
} SpriteSheet;

/* Runtime animation controller (one per entity) */
typedef struct AnimController {
    const SpriteSheet *sheet;
    int current_state;      /* AnimState value */
    int current_dir;        /* Direction value */
    int current_frame;
    float frame_timer;
    bool finished;          /* true when non-looping anim ends */
    int event_frame;        /* frame that triggers damage/event, -1 = none */
    bool event_fired;       /* has the event been triggered this cycle */
} AnimController;

/* Sprite sheet manager */
typedef struct SpriteSheetManager {
    SpriteSheet sheets[MAX_SPRITE_SHEETS];
    int count;
} SpriteSheetManager;

/* Manager lifecycle */
void spritesheet_manager_init(SpriteSheetManager *mgr);
void spritesheet_manager_shutdown(SpriteSheetManager *mgr);

/* Load a sprite sheet PNG and auto-subdivide into frames.
 * Layout: rows = state * 8 + direction, columns = frames.
 * Returns sheet index, or -1 on failure. */
int spritesheet_load(SpriteSheetManager *mgr, SDL_Renderer *renderer,
                     const char *path, int frame_w, int frame_h);

/* Get sheet by index. Returns NULL if invalid. */
const SpriteSheet *spritesheet_get(const SpriteSheetManager *mgr, int index);

/* Configure animation metadata for a sheet after loading.
 * Sets frame count, duration, looping, and event frame for a specific state. */
void spritesheet_set_anim(SpriteSheet *sheet, int anim_state,
                          int frame_count, float frame_duration,
                          bool looping, int event_frame);

/* Apply default animation metadata (frame counts, durations) to a sheet */
void spritesheet_apply_defaults(SpriteSheet *sheet);

/* Controller API */
void anim_controller_init(AnimController *ac, const SpriteSheet *sheet);
void anim_controller_set_state(AnimController *ac, int anim_state, int direction);
void anim_controller_update(AnimController *ac, float dt);
bool anim_controller_is_finished(const AnimController *ac);
SDL_Rect anim_controller_get_src_rect(const AnimController *ac);
/* Check if event frame was just reached (call once per frame after update) */
bool anim_controller_check_event(AnimController *ac);

#endif /* DIABLO_ENGINE_ANIMATION_H */
