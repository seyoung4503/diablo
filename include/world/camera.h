#ifndef DIABLO_WORLD_CAMERA_H
#define DIABLO_WORLD_CAMERA_H

#include "common.h"

typedef struct Camera {
    float x, y;              /* world position in pixels */
    float target_x, target_y; /* for smooth follow */
    float speed;             /* scroll speed (pixels/sec) */
    float lerp_factor;       /* for smooth following (0.1 = slow, 1.0 = instant) */
} Camera;

void camera_init(Camera *cam, float speed);
void camera_update(Camera *cam, float dt);
void camera_center_on_tile(Camera *cam, int tile_x, int tile_y);
void camera_scroll(Camera *cam, float dx, float dy);

/* Convert screen mouse coords to world coords accounting for camera */
void camera_screen_to_world(const Camera *cam, int screen_x, int screen_y,
                            int viewport_center_x, int *world_x, int *world_y);

#endif /* DIABLO_WORLD_CAMERA_H */
