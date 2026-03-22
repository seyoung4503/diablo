#ifndef DIABLO_WORLD_PATHFINDING_H
#define DIABLO_WORLD_PATHFINDING_H

#include "common.h"

#define MAX_PATH_LENGTH 256

typedef struct PathPoint {
    int x, y;
} PathPoint;

typedef struct Path {
    PathPoint points[MAX_PATH_LENGTH];
    int length;
    int current_step;  /* index of next point to walk to */
} Path;

/* Forward declare TileMap to avoid circular includes */
struct TileMap;

/* Find path from (sx,sy) to (ex,ey) on the given tilemap.
 * Returns true if path found, false if unreachable.
 * Uses A* with 8-directional movement (diagonal allowed).
 * path->length will be 0 if no path found.
 */
bool pathfind(const struct TileMap *map, int sx, int sy, int ex, int ey,
              Path *path);

/* Get the next point on the path. Returns false if path is complete. */
bool path_next(Path *path, int *out_x, int *out_y);

/* Check if path is complete */
bool path_is_complete(const Path *path);

/* Reset path */
void path_clear(Path *path);

#endif /* DIABLO_WORLD_PATHFINDING_H */
