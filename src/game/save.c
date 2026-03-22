#include "game/save.h"
#include "game/game.h"
#include "game/player.h"
#include "game/stats.h"
#include "game/inventory.h"
#include "game/item.h"
#include "npc/npc.h"
#include "npc/npc_relationship.h"
#include "npc/npc_memory.h"
#include "story/quest.h"

#include <stdio.h>
#include <string.h>

/* ---------- checksum ---------- */

static uint32_t compute_checksum(const SaveData *data)
{
    const uint8_t *bytes = (const uint8_t *)data;
    uint32_t chk = 0;
    for (size_t i = 0; i < sizeof(SaveData); i++) {
        chk ^= ((uint32_t)bytes[i]) << (8 * (i % 4));
    }
    return chk;
}

/* ---------- file I/O ---------- */

bool save_game(const char *path, const SaveData *data)
{
    if (!path || !data) return false;

    FILE *fp = fopen(path, "wb");
    if (!fp) {
        fprintf(stderr, "save_game: cannot open '%s' for writing\n", path);
        return false;
    }

    SaveHeader header;
    header.magic    = SAVE_MAGIC;
    header.version  = SAVE_VERSION;
    header.checksum = compute_checksum(data);

    bool ok = true;
    if (fwrite(&header, sizeof(header), 1, fp) != 1) ok = false;
    if (ok && fwrite(data, sizeof(SaveData), 1, fp) != 1) ok = false;

    fclose(fp);

    if (!ok) {
        fprintf(stderr, "save_game: write error\n");
    }
    return ok;
}

bool load_game(const char *path, SaveData *data)
{
    if (!path || !data) return false;

    FILE *fp = fopen(path, "rb");
    if (!fp) {
        fprintf(stderr, "load_game: cannot open '%s'\n", path);
        return false;
    }

    SaveHeader header;
    if (fread(&header, sizeof(header), 1, fp) != 1) {
        fprintf(stderr, "load_game: failed to read header\n");
        fclose(fp);
        return false;
    }

    if (header.magic != SAVE_MAGIC) {
        fprintf(stderr, "load_game: bad magic 0x%08X\n", header.magic);
        fclose(fp);
        return false;
    }

    if (header.version != SAVE_VERSION) {
        fprintf(stderr, "load_game: unsupported version %u\n", header.version);
        fclose(fp);
        return false;
    }

    if (fread(data, sizeof(SaveData), 1, fp) != 1) {
        fprintf(stderr, "load_game: failed to read data\n");
        fclose(fp);
        return false;
    }

    fclose(fp);

    uint32_t chk = compute_checksum(data);
    if (chk != header.checksum) {
        fprintf(stderr, "load_game: checksum mismatch (got 0x%08X, expected 0x%08X)\n",
                chk, header.checksum);
        return false;
    }

    return true;
}

bool save_exists(const char *path)
{
    if (!path) return false;
    FILE *fp = fopen(path, "rb");
    if (!fp) return false;
    fclose(fp);
    return true;
}

/* ---------- pack helpers ---------- */

static void pack_item(SavedItem *dst, const Item *src)
{
    dst->id            = src->id;
    memcpy(dst->name, src->name, sizeof(dst->name));
    dst->type          = (int)src->type;
    dst->rarity        = (int)src->rarity;
    dst->damage_min    = src->damage_min;
    dst->damage_max    = src->damage_max;
    dst->armor_value   = src->armor_value;
    dst->str_bonus     = src->str_bonus;
    dst->dex_bonus     = src->dex_bonus;
    dst->mag_bonus     = src->mag_bonus;
    dst->vit_bonus     = src->vit_bonus;
    dst->hp_bonus      = src->hp_bonus;
    dst->mana_bonus    = src->mana_bonus;
    dst->value         = src->value;
    dst->stackable     = src->stackable;
    dst->stack_count   = src->stack_count;
    dst->required_level = src->required_level;
}

