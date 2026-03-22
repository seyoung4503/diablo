#include "world/dungeon.h"
#include <limits.h>

/* BSP tree storage for generation */
static BSPNode bsp_nodes[MAX_BSP_NODES];
static int bsp_node_count;

/* Room collection during generation */
static Room gen_rooms[MAX_ROOMS];
static int gen_room_count;

/* ------------------------------------------------------------------ */
/* BSP helpers                                                         */
/* ------------------------------------------------------------------ */

static int bsp_create_node(int x, int y, int w, int h)
{
    if (bsp_node_count >= MAX_BSP_NODES)
        return -1;
    int idx = bsp_node_count++;
    bsp_nodes[idx].x = x;
    bsp_nodes[idx].y = y;
    bsp_nodes[idx].w = w;
    bsp_nodes[idx].h = h;
    bsp_nodes[idx].left = -1;
    bsp_nodes[idx].right = -1;
    bsp_nodes[idx].has_room = false;
    return idx;
}

static void bsp_split(int node_idx, int depth, int max_depth)
{
    BSPNode *node = &bsp_nodes[node_idx];

    if (depth >= max_depth)
        return;

    /* Alternate horizontal/vertical split; prefer splitting the longer axis */
    bool split_horizontal = (depth % 2 == 0);
    if (node->w > node->h * 1.5f)
        split_horizontal = false;
    else if (node->h > node->w * 1.5f)
        split_horizontal = true;

    int min_size = MIN_ROOM_SIZE + 2; /* room + padding */

    if (split_horizontal) {
        if (node->h < min_size * 2)
            return;
        /* Split ratio 40-60% for variety */
        int min_split = (int)(node->h * 0.4f);
        int max_split = (int)(node->h * 0.6f);
        if (min_split < min_size) min_split = min_size;
        if (max_split > node->h - min_size) max_split = node->h - min_size;
        if (min_split > max_split) return;

        int split = min_split + rand() % (max_split - min_split + 1);

        node->left = bsp_create_node(node->x, node->y, node->w, split);
        node->right = bsp_create_node(node->x, node->y + split,
                                      node->w, node->h - split);
    } else {
        if (node->w < min_size * 2)
            return;
        int min_split = (int)(node->w * 0.4f);
        int max_split = (int)(node->w * 0.6f);
        if (min_split < min_size) min_split = min_size;
        if (max_split > node->w - min_size) max_split = node->w - min_size;
        if (min_split > max_split) return;

        int split = min_split + rand() % (max_split - min_split + 1);

        node->left = bsp_create_node(node->x, node->y, split, node->h);
        node->right = bsp_create_node(node->x + split, node->y,
                                      node->w - split, node->h);
    }

    if (node->left >= 0)
        bsp_split(node->left, depth + 1, max_depth);
    if (node->right >= 0)
        bsp_split(node->right, depth + 1, max_depth);
}

/* ------------------------------------------------------------------ */
/* Room creation in leaf nodes                                         */
/* ------------------------------------------------------------------ */

static void bsp_create_rooms(int node_idx, TileMap *map)
{
    BSPNode *node = &bsp_nodes[node_idx];

    /* Leaf node: create a room */
    if (node->left == -1 && node->right == -1) {
        if (gen_room_count >= MAX_ROOMS)
            return;

        int max_w = MIN(MAX_ROOM_SIZE, node->w - 2);
        int max_h = MIN(MAX_ROOM_SIZE, node->h - 2);
        if (max_w < MIN_ROOM_SIZE) max_w = MIN_ROOM_SIZE;
        if (max_h < MIN_ROOM_SIZE) max_h = MIN_ROOM_SIZE;

        int rw = MIN_ROOM_SIZE + rand() % (max_w - MIN_ROOM_SIZE + 1);
        int rh = MIN_ROOM_SIZE + rand() % (max_h - MIN_ROOM_SIZE + 1);

        /* Random position within node, with 1-tile padding */
        int max_rx = node->x + node->w - rw - 1;
        int max_ry = node->y + node->h - rh - 1;
        int rx = node->x + 1;
        int ry = node->y + 1;
        if (max_rx > rx)
            rx += rand() % (max_rx - rx + 1);
        if (max_ry > ry)
            ry += rand() % (max_ry - ry + 1);

        Room room = { rx, ry, rw, rh };
        node->room = room;
        node->has_room = true;
        gen_rooms[gen_room_count++] = room;

        /* Carve room into tilemap */
        for (int y = ry; y < ry + rh; y++) {
            for (int x = rx; x < rx + rw; x++) {
                tilemap_set_tile(map, x, y, TILE_STONE_FLOOR);
            }
        }
        return;
    }

    if (node->left >= 0)
        bsp_create_rooms(node->left, map);
    if (node->right >= 0)
        bsp_create_rooms(node->right, map);
}

