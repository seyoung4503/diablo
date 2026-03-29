#include "enemy/enemy.h"
#include "world/map.h"

/* Enemy templates indexed by EnemyType */
typedef struct EnemyTemplate {
    const char *name;
    int max_hp;
    int armor;
    int damage_min, damage_max;
    int to_hit;
    int xp_value;
    int detection_range;
    int attack_range;
    float attack_cooldown;
    float move_speed;
    int gold_min, gold_max;
} EnemyTemplate;

static EnemyTemplate templates[ENEMY_TYPE_COUNT];
static bool defs_initialized = false;

void enemy_defs_init(void)
{
    if (defs_initialized)
        return;

    memset(templates, 0, sizeof(templates));

    /* Fallen: weak melee, in groups of 3-5 */
    templates[ENEMY_FALLEN] = (EnemyTemplate){
        .name = "Fallen",
        .max_hp = 8, .armor = 0,
        .damage_min = 1, .damage_max = 3,
        .to_hit = 25, .xp_value = 20,
        .detection_range = 5, .attack_range = 1,
        .attack_cooldown = 1.2f, .move_speed = 3.0f,
        .gold_min = 0, .gold_max = 5
    };

    /* Skeleton: medium, can be archers (ranged) */
    templates[ENEMY_SKELETON] = (EnemyTemplate){
        .name = "Skeleton",
        .max_hp = 15, .armor = 3,
        .damage_min = 3, .damage_max = 7,
        .to_hit = 35, .xp_value = 40,
        .detection_range = 6, .attack_range = 4,
        .attack_cooldown = 1.5f, .move_speed = 2.5f,
        .gold_min = 2, .gold_max = 15
    };

    /* Zombie: slow but tough */
    templates[ENEMY_ZOMBIE] = (EnemyTemplate){
        .name = "Zombie",
        .max_hp = 25, .armor = 5,
        .damage_min = 5, .damage_max = 10,
        .to_hit = 30, .xp_value = 50,
        .detection_range = 4, .attack_range = 1,
        .attack_cooldown = 2.0f, .move_speed = 1.5f,
        .gold_min = 5, .gold_max = 20
    };

    /* Scavenger: fast, weak, flees at 30% HP */
    templates[ENEMY_SCAVENGER] = (EnemyTemplate){
        .name = "Scavenger",
        .max_hp = 10, .armor = 1,
        .damage_min = 2, .damage_max = 5,
        .to_hit = 40, .xp_value = 30,
        .detection_range = 7, .attack_range = 1,
        .attack_cooldown = 1.0f, .move_speed = 4.0f,
        .gold_min = 1, .gold_max = 10
    };

    /* Hidden: ambush from darkness */
    templates[ENEMY_HIDDEN] = (EnemyTemplate){
        .name = "Hidden",
        .max_hp = 20, .armor = 2,
        .damage_min = 6, .damage_max = 12,
        .to_hit = 45, .xp_value = 60,
        .detection_range = 3, .attack_range = 1,
        .attack_cooldown = 1.8f, .move_speed = 3.0f,
        .gold_min = 10, .gold_max = 30
    };

    /* Goat Man: medium melee, appears in caves */
    templates[ENEMY_GOAT_MAN] = (EnemyTemplate){
        .name = "Goat Man",
        .max_hp = 30, .armor = 4,
        .damage_min = 5, .damage_max = 12,
        .to_hit = 40, .xp_value = 65,
        .detection_range = 6, .attack_range = 1,
        .attack_cooldown = 1.3f, .move_speed = 3.5f,
        .gold_min = 5, .gold_max = 25
    };

    /* Acid Dog: fast, weak */
    templates[ENEMY_ACID_DOG] = (EnemyTemplate){
        .name = "Acid Dog",
        .max_hp = 12, .armor = 1,
        .damage_min = 3, .damage_max = 8,
        .to_hit = 35, .xp_value = 35,
        .detection_range = 8, .attack_range = 1,
        .attack_cooldown = 0.8f, .move_speed = 5.0f,
        .gold_min = 0, .gold_max = 8
    };

    /* Mage: ranged caster */
    templates[ENEMY_MAGE] = (EnemyTemplate){
        .name = "Dark Mage",
        .max_hp = 18, .armor = 2,
        .damage_min = 8, .damage_max = 15,
        .to_hit = 50, .xp_value = 80,
        .detection_range = 7, .attack_range = 5,
        .attack_cooldown = 2.0f, .move_speed = 2.0f,
        .gold_min = 10, .gold_max = 35
    };

    /* Knight: tank */
    templates[ENEMY_KNIGHT] = (EnemyTemplate){
        .name = "Blood Knight",
        .max_hp = 45, .armor = 8,
        .damage_min = 8, .damage_max = 18,
        .to_hit = 45, .xp_value = 100,
        .detection_range = 5, .attack_range = 1,
        .attack_cooldown = 1.8f, .move_speed = 1.8f,
        .gold_min = 15, .gold_max = 50
    };

    /* Balrog: boss tier */
    templates[ENEMY_BALROG] = (EnemyTemplate){
        .name = "Balrog",
        .max_hp = 80, .armor = 10,
        .damage_min = 12, .damage_max = 25,
        .to_hit = 55, .xp_value = 200,
        .detection_range = 8, .attack_range = 2,
        .attack_cooldown = 2.5f, .move_speed = 2.5f,
        .gold_min = 30, .gold_max = 100
    };

    defs_initialized = true;
}

