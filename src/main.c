#include "common.h"

/* Demo map size (subset of full MAP_WIDTH/MAP_HEIGHT) */
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

/* Camera offset in screen pixels */
static float camera_x = 0.0f;
static float camera_y = 0.0f;

/* Textures */
static SDL_Texture *tex_floor = NULL;
static SDL_Texture *tex_wall  = NULL;
static SDL_Texture *tex_grass = NULL;
static SDL_Texture *tex_warrior = NULL;

/* Font for HUD */
static TTF_Font *hud_font = NULL;

/* Hovered tile under mouse */
static int hover_tile_x = -1;
static int hover_tile_y = -1;

/* ------------------------------------------------------------------ */
/* Helpers                                                             */
/* ------------------------------------------------------------------ */

static SDL_Texture *load_texture(SDL_Renderer *renderer, const char *path)
{
    SDL_Texture *tex = IMG_LoadTexture(renderer, path);
    if (!tex) {
        fprintf(stderr, "Failed to load texture '%s': %s\n", path, IMG_GetError());
    }
    return tex;
}

/*
 * Render a text string and return as a texture. Caller must free.
 * Returns NULL if font is unavailable.
 */
static SDL_Texture *render_text(SDL_Renderer *renderer, const char *text,
                                SDL_Color color, int *out_w, int *out_h)
{
    if (!hud_font) return NULL;
    SDL_Surface *surf = TTF_RenderText_Blended(hud_font, text, color);
    if (!surf) return NULL;
    SDL_Texture *tex = SDL_CreateTextureFromSurface(renderer, surf);
    if (out_w) *out_w = surf->w;
    if (out_h) *out_h = surf->h;
    SDL_FreeSurface(surf);
    return tex;
}

/*
 * Draw a filled isometric diamond outline for tile highlighting.
 * Draws a semi-transparent colored diamond at the given tile position.
 */
static void draw_tile_highlight(SDL_Renderer *renderer, int tile_x, int tile_y,
                                int cam_x, int cam_y, SDL_Color color)
{
    int sx, sy;
    iso_to_screen(tile_x, tile_y, &sx, &sy);

    /* Apply camera and center horizontally in viewport */
    int cx = sx - cam_x + SCREEN_WIDTH / 2;
    int cy = sy - cam_y;

    /* Diamond vertices: top, right, bottom, left */
    SDL_Point pts[5] = {
        { cx,                cy },                    /* top */
        { cx + TILE_HALF_W,  cy + TILE_HALF_H },     /* right */
        { cx,                cy + TILE_HEIGHT },      /* bottom */
        { cx - TILE_HALF_W,  cy + TILE_HALF_H },     /* left */
        { cx,                cy },                    /* close */
    };

    /* Fill with semi-transparent overlay using scanlines */
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);

    for (int row = 0; row < TILE_HEIGHT; row++) {
        float t;
        int half_w;
        if (row <= TILE_HALF_H) {
            t = (float)row / TILE_HALF_H;
        } else {
            t = (float)(TILE_HEIGHT - row) / TILE_HALF_H;
        }
        half_w = (int)(t * TILE_HALF_W);
        SDL_RenderDrawLine(renderer, cx - half_w, cy + row, cx + half_w, cy + row);
    }

    /* Draw diamond border */
    SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, 200);
    SDL_RenderDrawLines(renderer, pts, 5);
}

/* ------------------------------------------------------------------ */
/* Rendering                                                           */
/* ------------------------------------------------------------------ */

/*
 * Draw a single isometric tile texture at the given tile coordinates.
 * The texture is scaled to TILE_WIDTH x TILE_HEIGHT and positioned so
 * the top of the diamond aligns with iso_to_screen output.
 */
