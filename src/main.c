#include "common.h"
#include "engine/engine.h"
#include "engine/renderer.h"
#include "engine/input.h"
#include "engine/ui.h"
#include "world/camera.h"
#include "world/map.h"
#include "world/town.h"
#include "world/pathfinding.h"
#include "game/game.h"

/* Camera scroll speed in pixels per second */
#define CAMERA_SPEED 200

/* Player movement speed: tiles per second */
#define PLAYER_MOVE_SPEED 6.0f

/* Tile texture indices */
static SDL_Texture *tile_textures[TILE_TYPE_COUNT];

/* Player state */
static int    player_tile_x, player_tile_y;
static float  player_world_x, player_world_y;   /* sub-tile interpolated pixel pos */
static bool   player_moving;
static float  move_timer;
static int    move_from_x, move_from_y;
static int    move_to_x, move_to_y;
static Path   player_path;

/* Hovered tile under mouse */
static int hover_tile_x = -1;
static int hover_tile_y = -1;

/* Load a texture, returning NULL (not crashing) on failure */
static SDL_Texture *load_tex(ResourceManager *rm, const char *path)
{
    int idx = resource_load_texture(rm, path);
    return resource_get_texture(rm, idx);
}

/* Start moving toward the next waypoint on the path */
static bool begin_next_step(void)
{
    int nx, ny;
    if (!path_next(&player_path, &nx, &ny)) {
        player_moving = false;
        return false;
    }
    move_from_x = player_tile_x;
    move_from_y = player_tile_y;
    move_to_x = nx;
    move_to_y = ny;
    move_timer = 0.0f;
    player_moving = true;
    return true;
}

