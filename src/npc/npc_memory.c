#include "npc/npc_memory.h"
#include <string.h>
#include <math.h>

void memory_system_init(MemorySystem *sys)
{
    memset(sys, 0, sizeof(*sys));
}

void memory_add(MemorySystem *sys, int npc_id, MemoryEventType type,
                int subject_id, int object_id, int game_day,
                float emotional_weight)
{
    if (npc_id < 0 || npc_id >= MAX_TOWN_NPCS) return;

    MemoryBank *bank = &sys->banks[npc_id];
    NPCMemory *mem = &bank->memories[bank->write_index];

    mem->event_type       = type;
    mem->subject_id       = subject_id;
    mem->object_id        = object_id;
    mem->game_day         = game_day;
    mem->emotional_weight = CLAMP(emotional_weight, -1.0f, 1.0f);
    mem->salience         = 1.0f; /* fresh memories are fully vivid */

    bank->write_index = (bank->write_index + 1) % MAX_MEMORIES_PER_NPC;
    if (bank->count < MAX_MEMORIES_PER_NPC)
        bank->count++;
}

void memory_decay(MemorySystem *sys, int game_day)
{
    (void)game_day;

    for (int n = 0; n < MAX_TOWN_NPCS; n++) {
        MemoryBank *bank = &sys->banks[n];
        for (int i = 0; i < bank->count; i++) {
            NPCMemory *mem = &bank->memories[i];
            /* Decay salience by 5% per tick */
            mem->salience *= 0.95f;
            /* Floor: salience never drops below 30% of emotional weight magnitude */
            float floor = fabsf(mem->emotional_weight) * 0.3f;
            if (mem->salience < floor)
                mem->salience = floor;
        }
    }
}

int memory_count_about(const MemorySystem *sys, int npc_id, int subject_id)
{
    if (npc_id < 0 || npc_id >= MAX_TOWN_NPCS) return 0;

    const MemoryBank *bank = &sys->banks[npc_id];
    int count = 0;
    for (int i = 0; i < bank->count; i++) {
        if (bank->memories[i].subject_id == subject_id)
            count++;
    }
    return count;
}

float memory_sentiment_about(const MemorySystem *sys, int npc_id, int subject_id)
{
    if (npc_id < 0 || npc_id >= MAX_TOWN_NPCS) return 0.0f;

    const MemoryBank *bank = &sys->banks[npc_id];
    float total_weight = 0.0f;
    float total_salience = 0.0f;

    for (int i = 0; i < bank->count; i++) {
        const NPCMemory *mem = &bank->memories[i];
        if (mem->subject_id == subject_id) {
            /* Weight sentiment by salience so vivid memories matter more */
            total_weight += mem->emotional_weight * mem->salience;
            total_salience += mem->salience;
        }
    }

    if (total_salience < 0.001f) return 0.0f;
    return total_weight / total_salience;
}
