#ifndef DIABLO_TIMER_H
#define DIABLO_TIMER_H

#include "common.h"

typedef struct Timer {
    Uint64 perf_freq;
    Uint64 last_time;
    float delta_time;
    int frame_count;
    int fps;
    Uint32 fps_timer;
} Timer;

/* Initialize timer state. Call once before the game loop. */
void timer_init(Timer *timer);

/* Update delta_time and fps. Call once per frame at the top of the loop. */
void timer_update(Timer *timer);

#endif /* DIABLO_TIMER_H */
