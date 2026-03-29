#include "world/dungeon.h"
#include <limits.h>

/* BSP tree storage for generation */
static BSPNode bsp_nodes[MAX_BSP_NODES];
static int bsp_node_count;

/* Room collection during generation */
static Room gen_rooms[MAX_ROOMS];
static int gen_room_count;

/* Current level number (used by corridor width logic) */
static int gen_level_num;

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
/* Room shape carving helpers                                          */
/* ------------------------------------------------------------------ */

typedef enum {
    ROOM_RECTANGLE,
    ROOM_L_SHAPED,
    ROOM_CROSS,
    ROOM_ROUND
} RoomShape;

static RoomShape pick_room_shape(void)
{
    int roll = rand() % 100;
    if (roll < 10)      return ROOM_CROSS;    /* 10% */
    if (roll < 25)      return ROOM_ROUND;    /* 15% */
    if (roll < 45)      return ROOM_L_SHAPED; /* 20% */
    return ROOM_RECTANGLE;                    /* 55% */
}

static void carve_rectangle_room(TileMap *map, int x, int y, int w, int h)
{
    for (int ty = y; ty < y + h; ty++)
        for (int tx = x; tx < x + w; tx++)
            tilemap_set_tile(map, tx, ty, TILE_STONE_FLOOR);
}

static void carve_l_shaped_room(TileMap *map, int x, int y, int w, int h)
{
    int half_w = w / 2 + 1;
    int half_h = h / 2 + 1;
    /* Vertical arm */
    for (int ty = y; ty < y + h; ty++)
        for (int tx = x; tx < x + half_w; tx++)
            tilemap_set_tile(map, tx, ty, TILE_STONE_FLOOR);
    /* Horizontal arm */
    for (int ty = y; ty < y + half_h; ty++)
        for (int tx = x; tx < x + w; tx++)
            tilemap_set_tile(map, tx, ty, TILE_STONE_FLOOR);
}

static void carve_cross_room(TileMap *map, int x, int y, int w, int h)
{
    int arm_w = w / 3;
    int arm_h = h / 3;
    if (arm_w < 2) arm_w = 2;
    if (arm_h < 2) arm_h = 2;
    /* Horizontal bar */
    int hx = x;
    int hy = y + (h - arm_h) / 2;
    for (int ty = hy; ty < hy + arm_h; ty++)
        for (int tx = hx; tx < hx + w; tx++)
            tilemap_set_tile(map, tx, ty, TILE_STONE_FLOOR);
    /* Vertical bar */
    int vx = x + (w - arm_w) / 2;
    int vy = y;
    for (int ty = vy; ty < vy + h; ty++)
        for (int tx = vx; tx < vx + arm_w; tx++)
            tilemap_set_tile(map, tx, ty, TILE_STONE_FLOOR);
}