static void draw_tile(SDL_Renderer *renderer, SDL_Texture *tex,
                      int tile_x, int tile_y, int cam_x, int cam_y)
{
    if (!tex) return;

    int sx, sy;
    iso_to_screen(tile_x, tile_y, &sx, &sy);

    SDL_Rect dst = {
        .x = sx - TILE_HALF_W - cam_x + SCREEN_WIDTH / 2,
        .y = sy - cam_y,
        .w = TILE_WIDTH,
        .h = TILE_HEIGHT,
    };

    SDL_RenderCopy(renderer, tex, NULL, &dst);
}

/*
 * Draw a wall block: floor-height tile plus a raised portion to give
 * the illusion of a 3D wall block sitting on the grid.
 */
static void draw_wall(SDL_Renderer *renderer, SDL_Texture *tex,
                      int tile_x, int tile_y, int cam_x, int cam_y)
{
    if (!tex) return;

    int sx, sy;
    iso_to_screen(tile_x, tile_y, &sx, &sy);

    int wall_extra_h = TILE_HEIGHT; /* extra height above the base */

    SDL_Rect dst = {
        .x = sx - TILE_HALF_W - cam_x + SCREEN_WIDTH / 2,
        .y = sy - wall_extra_h - cam_y,
        .w = TILE_WIDTH,
        .h = TILE_HEIGHT + wall_extra_h,
    };

    SDL_RenderCopy(renderer, tex, NULL, &dst);
}

/*
 * Draw the warrior sprite centered on a tile.
 */
static void draw_sprite(SDL_Renderer *renderer, SDL_Texture *tex,
                        int tile_x, int tile_y, int cam_x, int cam_y)
{
    if (!tex) return;

    int sx, sy;
    iso_to_screen(tile_x, tile_y, &sx, &sy);

    /* Center sprite on the tile's center point */
    SDL_Rect dst = {
        .x = sx - SPRITE_SIZE / 2 - cam_x + SCREEN_WIDTH / 2,
        .y = sy + TILE_HALF_H - SPRITE_SIZE + 8 - cam_y, /* feet at tile center */
        .w = SPRITE_SIZE,
        .h = SPRITE_SIZE,
    };

    SDL_RenderCopy(renderer, tex, NULL, &dst);
}

/*
 * Render the entire scene: tiles back-to-front, then entities.
 */
static void render_scene(SDL_Renderer *renderer)
{
    int cam_x = (int)camera_x;
    int cam_y = (int)camera_y;

    /* Set clip rect to viewport (above the UI panel) */
    SDL_Rect viewport_clip = { 0, 0, SCREEN_WIDTH, VIEWPORT_HEIGHT };
    SDL_RenderSetClipRect(renderer, &viewport_clip);

    /* Clear viewport with dark background */
    SDL_SetRenderDrawColor(renderer, 15, 12, 20, 255);
    SDL_RenderFillRect(renderer, &viewport_clip);

    /* Draw tiles back-to-front (painter's algorithm) */
    for (int ty = 0; ty < DEMO_MAP_H; ty++) {
        for (int tx = 0; tx < DEMO_MAP_W; tx++) {
            switch (demo_map[ty][tx]) {
            case TILE_FLOOR:
                draw_tile(renderer, tex_floor, tx, ty, cam_x, cam_y);
                break;
            case TILE_WALL:
                /* Draw floor underneath the wall */
                draw_tile(renderer, tex_floor, tx, ty, cam_x, cam_y);
                draw_wall(renderer, tex_wall, tx, ty, cam_x, cam_y);
                break;
            case TILE_GRASS:
                draw_tile(renderer, tex_grass, tx, ty, cam_x, cam_y);
                break;
            }
        }
    }

    /* Draw tile highlight under mouse cursor */
    if (hover_tile_x >= 0 && hover_tile_x < DEMO_MAP_W &&
        hover_tile_y >= 0 && hover_tile_y < DEMO_MAP_H) {
        SDL_Color highlight = { 255, 200, 50, 80 };
        draw_tile_highlight(renderer, hover_tile_x, hover_tile_y,
                            cam_x, cam_y, highlight);
    }

    /* Draw player warrior */
    draw_sprite(renderer, tex_warrior, player_tile_x, player_tile_y, cam_x, cam_y);

    SDL_RenderSetClipRect(renderer, NULL);
}

