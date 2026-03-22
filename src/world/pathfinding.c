#include "world/pathfinding.h"
#include "world/map.h"

/* A* node for the search grid */
typedef struct AStarNode {
    int x, y;
    float g, h, f;          /* cost, heuristic, total */
    int parent_x, parent_y;
    bool open;
    bool closed;
} AStarNode;

/* 8-directional neighbor offsets */
static const int dx8[8] = { 0,  1, 1, 1, 0, -1, -1, -1 };
static const int dy8[8] = { -1, -1, 0, 1, 1,  1,  0, -1 };

/* Cardinal directions are indices 0, 2, 4, 6; diagonals are 1, 3, 5, 7.
 * For diagonal i, the two adjacent cardinals are (i-1)%8 and (i+1)%8. */

static float chebyshev(int x0, int y0, int x1, int y1)
{
    int adx = abs(x1 - x0);
    int ady = abs(y1 - y0);
    return (float)MAX(adx, ady);
}

bool pathfind(const struct TileMap *map, int sx, int sy, int ex, int ey,
              Path *path)
{
    path_clear(path);

    /* Quick reject: start or end out of bounds / not walkable */
    if (!tilemap_in_bounds(map, sx, sy) || !tilemap_in_bounds(map, ex, ey))
        return false;
    if (!tilemap_is_walkable(map, sx, sy) || !tilemap_is_walkable(map, ex, ey))
        return false;

    /* Trivial case: already at destination */
    if (sx == ex && sy == ey)
        return true;

    /* Node grid on the stack (~50 KB for 40x40) */
    AStarNode nodes[MAP_HEIGHT][MAP_WIDTH];
    memset(nodes, 0, sizeof(nodes));

    /* Initialize start node */
    AStarNode *start = &nodes[sy][sx];
    start->x = sx;
    start->y = sy;
    start->g = 0.0f;
    start->h = chebyshev(sx, sy, ex, ey);
    start->f = start->h;
    start->parent_x = -1;
    start->parent_y = -1;
    start->open = true;

    bool found = false;

    while (true) {
        /* Find the open node with lowest f score (linear scan) */
        AStarNode *current = NULL;
        float best_f = 1e9f;

        for (int y = 0; y < MAP_HEIGHT; y++) {
            for (int x = 0; x < MAP_WIDTH; x++) {
                if (nodes[y][x].open && nodes[y][x].f < best_f) {
                    best_f = nodes[y][x].f;
                    current = &nodes[y][x];
                }
            }
        }

        /* No open nodes left — path not found */
        if (!current)
            break;

        /* Reached the goal */
        if (current->x == ex && current->y == ey) {
            found = true;
            break;
        }

        /* Move current from open to closed */
        current->open = false;
        current->closed = true;

        int cx = current->x;
        int cy = current->y;

        /* Explore 8 neighbors */
        for (int i = 0; i < 8; i++) {
            int nx = cx + dx8[i];
            int ny = cy + dy8[i];

            if (!tilemap_in_bounds(map, nx, ny))
                continue;
            if (!tilemap_is_walkable(map, nx, ny))
                continue;

            /* Diagonal: prevent corner-cutting through walls.
             * Diagonal indices are odd (1, 3, 5, 7).
             * The two adjacent cardinals must both be walkable. */
            if (i & 1) {
                int card_a = (i + 7) & 7;  /* (i - 1) % 8 */
                int card_b = (i + 1) & 7;
                if (!tilemap_is_walkable(map, cx + dx8[card_a], cy + dy8[card_a]) ||
                    !tilemap_is_walkable(map, cx + dx8[card_b], cy + dy8[card_b]))
                    continue;
            }

            AStarNode *neighbor = &nodes[ny][nx];

            if (neighbor->closed)
                continue;

            float move_cost = (i & 1) ? 1.414f : 1.0f;
            float tentative_g = current->g + move_cost;

            if (!neighbor->open) {
                /* First time visiting this node */
                neighbor->x = nx;
                neighbor->y = ny;
                neighbor->g = tentative_g;
                neighbor->h = chebyshev(nx, ny, ex, ey);
                neighbor->f = neighbor->g + neighbor->h;
                neighbor->parent_x = cx;
                neighbor->parent_y = cy;
                neighbor->open = true;
            } else if (tentative_g < neighbor->g) {
                /* Found a better path to this node */
                neighbor->g = tentative_g;
                neighbor->f = neighbor->g + neighbor->h;
                neighbor->parent_x = cx;
                neighbor->parent_y = cy;
            }
        }
    }

    if (!found)
        return false;

    /* Backtrack from end to start to build path */
    PathPoint temp[MAX_PATH_LENGTH];
    int count = 0;
    int bx = ex;
    int by = ey;

    while (bx != -1 && by != -1 && count < MAX_PATH_LENGTH) {
        temp[count].x = bx;
        temp[count].y = by;
        count++;
        int px = nodes[by][bx].parent_x;
        int py = nodes[by][bx].parent_y;
        bx = px;
        by = py;
    }

    /* If path was truncated (didn't reach start), it's too long */
    if (bx != -1 || by != -1) {
        path_clear(path);
        return false;
    }

    /* Reverse into path (start → end order) */
    path->length = count;
    path->current_step = 0;
    for (int i = 0; i < count; i++) {
        path->points[i] = temp[count - 1 - i];
    }

    return true;
}

bool path_next(Path *path, int *out_x, int *out_y)
{
    if (!path || path->current_step >= path->length)
        return false;

    *out_x = path->points[path->current_step].x;
    *out_y = path->points[path->current_step].y;
    path->current_step++;
    return true;
}

bool path_is_complete(const Path *path)
{
    return !path || path->current_step >= path->length;
}

void path_clear(Path *path)
{
    if (path) {
        path->length = 0;
        path->current_step = 0;
    }
}