/* Check if a tile is visible in the current viewport */
static bool tile_visible(int tx, int ty, int cam_x, int cam_y)
{
    int sx, sy;
    iso_to_screen(tx, ty, &sx, &sy);
    sx -= cam_x;
    sy -= cam_y;
    sx += SCREEN_WIDTH / 2;
    /* Conservative check with generous margin */
    return sx > -TILE_WIDTH * 2 && sx < SCREEN_WIDTH + TILE_WIDTH * 2 &&
           sy > -TILE_HEIGHT * 4 && sy < VIEWPORT_HEIGHT + TILE_HEIGHT * 4;
}

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

    /* Initialize Tristram town */
    Town town;
    town_init(&town);

    /* Load tile textures */
    SDL_Texture *tex_grass  = load_tex(&engine.resources, "assets/tiles/town/grass.jpg");
    SDL_Texture *tex_dirt   = load_tex(&engine.resources, "assets/tiles/town/dirt_path.jpg");
    SDL_Texture *tex_stone  = load_tex(&engine.resources, "assets/tiles/floor_stone.jpg");
    SDL_Texture *tex_wall   = load_tex(&engine.resources, "assets/tiles/wall_stone.jpg");
    SDL_Texture *tex_water  = load_tex(&engine.resources, "assets/tiles/town/water.jpg");
    SDL_Texture *tex_tree   = load_tex(&engine.resources, "assets/tiles/town/tree.jpg");

    tile_textures[TILE_NONE]        = tex_stone;   /* fallback */
    tile_textures[TILE_GRASS]       = tex_grass;
    tile_textures[TILE_DIRT]        = tex_dirt;
    tile_textures[TILE_STONE_FLOOR] = tex_stone;
    tile_textures[TILE_WALL]        = tex_wall;
    tile_textures[TILE_WATER]       = tex_water;
    tile_textures[TILE_DOOR]        = tex_stone;    /* door drawn as stone floor */
    tile_textures[TILE_STAIRS_DOWN] = tex_stone;    /* stairs drawn as stone floor */
    tile_textures[TILE_TREE]        = tex_grass;    /* grass base under trees */

    /* Player sprite */
    SDL_Texture *tex_warrior = load_tex(&engine.resources, "assets/sprites/player/warrior_test.jpg");

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
    if (!hud_font)
        fprintf(stderr, "Warning: No font loaded, HUD text disabled\n");

    /* Initialize subsystems */
    game_init(&game);
    camera_init(&camera, CAMERA_SPEED);
    iso_renderer_init(&iso_renderer, engine.renderer, SCREEN_WIDTH);
    input_init(&input);
    ui_init(&ui, engine.renderer, hud_font);

    /* Player starts at the town square */
    player_tile_x = town.player_start_x;
    player_tile_y = town.player_start_y;
    player_moving = false;
    path_clear(&player_path);

    /* Compute initial world position from tile coords */
    {
        int sx, sy;
        iso_to_screen(player_tile_x, player_tile_y, &sx, &sy);
        player_world_x = (float)sx;
        player_world_y = (float)sy;
    }

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
                    /* Click-to-move: pathfind to hovered tile */
                    if (hover_tile_x >= 0 && hover_tile_y >= 0 &&
                        tilemap_in_bounds(&town.map, hover_tile_x, hover_tile_y) &&
                        tilemap_is_walkable(&town.map, hover_tile_x, hover_tile_y)) {
                        if (pathfind(&town.map, player_tile_x, player_tile_y,
                                     hover_tile_x, hover_tile_y, &player_path)) {
                            /* Skip first point (current position) if present */
                            if (player_path.length > 0 &&
                                player_path.points[0].x == player_tile_x &&
                                player_path.points[0].y == player_tile_y) {
                                player_path.current_step = 1;
                            }
                            player_moving = false;
                            move_timer = 0.0f;
                            begin_next_step();
                        }
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

        /* Player movement interpolation */
        if (player_moving) {
            move_timer += dt * PLAYER_MOVE_SPEED;
            if (move_timer >= 1.0f) {
                /* Arrived at waypoint */
                player_tile_x = move_to_x;
                player_tile_y = move_to_y;
                int sx, sy;
                iso_to_screen(player_tile_x, player_tile_y, &sx, &sy);
                player_world_x = (float)sx;
                player_world_y = (float)sy;

                /* Try next step */
                if (!begin_next_step()) {
                    player_moving = false;
                }
            } else {
                /* Interpolate between tiles */
                int from_sx, from_sy, to_sx, to_sy;
                iso_to_screen(move_from_x, move_from_y, &from_sx, &from_sy);
                iso_to_screen(move_to_x, move_to_y, &to_sx, &to_sy);
                float t = move_timer;
                player_world_x = from_sx + (to_sx - from_sx) * t;
                player_world_y = from_sy + (to_sy - from_sy) * t;
            }
        }

        /* Camera follows player (smooth) */
        if (!input_key_held(&input, SDL_SCANCODE_LEFT) &&
            !input_key_held(&input, SDL_SCANCODE_RIGHT) &&
            !input_key_held(&input, SDL_SCANCODE_UP) &&
            !input_key_held(&input, SDL_SCANCODE_DOWN) &&
            !input_key_held(&input, SDL_SCANCODE_A) &&
            !input_key_held(&input, SDL_SCANCODE_D) &&
            !input_key_held(&input, SDL_SCANCODE_W) &&
            !input_key_held(&input, SDL_SCANCODE_S)) {
            camera_center_on_tile(&camera, player_tile_x, player_tile_y);
        }

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

        /* Draw tiles back-to-front (painter's algorithm) with frustum culling */
        for (int ty = 0; ty < town.map.height; ty++) {
            for (int tx = 0; tx < town.map.width; tx++) {
                if (!tile_visible(tx, ty, cam_x, cam_y))
                    continue;

                TileType type = tilemap_get_type(&town.map, tx, ty);
                SDL_Texture *floor_tex = tile_textures[type];

                /* Always draw the floor layer */
                if (floor_tex)
                    iso_draw_tile(&iso_renderer, floor_tex, tx, ty, cam_x, cam_y);

                /* Elevated tiles: walls and trees get a wall block on top */
                if (type == TILE_WALL) {
                    if (tex_wall)
                        iso_draw_wall(&iso_renderer, tex_wall, tx, ty, cam_x, cam_y, TILE_HEIGHT);
                } else if (type == TILE_TREE) {
                    /* Draw tree sprite on top of grass base */
                    SDL_Texture *tree_overlay = tex_tree ? tex_tree : tex_wall;
                    if (tree_overlay)
                        iso_draw_wall(&iso_renderer, tree_overlay, tx, ty, cam_x, cam_y, TILE_HEIGHT);
                }
            }
        }

        /* Path visualization: highlight planned path tiles */
        if (!path_is_complete(&player_path)) {
            SDL_Color path_color = { 100, 180, 255, 50 };
            for (int i = player_path.current_step; i < player_path.length; i++) {
                int px = player_path.points[i].x;
                int py = player_path.points[i].y;
                if (tile_visible(px, py, cam_x, cam_y))
                    iso_draw_tile_highlight(&iso_renderer, px, py, cam_x, cam_y, path_color);
            }
        }

        /* Tile highlight under mouse cursor */
        if (hover_tile_x >= 0 && hover_tile_y >= 0 &&
            tilemap_in_bounds(&town.map, hover_tile_x, hover_tile_y)) {
            SDL_Color highlight;
            if (tilemap_is_walkable(&town.map, hover_tile_x, hover_tile_y))
                highlight = (SDL_Color){ 255, 200, 50, 80 };
            else
                highlight = (SDL_Color){ 255, 60, 60, 80 };
            iso_draw_tile_highlight(&iso_renderer, hover_tile_x, hover_tile_y,
                                    cam_x, cam_y, highlight);
        }

        /* Player sprite — use interpolated world position */
        {
            int draw_sx = (int)player_world_x - cam_x + (SCREEN_WIDTH / 2) - SPRITE_SIZE / 2;
            int draw_sy = (int)player_world_y - cam_y - SPRITE_SIZE + TILE_HALF_H;
            if (tex_warrior) {
                SDL_Rect dst = { draw_sx, draw_sy, SPRITE_SIZE, SPRITE_SIZE };
                SDL_RenderCopy(engine.renderer, tex_warrior, NULL, &dst);
            }
        }

        SDL_RenderSetClipRect(engine.renderer, NULL);

        /* UI overlay */
        ui_draw_panel(&ui, VIEWPORT_HEIGHT, PANEL_HEIGHT, SCREEN_WIDTH);
        ui_draw_fps(&ui, engine.fps);

        /* HUD info: tile coords, player pos, game time */
        char info[256];
        snprintf(info, sizeof(info),
                 "Tile: (%d, %d)  Player: (%d, %d)  Day %d, %02d:00",
                 hover_tile_x, hover_tile_y,
                 player_tile_x, player_tile_y,
                 game.game_day, game.game_hour);
        ui_draw_panel_info(&ui, VIEWPORT_HEIGHT, info);

        engine_end_frame(&engine);
    }

    engine_shutdown(&engine);
    return 0;
}
