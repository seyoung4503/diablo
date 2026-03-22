#ifndef DIABLO_ENGINE_H
#define DIABLO_ENGINE_H

#include "common.h"
#include "engine/timer.h"
#include "engine/resource.h"

typedef struct Engine {
    SDL_Window *window;
    SDL_Renderer *renderer;
    bool running;
    float delta_time;
    int fps;
    Timer timer;
    ResourceManager resources;
} Engine;

/*
 * Initialize SDL subsystems, create window and renderer, and set up
 * the timer and resource manager.  Returns true on success.
 */
bool engine_init(Engine *engine, const char *title, int width, int height);

/* Tear down all engine resources and quit SDL. */
void engine_shutdown(Engine *engine);

/* Call at the top of each frame: clears the screen and updates timing. */
void engine_begin_frame(Engine *engine);

/* Call at the end of each frame: presents the renderer and computes FPS. */
void engine_end_frame(Engine *engine);

#endif /* DIABLO_ENGINE_H */