static void carve_round_room(TileMap *map, int x, int y, int w, int h)
{
    float cx = x + w / 2.0f;
    float cy = y + h / 2.0f;
    float rx = w / 2.0f;
    float ry = h / 2.0f;
    for (int ty = y; ty < y + h; ty++) {
        for (int tx = x; tx < x + w; tx++) {
            float dx = (tx + 0.5f - cx) / rx;
            float dy = (ty + 0.5f - cy) / ry;
            if (dx * dx + dy * dy <= 1.0f)
                tilemap_set_tile(map, tx, ty, TILE_STONE_FLOOR);
        }
    }
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

        /* Pick a room shape and carve it */
        RoomShape shape = pick_room_shape();
        switch (shape) {
        case ROOM_L_SHAPED:
            carve_l_shaped_room(map, rx, ry, rw, rh);
            break;
        case ROOM_CROSS:
            carve_cross_room(map, rx, ry, rw, rh);
            break;
        case ROOM_ROUND:
            carve_round_room(map, rx, ry, rw, rh);
            break;
        default:
            carve_rectangle_room(map, rx, ry, rw, rh);
            break;
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

static void carve_corridor_h(TileMap *map, int x1, int x2, int y, int width)
{
    int start = MIN(x1, x2);
    int end = MAX(x1, x2);
    for (int x = start; x <= end; x++) {
        for (int w = 0; w < width; w++) {
            if (tilemap_in_bounds(map, x, y + w))
                tilemap_set_tile(map, x, y + w, TILE_STONE_FLOOR);
        }
    }
}

static void carve_corridor_v(TileMap *map, int y1, int y2, int x, int width)
{
    int start = MIN(y1, y2);
    int end = MAX(y1, y2);
    for (int y = start; y <= end; y++) {
        for (int w = 0; w < width; w++) {
            if (tilemap_in_bounds(map, x + w, y))
                tilemap_set_tile(map, x + w, y, TILE_STONE_FLOOR);
        }
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

    /* Wider corridors for deeper levels (9+) */
    int corridor_width = (gen_level_num >= 9) ? 2 : 1;

    /* L-shaped corridor: randomly horizontal-first or vertical-first */
    if (rand() % 2 == 0) {
        carve_corridor_h(map, lx, rx, ly, corridor_width);
        carve_corridor_v(map, ly, ry, rx, corridor_width);
    } else {
        carve_corridor_v(map, ly, ry, lx, corridor_width);
        carve_corridor_h(map, lx, rx, ry, corridor_width);
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
/* Special room features                                               */
/* ------------------------------------------------------------------ */

static void place_special_features(DungeonLevel *level)
{
    if (level->room_count < 3)
        return;

    /* Water pool in one random room */
    int pool_room = rand() % level->room_count;
    int cx = level->rooms[pool_room].x + level->rooms[pool_room].w / 2;
    int cy = level->rooms[pool_room].y + level->rooms[pool_room].h / 2;
    /* 3x3 water pool */
    for (int dy = -1; dy <= 1; dy++)
        for (int dx = -1; dx <= 1; dx++)
            if (tilemap_in_bounds(&level->map, cx + dx, cy + dy))
                tilemap_set_tile(&level->map, cx + dx, cy + dy, TILE_WATER);

    /* Theme-specific decorations */
    switch (level->theme) {
    case THEME_CATHEDRAL:
        /* Holy water fonts in room corners */
        for (int r = 0; r < level->room_count; r++) {
            if (rand() % 3 != 0) continue; /* ~33% of rooms */
            const Room *room = &level->rooms[r];
            int corners[4][2] = {
                { room->x + 1, room->y + 1 },
                { room->x + room->w - 2, room->y + 1 },
                { room->x + 1, room->y + room->h - 2 },
                { room->x + room->w - 2, room->y + room->h - 2 },
            };
            int ci = rand() % 4;
            if (tilemap_in_bounds(&level->map, corners[ci][0], corners[ci][1]))
                tilemap_set_tile(&level->map, corners[ci][0], corners[ci][1],
                                 TILE_WATER);
        }
        break;

    case THEME_CATACOMBS:
        /* Void/pit tiles scattered (1-2 per room) */
        for (int r = 0; r < level->room_count; r++) {
            const Room *room = &level->rooms[r];
            int pits = 1 + rand() % 2;
            for (int p = 0; p < pits; p++) {
                int px = room->x + 1 + rand() % MAX(1, room->w - 2);
                int py = room->y + 1 + rand() % MAX(1, room->h - 2);
                /* Don't place on stairs */
                if ((px == level->stairs_up_x && py == level->stairs_up_y) ||
                    (px == level->stairs_down_x && py == level->stairs_down_y))
                    continue;
                if (tilemap_in_bounds(&level->map, px, py))
                    tilemap_set_tile(&level->map, px, py, TILE_NONE);
            }
        }
        break;

    case THEME_CAVES:
        /* Erode wall corners for irregular walls */
        for (int r = 0; r < level->room_count; r++) {
            const Room *room = &level->rooms[r];
            /* Check tiles just outside each room edge */
            for (int x = room->x - 1; x <= room->x + room->w; x++) {
                for (int y = room->y - 1; y <= room->y + room->h; y++) {
                    if (!tilemap_in_bounds(&level->map, x, y)) continue;
                    if (tilemap_get_type(&level->map, x, y) != TILE_WALL)
                        continue;
                    /* Only erode corner-adjacent walls with ~25% chance */
                    if (rand() % 4 == 0)
                        tilemap_set_tile(&level->map, x, y, TILE_STONE_FLOOR);
                }
            }
        }
        break;

    case THEME_HELL:
        /* Lava pools (reuse TILE_WATER) in random rooms */
        for (int r = 0; r < level->room_count; r++) {
            if (rand() % 2 != 0) continue; /* ~50% of rooms */
            const Room *room = &level->rooms[r];
            int lx = room->x + room->w / 2;
            int ly = room->y + room->h / 2;
            /* 2x2 lava pool */
            for (int dy = 0; dy <= 1; dy++)
                for (int dx = 0; dx <= 1; dx++)
                    if (tilemap_in_bounds(&level->map, lx + dx, ly + dy))
                        tilemap_set_tile(&level->map, lx + dx, ly + dy,
                                         TILE_WATER);
        }
        break;
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
    gen_level_num = level_num;

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

    /* Add special room features (water pools, theme decorations) */
    place_special_features(level);

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