/* ------------------------------------------------------------------ */
/* Room center lookup for corridor connections                         */
/* ------------------------------------------------------------------ */

static bool bsp_get_room_center(int node_idx, int *cx, int *cy)
{
    BSPNode *node = &bsp_nodes[node_idx];
    if (node->has_room) {
        *cx = node->room.x + node->room.w / 2;
        *cy = node->room.y + node->room.h / 2;
        return true;
    }
    if (node->left >= 0 && bsp_get_room_center(node->left, cx, cy))
        return true;
    if (node->right >= 0 && bsp_get_room_center(node->right, cx, cy))
        return true;
    return false;
}

/* ------------------------------------------------------------------ */
/* Corridor carving                                                    */
/* ------------------------------------------------------------------ */

static void carve_corridor_h(TileMap *map, int x1, int x2, int y)
{
    int start = MIN(x1, x2);
    int end = MAX(x1, x2);
    for (int x = start; x <= end; x++) {
        if (tilemap_in_bounds(map, x, y))
            tilemap_set_tile(map, x, y, TILE_STONE_FLOOR);
    }
}

static void carve_corridor_v(TileMap *map, int y1, int y2, int x)
{
    int start = MIN(y1, y2);
    int end = MAX(y1, y2);
    for (int y = start; y <= end; y++) {
        if (tilemap_in_bounds(map, x, y))
            tilemap_set_tile(map, x, y, TILE_STONE_FLOOR);
    }
}

/* Connect sibling subtrees with L-shaped corridors */
static void bsp_connect_rooms(int node_idx, TileMap *map)
{
    BSPNode *node = &bsp_nodes[node_idx];
    if (node->left == -1 || node->right == -1)
        return;

    /* Recurse to connect children's subtrees first */
    bsp_connect_rooms(node->left, map);
    bsp_connect_rooms(node->right, map);

    /* Find a room center in each subtree */
    int lx, ly, rx, ry;
    if (!bsp_get_room_center(node->left, &lx, &ly)) return;
    if (!bsp_get_room_center(node->right, &rx, &ry)) return;

    /* L-shaped corridor: randomly horizontal-first or vertical-first */
    if (rand() % 2 == 0) {
        carve_corridor_h(map, lx, rx, ly);
        carve_corridor_v(map, ly, ry, rx);
    } else {
        carve_corridor_v(map, ly, ry, lx);
        carve_corridor_h(map, lx, rx, ry);
    }
}

/* ------------------------------------------------------------------ */
/* Door placement at room/corridor junctions                           */
/* ------------------------------------------------------------------ */

static bool point_in_any_room(int x, int y, const Room *rooms, int count)
{
    for (int i = 0; i < count; i++) {
        if (x >= rooms[i].x && x < rooms[i].x + rooms[i].w &&
            y >= rooms[i].y && y < rooms[i].y + rooms[i].h) {
            return true;
        }
    }
    return false;
}

static void place_doors(TileMap *map, const Room *rooms, int room_count)
{
    for (int i = 0; i < room_count; i++) {
        const Room *r = &rooms[i];

        /* Scan each edge tile of the room */
        for (int x = r->x; x < r->x + r->w; x++) {
            for (int y = r->y; y < r->y + r->h; y++) {
                bool on_edge = (x == r->x || x == r->x + r->w - 1 ||
                                y == r->y || y == r->y + r->h - 1);
                if (!on_edge) continue;
                if (!tilemap_in_bounds(map, x, y)) continue;
                if (tilemap_get_type(map, x, y) != TILE_STONE_FLOOR) continue;

                /* Check if an adjacent tile is corridor floor (not in any room) */
                int dx[] = {-1, 1, 0, 0};
                int dy[] = {0, 0, -1, 1};
                for (int d = 0; d < 4; d++) {
                    int nx = x + dx[d];
                    int ny = y + dy[d];
                    if (!tilemap_in_bounds(map, nx, ny)) continue;
                    if (tilemap_get_type(map, nx, ny) == TILE_STONE_FLOOR &&
                        !point_in_any_room(nx, ny, rooms, room_count)) {
                        tilemap_set_tile(map, x, y, TILE_DOOR);
                        goto next_edge;
                    }
                }
                next_edge:;
            }
        }
    }
}

