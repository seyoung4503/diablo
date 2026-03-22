#ifndef DIABLO_ENGINE_AUDIO_H
#define DIABLO_ENGINE_AUDIO_H

#include <SDL_mixer.h>
#include <stdbool.h>

#define MAX_MUSIC 8
#define MAX_SFX   32

typedef enum {
    MUSIC_TOWN,
    MUSIC_DUNGEON_1,    /* cathedral */
    MUSIC_DUNGEON_2,    /* catacombs */
    MUSIC_DUNGEON_3,    /* caves */
    MUSIC_DUNGEON_4,    /* hell */
    MUSIC_TITLE,
    MUSIC_COUNT
} MusicTrack;

typedef enum {
    SFX_SWORD_SWING,
    SFX_HIT,
    SFX_PLAYER_HIT,
    SFX_ENEMY_DIE,
    SFX_PLAYER_DIE,
    SFX_FOOTSTEP,
    SFX_PICKUP,
    SFX_POTION,
    SFX_DOOR_OPEN,
    SFX_LEVEL_UP,
    SFX_QUEST_COMPLETE,
    SFX_UI_CLICK,
    SFX_COUNT
} SFXType;

typedef struct AudioSystem {
    bool initialized;
    Mix_Music *music[MUSIC_COUNT];
    Mix_Chunk *sfx[SFX_COUNT];
    MusicTrack current_music;
    int music_volume;   /* 0-128 */
    int sfx_volume;     /* 0-128 */
} AudioSystem;

bool audio_init(AudioSystem *audio);
void audio_shutdown(AudioSystem *audio);
void audio_play_music(AudioSystem *audio, MusicTrack track);
void audio_stop_music(AudioSystem *audio);
void audio_play_sfx(AudioSystem *audio, SFXType sfx);
void audio_set_music_volume(AudioSystem *audio, int volume);
void audio_set_sfx_volume(AudioSystem *audio, int volume);

#endif /* DIABLO_ENGINE_AUDIO_H */