static void unpack_item(Item *dst, const SavedItem *src)
{
    dst->id            = src->id;
    memcpy(dst->name, src->name, sizeof(dst->name));
    dst->type          = (ItemType)src->type;
    dst->rarity        = (ItemRarity)src->rarity;
    dst->damage_min    = src->damage_min;
    dst->damage_max    = src->damage_max;
    dst->armor_value   = src->armor_value;
    dst->str_bonus     = src->str_bonus;
    dst->dex_bonus     = src->dex_bonus;
    dst->mag_bonus     = src->mag_bonus;
    dst->vit_bonus     = src->vit_bonus;
    dst->hp_bonus      = src->hp_bonus;
    dst->mana_bonus    = src->mana_bonus;
    dst->value         = src->value;
    dst->stackable     = src->stackable;
    dst->stack_count   = src->stack_count;
    dst->required_level = src->required_level;
}

/* ---------- save_pack ---------- */

void save_pack(SaveData *data, const Game *game,
               const Player *player, const Inventory *inv,
               const NPCManager *npcs,
               const RelationshipGraph *graph,
               const MemorySystem *memory,
               const QuestLog *quests,
               const int *flags, int flag_count,
               int scene, int dlevel)
{
    memset(data, 0, sizeof(SaveData));

    /* Game clock */
    data->game_day      = game->game_day;
    data->game_hour     = game->game_hour;
    data->game_time_acc = game->game_time_acc;
    data->current_scene = scene;
    data->dungeon_level = dlevel;

    /* Player */
    data->player_tile_x = player->tile_x;
    data->player_tile_y = player->tile_y;
    data->player_facing = (int)player->facing;

    /* Stats */
    const PlayerStats *s = &player->stats;
    data->stat_level       = s->level;
    data->stat_xp          = s->xp;
    data->stat_xp_to_next  = s->xp_to_next;
    data->stat_points      = s->stat_points;
    data->stat_strength    = s->strength;
    data->stat_dexterity   = s->dexterity;
    data->stat_magic       = s->magic;
    data->stat_vitality    = s->vitality;
    data->stat_max_hp      = s->max_hp;
    data->stat_current_hp  = s->current_hp;
    data->stat_max_mana    = s->max_mana;
    data->stat_current_mana = s->current_mana;
    data->stat_armor_class = s->armor_class;
    data->stat_to_hit_bonus = s->to_hit_bonus;
    data->stat_min_damage  = s->min_damage;
    data->stat_max_damage  = s->max_damage;

    /* Inventory */
    data->inventory_gold = inv->gold;
    for (int i = 0; i < SAVE_INV_SLOTS && i < INVENTORY_SIZE; i++) {
        pack_item(&data->inv_slots[i], &inv->slots[i]);
    }
    for (int i = 0; i < SAVE_EQUIP_SLOTS && i < EQUIP_SLOT_COUNT; i++) {
        pack_item(&data->inv_equipped[i], &inv->equipped[i]);
    }

    /* NPCs */
    int nc = npcs->count < SAVE_MAX_NPCS ? npcs->count : SAVE_MAX_NPCS;
    data->npc_count = nc;
    for (int i = 0; i < nc; i++) {
        const NPC *n = &npcs->npcs[i];
        data->npc_states[i].mood          = n->mood;
        data->npc_states[i].stress        = n->stress;
        data->npc_states[i].trust_player  = n->trust_player;
        data->npc_states[i].tile_x        = n->tile_x;
        data->npc_states[i].tile_y        = n->tile_y;
        data->npc_states[i].arc_stage     = n->arc_stage;
        data->npc_states[i].is_alive      = n->is_alive;
        data->npc_states[i].has_left_town = n->has_left_town;
        data->npc_states[i].need_safety   = n->need_safety;
        data->npc_states[i].need_social   = n->need_social;
        data->npc_states[i].need_economic = n->need_economic;
        data->npc_states[i].need_purpose  = n->need_purpose;
    }

    /* Relationships */
    for (int i = 0; i < SAVE_MAX_NPCS; i++) {
        for (int j = 0; j < SAVE_MAX_NPCS; j++) {
            const Relationship *r = &graph->matrix[i][j];
            data->relationships[i][j].affection      = r->affection;
            data->relationships[i][j].trust           = r->trust;
            data->relationships[i][j].respect         = r->respect;
            data->relationships[i][j].type            = (int)r->type;
            data->relationships[i][j].shared_history  = r->shared_history;
        }
    }

    /* Memories */
    for (int i = 0; i < SAVE_MAX_NPCS; i++) {
        const MemoryBank *bank = &memory->banks[i];
        data->memory_banks[i].count       = bank->count;
        data->memory_banks[i].write_index = bank->write_index;
        int mc = bank->count < MAX_MEMORIES_PER_NPC ? bank->count : MAX_MEMORIES_PER_NPC;
        for (int m = 0; m < mc; m++) {
            const NPCMemory *mem = &bank->memories[m];
            data->memory_banks[i].memories[m].event_type        = (int)mem->event_type;
            data->memory_banks[i].memories[m].subject_id        = mem->subject_id;
            data->memory_banks[i].memories[m].object_id         = mem->object_id;
            data->memory_banks[i].memories[m].game_day          = mem->game_day;
            data->memory_banks[i].memories[m].emotional_weight  = mem->emotional_weight;
            data->memory_banks[i].memories[m].salience          = mem->salience;
        }
    }

    /* Quests */
    int qc = quests->count < 32 ? quests->count : 32;
    data->quest_count = qc;
    for (int i = 0; i < qc; i++) {
        const Quest *q = &quests->quests[i];
        data->quests[i].id             = q->id;
        memcpy(data->quests[i].name, q->name, sizeof(data->quests[i].name));
        memcpy(data->quests[i].description, q->description, sizeof(data->quests[i].description));
        data->quests[i].state          = (int)q->state;
        data->quests[i].giver_npc_id   = q->giver_npc_id;
        data->quests[i].objective_count = q->objective_count;
        for (int o = 0; o < q->objective_count && o < MAX_OBJECTIVES; o++) {
            data->quests[i].objectives[o].type      = (int)q->objectives[o].type;
            data->quests[i].objectives[o].param1    = q->objectives[o].param1;
            data->quests[i].objectives[o].param2    = q->objectives[o].param2;
            memcpy(data->quests[i].objectives[o].description,
                   q->objectives[o].description,
                   sizeof(data->quests[i].objectives[o].description));
            data->quests[i].objectives[o].completed = q->objectives[o].completed;
        }
        data->quests[i].outcome_count  = q->outcome_count;
        for (int o = 0; o < q->outcome_count && o < MAX_OUTCOMES; o++) {
            memcpy(data->quests[i].outcomes[o].description,
                   q->outcomes[o].description,
                   sizeof(data->quests[i].outcomes[o].description));
            data->quests[i].outcomes[o].triggered_flag = q->outcomes[o].triggered_flag;
            memcpy(data->quests[i].outcomes[o].world_events,
                   q->outcomes[o].world_events,
                   sizeof(data->quests[i].outcomes[o].world_events));
            memcpy(data->quests[i].outcomes[o].event_targets,
                   q->outcomes[o].event_targets,
                   sizeof(data->quests[i].outcomes[o].event_targets));
            memcpy(data->quests[i].outcomes[o].event_magnitudes,
                   q->outcomes[o].event_magnitudes,
                   sizeof(data->quests[i].outcomes[o].event_magnitudes));
            data->quests[i].outcomes[o].event_count  = q->outcomes[o].event_count;
            data->quests[i].outcomes[o].xp_reward    = q->outcomes[o].xp_reward;
            data->quests[i].outcomes[o].gold_reward   = q->outcomes[o].gold_reward;
        }
        data->quests[i].chosen_outcome = q->chosen_outcome;
        data->quests[i].deadline_day   = q->deadline_day;
    }

    /* Flags */
    int fc = flag_count < SAVE_MAX_FLAGS ? flag_count : SAVE_MAX_FLAGS;
    data->flag_count = fc;
    if (flags && fc > 0) {
        memcpy(data->flags, flags, sizeof(int) * fc);
    }
}

