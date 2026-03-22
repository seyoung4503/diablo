#include "engine/audio.h"
#include <stdio.h>
#include <string.h>

/* File paths for music tracks */
static const char *music_files[MUSIC_COUNT] = {
    "assets/audio/music/town.mp3",
    "assets/audio/music/dungeon1.mp3",
    "assets/audio/music/dungeon2.mp3",
    "assets/audio/music/dungeon3.mp3",
    "assets/audio/music/dungeon4.mp3",
    "assets/audio/music/title.mp3",
};

/* File paths for sound effects */
static const char *sfx_files[SFX_COUNT] = {
    "assets/audio/sfx/sword_swing.wav",
    "assets/audio/sfx/hit.wav",
    "assets/audio/sfx/player_hit.wav",
    "assets/audio/sfx/enemy_die.wav",
    "assets/audio/sfx/player_die.wav",
    "assets/audio/sfx/footstep.wav",
    "assets/audio/sfx/pickup.wav",
    "assets/audio/sfx/potion.wav",
    "assets/audio/sfx/door_open.wav",
    "assets/audio/sfx/level_up.wav",
    "assets/audio/sfx/quest_complete.wav",
    "assets/audio/sfx/ui_click.wav",
};

bool audio_init(AudioSystem *audio)
{
    memset(audio, 0, sizeof(AudioSystem));
    audio->initialized = false;
    audio->current_music = MUSIC_COUNT; /* none playing */
    audio->music_volume = MIX_MAX_VOLUME;
    audio->sfx_volume = MIX_MAX_VOLUME;

    if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048) < 0) {
        fprintf(stderr, "Audio: Mix_OpenAudio failed: %s\n", Mix_GetError());
        return false;
    }

    audio->initialized = true;

    /* Load music — skip gracefully if files don't exist */
    for (int i = 0; i < MUSIC_COUNT; i++) {
        audio->music[i] = Mix_LoadMUS(music_files[i]);
        if (!audio->music[i]) {
            /* Not an error — files may not exist yet */
        }
    }

    /* Load sound effects — skip gracefully if files don't exist */
    for (int i = 0; i < SFX_COUNT; i++) {
        audio->sfx[i] = Mix_LoadWAV(sfx_files[i]);
        if (!audio->sfx[i]) {
            /* Not an error — files may not exist yet */
        }
    }

    Mix_VolumeMusic(audio->music_volume);

    return true;
}

void audio_shutdown(AudioSystem *audio)
{
    if (!audio->initialized) return;

    Mix_HaltMusic();

    for (int i = 0; i < MUSIC_COUNT; i++) {
        if (audio->music[i]) {
            Mix_FreeMusic(audio->music[i]);
            audio->music[i] = NULL;
        }
    }

    for (int i = 0; i < SFX_COUNT; i++) {
        if (audio->sfx[i]) {
            Mix_FreeChunk(audio->sfx[i]);
            audio->sfx[i] = NULL;
        }
    }

    Mix_CloseAudio();
    audio->initialized = false;
}

void audio_play_music(AudioSystem *audio, MusicTrack track)
{
    if (!audio->initialized) return;
    if (track < 0 || track >= MUSIC_COUNT) return;
    if (!audio->music[track]) return;
    if (audio->current_music == track && Mix_PlayingMusic()) return;

    /* Fade out current track, then fade in new one */
    if (Mix_PlayingMusic()) {
        Mix_FadeOutMusic(500);
    }

    if (Mix_FadeInMusic(audio->music[track], -1, 1000) < 0) {
        fprintf(stderr, "Audio: Failed to play music track %d: %s\n",
                track, Mix_GetError());
        return;
    }

    audio->current_music = track;
}

void audio_stop_music(AudioSystem *audio)
{
    if (!audio->initialized) return;

    Mix_FadeOutMusic(500);
    audio->current_music = MUSIC_COUNT;
}

void audio_play_sfx(AudioSystem *audio, SFXType sfx)
{
    if (!audio->initialized) return;
    if (sfx < 0 || sfx >= SFX_COUNT) return;
    if (!audio->sfx[sfx]) return;

    Mix_PlayChannel(-1, audio->sfx[sfx], 0);
}

void audio_set_music_volume(AudioSystem *audio, int volume)
{
    audio->music_volume = volume < 0 ? 0 : (volume > MIX_MAX_VOLUME ? MIX_MAX_VOLUME : volume);

    if (audio->initialized) {
        Mix_VolumeMusic(audio->music_volume);
    }
}

void audio_set_sfx_volume(AudioSystem *audio, int volume)
{
    audio->sfx_volume = volume < 0 ? 0 : (volume > MIX_MAX_VOLUME ? MIX_MAX_VOLUME : volume);

    if (!audio->initialized) return;

    /* Apply volume to all loaded sfx chunks */
    for (int i = 0; i < SFX_COUNT; i++) {
        if (audio->sfx[i]) {
            Mix_VolumeChunk(audio->sfx[i], audio->sfx_volume);
        }
    }
}
