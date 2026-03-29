#!/usr/bin/env python3
"""Generate placeholder WAV sound effects for Diablo 1 clone.

Uses only stdlib modules (wave, struct, math, random).
Produces 16-bit mono WAV files at 22050 Hz.
"""

import wave, struct, math, random, os

SAMPLE_RATE = 22050
CHANNELS = 1
SAMPLE_WIDTH = 2  # 16-bit

BASE_DIR = os.path.join(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
SFX_DIR = os.path.join(BASE_DIR, "assets", "audio", "sfx")
MUSIC_DIR = os.path.join(BASE_DIR, "assets", "audio", "music")


def write_wav(filename, samples, sample_rate=SAMPLE_RATE):
    """Write list of float samples (-1.0 to 1.0) to WAV file."""
    os.makedirs(os.path.dirname(filename), exist_ok=True)
    with wave.open(filename, 'w') as f:
        f.setnchannels(CHANNELS)
        f.setsampwidth(SAMPLE_WIDTH)
        f.setframerate(sample_rate)
        data = b''
        for s in samples:
            s = max(-1.0, min(1.0, s))
            data += struct.pack('<h', int(s * 32767))
        f.writeframes(data)
    print(f"  Created: {filename} ({len(samples)} samples, {len(samples)/sample_rate:.2f}s)")


def gen_sine(freq, duration, volume=0.5):
    """Generate sine wave samples."""
    n = int(SAMPLE_RATE * duration)
    return [volume * math.sin(2 * math.pi * freq * i / SAMPLE_RATE) for i in range(n)]


def gen_noise(duration, volume=0.3):
    """Generate white noise samples."""
    n = int(SAMPLE_RATE * duration)
    return [volume * (random.random() * 2 - 1) for _ in range(n)]


def apply_envelope(samples, attack=0.01, decay=0.1):
    """Apply attack-decay envelope."""
    n = len(samples)
    if n == 0:
        return samples
    attack_n = max(1, int(SAMPLE_RATE * attack))
    decay_n = max(1, int(SAMPLE_RATE * decay))
    decay_start = n - decay_n
    for i in range(n):
        if i < attack_n:
            samples[i] *= i / attack_n
        elif i > decay_start and decay_start > 0:
            samples[i] *= (n - i) / decay_n
    return samples


def mix(a, b):
    """Mix two sample lists together (add and clip)."""
    length = max(len(a), len(b))
    result = [0.0] * length
    for i in range(length):
        va = a[i] if i < len(a) else 0.0
        vb = b[i] if i < len(b) else 0.0
        result[i] = max(-1.0, min(1.0, va + vb))
    return result


def concat(*parts):
    """Concatenate multiple sample lists."""
    result = []
    for p in parts:
        result.extend(p)
    return result


# ── Sound Effect Generators ──────────────────────────────────────────

def gen_sword_swing():
    """Short whoosh: filtered noise with fast attack and quick decay."""
    samples = gen_noise(0.2, volume=0.6)
    # Simulate band-pass by mixing with a mid-freq sine
    tone = gen_sine(800, 0.2, volume=0.15)
    samples = mix(samples, tone)
    return apply_envelope(samples, attack=0.005, decay=0.15)


def gen_hit():
    """Impact thud: low frequency sine + noise burst."""
    low = gen_sine(80, 0.15, volume=0.7)
    noise = gen_noise(0.08, volume=0.5)
    # Pad noise to match length
    noise.extend([0.0] * (len(low) - len(noise)))
    samples = mix(low, noise)
    return apply_envelope(samples, attack=0.002, decay=0.12)


def gen_player_hit():
    """Grunt-like: medium frequency with vibrato."""
    duration = 0.2
    n = int(SAMPLE_RATE * duration)
    samples = []
    for i in range(n):
        t = i / SAMPLE_RATE
        vibrato = math.sin(2 * math.pi * 30 * t) * 50  # vibrato
        freq = 200 + vibrato
        samples.append(0.5 * math.sin(2 * math.pi * freq * t))
    noise = gen_noise(0.1, volume=0.2)
    noise.extend([0.0] * (len(samples) - len(noise)))
    samples = mix(samples, noise)
    return apply_envelope(samples, attack=0.005, decay=0.15)


def gen_enemy_die():
    """Descending tone with noise."""
    duration = 0.4
    n = int(SAMPLE_RATE * duration)
    samples = []
    for i in range(n):
        t = i / SAMPLE_RATE
        freq = 400 - 300 * (t / duration)  # descend from 400 to 100 Hz
        samples.append(0.5 * math.sin(2 * math.pi * freq * t))
    noise = gen_noise(0.4, volume=0.15)
    samples = mix(samples, noise)
    return apply_envelope(samples, attack=0.01, decay=0.2)


def gen_player_die():
    """Long descending sad tone."""
    duration = 0.6
    n = int(SAMPLE_RATE * duration)
    samples = []
    for i in range(n):
        t = i / SAMPLE_RATE
        freq = 300 - 200 * (t / duration)  # descend from 300 to 100
        # Add slight vibrato for sadness
        vibrato = math.sin(2 * math.pi * 5 * t) * 10
        samples.append(0.5 * math.sin(2 * math.pi * (freq + vibrato) * t))
    return apply_envelope(samples, attack=0.02, decay=0.3)


def gen_footstep():
    """Short soft thud."""
    noise = gen_noise(0.08, volume=0.3)
    low = gen_sine(60, 0.08, volume=0.4)
    samples = mix(noise, low)
    return apply_envelope(samples, attack=0.002, decay=0.06)


def gen_pickup():
    """Bright ascending chime: sine wave going up."""
    duration = 0.2
    n = int(SAMPLE_RATE * duration)
    samples = []
    for i in range(n):
        t = i / SAMPLE_RATE
        freq = 600 + 800 * (t / duration)  # ascend from 600 to 1400
        samples.append(0.4 * math.sin(2 * math.pi * freq * t))
    return apply_envelope(samples, attack=0.005, decay=0.1)


def gen_potion():
    """Bubbling sound: multiple short sine bursts."""
    samples = []
    duration = 0.3
    num_bubbles = 8
    bubble_len = int(SAMPLE_RATE * duration / num_bubbles)
    for b in range(num_bubbles):
        freq = 400 + random.randint(-100, 200)
        burst = [0.35 * math.sin(2 * math.pi * freq * i / SAMPLE_RATE) for i in range(bubble_len)]
        burst = apply_envelope(burst, attack=0.005, decay=0.01)
        samples.extend(burst)
    return apply_envelope(samples, attack=0.01, decay=0.1)


def gen_door_open():
    """Creaking door: frequency-modulated sine with noise."""
    duration = 0.4
    n = int(SAMPLE_RATE * duration)
    samples = []
    for i in range(n):
        t = i / SAMPLE_RATE
        # Creaky modulation
        mod = math.sin(2 * math.pi * 7 * t) * 100
        freq = 150 + mod + 50 * (t / duration)
        samples.append(0.4 * math.sin(2 * math.pi * freq * t))
    noise = gen_noise(0.4, volume=0.1)
    samples = mix(samples, noise)
    return apply_envelope(samples, attack=0.02, decay=0.15)


def gen_level_up():
    """Triumphant ascending arpeggio: 3 notes going up."""
    # C5, E5, G5 (major triad)
    note1 = gen_sine(523, 0.15, volume=0.4)
    note2 = gen_sine(659, 0.15, volume=0.4)
    note3 = gen_sine(784, 0.2, volume=0.5)
    note1 = apply_envelope(note1, attack=0.005, decay=0.05)
    note2 = apply_envelope(note2, attack=0.005, decay=0.05)
    note3 = apply_envelope(note3, attack=0.005, decay=0.1)
    return concat(note1, note2, note3)


def gen_quest_complete():
    """Fanfare: ascending arpeggio with harmony."""
    # G4, B4, D5, G5
    notes = [392, 494, 587, 784]
    samples = []
    for i, freq in enumerate(notes):
        dur = 0.12 if i < 3 else 0.25
        vol = 0.35 + 0.05 * i
        note = gen_sine(freq, dur, volume=vol)
        # Add octave harmonic
        harmonic = gen_sine(freq * 2, dur, volume=vol * 0.3)
        note = mix(note, harmonic)
        note = apply_envelope(note, attack=0.005, decay=0.04)
        samples.extend(note)
    return apply_envelope(samples, attack=0.005, decay=0.1)


def gen_ui_click():
    """Short click: very short 1000Hz sine."""
    samples = gen_sine(1000, 0.05, volume=0.4)
    return apply_envelope(samples, attack=0.001, decay=0.03)


# ── Music Generator ──────────────────────────────────────────────────

def gen_town_music():
    """Dark ambient drone: low sine waves with subtle modulation, 10s loopable."""
    duration = 10.0
    n = int(SAMPLE_RATE * duration)
    samples = [0.0] * n
    # Base drone on D2 (~73 Hz)
    for i in range(n):
        t = i / SAMPLE_RATE
        # Fundamental
        s = 0.2 * math.sin(2 * math.pi * 73.4 * t)
        # Fifth above (A2 ~110 Hz)
        s += 0.12 * math.sin(2 * math.pi * 110 * t)
        # Subtle LFO modulation
        lfo = math.sin(2 * math.pi * 0.3 * t)
        s += 0.08 * math.sin(2 * math.pi * (146.8 + lfo * 5) * t)
        # Very subtle high harmonic
        s += 0.04 * math.sin(2 * math.pi * 220 * t + lfo * 2)
        samples[i] = s
    # Gentle fade in/out for seamless looping
    fade = int(SAMPLE_RATE * 0.5)
    for i in range(fade):
        factor = i / fade
        samples[i] *= factor
        samples[-(i + 1)] *= factor
    return samples


def gen_dungeon_music(variation=0):
    """Dark dungeon ambient: ominous low drones, 10s loopable."""
    duration = 10.0
    n = int(SAMPLE_RATE * duration)
    samples = [0.0] * n
    # Different base frequencies per variation
    base_freqs = [
        (55.0, 82.4),    # dungeon1: A1 + E2
        (49.0, 73.4),    # dungeon2: G1 + D2
        (46.2, 69.3),    # dungeon3: Gb1 + Db2
        (41.2, 61.7),    # dungeon4: E1 + B1
    ]
    f1, f2 = base_freqs[variation % len(base_freqs)]
    for i in range(n):
        t = i / SAMPLE_RATE
        lfo1 = math.sin(2 * math.pi * 0.15 * t)
        lfo2 = math.sin(2 * math.pi * 0.23 * t)
        s = 0.2 * math.sin(2 * math.pi * f1 * t + lfo1 * 0.5)
        s += 0.15 * math.sin(2 * math.pi * f2 * t + lfo2 * 0.3)
        # Dissonant overtone for creepiness
        s += 0.06 * math.sin(2 * math.pi * (f1 * 3.01) * t)
        # Noise-like texture
        s += 0.03 * math.sin(2 * math.pi * (f2 * 5.03 + lfo1 * 10) * t)
        samples[i] = s
    # Loop-friendly fade
    fade = int(SAMPLE_RATE * 0.5)
    for i in range(fade):
        factor = i / fade
        samples[i] *= factor
        samples[-(i + 1)] *= factor
    return samples


def gen_title_music():
    """Title screen: dark, slow, ominous melody over drone, 10s loopable."""
    duration = 10.0
    n = int(SAMPLE_RATE * duration)
    samples = [0.0] * n
    # Low drone
    for i in range(n):
        t = i / SAMPLE_RATE
        lfo = math.sin(2 * math.pi * 0.2 * t)
        s = 0.15 * math.sin(2 * math.pi * 55 * t)
        s += 0.1 * math.sin(2 * math.pi * 82.4 * t + lfo * 0.3)
        # Slow melody notes (minor scale feel)
        melody_freq = 220  # A3
        beat = (t * 2) % 4  # slow tempo
        if beat < 1:
            melody_freq = 220
        elif beat < 2:
            melody_freq = 196  # G3
        elif beat < 3:
            melody_freq = 174.6  # F3
        else:
            melody_freq = 164.8  # E3
        mel_env = math.sin(math.pi * (beat % 1))  # shape each note
        s += 0.08 * mel_env * math.sin(2 * math.pi * melody_freq * t)
        samples[i] = s
    fade = int(SAMPLE_RATE * 0.5)
    for i in range(fade):
        factor = i / fade
        samples[i] *= factor
        samples[-(i + 1)] *= factor
    return samples


# ── Main ─────────────────────────────────────────────────────────────

def main():
    random.seed(42)  # Reproducible output

    print("Generating SFX...")
    sfx = [
        ("sword_swing.wav", gen_sword_swing()),
        ("hit.wav", gen_hit()),
        ("player_hit.wav", gen_player_hit()),
        ("enemy_die.wav", gen_enemy_die()),
        ("player_die.wav", gen_player_die()),
        ("footstep.wav", gen_footstep()),
        ("pickup.wav", gen_pickup()),
        ("potion.wav", gen_potion()),
        ("door_open.wav", gen_door_open()),
        ("level_up.wav", gen_level_up()),
        ("quest_complete.wav", gen_quest_complete()),
        ("ui_click.wav", gen_ui_click()),
    ]
    for name, samples in sfx:
        write_wav(os.path.join(SFX_DIR, name), samples)

    print("\nGenerating music...")
    # Note: audio.c expects .mp3 but we generate .wav
    # SDL2_mixer's Mix_LoadMUS can load WAV files too.
    # We generate both .wav and create a copy as .mp3 extension workaround
    # Actually, let's just generate with the exact filenames audio.c expects
    # Mix_LoadMUS detects format by content, not extension — .mp3-named WAV files
    # won't work. Instead we generate .wav alongside. The code should be updated
    # to use .wav, but for now generate both naming conventions.

    music_tracks = [
        ("town", gen_town_music()),
        ("dungeon1", gen_dungeon_music(0)),
        ("dungeon2", gen_dungeon_music(1)),
        ("dungeon3", gen_dungeon_music(2)),
        ("dungeon4", gen_dungeon_music(3)),
        ("title", gen_title_music()),
    ]
    for name, samples in music_tracks:
        # Generate as .wav (the actual working format)
        write_wav(os.path.join(MUSIC_DIR, f"{name}.wav"), samples)

    print("\nDone! All audio files generated.")
    print("\nNote: audio.c references music files as .mp3 but we generated .wav.")
    print("Update music_files[] in src/engine/audio.c to use .wav extension.")


if __name__ == "__main__":
    main()