/*
 * Draw the bottom UI panel area.
 */
static void render_panel(SDL_Renderer *renderer)
{
    SDL_Rect panel = { 0, VIEWPORT_HEIGHT, SCREEN_WIDTH, PANEL_HEIGHT };
    SDL_SetRenderDrawColor(renderer, 30, 25, 35, 255);
    SDL_RenderFillRect(renderer, &panel);

    /* Panel border */
    SDL_SetRenderDrawColor(renderer, 80, 70, 60, 255);
    SDL_RenderDrawLine(renderer, 0, VIEWPORT_HEIGHT, SCREEN_WIDTH, VIEWPORT_HEIGHT);

    /* Panel text info */
    SDL_Color gold = { 200, 180, 120, 255 };
    char info[128];
    snprintf(info, sizeof(info), "Tile: (%d, %d)  Player: (%d, %d)",
             hover_tile_x, hover_tile_y, player_tile_x, player_tile_y);

    int tw, th;
    SDL_Texture *info_tex = render_text(renderer, info, gold, &tw, &th);
    if (info_tex) {
        SDL_Rect dst = { 16, VIEWPORT_HEIGHT + 16, tw, th };
        SDL_RenderCopy(renderer, info_tex, NULL, &dst);
        SDL_DestroyTexture(info_tex);
    }
}

/*
 * Draw FPS counter in top-left corner.
 */
static void render_fps(SDL_Renderer *renderer, int fps)
{
    SDL_Color green = { 100, 255, 100, 255 };
    char buf[32];
    snprintf(buf, sizeof(buf), "FPS: %d", fps);

    int tw, th;
    SDL_Texture *tex = render_text(renderer, buf, green, &tw, &th);
    if (tex) {
        SDL_Rect dst = { 4, 4, tw, th };
        SDL_RenderCopy(renderer, tex, NULL, &dst);
        SDL_DestroyTexture(tex);
    }
}

/* ------------------------------------------------------------------ */
/* Main                                                                */
/* ------------------------------------------------------------------ */

