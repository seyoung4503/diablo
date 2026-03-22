#ifndef DIABLO_UI_H
#define DIABLO_UI_H

#include "common.h"

typedef struct UI {
    TTF_Font *font;
    SDL_Renderer *renderer;
} UI;

/* Initialize UI with renderer and font. Font may be NULL (text drawing becomes no-op). */
void ui_init(UI *ui, SDL_Renderer *renderer, TTF_Font *font);

/* Render text at (x, y). Creates and destroys texture each call. */
void ui_draw_text(UI *ui, const char *text, int x, int y, SDL_Color color);

/* Draw the bottom UI panel background and border. */
void ui_draw_panel(UI *ui, int viewport_height, int panel_height, int screen_width);

/* Draw FPS counter in the top-left corner. */
void ui_draw_fps(UI *ui, int fps);

/* Draw info text inside the bottom panel. */
void ui_draw_panel_info(UI *ui, int viewport_height, const char *text);

#endif /* DIABLO_UI_H */
