#include "engine/ui.h"

void ui_init(UI *ui, SDL_Renderer *renderer, TTF_Font *font)
{
    ui->renderer = renderer;
    ui->font = font;
}

void ui_draw_text(UI *ui, const char *text, int x, int y, SDL_Color color)
{
    if (!ui->font) return;

    SDL_Surface *surf = TTF_RenderText_Blended(ui->font, text, color);
    if (!surf) return;

    SDL_Texture *tex = SDL_CreateTextureFromSurface(ui->renderer, surf);
    if (tex) {
        SDL_Rect dst = { x, y, surf->w, surf->h };
        SDL_RenderCopy(ui->renderer, tex, NULL, &dst);
        SDL_DestroyTexture(tex);
    }
    SDL_FreeSurface(surf);
}

void ui_draw_panel(UI *ui, int viewport_height, int panel_height, int screen_width)
{
    SDL_Rect panel = { 0, viewport_height, screen_width, panel_height };
    SDL_SetRenderDrawColor(ui->renderer, 30, 25, 35, 255);
    SDL_RenderFillRect(ui->renderer, &panel);

    /* Panel top border */
    SDL_SetRenderDrawColor(ui->renderer, 80, 70, 60, 255);
    SDL_RenderDrawLine(ui->renderer, 0, viewport_height, screen_width, viewport_height);
}

void ui_draw_fps(UI *ui, int fps)
{
    SDL_Color green = { 100, 255, 100, 255 };
    char buf[32];
    snprintf(buf, sizeof(buf), "FPS: %d", fps);
    ui_draw_text(ui, buf, 4, 4, green);
}

void ui_draw_panel_info(UI *ui, int viewport_height, const char *text)
{
    SDL_Color gold = { 200, 180, 120, 255 };
    ui_draw_text(ui, text, 16, viewport_height + 16, gold);
}
