#include "game/spell.h"

static Spell spell_data[SPELL_TYPE_COUNT];
static bool spell_data_initialized = false;

void spell_init_data(void)
{
    if (spell_data_initialized) return;

    spell_data[SPELL_NONE] = (Spell){SPELL_NONE, "None", 0, 0, 0, 0, 0, 0, 0, {0,0,0,0}};

    spell_data[SPELL_FIREBALL] = (Spell){
        .type = SPELL_FIREBALL,
        .name = "Fireball",
        .mana_cost = 8,
        .damage_min = 6, .damage_max = 12,
        .range = 8,
        .cooldown = 1.5f,
        .cast_time = 0.4f,
        .required_magic = 15,
        .color = {255, 100, 20, 255}
    };

    spell_data[SPELL_HEAL] = (Spell){
        .type = SPELL_HEAL,
        .name = "Heal",
        .mana_cost = 10,
        .damage_min = 15, .damage_max = 25,  /* heal amount */
        .range = 0,  /* self */
        .cooldown = 3.0f,
        .cast_time = 0.5f,
        .required_magic = 10,
        .color = {80, 255, 80, 255}
    };

    spell_data[SPELL_LIGHTNING] = (Spell){
        .type = SPELL_LIGHTNING,
        .name = "Lightning",
        .mana_cost = 12,
        .damage_min = 8, .damage_max = 16,
        .range = 6,
        .cooldown = 2.0f,
        .cast_time = 0.3f,
        .required_magic = 20,
        .color = {100, 150, 255, 255}
    };

    spell_data_initialized = true;
}

const Spell *spell_get(SpellType type)
{
    if (!spell_data_initialized) spell_init_data();
    if (type < 0 || type >= SPELL_TYPE_COUNT) return NULL;
    return &spell_data[type];
}

void spellbook_init(SpellBook *sb)
{
    memset(sb, 0, sizeof(*sb));
    /* Player starts knowing Heal */
    sb->known[SPELL_HEAL] = true;
}

void spellbook_update(SpellBook *sb, float dt)
{
    for (int i = 0; i < SPELL_TYPE_COUNT; i++) {
        if (sb->cooldowns[i] > 0)
            sb->cooldowns[i] -= dt;
    }
}

bool spellbook_can_cast(const SpellBook *sb, SpellType type, int current_mana, int magic_stat)
{
    if (type <= SPELL_NONE || type >= SPELL_TYPE_COUNT) return false;
    if (!sb->known[type]) return false;
    if (sb->cooldowns[type] > 0) return false;
    const Spell *sp = spell_get(type);
    if (!sp) return false;
    if (current_mana < sp->mana_cost) return false;
    if (magic_stat < sp->required_magic) return false;
    return true;
}

void spellbook_start_cooldown(SpellBook *sb, SpellType type)
{
    const Spell *sp = spell_get(type);
    if (sp) sb->cooldowns[type] = sp->cooldown;
}

int spell_calc_damage(SpellType type, int magic_stat)
{
    const Spell *sp = spell_get(type);
    if (!sp) return 0;
    int bonus = magic_stat / 5;
    int dmg_min = sp->damage_min + bonus;
    int dmg_max = sp->damage_max + bonus;
    if (dmg_min > dmg_max) dmg_max = dmg_min;
    return dmg_min + rand() % (dmg_max - dmg_min + 1);
}