/* ---------- save_unpack ---------- */

void save_unpack(const SaveData *data, Game *game,
                 Player *player, Inventory *inv,
                 NPCManager *npcs,
                 RelationshipGraph *graph,
                 MemorySystem *memory,
                 QuestLog *quests,
                 int *flags, int *flag_count,
                 int *scene, int *dlevel)
{
    /* Game clock */
    game->game_day      = data->game_day;
    game->game_hour     = data->game_hour;
    game->game_time_acc = data->game_time_acc;
    *scene  = data->current_scene;
    *dlevel = data->dungeon_level;

    /* Player */
    player->tile_x = data->player_tile_x;
    player->tile_y = data->player_tile_y;
    player->facing = (Direction)data->player_facing;
    player->moving = false;

    /* Recalculate world position from tile */
    int sx, sy;
    iso_to_screen(player->tile_x, player->tile_y, &sx, &sy);
    player->world_x = (float)sx;
    player->world_y = (float)sy;

    /* Stats */
    PlayerStats *s = &player->stats;
    s->level        = data->stat_level;
    s->xp           = data->stat_xp;
    s->xp_to_next   = data->stat_xp_to_next;
    s->stat_points  = data->stat_points;
    s->strength     = data->stat_strength;
    s->dexterity    = data->stat_dexterity;
    s->magic        = data->stat_magic;
    s->vitality     = data->stat_vitality;
    s->max_hp       = data->stat_max_hp;
    s->current_hp   = data->stat_current_hp;
    s->max_mana     = data->stat_max_mana;
    s->current_mana = data->stat_current_mana;
    s->armor_class  = data->stat_armor_class;
    s->to_hit_bonus = data->stat_to_hit_bonus;
    s->min_damage   = data->stat_min_damage;
    s->max_damage   = data->stat_max_damage;
    stats_recalculate(s);

    /* Inventory */
    inventory_init(inv);
    inv->gold = data->inventory_gold;
    for (int i = 0; i < SAVE_INV_SLOTS && i < INVENTORY_SIZE; i++) {
        unpack_item(&inv->slots[i], &data->inv_slots[i]);
    }
    for (int i = 0; i < SAVE_EQUIP_SLOTS && i < EQUIP_SLOT_COUNT; i++) {
        unpack_item(&inv->equipped[i], &data->inv_equipped[i]);
    }

    /* NPCs — restore mutable state; immutable definitions are re-inited elsewhere */
    int nc = data->npc_count < SAVE_MAX_NPCS ? data->npc_count : SAVE_MAX_NPCS;
    for (int i = 0; i < nc && i < npcs->count; i++) {
        NPC *n = &npcs->npcs[i];
        n->mood          = data->npc_states[i].mood;
        n->stress        = data->npc_states[i].stress;
        n->trust_player  = data->npc_states[i].trust_player;
        n->tile_x        = data->npc_states[i].tile_x;
        n->tile_y        = data->npc_states[i].tile_y;
        n->arc_stage     = data->npc_states[i].arc_stage;
        n->is_alive      = data->npc_states[i].is_alive;
        n->has_left_town = data->npc_states[i].has_left_town;
        n->need_safety   = data->npc_states[i].need_safety;
        n->need_social   = data->npc_states[i].need_social;
        n->need_economic = data->npc_states[i].need_economic;
        n->need_purpose  = data->npc_states[i].need_purpose;
        n->moving = false;

        /* Recalculate NPC world position */
        iso_to_screen(n->tile_x, n->tile_y, &sx, &sy);
        n->world_x = (float)sx;
        n->world_y = (float)sy;
    }

    /* Relationships */
    for (int i = 0; i < SAVE_MAX_NPCS && i < MAX_TOWN_NPCS; i++) {
        for (int j = 0; j < SAVE_MAX_NPCS && j < MAX_TOWN_NPCS; j++) {
            Relationship *r = &graph->matrix[i][j];
            r->affection      = data->relationships[i][j].affection;
            r->trust           = data->relationships[i][j].trust;
            r->respect         = data->relationships[i][j].respect;
            r->type            = (RelationType)data->relationships[i][j].type;
            r->shared_history  = data->relationships[i][j].shared_history;
        }
    }

    /* Memories */
    for (int i = 0; i < SAVE_MAX_NPCS && i < MAX_TOWN_NPCS; i++) {
        MemoryBank *bank = &memory->banks[i];
        bank->count       = data->memory_banks[i].count;
        bank->write_index = data->memory_banks[i].write_index;
        int mc = bank->count < MAX_MEMORIES_PER_NPC ? bank->count : MAX_MEMORIES_PER_NPC;
        for (int m = 0; m < mc; m++) {
            NPCMemory *mem = &bank->memories[m];
            mem->event_type       = (MemoryEventType)data->memory_banks[i].memories[m].event_type;
            mem->subject_id       = data->memory_banks[i].memories[m].subject_id;
            mem->object_id        = data->memory_banks[i].memories[m].object_id;
            mem->game_day         = data->memory_banks[i].memories[m].game_day;
            mem->emotional_weight = data->memory_banks[i].memories[m].emotional_weight;
            mem->salience         = data->memory_banks[i].memories[m].salience;
        }
    }

    /* Quests */
    int qc = data->quest_count < MAX_QUESTS ? data->quest_count : MAX_QUESTS;
    quests->count = qc;
    for (int i = 0; i < qc; i++) {
        Quest *q = &quests->quests[i];
        q->id = data->quests[i].id;
        memcpy(q->name, data->quests[i].name, sizeof(q->name));
        memcpy(q->description, data->quests[i].description, sizeof(q->description));
        q->state          = (QuestState)data->quests[i].state;
        q->giver_npc_id   = data->quests[i].giver_npc_id;
        q->objective_count = data->quests[i].objective_count;
        for (int o = 0; o < q->objective_count && o < MAX_OBJECTIVES; o++) {
            q->objectives[o].type      = (ObjectiveType)data->quests[i].objectives[o].type;
            q->objectives[o].param1    = data->quests[i].objectives[o].param1;
            q->objectives[o].param2    = data->quests[i].objectives[o].param2;
            memcpy(q->objectives[o].description,
                   data->quests[i].objectives[o].description,
                   sizeof(q->objectives[o].description));
            q->objectives[o].completed = data->quests[i].objectives[o].completed;
        }
        q->outcome_count = data->quests[i].outcome_count;
        for (int o = 0; o < q->outcome_count && o < MAX_OUTCOMES; o++) {
            memcpy(q->outcomes[o].description,
                   data->quests[i].outcomes[o].description,
                   sizeof(q->outcomes[o].description));
            q->outcomes[o].triggered_flag = data->quests[i].outcomes[o].triggered_flag;
            memcpy(q->outcomes[o].world_events,
                   data->quests[i].outcomes[o].world_events,
                   sizeof(q->outcomes[o].world_events));
            memcpy(q->outcomes[o].event_targets,
                   data->quests[i].outcomes[o].event_targets,
                   sizeof(q->outcomes[o].event_targets));
            memcpy(q->outcomes[o].event_magnitudes,
                   data->quests[i].outcomes[o].event_magnitudes,
                   sizeof(q->outcomes[o].event_magnitudes));
            q->outcomes[o].event_count  = data->quests[i].outcomes[o].event_count;
            q->outcomes[o].xp_reward    = data->quests[i].outcomes[o].xp_reward;
            q->outcomes[o].gold_reward  = data->quests[i].outcomes[o].gold_reward;
        }
        q->chosen_outcome = data->quests[i].chosen_outcome;
        q->deadline_day   = data->quests[i].deadline_day;
    }

    /* Flags */
    int fc = data->flag_count < SAVE_MAX_FLAGS ? data->flag_count : SAVE_MAX_FLAGS;
    *flag_count = fc;
    if (flags && fc > 0) {
        memcpy(flags, data->flags, sizeof(int) * fc);
    }
}
