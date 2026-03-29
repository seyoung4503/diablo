#ifndef DIABLO_ENEMY_ENEMY_H
#define DIABLO_ENEMY_ENEMY_H

#include "common.h"
#include "engine/animation.h"
#include "world/pathfinding.h"

#define MAX_ENEMIES_PER_LEVEL 32

/* Forward declarations */
struct TileMap;

typedef enum {
    ENEMY_NONE = 0,
    ENEMY_FALLEN,      /* weak melee, in groups */
    ENEMY_SKELETON,    /* medium melee, can be archers */
    ENEMY_ZOMBIE,      /* slow, tanky */
    ENEMY_SCAVENGER,   /* fast, weak, flees when hurt */
    ENEMY_HIDDEN,      /* ambush from darkness */
    ENEMY_GOAT_MAN,    /* medium melee, charges */
    ENEMY_ACID_DOG,    /* fast, poison effect */
    ENEMY_MAGE,        /* ranged magic caster */
    ENEMY_KNIGHT,      /* heavy armor, slow */
    ENEMY_BALROG,      /* boss-tier, rare */
    ENEMY_TYPE_COUNT
} EnemyType;

typedef enum {
    ENEMY_STATE_IDLE,
    ENEMY_STATE_PATROL,
    ENEMY_STATE_CHASE,
    ENEMY_STATE_ATTACK,
    ENEMY_STATE_FLEE,
    ENEMY_STATE_DEAD
} EnemyState;

typedef struct Enemy {
    uint32_t id;
    EnemyType type;
    char name[32];
    EnemyState state;

    /* Position */
    int tile_x, tile_y;
    float world_x, world_y;
    Direction facing;

    /* Movement */
    Path path;
    bool moving;
    int move_from_x, move_from_y;
    int move_to_x, move_to_y;
    float move_timer;
    float move_speed;

    /* Stats */
    int max_hp, current_hp;
    int armor;
    int damage_min, damage_max;
    int to_hit;
    int xp_value;
    int detection_range;   /* tiles */
    int attack_range;      /* 1 for melee */
    float attack_cooldown;
    float attack_timer;

    /* Loot */
    int loot_table_id;
    int gold_min, gold_max;

    bool alive;
    bool is_ranged;         /* uses projectile attacks instead of melee */

    /* Animation */
    AnimController anim;
    int sprite_sheet_id;    /* index into SpriteSheetManager, -1 = none */
} Enemy;

typedef struct EnemyManager {
    Enemy enemies[MAX_ENEMIES_PER_LEVEL];
    int count;
    uint32_t next_id;
} EnemyManager;

/* Initialize enemy manager (clear all enemies) */
void enemy_manager_init(EnemyManager *mgr);

/* Update all enemies (movement interpolation) */
void enemy_manager_update(EnemyManager *mgr, float dt, const struct TileMap *map);

/* Spawn a single enemy at the given tile */
void enemy_spawn(EnemyManager *mgr, EnemyType type, int x, int y);

/* Get enemy at tile, or NULL if none */
Enemy *enemy_at_tile(EnemyManager *mgr, int x, int y);

/* Apply damage to enemy, kill if HP <= 0 */
void enemy_take_damage(Enemy *enemy, int damage);

/* Count living enemies */
int enemy_count_alive(const EnemyManager *mgr);

/* Initialize enemy type templates (call once at startup) */
void enemy_defs_init(void);

/* Apply template stats (HP, damage, etc.) to enemy based on its type */
void enemy_apply_template(Enemy *enemy);

/* Spawn a group of enemies around a center point */
void enemy_spawn_group(EnemyManager *mgr, EnemyType type,
                       int center_x, int center_y, int count,
                       const struct TileMap *map);

#endif /* DIABLO_ENEMY_ENEMY_H */