/* ------------------------------------------------------------------ */
/* Public API                                                          */
/* ------------------------------------------------------------------ */

void dungeon_init(Dungeon *dungeon)
{
    memset(dungeon, 0, sizeof(Dungeon));
    dungeon->current_level = 0;
}

void dungeon_get_theme(int level_num, int *theme)
{
    if (level_num <= 4)       *theme = THEME_CATHEDRAL;
    else if (level_num <= 8)  *theme = THEME_CATACOMBS;
    else if (level_num <= 12) *theme = THEME_CAVES;
    else                      *theme = THEME_HELL;
}

void dungeon_generate_level(DungeonLevel *level, int level_num, unsigned int seed)
{
    srand(seed);

    memset(level, 0, sizeof(DungeonLevel));
    level->level_num = level_num;
    dungeon_get_theme(level_num, &level->theme);

    /* Initialize tilemap and fill with walls */
    tilemap_init(&level->map, DUNGEON_WIDTH, DUNGEON_HEIGHT);
    for (int y = 0; y < DUNGEON_HEIGHT; y++) {
        for (int x = 0; x < DUNGEON_WIDTH; x++) {
            tilemap_set_tile(&level->map, x, y, TILE_WALL);
        }
    }

    /* Reset BSP state */
    bsp_node_count = 0;
    gen_room_count = 0;

    /* Create root BSP node with 1-tile border */
    int root = bsp_create_node(1, 1, DUNGEON_WIDTH - 2, DUNGEON_HEIGHT - 2);

    /* Split 4-5 times for good room variety */
    int split_depth = 4 + rand() % 2;
    bsp_split(root, 0, split_depth);

    /* Create rooms in leaf nodes */
    bsp_create_rooms(root, &level->map);

    /* Connect sibling rooms with L-shaped corridors */
    bsp_connect_rooms(root, &level->map);

    /* Place doors where corridors meet room edges */
    place_doors(&level->map, gen_rooms, gen_room_count);

    /* Store rooms in level */
    level->room_count = gen_room_count;
    for (int i = 0; i < gen_room_count; i++) {
        level->rooms[i] = gen_rooms[i];
    }

    /* Place stairs */
    if (gen_room_count > 0) {
        /* Stairs up: room closest to origin */
        int best_up = 0;
        int best_up_dist = INT_MAX;
        for (int i = 0; i < gen_room_count; i++) {
            int cx = gen_rooms[i].x + gen_rooms[i].w / 2;
            int cy = gen_rooms[i].y + gen_rooms[i].h / 2;
            int dist = cx * cx + cy * cy;
            if (dist < best_up_dist) {
                best_up_dist = dist;
                best_up = i;
            }
        }
        level->stairs_up_x = gen_rooms[best_up].x + gen_rooms[best_up].w / 2;
        level->stairs_up_y = gen_rooms[best_up].y + gen_rooms[best_up].h / 2;
        tilemap_set_tile(&level->map, level->stairs_up_x, level->stairs_up_y,
                         TILE_STAIRS_UP);

        /* Stairs down: room farthest from stairs_up */
        int best_down = 0;
        int best_down_dist = 0;
        for (int i = 0; i < gen_room_count; i++) {
            int cx = gen_rooms[i].x + gen_rooms[i].w / 2;
            int cy = gen_rooms[i].y + gen_rooms[i].h / 2;
            int dx = cx - level->stairs_up_x;
            int dy = cy - level->stairs_up_y;
            int dist = dx * dx + dy * dy;
            if (dist > best_down_dist) {
                best_down_dist = dist;
                best_down = i;
            }
        }
        level->stairs_down_x = gen_rooms[best_down].x + gen_rooms[best_down].w / 2;
        level->stairs_down_y = gen_rooms[best_down].y + gen_rooms[best_down].h / 2;
        tilemap_set_tile(&level->map, level->stairs_down_x, level->stairs_down_y,
                         TILE_STAIRS_DOWN);
    }
}
