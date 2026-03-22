#include "engine/engine.h"

bool engine_init(Engine *engine, const char *title, int width, int height)
{
    memset(engine, 0, sizeof(*engine));

    /* Initialize SDL subsystems */
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER) != 0) {
        fprintf(stderr, "SDL_Init failed: %s\n", SDL_GetError());
        return false;
    }

    int img_flags = IMG_INIT_JPG | IMG_INIT_PNG;
    if ((IMG_Init(img_flags) & img_flags) != img_flags) {
        fprintf(stderr, "IMG_Init failed: %s\n", IMG_GetError());
        SDL_Quit();
        return false;
    }

    if (TTF_Init() != 0) {
        fprintf(stderr, "TTF_Init failed: %s\n", TTF_GetError());
        IMG_Quit();
        SDL_Quit();
        return false;
    }

    engine->window = SDL_CreateWindow(
        title,
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        width, height,
        SDL_WINDOW_SHOWN
    );
    if (!engine->window) {
        fprintf(stderr, "SDL_CreateWindow failed: %s\n", SDL_GetError());
        TTF_Quit();
        IMG_Quit();
        SDL_Quit();
        return false;
    }

    engine->renderer = SDL_CreateRenderer(
        engine->window, -1,
        SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC
    );
    if (!engine->renderer) {
        fprintf(stderr, "SDL_CreateRenderer failed: %s\n", SDL_GetError());
        SDL_DestroyWindow(engine->window);
        TTF_Quit();
        IMG_Quit();
        SDL_Quit();
        return false;
    }

    SDL_SetRenderDrawBlendMode(engine->renderer, SDL_BLENDMODE_BLEND);

    /* Set up timer and resource manager */
    timer_init(&engine->timer);
    resource_init(&engine->resources, engine->renderer);

    engine->running = true;
    engine->delta_time = 0.0f;
    engine->fps = 0;

    return true;
}

void engine_shutdown(Engine *engine)
{
    resource_shutdown(&engine->resources);

    if (engine->renderer)
        SDL_DestroyRenderer(engine->renderer);
    if (engine->window)
        SDL_DestroyWindow(engine->window);

    TTF_Quit();
    IMG_Quit();
    SDL_Quit();
}

void engine_begin_frame(Engine *engine)
{
    timer_update(&engine->timer);
    engine->delta_time = engine->timer.delta_time;
    engine->fps = engine->timer.fps;

    SDL_SetRenderDrawColor(engine->renderer, 0, 0, 0, 255);
    SDL_RenderClear(engine->renderer);
}

void engine_end_frame(Engine *engine)
{
    SDL_RenderPresent(engine->renderer);
}
