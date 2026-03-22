#include "world/camera.h"

void camera_init(Camera *cam, float speed)
{
    cam->x = 0.0f;
    cam->y = 0.0f;
    cam->target_x = 0.0f;
    cam->target_y = 0.0f;
    cam->speed = speed;
    cam->lerp_factor = 0.1f;
}

/*
 * Smoothly move camera toward target using linear interpolation.
 * Called once per frame with delta time.
 */
void camera_update(Camera *cam, float dt)
{
    /* Frame-rate independent lerp: 1 - (1 - factor)^(dt * 60)
     * At 60fps this behaves like the raw lerp_factor */
    float t = 1.0f - powf(1.0f - cam->lerp_factor, dt * 60.0f);
    cam->x += (cam->target_x - cam->x) * t;
    cam->y += (cam->target_y - cam->y) * t;
}

/*
 * Set the camera target to center on the given tile.
 * Uses iso_to_screen to convert tile coords, then adjusts
 * so the tile appears at the center of the viewport.
 */
void camera_center_on_tile(Camera *cam, int tile_x, int tile_y)
{
    int sx, sy;
    iso_to_screen(tile_x, tile_y, &sx, &sy);
    cam->target_x = (float)sx;
    cam->target_y = (float)(sy - VIEWPORT_HEIGHT / 2 + TILE_HALF_H);
}

/*
 * Manual scroll by keyboard. Directly adjusts both position and
 * target so there's no lerp fighting with manual input.
 */
void camera_scroll(Camera *cam, float dx, float dy)
{
    cam->x += dx;
    cam->y += dy;
    cam->target_x = cam->x;
    cam->target_y = cam->y;
}

/*
 * Convert screen mouse coordinates to world coordinates,
 * accounting for camera offset.
 */
void camera_screen_to_world(const Camera *cam, int screen_x, int screen_y,
                            int viewport_center_x, int *world_x, int *world_y)
{
    *world_x = screen_x - viewport_center_x + (int)cam->x;
    *world_y = screen_y + (int)cam->y;
}
