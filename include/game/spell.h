#ifndef DIABLO_GAME_SPELL_H
#define DIABLO_GAME_SPELL_H

#include "common.h"

typedef enum {
    SPELL_NONE = 0,
    SPELL_FIREBALL,
    SPELL_HEAL,
    SPELL_LIGHTNING,
    SPELL_TYPE_COUNT
} SpellType;

typedef struct Spell {
    SpellType type;
    const char *name;
    int mana_cost;
    int damage_min, damage_max;
    int range;              /* tiles, 0 = self */
    float cooldown;
    float cast_time;
    int required_magic;
    SDL_Color color;        /* visual color for projectile/effect */
} Spell;

typedef struct SpellBook {
    bool known[SPELL_TYPE_COUNT];
    float cooldowns[SPELL_TYPE_COUNT];
} SpellBook;

void spell_init_data(void);
const Spell *spell_get(SpellType type);

void spellbook_init(SpellBook *sb);
void spellbook_update(SpellBook *sb, float dt);
bool spellbook_can_cast(const SpellBook *sb, SpellType type, int current_mana, int magic_stat);
void spellbook_start_cooldown(SpellBook *sb, SpellType type);

/* Calculate spell damage based on magic stat */
int spell_calc_damage(SpellType type, int magic_stat);

#endif
