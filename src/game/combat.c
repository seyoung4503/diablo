#include "game/combat.h"
#include "game/player.h"
#include "game/stats.h"
#include "game/inventory.h"
#include "enemy/enemy.h"

/* Random integer in [min, max] inclusive */
static int rand_range(int min, int max)
{
    if (min >= max)
        return min;
    return min + rand() % (max - min + 1);
}

bool combat_in_range(int ax, int ay, int bx, int by, int range)
{
    int dx = abs(ax - bx);
    int dy = abs(ay - by);
    return MAX(dx, dy) <= range;
}

CombatResult combat_player_attack(Player *player, Enemy *enemy,
                                  const Inventory *inv)
{
    CombatResult result = {false, 0, false, 0};

    if (!player || !enemy || !enemy->alive)
        return result;

    /* Get weapon damage from equipped weapon + stat bonuses */
    int str_bonus = 0, dex_bonus = 0, mag_bonus = 0, vit_bonus = 0;
    int hp_bonus = 0, mana_bonus = 0, armor_bonus = 0;
    int weapon_dmg_min = 0, weapon_dmg_max = 0;
    inventory_calc_bonuses(inv, &str_bonus, &dex_bonus, &mag_bonus,
                           &vit_bonus, &hp_bonus, &mana_bonus,
                           &armor_bonus, &weapon_dmg_min, &weapon_dmg_max);

    /* Use base stats if no weapon equipped */
    if (weapon_dmg_max == 0) {
        weapon_dmg_min = 1;
        weapon_dmg_max = 3;
    }

    int total_str = player->stats.strength + str_bonus;

    /* Hit chance: min(95, max(5, 50 + to_hit - enemy.armor * 2))
     * to_hit = base dexterity + equipment dex bonus (the 50 base is in the formula) */
    int to_hit = player->stats.dexterity + dex_bonus;
    int hit_chance = 50 + to_hit - enemy->armor * 2;
    hit_chance = CLAMP(hit_chance, 5, 95);

    int roll = rand() % 100;
    if (roll < hit_chance) {
        result.hit = true;

        /* Damage: rand_range(weapon_min + str/5, weapon_max + str/2) */
        int dmg_min = weapon_dmg_min + total_str / 5;
        int dmg_max = weapon_dmg_max + total_str / 2;
        result.damage = rand_range(dmg_min, dmg_max);
        if (result.damage < 1)
            result.damage = 1;

        enemy_take_damage(enemy, result.damage);

        if (!enemy->alive) {
            result.killed = true;
            result.xp_gained = enemy->xp_value;
            stats_add_xp(&player->stats, enemy->xp_value);
        }
    }

    return result;
}

CombatResult combat_enemy_attack(Enemy *enemy, Player *player,
                                 const Inventory *inv)
{
    CombatResult result = {false, 0, false, 0};

    if (!player || !enemy || !enemy->alive)
        return result;

    /* Total armor = base armor_class (from dex) + equipped armor bonuses */
    int equip_armor = 0;
    if (inv) {
        int s, d, m, v, h, mn, dmn, dmx;
        inventory_calc_bonuses(inv, &s, &d, &m, &v, &h, &mn,
                               &equip_armor, &dmn, &dmx);
    }
    int total_armor = player->stats.armor_class + equip_armor;

    /* Hit chance: min(90, max(10, enemy.to_hit - total_armor)) */
    int hit_chance = enemy->to_hit - total_armor;
    hit_chance = CLAMP(hit_chance, 10, 90);

    int roll = rand() % 100;
    if (roll < hit_chance) {
        result.hit = true;
        result.damage = rand_range(enemy->damage_min, enemy->damage_max);
        if (result.damage < 1)
            result.damage = 1;

        player->stats.current_hp -= result.damage;
        if (player->stats.current_hp <= 0) {
            player->stats.current_hp = 0;
            result.killed = true;
        }
    }

    return result;
}

bool combat_enemy_is_ranged(const Enemy *enemy)
{
    return enemy->is_ranged;
}

void combat_update(EnemyManager *enemies, Player *player,
                   const Inventory *inv, float dt)
{
    (void)dt;

    /* Don't process attacks if player is dead */
    if (player->stats.current_hp <= 0)
        return;

    for (int i = 0; i < enemies->count; i++) {
        Enemy *e = &enemies->enemies[i];
        if (!e->alive || e->state != ENEMY_STATE_ATTACK)
            continue;

        /* Check range */
        if (!combat_in_range(e->tile_x, e->tile_y,
                             player->tile_x, player->tile_y,
                             e->attack_range))
            continue;

        /* Attack on cooldown */
        if (e->attack_timer <= 0.0f) {
            combat_enemy_attack(e, player, inv);
            e->attack_timer = e->attack_cooldown;

            /* Stop if player died this frame */
            if (player->stats.current_hp <= 0)
                return;
        }
    }
}
