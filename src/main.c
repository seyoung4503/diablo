#include "common.h"
#include "engine/engine.h"
#include "engine/renderer.h"
#include "engine/input.h"
#include "engine/ui.h"
#include "world/camera.h"
#include "game/game.h"

/* Demo map size */
#define DEMO_MAP_W 12
#define DEMO_MAP_H 12

/* Camera scroll speed in pixels per second */
#define CAMERA_SPEED 200

/* Tile types for the demo map */
enum {
    TILE_FLOOR = 0,
    TILE_WALL  = 1,
    TILE_GRASS = 2,
};

/* Simple demo map layout */
static int demo_map[DEMO_MAP_H][DEMO_MAP_W] = {
    { 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2 },
    { 2, 2, 2, 2, 0, 0, 0, 0, 2, 2, 2, 2 },
    { 2, 2, 0, 0, 0, 0, 0, 0, 0, 0, 2, 2 },
    { 2, 2, 0, 0, 0, 0, 0, 0, 0, 0, 2, 2 },
    { 2, 0, 0, 0, 1, 0, 0, 1, 0, 0, 0, 2 },
    { 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2 },
    { 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2 },
    { 2, 0, 0, 0, 1, 0, 0, 1, 0, 0, 0, 2 },
    { 2, 2, 0, 0, 0, 0, 0, 0, 0, 0, 2, 2 },
    { 2, 2, 0, 0, 0, 0, 0, 0, 0, 0, 2, 2 },
    { 2, 2, 2, 2, 0, 0, 0, 0, 2, 2, 2, 2 },
    { 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2 },
};

/* Player position in tile coords */
static int player_tile_x = 5;
static int player_tile_y = 5;

/* Hovered tile under mouse */
static int hover_tile_x = -1;
static int hover_tile_y = -1;

