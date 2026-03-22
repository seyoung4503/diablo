#include "engine/timer.h"

void timer_init(Timer *timer)
{
    timer->perf_freq = SDL_GetPerformanceFrequency();
    timer->last_time = SDL_GetPerformanceCounter();
    timer->delta_time = 0.0f;
    timer->frame_count = 0;
    timer->fps = 0;
    timer->fps_timer = SDL_GetTicks();
}

void timer_update(Timer *timer)
{
    Uint64 now = SDL_GetPerformanceCounter();
    timer->delta_time = (float)(now - timer->last_time) / (float)timer->perf_freq;
    timer->last_time = now;

    /* Cap delta time to avoid spiral of death */
    if (timer->delta_time > 0.05f)
        timer->delta_time = 0.05f;

    /* FPS counter — update once per second */
    timer->frame_count++;
    Uint32 tick_now = SDL_GetTicks();
    if (tick_now - timer->fps_timer >= 1000) {
        timer->fps = timer->frame_count;
        timer->frame_count = 0;
        timer->fps_timer = tick_now;
    }
}
