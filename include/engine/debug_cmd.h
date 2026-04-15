#ifndef DIABLO_ENGINE_DEBUG_CMD_H
#define DIABLO_ENGINE_DEBUG_CMD_H

#include "common.h"
#include "engine/ui.h"

/* Forward declarations */
struct Player;
struct EnemyManager;
struct NPCManager;
struct Game;
struct Inventory;
struct TileMap;
struct SpriteSheetManager;
struct ResourceManager;
struct Camera;

#define CMD_PIPE_PATH     "debug/game.pipe"
#define CMD_RESPONSE_PATH "debug/response.txt"
#define CMD_SCREENSHOT_PATH "debug/cmd_screenshot.png"

/* ---- Adapter ---- */

typedef struct DebugCmdAdapter {
    int pipe_fd;
    bool active;
    char line_buf[2048];
    int line_len;
    int wait_frames;          /* countdown, 0 = ready */
    bool response_pending;    /* write response after next render */
    char response_text[8192]; /* pre-formatted response */
} DebugCmdAdapter;

/* ---- Game context (passed each frame) ---- */

typedef struct DebugCmdContext {
    struct Player *player;
    struct Camera *camera;
    const struct EnemyManager *enemies;
    const struct Game *game;
    const struct NPCManager *npcs;
    const struct Inventory *inventory;
    const struct TileMap *active_map;
    const struct SpriteSheetManager *sprites;
    const struct ResourceManager *resources;
    SDL_Renderer *renderer;
    UI *ui;
    int scene_type;
    int dungeon_level;
    int fps;
} DebugCmdContext;

/* ---- Output flags for main.c to handle complex actions ---- */

typedef struct DebugCmdOutput {
    bool attack_requested;
    int attack_x, attack_y;
    bool interact_requested;
    int interact_x, interact_y;
    bool enter_stairs;
    bool exit_dungeon;
    bool use_item_requested;
    int use_item_slot;
    bool select_spell;
    int spell_id;
    bool screenshot_requested;
    bool dump_requested;
    bool quit_requested;
} DebugCmdOutput;

/* Initialize the command adapter. No-op if enabled=false. */
void debug_cmd_init(DebugCmdAdapter *adapter, bool enabled);

/* Poll for commands from the pipe. Called early in the game loop.
 * Parses commands, executes simple ones, sets output flags for complex ones. */
void debug_cmd_poll(DebugCmdAdapter *adapter, DebugCmdContext *ctx,
                    DebugCmdOutput *output);

/* Write pending response to file + auto-screenshot.
 * Call after rendering, before SDL_RenderPresent. */
void debug_cmd_flush_response(DebugCmdAdapter *adapter, DebugCmdContext *ctx);

/* Shutdown and cleanup FIFO. */
void debug_cmd_shutdown(DebugCmdAdapter *adapter);

#endif /* DIABLO_ENGINE_DEBUG_CMD_H */