int main(int argc, char *argv[])
{
    (void)argc;
    (void)argv;

    /* Initialize SDL subsystems */
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER) != 0) {
        fprintf(stderr, "SDL_Init failed: %s\n", SDL_GetError());
        return 1;
    }

    int img_flags = IMG_INIT_JPG | IMG_INIT_PNG;
    if ((IMG_Init(img_flags) & img_flags) != img_flags) {
        fprintf(stderr, "IMG_Init failed: %s\n", IMG_GetError());
        SDL_Quit();
        return 1;
    }

    if (TTF_Init() != 0) {
        fprintf(stderr, "TTF_Init failed: %s\n", TTF_GetError());
        IMG_Quit();
        SDL_Quit();
        return 1;
    }

    SDL_Window *window = SDL_CreateWindow(
        "Descent into Darkness",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        SCREEN_WIDTH, SCREEN_HEIGHT,
        SDL_WINDOW_SHOWN
    );
    if (!window) {
        fprintf(stderr, "SDL_CreateWindow failed: %s\n", SDL_GetError());
        TTF_Quit();
        IMG_Quit();
        SDL_Quit();
        return 1;
    }

    SDL_Renderer *renderer = SDL_CreateRenderer(
        window, -1,
        SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC
    );
    if (!renderer) {
        fprintf(stderr, "SDL_CreateRenderer failed: %s\n", SDL_GetError());
        SDL_DestroyWindow(window);
        TTF_Quit();
        IMG_Quit();
        SDL_Quit();
        return 1;
    }

    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);

    /* Load textures */
    tex_floor   = load_texture(renderer, "assets/tiles/floor_stone.jpg");
    tex_wall    = load_texture(renderer, "assets/tiles/wall_stone.jpg");
    tex_grass   = load_texture(renderer, "assets/tiles/town/grass.jpg");
    tex_warrior = load_texture(renderer, "assets/sprites/player/warrior_test.jpg");

    /* Load HUD font — try several system font paths */
    const char *font_paths[] = {
        "/System/Library/Fonts/Monaco.ttf",
        "/System/Library/Fonts/Geneva.ttf",
        "/Library/Fonts/Arial Unicode.ttf",
        "/System/Library/Fonts/Helvetica.ttc",
        NULL,
    };
    for (int i = 0; font_paths[i] != NULL; i++) {
        hud_font = TTF_OpenFont(font_paths[i], 14);
        if (hud_font) break;
    }
    if (!hud_font) {
        fprintf(stderr, "Warning: No font loaded, HUD text disabled\n");
    }

    /* Center camera on the player's initial position */
    {
        int sx, sy;
        iso_to_screen(player_tile_x, player_tile_y, &sx, &sy);
        camera_x = (float)sx;
        camera_y = (float)(sy - VIEWPORT_HEIGHT / 2 + TILE_HALF_H);
    }

    /* Game loop */
    bool running = true;
    Uint64 perf_freq = SDL_GetPerformanceFrequency();
    Uint64 last_time = SDL_GetPerformanceCounter();
    int frame_count = 0;
    int display_fps = 0;
    Uint32 fps_timer = SDL_GetTicks();

    while (running) {
        /* Delta time */
        Uint64 now = SDL_GetPerformanceCounter();
        float dt = (float)(now - last_time) / (float)perf_freq;
        last_time = now;

        /* Cap delta time to avoid spiral of death */
        if (dt > 0.05f) dt = 0.05f;

        /* FPS counter */
        frame_count++;
        Uint32 tick_now = SDL_GetTicks();
        if (tick_now - fps_timer >= 1000) {
            display_fps = frame_count;
            frame_count = 0;
            fps_timer = tick_now;
        }

        /* Event handling */
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            switch (event.type) {
            case SDL_QUIT:
                running = false;
                break;
            case SDL_KEYDOWN:
                if (event.key.keysym.sym == SDLK_ESCAPE)
                    running = false;
                /* Click to move player */
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

        /* Keyboard state for smooth camera scrolling */
        const Uint8 *keys = SDL_GetKeyboardState(NULL);
        float scroll_speed = CAMERA_SPEED * dt;

        if (keys[SDL_SCANCODE_LEFT] || keys[SDL_SCANCODE_A])
            camera_x -= scroll_speed;
        if (keys[SDL_SCANCODE_RIGHT] || keys[SDL_SCANCODE_D])
            camera_x += scroll_speed;
        if (keys[SDL_SCANCODE_UP] || keys[SDL_SCANCODE_W])
            camera_y -= scroll_speed;
        if (keys[SDL_SCANCODE_DOWN] || keys[SDL_SCANCODE_S])
            camera_y += scroll_speed;

        /* Mouse tile picking */
        {
            int mx, my;
            SDL_GetMouseState(&mx, &my);

            /* Only pick tiles in the viewport area (above the panel) */
            if (my < VIEWPORT_HEIGHT) {
                /* Reverse the rendering transform to get world coords */
                int world_x = mx - SCREEN_WIDTH / 2 + (int)camera_x;
                int world_y = my + (int)camera_y;

                screen_to_iso(world_x, world_y, &hover_tile_x, &hover_tile_y);
            } else {
                hover_tile_x = -1;
                hover_tile_y = -1;
            }
        }

        /* Render */
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);

        render_scene(renderer);
        render_panel(renderer);
        render_fps(renderer, display_fps);

        SDL_RenderPresent(renderer);
    }

    /* Cleanup */
    if (hud_font)    TTF_CloseFont(hud_font);
    if (tex_floor)   SDL_DestroyTexture(tex_floor);
    if (tex_wall)    SDL_DestroyTexture(tex_wall);
    if (tex_grass)   SDL_DestroyTexture(tex_grass);
    if (tex_warrior) SDL_DestroyTexture(tex_warrior);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    TTF_Quit();
    IMG_Quit();
    SDL_Quit();

    return 0;
}
