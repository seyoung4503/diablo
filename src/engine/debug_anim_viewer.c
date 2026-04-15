#include "engine/debug.h"
#include "game/player.h"
#include <time.h>
#include <sys/stat.h>
#include <errno.h>

static const char *state_labels[] = {
    "IDLE", "WALKING", "ATTACKING", "HIT", "DEATH"
};

static const char *dir_labels[] = {
    "S", "SW", "W", "NW", "N", "NE", "E", "SE"
};

void debug_anim_viewer_init(DebugAnimViewer *viewer)
{
    memset(viewer, 0, sizeof(*viewer));
    viewer->active = false;
    viewer->selected_sheet = 0;
    viewer->selected_state = 0;
    viewer->selected_dir = 0;
    viewer->playing = true;
    memset(&viewer->preview_anim, 0, sizeof(viewer->preview_anim));
}

/* Rebind the preview AnimController to the current selection */
static void rebind_preview(DebugAnimViewer *viewer, const SpriteSheetManager *sprites)
{
    if (viewer->selected_sheet < 0 || viewer->selected_sheet >= sprites->count)
        return;

    const SpriteSheet *sheet = &sprites->sheets[viewer->selected_sheet];
    if (!sheet->loaded) return;

    anim_controller_init(&viewer->preview_anim, sheet);
    anim_controller_set_state(&viewer->preview_anim,
                              viewer->selected_state,
                              viewer->selected_dir);
}

bool debug_anim_viewer_handle_key(DebugAnimViewer *viewer,
                                  SDL_Keycode key,
                                  const SpriteSheetManager *sprites)
{
    if (!viewer->active) return false;

    switch (key) {
    case SDLK_ESCAPE:
        viewer->active = false;
        return true;

    case SDLK_LEFT:
        viewer->selected_dir = (viewer->selected_dir + 7) % 8;
        rebind_preview(viewer, sprites);
        return true;

    case SDLK_RIGHT:
        viewer->selected_dir = (viewer->selected_dir + 1) % 8;
        rebind_preview(viewer, sprites);
        return true;

    case SDLK_UP:
        viewer->selected_state = (viewer->selected_state + 4) % 5;
        rebind_preview(viewer, sprites);
        return true;

    case SDLK_DOWN:
        viewer->selected_state = (viewer->selected_state + 1) % 5;
        rebind_preview(viewer, sprites);
        return true;

    case SDLK_TAB:
        if (sprites->count > 0) {
            viewer->selected_sheet = (viewer->selected_sheet + 1) % sprites->count;
            rebind_preview(viewer, sprites);
        }
        return true;

    case SDLK_SPACE:
        viewer->playing = !viewer->playing;
        return true;

    case SDLK_RETURN:
    case SDLK_p:
        /* Capture will be triggered after render */
        return true;

    default:
        return true; /* consume all keys while viewer is active */
    }
}

void debug_anim_viewer_update(DebugAnimViewer *viewer, float dt)
{
    if (!viewer->active || !viewer->playing) return;
    if (!viewer->preview_anim.sheet) return;

    anim_controller_update(&viewer->preview_anim, dt);
}