void enemy_apply_template(Enemy *enemy)
{
    if (!defs_initialized)
        enemy_defs_init();

    if (enemy->type <= ENEMY_NONE || enemy->type >= ENEMY_TYPE_COUNT)
        return;

    const EnemyTemplate *t = &templates[enemy->type];
    strncpy(enemy->name, t->name, sizeof(enemy->name) - 1);
    enemy->name[sizeof(enemy->name) - 1] = '\0';
    enemy->max_hp = t->max_hp;
    enemy->current_hp = t->max_hp;
    enemy->armor = t->armor;
    enemy->damage_min = t->damage_min;
    enemy->damage_max = t->damage_max;
    enemy->to_hit = t->to_hit;
    enemy->xp_value = t->xp_value;
    enemy->detection_range = t->detection_range;
    enemy->attack_range = t->attack_range;
    enemy->attack_cooldown = t->attack_cooldown;
    enemy->attack_timer = 0.0f;
    enemy->move_speed = t->move_speed;
    enemy->gold_min = t->gold_min;
    enemy->gold_max = t->gold_max;
}

void enemy_spawn_group(EnemyManager *mgr, EnemyType type,
                       int center_x, int center_y, int count,
                       const struct TileMap *map)
{
    if (!defs_initialized)
        enemy_defs_init();

    for (int i = 0; i < count; i++) {
        /* Find a walkable tile near center */
        int x = center_x + (rand() % 5) - 2;
        int y = center_y + (rand() % 5) - 2;
        x = CLAMP(x, 1, MAP_WIDTH - 2);
        y = CLAMP(y, 1, MAP_HEIGHT - 2);

        /* Search for walkable tile if chosen spot isn't */
        if (!tilemap_is_walkable(map, x, y)) {
            bool found = false;
            for (int dy = -2; dy <= 2 && !found; dy++) {
                for (int dx = -2; dx <= 2 && !found; dx++) {
                    int tx = center_x + dx;
                    int ty = center_y + dy;
                    if (tilemap_is_walkable(map, tx, ty) &&
                        !enemy_at_tile(mgr, tx, ty)) {
                        x = tx;
                        y = ty;
                        found = true;
                    }
                }
            }
            if (!found)
                continue;  /* skip this enemy if no spot found */
        }

        /* Don't stack enemies on same tile */
        if (enemy_at_tile(mgr, x, y))
            continue;

        enemy_spawn(mgr, type, x, y);
        enemy_apply_template(&mgr->enemies[mgr->count - 1]);
    }
}
