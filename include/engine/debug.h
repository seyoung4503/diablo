#ifndef DIABLO_ENGINE_DEBUG_H
#define DIABLO_ENGINE_DEBUG_H

#include "common.h"
#include "engine/animation.h"
#include "engine/resource.h"
#include "engine/ui.h"

/* Forward declarations */
struct Player;
struct Game;
struct EnemyManager;

#define DEBUG_OUTPUT_DIR "debug"

/* ---- Screenshot Capture ---- */

/* Capture current renderer contents to debug/screenshot_YYYYMMDD_HHMMSS.png.
 * Must be called BEFORE SDL_RenderPresent(). Returns true on success. */
bool debug_screenshot(SDL_Renderer *renderer, int width, int height);

/* Capture to a specific file path. */
bool debug_screenshot_to(SDL_Renderer *renderer, int width, int height,
                         const char *path);

/* ---- Game State Dump ---- */

/* Write game state to debug/state_YYYYMMDD_HHMMSS.txt. */
bool debug_state_dump(const struct Player *player,
                      const struct Game *game,
                      const struct EnemyManager *enemies,
                      const SpriteSheetManager *sprites,
                      const ResourceManager *resources,
                      int scene_type, int dungeon_level);

/* ---- Debug Overlay ---- */

/* Draw entity/animation debug info on the left side of the screen.
 * Complements the existing NPC debug overlay (right side, town only). */
void debug_draw_overlay(UI *ui, SDL_Renderer *renderer,
                        const struct Player *player,
                        const struct EnemyManager *enemies,
                        int fps, int scene_type);

/* ---- Animation Viewer ---- */

typedef struct DebugAnimViewer {
    bool active;
    int selected_sheet;       /* index into SpriteSheetManager */
    int selected_state;       /* 0-4 (AnimState) */
    int selected_dir;         /* 0-7 (Direction) */
    bool playing;             /* auto-advance frames */
    AnimController preview_anim;
} DebugAnimViewer;

void debug_anim_viewer_init(DebugAnimViewer *viewer);

/* Handle input. Returns true if input was consumed. */
bool debug_anim_viewer_handle_key(DebugAnimViewer *viewer,
                                  SDL_Keycode key,
                                  const SpriteSheetManager *sprites);

/* Update animation playback. */
void debug_anim_viewer_update(DebugAnimViewer *viewer, float dt);

/* Render the full-screen animation viewer. */
void debug_anim_viewer_render(const DebugAnimViewer *viewer,
                              SDL_Renderer *renderer, UI *ui,
                              const SpriteSheetManager *sprites);

/* Capture current viewer to PNG. */
bool debug_anim_viewer_capture(const DebugAnimViewer *viewer,
                               SDL_Renderer *renderer, UI *ui,
                               const SpriteSheetManager *sprites);

#endif /* DIABLO_ENGINE_DEBUG_H */