int main(int argc, char *argv[])
{
    (void)argc;
    (void)argv;

    /* Core systems */
    Engine engine;
    Game game;
    Camera camera;
    IsoRenderer iso_renderer;
    InputState input;
    UI ui;

    /* Initialize engine (SDL, window, renderer, timer, resource manager) */
    if (!engine_init(&engine, "Descent into Darkness", SCREEN_WIDTH, SCREEN_HEIGHT))
        return 1;

    /* Load textures via resource manager */
    int idx_floor   = resource_load_texture(&engine.resources, "assets/tiles/floor_stone.jpg");
    int idx_wall    = resource_load_texture(&engine.resources, "assets/tiles/wall_stone.jpg");
    int idx_grass   = resource_load_texture(&engine.resources, "assets/tiles/town/grass.jpg");
    int idx_warrior = resource_load_texture(&engine.resources, "assets/sprites/player/warrior_test.jpg");

    SDL_Texture *tex_floor   = resource_get_texture(&engine.resources, idx_floor);
    SDL_Texture *tex_wall    = resource_get_texture(&engine.resources, idx_wall);
    SDL_Texture *tex_grass   = resource_get_texture(&engine.resources, idx_grass);
    SDL_Texture *tex_warrior = resource_get_texture(&engine.resources, idx_warrior);

    /* Load HUD font — try several system font paths */
    const char *font_paths[] = {
        "/System/Library/Fonts/Monaco.ttf",
        "/System/Library/Fonts/Geneva.ttf",
        "/Library/Fonts/Arial Unicode.ttf",
        "/System/Library/Fonts/Helvetica.ttc",
        NULL,
    };
    int font_idx = -1;
    for (int i = 0; font_paths[i] != NULL; i++) {
        font_idx = resource_load_font(&engine.resources, font_paths[i], 14);
        if (font_idx >= 0) break;
    }
    TTF_Font *hud_font = resource_get_font(&engine.resources, font_idx);
    if (!hud_font) {
        fprintf(stderr, "Warning: No font loaded, HUD text disabled\n");
    }

    /* Initialize subsystems */
    game_init(&game);
    camera_init(&camera, CAMERA_SPEED);
    iso_renderer_init(&iso_renderer, engine.renderer, SCREEN_WIDTH);
    input_init(&input);
    ui_init(&ui, engine.renderer, hud_font);

    /* Center camera on the player and snap immediately */
    camera_center_on_tile(&camera, player_tile_x, player_tile_y);
    camera.x = camera.target_x;
    camera.y = camera.target_y;

    /* ---- Game loop ---- */
    while (engine.running) {
        engine_begin_frame(&engine);
        float dt = engine.delta_time;

        /* Snapshot previous input state before polling events */
        input_update(&input);

        /* Event handling */
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            switch (event.type) {
            case SDL_QUIT:
                engine.running = false;
                break;
            case SDL_KEYDOWN:
                if (event.key.keysym.sym == SDLK_ESCAPE)
                    engine.running = false;
                break;
            case SDL_MOUSEBUTTONDOWN:
                if (event.button.button == SDL_BUTTON_LEFT) {
                    if (hover_tile_x >= 0 && hover_tile_x < DEMO_MAP_W &&
                        hover_tile_y >= 0 && hover_tile_y < DEMO_MAP_H &&
                        demo_map[hover_tile_y][hover_tile_x] != TILE_WALL) {
                        player_tile_x = hover_tile_x;
                        player_tile_y = hover_tile_y;
                    }
                }
                break;
            }
        }

        /* Camera scroll from keyboard input */
        float scroll_speed = camera.speed * dt;
        if (input_key_held(&input, SDL_SCANCODE_LEFT) || input_key_held(&input, SDL_SCANCODE_A))
            camera_scroll(&camera, -scroll_speed, 0);
        if (input_key_held(&input, SDL_SCANCODE_RIGHT) || input_key_held(&input, SDL_SCANCODE_D))
            camera_scroll(&camera, scroll_speed, 0);
        if (input_key_held(&input, SDL_SCANCODE_UP) || input_key_held(&input, SDL_SCANCODE_W))
            camera_scroll(&camera, 0, -scroll_speed);
        if (input_key_held(&input, SDL_SCANCODE_DOWN) || input_key_held(&input, SDL_SCANCODE_S))
            camera_scroll(&camera, 0, scroll_speed);

        camera_update(&camera, dt);
        game_update(&game, dt);

        /* Mouse tile picking */
        if (input.mouse_y < VIEWPORT_HEIGHT) {
            int world_x, world_y;
            camera_screen_to_world(&camera, input.mouse_x, input.mouse_y,
                                   SCREEN_WIDTH / 2, &world_x, &world_y);
            screen_to_iso(world_x, world_y, &hover_tile_x, &hover_tile_y);
        } else {
            hover_tile_x = -1;
            hover_tile_y = -1;
        }

        /* ---- Render ---- */
        int cam_x = (int)camera.x;
        int cam_y = (int)camera.y;

        /* Set clip rect to viewport (above the UI panel) */
        SDL_Rect viewport_clip = { 0, 0, SCREEN_WIDTH, VIEWPORT_HEIGHT };
        SDL_RenderSetClipRect(engine.renderer, &viewport_clip);

        /* Dark viewport background */
        SDL_SetRenderDrawColor(engine.renderer, 15, 12, 20, 255);
        SDL_RenderFillRect(engine.renderer, &viewport_clip);

        /* Draw tiles back-to-front (painter's algorithm) */
        for (int ty = 0; ty < DEMO_MAP_H; ty++) {
            for (int tx = 0; tx < DEMO_MAP_W; tx++) {
                switch (demo_map[ty][tx]) {
                case TILE_FLOOR:
                    iso_draw_tile(&iso_renderer, tex_floor, tx, ty, cam_x, cam_y);
                    break;
                case TILE_WALL:
                    iso_draw_tile(&iso_renderer, tex_floor, tx, ty, cam_x, cam_y);
                    iso_draw_wall(&iso_renderer, tex_wall, tx, ty, cam_x, cam_y, TILE_HEIGHT);
                    break;
                case TILE_GRASS:
                    iso_draw_tile(&iso_renderer, tex_grass, tx, ty, cam_x, cam_y);
                    break;
                }
            }
        }

        /* Tile highlight under mouse cursor */
        if (hover_tile_x >= 0 && hover_tile_x < DEMO_MAP_W &&
            hover_tile_y >= 0 && hover_tile_y < DEMO_MAP_H) {
            SDL_Color highlight = { 255, 200, 50, 80 };
            iso_draw_tile_highlight(&iso_renderer, hover_tile_x, hover_tile_y,
                                    cam_x, cam_y, highlight);
        }

        /* Player sprite */
        iso_draw_sprite(&iso_renderer, tex_warrior, player_tile_x, player_tile_y,
                        cam_x, cam_y, SPRITE_SIZE, SPRITE_SIZE);

        SDL_RenderSetClipRect(engine.renderer, NULL);

        /* UI overlay */
        ui_draw_panel(&ui, VIEWPORT_HEIGHT, PANEL_HEIGHT, SCREEN_WIDTH);
        ui_draw_fps(&ui, engine.fps);

        char info[128];
        snprintf(info, sizeof(info), "Tile: (%d, %d)  Player: (%d, %d)",
                 hover_tile_x, hover_tile_y, player_tile_x, player_tile_y);
        ui_draw_panel_info(&ui, VIEWPORT_HEIGHT, info);

        engine_end_frame(&engine);
    }

    engine_shutdown(&engine);
    return 0;
}
