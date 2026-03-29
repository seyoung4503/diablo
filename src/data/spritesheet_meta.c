#include "engine/animation.h"

/* Default frame counts and durations for character animations */
void spritesheet_apply_defaults(SpriteSheet *sheet)
{
    if (!sheet || !sheet->loaded) return;

    /* IDLE: 4 frames, 0.25s each, loops */
    spritesheet_set_anim(sheet, 0, 4, 0.25f, true, -1);

    /* WALKING: 6 frames, 0.12s each, loops */
    spritesheet_set_anim(sheet, 1, 6, 0.12f, true, -1);

    /* ATTACKING: 6 frames, 0.08s each, no loop, event at frame 3 (hit) */
    spritesheet_set_anim(sheet, 2, 6, 0.08f, false, 3);

    /* HIT: 2 frames, 0.1s each, no loop */
    spritesheet_set_anim(sheet, 3, 2, 0.10f, false, -1);

    /* DEATH: 4 frames, 0.2s each, no loop */
    spritesheet_set_anim(sheet, 4, 4, 0.20f, false, -1);
}
