#include "world/dungeon.h"

/* Theme definitions per spec:
 * Cathedral (1-4):  Fallen, Skeleton, Zombie           1-3/room
 * Catacombs (5-8):  Skeleton, Zombie, Hidden, Mage     2-4/room
 * Caves (9-12):     Scavenger, Goat Man, Acid Dog, Hidden  2-5/room
 * Hell (13-16):     Knight, Balrog, Mage, Hidden       3-6/room
 */
static DungeonTheme themes[THEME_COUNT];
static bool themes_initialized = false;

void dungeon_theme_init(void)
{
    if (themes_initialized) return;

    themes[THEME_CATHEDRAL] = (DungeonTheme){
        .theme_id = THEME_CATHEDRAL,
        .name = "Cathedral",
        .enemy_types = { ENEMY_FALLEN, ENEMY_SKELETON, ENEMY_ZOMBIE, ENEMY_FALLEN },
        .enemy_count_min = 1,
        .enemy_count_max = 3,
        .enemy_level_bonus = 0,
    };

    themes[THEME_CATACOMBS] = (DungeonTheme){
        .theme_id = THEME_CATACOMBS,
        .name = "Catacombs",
        .enemy_types = { ENEMY_SKELETON, ENEMY_ZOMBIE, ENEMY_HIDDEN, ENEMY_MAGE },
        .enemy_count_min = 2,
        .enemy_count_max = 4,
        .enemy_level_bonus = 2,
    };

    themes[THEME_CAVES] = (DungeonTheme){
        .theme_id = THEME_CAVES,
        .name = "Caves",
        .enemy_types = { ENEMY_SCAVENGER, ENEMY_GOAT_MAN, ENEMY_ACID_DOG, ENEMY_HIDDEN },
        .enemy_count_min = 2,
        .enemy_count_max = 5,
        .enemy_level_bonus = 4,
    };

    themes[THEME_HELL] = (DungeonTheme){
        .theme_id = THEME_HELL,
        .name = "Hell",
        .enemy_types = { ENEMY_KNIGHT, ENEMY_BALROG, ENEMY_MAGE, ENEMY_HIDDEN },
        .enemy_count_min = 3,
        .enemy_count_max = 6,
        .enemy_level_bonus = 8,
    };

    themes_initialized = true;
}

const DungeonTheme *dungeon_theme_get(int theme_id)
{
    if (!themes_initialized)
        dungeon_theme_init();
    if (theme_id < 0 || theme_id >= THEME_COUNT)
        return &themes[THEME_CATHEDRAL];
    return &themes[theme_id];
}

void dungeon_populate_enemies(DungeonLevel *level, EnemyManager *mgr)
{
    const DungeonTheme *theme = dungeon_theme_get(level->theme);

    /* Spawn enemies in each room except the stairs_up room */
    int room_count = level->room_count < MAX_ROOMS ? level->room_count : MAX_ROOMS;
    for (int r = 0; r < room_count; r++) {
        const Room *room = &level->rooms[r];

        /* Skip the room containing stairs_up (safe spawn point) */
        int rcx = room->x + room->w / 2;
        int rcy = room->y + room->h / 2;
        if (rcx == level->stairs_up_x && rcy == level->stairs_up_y)
            continue;

        /* Random enemy count within theme range */
        int range = theme->enemy_count_max - theme->enemy_count_min + 1;
        int count = theme->enemy_count_min + rand() % range;

        /* Don't exceed manager capacity */
        if (mgr->count + count > MAX_ENEMIES_PER_LEVEL)
            count = MAX_ENEMIES_PER_LEVEL - mgr->count;
        if (count <= 0)
            break;

        for (int e = 0; e < count; e++) {
            /* Pick a random theme-appropriate enemy type */
            EnemyType etype = theme->enemy_types[rand() % 4];

            /* Find a walkable position within the room */
            int attempts = 0;
            int ex, ey;
            do {
                ex = room->x + 1 + rand() % MAX(1, room->w - 2);
                ey = room->y + 1 + rand() % MAX(1, room->h - 2);
                attempts++;
            } while (!tilemap_is_walkable(&level->map, ex, ey) && attempts < 20);

            if (!tilemap_is_walkable(&level->map, ex, ey))
                continue;

            /* Don't spawn on stairs */
            if ((ex == level->stairs_down_x && ey == level->stairs_down_y) ||
                (ex == level->stairs_up_x && ey == level->stairs_up_y))
                continue;

            enemy_spawn(mgr, etype, ex, ey);

            /* Apply level bonus to spawned enemy stats */
            if (theme->enemy_level_bonus > 0 && mgr->count > 0) {
                Enemy *spawned = &mgr->enemies[mgr->count - 1];
                spawned->max_hp += theme->enemy_level_bonus * 2;
                spawned->current_hp = spawned->max_hp;
                spawned->damage_min += theme->enemy_level_bonus;
                spawned->damage_max += theme->enemy_level_bonus;
                spawned->xp_value += theme->enemy_level_bonus * 3;
            }
        }
    }
}