void debug_anim_viewer_render(const DebugAnimViewer *viewer,
                              SDL_Renderer *renderer, UI *ui,
                              const SpriteSheetManager *sprites)
{
    /* Dark background */
    SDL_SetRenderDrawColor(renderer, 20, 16, 28, 255);
    SDL_RenderClear(renderer);

    SDL_Color white  = { 255, 255, 255, 255 };
    SDL_Color yellow = { 255, 220, 100, 255 };
    SDL_Color gray   = { 140, 140, 140, 255 };
    SDL_Color red    = { 255, 80, 80, 255 };
    SDL_Color green  = { 80, 255, 120, 255 };

    char buf[256];

    /* Title bar */
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    SDL_Rect title_bg = { 0, 0, SCREEN_WIDTH, 28 };
    SDL_SetRenderDrawColor(renderer, 40, 32, 50, 255);
    SDL_RenderFillRect(renderer, &title_bg);

    ui_draw_text(ui, "ANIMATION VIEWER", 10, 6, yellow);

    /* No sheets loaded — show help message */
    if (sprites->count == 0) {
        int cy = SCREEN_HEIGHT / 2 - 60;
        ui_draw_text(ui, "No sprite sheets loaded.", 160, cy, red);
        cy += 24;
        ui_draw_text(ui, "Expected sprite sheet files:", 140, cy, white);
        cy += 20;
        ui_draw_text(ui, "  assets/sprites/player/warrior_sheet.png", 80, cy, gray);
        cy += 18;
        ui_draw_text(ui, "  (96x96 frames, 40 rows x 6 cols)", 80, cy, gray);
        cy += 18;
        ui_draw_text(ui, "  Rows: state*8 + direction", 80, cy, gray);
        cy += 30;
        ui_draw_text(ui, "Press ESC to return to game", 190, cy, yellow);
        return;
    }

    /* Sheet info */
    const SpriteSheet *sheet = NULL;
    if (viewer->selected_sheet >= 0 && viewer->selected_sheet < sprites->count) {
        sheet = &sprites->sheets[viewer->selected_sheet];
        if (!sheet->loaded) sheet = NULL;
    }

    snprintf(buf, sizeof(buf), "Sheet %d/%d: %s",
             viewer->selected_sheet + 1, sprites->count,
             sheet ? sheet->name : "(empty)");
    ui_draw_text(ui, buf, 200, 6, white);

    if (sheet) {
        snprintf(buf, sizeof(buf), "%dx%d frames",
                 sheet->frame_width, sheet->frame_height);
        ui_draw_text(ui, buf, 500, 6, gray);
    }

    /* State/direction selector */
    int sel_y = 36;
    snprintf(buf, sizeof(buf), "State: %s    Dir: %s    %s",
             state_labels[viewer->selected_state],
             dir_labels[viewer->selected_dir],
             viewer->playing ? "[PLAYING]" : "[PAUSED]");
    ui_draw_text(ui, buf, 10, sel_y, white);

    if (!sheet) {
        ui_draw_text(ui, "Selected sheet not loaded", 200, 200, red);
        return;
    }

    /* Frame grid area (left side) */
    int grid_x = 10;
    int grid_y = 60;
    int grid_w = 400;
    int grid_h = 340;

    /* Draw grid background */
    SDL_Rect grid_bg = { grid_x - 2, grid_y - 2, grid_w + 4, grid_h + 4 };
    SDL_SetRenderDrawColor(renderer, 30, 24, 38, 255);
    SDL_RenderFillRect(renderer, &grid_bg);
    SDL_SetRenderDrawColor(renderer, 80, 70, 100, 255);
    SDL_RenderDrawRect(renderer, &grid_bg);

    /* Get the sequence for current state+direction */
    const AnimSequence *seq = &sheet->sequences[viewer->selected_state][viewer->selected_dir];

    if (seq->frame_count == 0) {
        ui_draw_text(ui, "No frames for this state/dir", grid_x + 80, grid_y + 150, red);
    } else {
        /* Calculate frame display size (fit in grid) */
        int fw = sheet->frame_width;
        int fh = sheet->frame_height;

        /* Scale to fit: max 4 frames per row */
        int cols = (seq->frame_count <= 4) ? seq->frame_count : 4;
        int rows = (seq->frame_count + cols - 1) / cols;
        int cell_w = (grid_w - 20) / cols;
        int cell_h = (grid_h - 40) / rows;
        int scale = MIN(cell_w / fw, cell_h / fh);
        if (scale < 1) scale = 1;
        int disp_w = fw * scale;
        int disp_h = fh * scale;

        for (int i = 0; i < seq->frame_count; i++) {
            int col = i % cols;
            int row = i / cols;
            int dx = grid_x + 10 + col * (disp_w + 8);
            int dy = grid_y + 10 + row * (disp_h + 24);

            /* Frame label */
            snprintf(buf, sizeof(buf), "F%d", i);
            ui_draw_text(ui, buf, dx, dy - 2, gray);

            /* Highlight current frame */
            bool is_current = (i == viewer->preview_anim.current_frame);
            if (is_current) {
                SDL_Rect hl = { dx - 2, dy + 12, disp_w + 4, disp_h + 4 };
                SDL_SetRenderDrawColor(renderer, 255, 220, 100, 255);
                SDL_RenderDrawRect(renderer, &hl);
            }

            /* Draw the frame from the sheet texture */
            SDL_Rect src = seq->frames[i].src_rect;
            SDL_Rect dst = { dx, dy + 14, disp_w, disp_h };
            SDL_RenderCopy(renderer, sheet->texture, &src, &dst);

            /* Duration label */
            snprintf(buf, sizeof(buf), "%.0fms", seq->frames[i].duration * 1000);
            ui_draw_text(ui, buf, dx, dy + 14 + disp_h + 2, gray);
        }
    }

    /* Preview area (right side) — animated playback */
    int prev_x = 430;
    int prev_y = 60;
    int prev_w = 200;
    int prev_h = 240;

    SDL_Rect prev_bg = { prev_x - 2, prev_y - 2, prev_w + 4, prev_h + 4 };
    SDL_SetRenderDrawColor(renderer, 30, 24, 38, 255);
    SDL_RenderFillRect(renderer, &prev_bg);
    SDL_SetRenderDrawColor(renderer, 80, 70, 100, 255);
    SDL_RenderDrawRect(renderer, &prev_bg);

    ui_draw_text(ui, "PREVIEW", prev_x + 60, prev_y + 4, yellow);

    if (viewer->preview_anim.sheet && seq->frame_count > 0) {
        /* Draw current animation frame centered in preview */
        SDL_Rect src = anim_controller_get_src_rect(&viewer->preview_anim);
        int scale = MIN((prev_w - 20) / sheet->frame_width,
                        (prev_h - 60) / sheet->frame_height);
        if (scale < 1) scale = 1;
        int pw = sheet->frame_width * scale;
        int ph = sheet->frame_height * scale;
        SDL_Rect dst = {
            prev_x + (prev_w - pw) / 2,
            prev_y + 30 + (prev_h - 60 - ph) / 2,
            pw, ph
        };
        SDL_RenderCopy(renderer, sheet->texture, &src, &dst);

        /* Frame info below preview */
        snprintf(buf, sizeof(buf), "Frame %d/%d",
                 viewer->preview_anim.current_frame + 1, seq->frame_count);
        ui_draw_text(ui, buf, prev_x + 50, prev_y + prev_h - 20, white);
    }

    /* All states overview (right side, below preview) */
    int overview_y = prev_y + prev_h + 16;
    ui_draw_text(ui, "ALL STATES:", prev_x, overview_y, yellow);
    overview_y += 18;

    for (int s = 0; s < ANIM_STATE_COUNT; s++) {
        const AnimSequence *oseq = &sheet->sequences[s][viewer->selected_dir];
        bool selected = (s == viewer->selected_state);
        snprintf(buf, sizeof(buf), "%s%s: %d frames%s",
                 selected ? "> " : "  ",
                 state_labels[s],
                 oseq->frame_count,
                 oseq->looping ? " (loop)" : "");
        ui_draw_text(ui, buf, prev_x, overview_y, selected ? yellow : gray);
        overview_y += 16;
    }

    /* Controls help at bottom */
    int help_y = SCREEN_HEIGHT - 24;
    SDL_Rect help_bg = { 0, help_y - 4, SCREEN_WIDTH, 28 };
    SDL_SetRenderDrawColor(renderer, 40, 32, 50, 255);
    SDL_RenderFillRect(renderer, &help_bg);

    ui_draw_text(ui, "Up/Down:State  Left/Right:Dir  Tab:Sheet  Space:Play/Pause  Enter:Capture  Esc:Exit",
                 10, help_y, green);
}

bool debug_anim_viewer_capture(const DebugAnimViewer *viewer,
                               SDL_Renderer *renderer, UI *ui,
                               const SpriteSheetManager *sprites)
{
    /* Render the viewer first */
    debug_anim_viewer_render(viewer, renderer, ui, sprites);

    /* Ensure directory */
    if (mkdir(DEBUG_OUTPUT_DIR, 0755) == -1 && errno != EEXIST)
        return false;

    char ts[32];
    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    strftime(ts, sizeof(ts), "%Y%m%d_%H%M%S", t);

    char path[256];
    snprintf(path, sizeof(path), "%s/anim_%s_%s_%s.png",
             DEBUG_OUTPUT_DIR, state_labels[viewer->selected_state],
             dir_labels[viewer->selected_dir], ts);

    return debug_screenshot_to(renderer, SCREEN_WIDTH, SCREEN_HEIGHT, path);
}
