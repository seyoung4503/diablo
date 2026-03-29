#include "engine/animation.h"

void spritesheet_manager_init(SpriteSheetManager *mgr)
{
    memset(mgr, 0, sizeof(*mgr));
}

void spritesheet_manager_shutdown(SpriteSheetManager *mgr)
{
    for (int i = 0; i < mgr->count; i++) {
        if (mgr->sheets[i].texture) {
            SDL_DestroyTexture(mgr->sheets[i].texture);
            mgr->sheets[i].texture = NULL;
        }
        mgr->sheets[i].loaded = false;
    }
    mgr->count = 0;
}

int spritesheet_load(SpriteSheetManager *mgr, SDL_Renderer *renderer,
                     const char *path, int frame_w, int frame_h)
{
    if (mgr->count >= MAX_SPRITE_SHEETS) {
        fprintf(stderr, "spritesheet_load: max sheets reached\n");
        return -1;
    }

    SDL_Texture *tex = IMG_LoadTexture(renderer, path);
    if (!tex) {
        fprintf(stderr, "spritesheet_load: failed to load '%s': %s\n",
                path, SDL_GetError());
        return -1;
    }

    SDL_SetTextureBlendMode(tex, SDL_BLENDMODE_BLEND);

    int idx = mgr->count;
    SpriteSheet *sheet = &mgr->sheets[idx];
    memset(sheet, 0, sizeof(*sheet));

    sheet->texture = tex;
    SDL_QueryTexture(tex, NULL, NULL, &sheet->sheet_width, &sheet->sheet_height);
    sheet->frame_width = frame_w;
    sheet->frame_height = frame_h;

    /* Default pivot at feet */
    sheet->pivot_x = frame_w / 2;
    sheet->pivot_y = frame_h - 8;

    /* Extract filename for name */
    const char *slash = strrchr(path, '/');
    const char *fname = slash ? slash + 1 : path;
    snprintf(sheet->name, sizeof(sheet->name), "%s", fname);

    int cols = sheet->sheet_width / frame_w;
    int rows = sheet->sheet_height / frame_h;

    /* Auto-subdivide: each row maps to state * 8 + direction */
    for (int row = 0; row < rows; row++) {
        int anim_state = row / DIR_COUNT_ANIM;
        int direction  = row % DIR_COUNT_ANIM;

        if (anim_state >= ANIM_STATE_COUNT) break;

        AnimSequence *seq = &sheet->sequences[anim_state][direction];
        seq->frame_count = MIN(cols, MAX_ANIM_FRAMES);
        /* Default: IDLE(0) and WALKING(1) loop, others don't */
        seq->looping = (anim_state <= 1);

        float default_duration = 0.15f;
        for (int col = 0; col < seq->frame_count; col++) {
            SpriteFrame *f = &seq->frames[col];
            f->src_rect.x = col * frame_w;
            f->src_rect.y = row * frame_h;
            f->src_rect.w = frame_w;
            f->src_rect.h = frame_h;
            f->duration = default_duration;
            f->offset_x = 0;
            f->offset_y = 0;
        }
        seq->total_duration = seq->frame_count * default_duration;
    }

    sheet->loaded = true;
    mgr->count++;
    return idx;
}

const SpriteSheet *spritesheet_get(const SpriteSheetManager *mgr, int index)
{
    if (index < 0 || index >= mgr->count) return NULL;
    if (!mgr->sheets[index].loaded) return NULL;
    return &mgr->sheets[index];
}

void spritesheet_set_anim(SpriteSheet *sheet, int anim_state,
                          int frame_count, float frame_duration,
                          bool looping, int event_frame)
{
    if (!sheet || anim_state < 0 || anim_state >= ANIM_STATE_COUNT) return;

    int count = MIN(frame_count, MAX_ANIM_FRAMES);

    for (int dir = 0; dir < DIR_COUNT_ANIM; dir++) {
        AnimSequence *seq = &sheet->sequences[anim_state][dir];
        seq->frame_count = count;
        seq->looping = looping;
        seq->total_duration = count * frame_duration;

        for (int i = 0; i < count; i++) {
            seq->frames[i].duration = frame_duration;
        }
    }

    /* Store event frame in all directions via controller init */
    (void)event_frame; /* event_frame is per-controller, stored below */

    /* We need a way to propagate event_frame — store it as a convention:
     * negative offset_y on the event frame as a marker won't work cleanly.
     * Instead, we store it in frame[0].offset_x as metadata.
     * Actually, event_frame is best handled at the controller level.
     * For now, we store it in the sequence via a simple convention:
     * frames[0].offset_x encodes the event frame index. */
    for (int dir = 0; dir < DIR_COUNT_ANIM; dir++) {
        AnimSequence *seq = &sheet->sequences[anim_state][dir];
        /* Use offset_x of the first frame to store event_frame metadata */
        if (seq->frame_count > 0) {
            seq->frames[0].offset_x = event_frame;
        }
    }
}

void anim_controller_init(AnimController *ac, const SpriteSheet *sheet)
{
    ac->sheet = sheet;
    ac->current_state = 0;
    ac->current_dir = 0;
    ac->current_frame = 0;
    ac->frame_timer = 0.0f;
    ac->finished = false;
    ac->event_frame = -1;
    ac->event_fired = false;
}

void anim_controller_set_state(AnimController *ac, int anim_state, int direction)
{
    if (ac->current_state == anim_state && ac->current_dir == direction)
        return;

    ac->current_state = anim_state;
    ac->current_dir = direction;
    ac->current_frame = 0;
    ac->frame_timer = 0.0f;
    ac->finished = false;
    ac->event_fired = false;

    /* Pull event_frame from sheet metadata */
    if (ac->sheet) {
        const AnimSequence *seq = &ac->sheet->sequences[anim_state][direction];
        if (seq->frame_count > 0) {
            ac->event_frame = seq->frames[0].offset_x;
        } else {
            ac->event_frame = -1;
        }
    }
}

void anim_controller_update(AnimController *ac, float dt)
{
    if (!ac->sheet || ac->finished) return;

    const AnimSequence *seq =
        &ac->sheet->sequences[ac->current_state][ac->current_dir];

    if (seq->frame_count <= 0) return;

    ac->frame_timer += dt;

    float frame_dur = seq->frames[ac->current_frame].duration;
    while (ac->frame_timer >= frame_dur && !ac->finished) {
        ac->frame_timer -= frame_dur;
        ac->current_frame++;

        if (ac->current_frame >= seq->frame_count) {
            if (seq->looping) {
                ac->current_frame = 0;
            } else {
                ac->current_frame = seq->frame_count - 1;
                ac->finished = true;
            }
        }

        /* Update frame duration for next iteration */
        frame_dur = seq->frames[ac->current_frame].duration;
    }
}

bool anim_controller_is_finished(const AnimController *ac)
{
    return ac->finished;
}

bool anim_controller_check_event(AnimController *ac)
{
    if (ac->event_frame >= 0 &&
        ac->current_frame == ac->event_frame &&
        !ac->event_fired) {
        ac->event_fired = true;
        return true;
    }
    return false;
}

SDL_Rect anim_controller_get_src_rect(const AnimController *ac)
{
    SDL_Rect empty = {0, 0, 0, 0};
    if (!ac->sheet) return empty;

    const AnimSequence *seq =
        &ac->sheet->sequences[ac->current_state][ac->current_dir];

    if (seq->frame_count <= 0) return empty;

    return seq->frames[ac->current_frame].src_rect;
}
