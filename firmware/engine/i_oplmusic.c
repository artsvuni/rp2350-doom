/*
 * Lightweight fixed-memory music backend for the RP2350 handheld.
 *
 * The shareware WAD contains the Doom score, converted by whd_gen to the
 * compact MUSX event stream. The original rp2040-doom port renders it through
 * a full emu8950 OPL emulator, but that path allocates state dynamically and
 * is relatively expensive. This backend instead plays the real MUSX notes
 * with nine tiny integer oscillators. The optional speaker-mastering path
 * rounds their timbre, removes note-edge clicks, constrains the spectrum for
 * the board's 12x10mm speaker, and keeps the score behind sound effects. It is
 * still not cycle-accurate AdLib, but remains fixed-memory and bounded-time.
 */

#include "config.h"

#include <stdint.h>
#include <string.h>

#include "doomtype.h"
#include "i_sound.h"

#if PICO_ON_DEVICE && DEBUG_NO_MUSIC

// Effects-only is the hardware default. Keep a tiny interface-compatible
// module so the complete experimental backend below remains one CMake switch
// away without reserving its voices and MUSX decoder state in normal builds.
uint8_t restart_song_state;

static const snddevice_t music_disabled_devices[] = { SNDDEVICE_ADLIB };

static boolean Disabled_Init(void) { return false; }
static void Disabled_Shutdown(void) {}
static void Disabled_SetVolume(int volume) { (void)volume; }
static void Disabled_Pause(void) {}
static void Disabled_Resume(void) {}
static void *Disabled_Register(should_be_const void *data, int len)
{
    (void)data;
    (void)len;
    return NULL;
}
static void Disabled_UnRegister(void *handle) { (void)handle; }
static void Disabled_Play(void *handle, boolean looping)
{
    (void)handle;
    (void)looping;
}
static void Disabled_Stop(void) {}
static boolean Disabled_IsPlaying(void) { return false; }

void I_SetOPLDriverVer(opl_driver_ver_t ver) { (void)ver; }

const music_module_t music_opl_module =
{
    music_disabled_devices,
    arrlen(music_disabled_devices),
    Disabled_Init,
    Disabled_Shutdown,
    Disabled_SetVolume,
    Disabled_Pause,
    Disabled_Resume,
    Disabled_Register,
    Disabled_UnRegister,
    Disabled_Play,
    Disabled_Stop,
    Disabled_IsPlaying,
    NULL,
};

#else

#include "midifile.h"
#include "pico/i_picosound.h"
#include "pico/mutex.h"

#define MUSIC_VOICES 9
#define MUSX_TICKS_PER_BEAT 70u
#define DEFAULT_US_PER_BEAT 500000u

#ifndef DOOM_MUSIC_LAB
#define DOOM_MUSIC_LAB 0
#endif
#ifndef DOOM_MUSIC_LAB_VARIANT
#define DOOM_MUSIC_LAB_VARIANT 0
#endif
#ifndef DOOM_MUSIC_SMOOTH_SYNTH
#define DOOM_MUSIC_SMOOTH_SYNTH 0
#endif
#ifndef DOOM_MUSIC_PERCUSSION_PERCENT
#define DOOM_MUSIC_PERCUSSION_PERCENT 100
#endif
#define DOOM_MUSIC_LAB_SMOOTH \
    (DOOM_MUSIC_SMOOTH_SYNTH \
     || (DOOM_MUSIC_LAB && DOOM_MUSIC_LAB_VARIANT > 0))

typedef struct
{
    uint32_t phase;
    uint32_t step;
    uint32_t age;
    uint8_t note;
    uint8_t channel;
    uint8_t velocity;
    uint8_t gain;
    uint8_t waveform;
#if DOOM_MUSIC_LAB_SMOOTH
    uint8_t target_gain;
#endif
#if DOOM_ENABLE_MUSIC && DOOM_MUSIC_SPEAKER_MASTERING
    uint16_t envelope;
    int16_t noise_filter;
    boolean releasing;
#endif
    boolean active;
} music_voice_t;

typedef struct
{
    uint8_t volume;
    uint8_t program;
} music_channel_t;

// pd_render.cpp uses bit 0 to mark deep render call stacks. Bit 1 is our
// deferred-loop request, matching the convention used by upstream's OPL path.
uint8_t restart_song_state;

static mutex_t music_mutex;
static boolean music_initialized;
static boolean music_playing;
static boolean music_paused;
static boolean song_looping;
static boolean restart_pending;
static uint8_t music_volume = 64;
static uint32_t voice_age;
static uint32_t noise_state = 0x13579bdfu;
#if DOOM_ENABLE_MUSIC && DOOM_MUSIC_SPEAKER_MASTERING
static int32_t music_filter_input;
static int32_t music_highpass;
static int32_t music_lowpass;
#endif
static uint64_t samples_to_event;
static midi_file_t *song_file;
static midi_track_iter_t *song_iter;
static music_voice_t voices[MUSIC_VOICES];
static music_channel_t music_channels[MIDI_CHANNELS_PER_TRACK];

static const snddevice_t music_devices[] =
{
    SNDDEVICE_ADLIB,
    SNDDEVICE_SB,
};

static uint32_t note_step(unsigned int note)
{
    // Phase increments for MIDI notes 0..11 (C-1..B-1) at 44.1 kHz.
    // Higher octaves are exact powers of two, so no float or lookup table is
    // needed in the mixer.
    static const uint32_t octave_minus_one[12] =
    {
         796279u,  843631u,  893796u,  946939u,
        1003255u, 1062913u, 1126119u, 1193088u,
        1264052u, 1339218u, 1418887u, 1503294u,
    };

    if (note > 127) note = 127;
    // Scale at note-on rather than in the sample loop. Profile 2 runs the
    // whole real audio path at 22.05kHz, where phase increments must double
    // to preserve pitch.
    uint64_t step = (uint64_t)octave_minus_one[note % 12]
                  << (note / 12);
    step = (step * 44100u) / PICO_SOUND_SAMPLE_FREQ;
    return (uint32_t)step;
}

static uint8_t voice_gain(const music_voice_t *voice)
{
    uint32_t gain = voice->velocity;
    gain *= music_channels[voice->channel].volume;
    gain *= music_volume;
    gain >>= 14;
    // General MIDI channel 10 is percussion (zero-based index 9). Apply this
    // here rather than only at note-on so later channel-volume events retain
    // the intended drum balance.
    if (voice->channel == 9)
    {
        gain = gain * DOOM_MUSIC_PERCUSSION_PERCENT / 100u;
    }
    return gain > 127 ? 127 : (uint8_t)gain;
}

static void refresh_channel_gains(unsigned int channel)
{
    for (unsigned int i = 0; i < MUSIC_VOICES; ++i)
    {
        if (voices[i].active && voices[i].channel == channel)
        {
#if DOOM_MUSIC_LAB_SMOOTH
            voices[i].target_gain = voice_gain(&voices[i]);
#else
            voices[i].gain = voice_gain(&voices[i]);
#endif
        }
    }
}

static void silence_voices(void)
{
    memset(voices, 0, sizeof(voices));
#if DOOM_ENABLE_MUSIC && DOOM_MUSIC_SPEAKER_MASTERING
    music_filter_input = 0;
    music_highpass = 0;
    music_lowpass = 0;
#endif
}

static void reset_channels(void)
{
    for (unsigned int i = 0; i < MIDI_CHANNELS_PER_TRACK; ++i)
    {
        music_channels[i].volume = 100;
        music_channels[i].program = 0;
    }
}

static music_voice_t *allocate_voice(unsigned int channel, unsigned int note)
{
    music_voice_t *oldest = &voices[0];

    for (unsigned int i = 0; i < MUSIC_VOICES; ++i)
    {
        if (voices[i].active
         && voices[i].channel == channel
         && voices[i].note == note)
        {
            return &voices[i];
        }
        if (!voices[i].active)
        {
            return &voices[i];
        }
        if (voices[i].age < oldest->age)
        {
            oldest = &voices[i];
        }
    }

    return oldest;
}

static void note_off(unsigned int channel, unsigned int note)
{
    for (unsigned int i = 0; i < MUSIC_VOICES; ++i)
    {
        if (voices[i].active && voices[i].channel == channel
         && voices[i].note == note)
        {
#if DOOM_ENABLE_MUSIC && DOOM_MUSIC_SPEAKER_MASTERING
            voices[i].releasing = true;
#elif DOOM_MUSIC_LAB_SMOOTH
            voices[i].target_gain = 0;
#else
            voices[i].active = false;
#endif
        }
    }
}

static void note_on(unsigned int channel, unsigned int note,
                    unsigned int velocity)
{
    if (channel >= MIDI_CHANNELS_PER_TRACK || note > 127)
    {
        return;
    }
    if (velocity == 0)
    {
        note_off(channel, note);
        return;
    }

    music_voice_t *voice = allocate_voice(channel, note);
    voice->phase = 0;
    voice->step = note_step(note);
    voice->age = ++voice_age;
    voice->note = (uint8_t)note;
    voice->channel = (uint8_t)channel;
    voice->velocity = (uint8_t)velocity;
    if (channel == 9)
    {
        voice->waveform = 4;
    }
#if DOOM_MUSIC_LAB_SMOOTH
    else
    {
        // A compact instrument-family map gives the sixteen General MIDI
        // program groups a less repetitive identity without storing samples
        // or running an OPL emulator.
        static const uint8_t program_family[16] =
        {
            0, 1, 1, 2, 0, 0, 2, 1,
            3, 2, 0, 1, 3, 2, 0, 1,
        };
        voice->waveform = program_family[music_channels[channel].program >> 4];
    }
#else
    else
    {
        voice->waveform = (music_channels[channel].program >> 4) & 3;
    }
#endif
#if DOOM_ENABLE_MUSIC && DOOM_MUSIC_SPEAKER_MASTERING
    voice->envelope = 0;
    voice->noise_filter = 0;
    voice->releasing = false;
#endif
    voice->active = true;
#if DOOM_MUSIC_LAB_SMOOTH
    voice->gain = 0;
    voice->target_gain = voice_gain(voice);
    if (channel == 9)
    {
        voice->target_gain = (voice->target_gain * 3u) >> 2;
    }
#else
    voice->gain = voice_gain(voice);
#endif
}

static void process_event(const midi_event_t *event)
{
    if (event->event_type == MIDI_EVENT_META)
    {
        if (event->data.meta.type == MIDI_META_END_OF_TRACK)
        {
            silence_voices();
            if (song_looping)
            {
                restart_pending = true;
                restart_song_state |= 2;
            }
            else
            {
                music_playing = false;
                I_PicoSoundSetMusicGenerator(NULL);
            }
        }
        return;
    }

    unsigned int channel = event->data.channel.channel;
    unsigned int param1 = event->data.channel.param1;
    unsigned int param2 = event->data.channel.param2;
    if (channel >= MIDI_CHANNELS_PER_TRACK)
    {
        return;
    }

    switch (event->event_type)
    {
        case MIDI_EVENT_NOTE_ON:
            note_on(channel, param1, param2);
            break;
        case MIDI_EVENT_NOTE_OFF:
            note_off(channel, param1);
            break;
        case MIDI_EVENT_PROGRAM_CHANGE:
            music_channels[channel].program = (uint8_t)param1;
            break;
        case MIDI_EVENT_CONTROLLER:
            if (param1 == MIDI_CONTROLLER_MAIN_VOLUME)
            {
                music_channels[channel].volume = (uint8_t)param2;
                refresh_channel_gains(channel);
            }
            else if (param1 == MIDI_CONTROLLER_ALL_NOTES_OFF)
            {
                for (unsigned int i = 0; i < MUSIC_VOICES; ++i)
                {
                    if (voices[i].channel == channel)
                    {
#if DOOM_ENABLE_MUSIC && DOOM_MUSIC_SPEAKER_MASTERING
                        voices[i].releasing = true;
#elif DOOM_MUSIC_LAB_SMOOTH
                        voices[i].target_gain = 0;
#else
                        voices[i].active = false;
#endif
                    }
                }
            }
            break;
        default:
            // Pitch bend, modulation and pan are deliberately omitted in the
            // first low-cost backend. Notes, programs and volume carry the
            // recognizable score while keeping the inner loop tiny.
            break;
    }
}

static uint64_t ticks_to_samples(unsigned int ticks)
{
    return ((uint64_t)ticks * DEFAULT_US_PER_BEAT * PICO_SOUND_SAMPLE_FREQ)
         / ((uint64_t)MUSX_TICKS_PER_BEAT * 1000000u);
}

static void restart_if_safe(void)
{
    if (!restart_pending || (restart_song_state & 1) || song_iter == NULL)
    {
        return;
    }

    MIDI_RestartIterator(song_iter);
    reset_channels();
    silence_voices();
    samples_to_event = 0;
    restart_pending = false;
    restart_song_state &= (uint8_t)~2u;
}

static int32_t oscillator_sample(music_voice_t *voice)
{
    uint32_t phase = voice->phase;
    int32_t wave;

    voice->phase += voice->step;
#if DOOM_MUSIC_LAB_SMOOTH
    // Fold a short attack/release into the existing gain multiply. This
    // removes the largest note-edge clicks for only a comparison and an
    // increment per active voice, unlike the rejected heavy mastering path.
    if (voice->gain < voice->target_gain)
    {
        voice->gain++;
    }
    else if (voice->gain > voice->target_gain)
    {
        voice->gain--;
        if (voice->gain == 0 && voice->target_gain == 0)
        {
            voice->active = false;
            return 0;
        }
    }

    if (voice->waveform < 4)
    {
        uint32_t p = phase >> 16;
        int32_t fundamental = p < 32768u
            ? (int32_t)(p << 1) - 32768
            : 98303 - (int32_t)(p << 1);
        uint32_t p2 = (phase * 2u) >> 16;
        int32_t second = p2 < 32768u
            ? (int32_t)(p2 << 1) - 32768
            : 98303 - (int32_t)(p2 << 1);
        uint32_t p3 = (phase * 3u) >> 16;
        int32_t third = p3 < 32768u
            ? (int32_t)(p3 << 1) - 32768
            : 98303 - (int32_t)(p3 << 1);

        switch (voice->waveform)
        {
            case 0: wave = fundamental; break;
            case 1: wave = fundamental + (second >> 2); break;
            case 2: wave = fundamental - (third >> 3); break;
            default: wave = (fundamental >> 1) + (second >> 1); break;
        }
    }
    else
    {
        noise_state ^= noise_state << 13;
        noise_state ^= noise_state >> 17;
        noise_state ^= noise_state << 5;
        wave = (int16_t)(noise_state >> 16);
    }

    return (wave * voice->gain) >> 11;
#elif DOOM_ENABLE_MUSIC && DOOM_MUSIC_SPEAKER_MASTERING
    // Fast attack removes the hard phase-reset click without making Doom's
    // score feel sluggish. A short release removes note-off clicks and gives
    // the tiny speaker a little space between dense events.
    if (voice->releasing)
    {
        if (voice->envelope <= 12u)
        {
            voice->active = false;
            return 0;
        }
        voice->envelope -= 12u;
    }
    else if (voice->envelope < 32767u)
    {
        uint32_t next = voice->envelope + 160u;
        voice->envelope = next > 32767u ? 32767u : (uint16_t)next;
    }

    // Integer parabolic sine approximation. Unlike the old square/saw/pulse
    // set it does not inject large discontinuities at every period boundary.
    int32_t x = (int16_t)(phase >> 16);
    int32_t sine = (x * (32768 - (x < 0 ? -x : x))) >> 13;
    int32_t harmonic;

    switch (voice->waveform)
    {
        case 0: // mellow fundamental
            wave = sine;
            break;
        case 1: // triangle: brighter but still continuous
        {
            uint32_t p = phase >> 16;
            wave = p < 32768u ? (int32_t)(p << 1) - 32768
                              : 98303 - (int32_t)(p << 1);
            break;
        }
        case 2: // fundamental plus a restrained second harmonic
            x = (int16_t)((phase * 2u) >> 16);
            harmonic = (x * (32768 - (x < 0 ? -x : x))) >> 13;
            wave = sine + (harmonic >> 2);
            break;
        case 3: // fundamental plus a quieter third harmonic
            x = (int16_t)((phase * 3u) >> 16);
            harmonic = (x * (32768 - (x < 0 ? -x : x))) >> 13;
            wave = sine + (harmonic >> 3);
            break;
        default: // percussion: low-pass the deterministic noise per voice
            noise_state ^= noise_state << 13;
            noise_state ^= noise_state >> 17;
            noise_state ^= noise_state << 5;
            harmonic = (int16_t)(noise_state >> 16);
            voice->noise_filter += (harmonic - voice->noise_filter) >> 3;
            wave = voice->noise_filter;
            break;
    }

    int32_t sample = (wave * voice->gain) >> 11;
    return (sample * voice->envelope) >> 15;
#else
    switch (voice->waveform)
    {
        case 0: // triangle
        {
            uint32_t p = phase >> 16;
            wave = p < 32768u ? (int32_t)(p << 1) - 32768
                              : 98303 - (int32_t)(p << 1);
            break;
        }
        case 1: // square
            wave = (phase & 0x80000000u) ? 32767 : -32768;
            break;
        case 2: // saw
            wave = (int16_t)(phase >> 16);
            break;
        case 3: // narrow pulse
            wave = (phase >> 29) == 0 ? 32767 : -16384;
            break;
        default: // percussion: cheap deterministic noise
            noise_state ^= noise_state << 13;
            noise_state ^= noise_state >> 17;
            noise_state ^= noise_state << 5;
            wave = (int16_t)(noise_state >> 16);
            break;
    }

    return (wave * voice->gain) >> 11;
#endif
}

#if DOOM_ENABLE_MUSIC && DOOM_MUSIC_SPEAKER_MASTERING
static int32_t master_music_sample(int32_t mixed)
{
    // Approximately 90Hz DC/high-pass followed by a 7kHz low-pass at 44.1kHz.
    // This removes DC and the most brittle oscillator harmonics without
    // erasing the title score's low material. Coefficients are Q15 and require
    // no floating point.
    int32_t highpass = music_highpass + mixed - music_filter_input;
    music_filter_input = mixed;
    music_highpass = (int32_t)(((int64_t)32352 * highpass) >> 15);
    music_lowpass += (int32_t)(((int64_t)16384
                              * (music_highpass - music_lowpass)) >> 15);

    // Gentle peak knee, not hard clipping. At this level the limiter should
    // act only on unusually dense passages.
    if (mixed > 12000) mixed = 12000 + ((mixed - 12000) >> 2);
    else if (mixed < -12000) mixed = -12000 + ((mixed + 12000) >> 2);
    if (mixed > 16000) mixed = 16000;
    else if (mixed < -16000) mixed = -16000;
    return mixed;
}
#endif

static void render_voices(int16_t *samples, size_t count)
{
    for (size_t s = 0; s < count; ++s)
    {
        int32_t mixed = 0;
        for (unsigned int i = 0; i < MUSIC_VOICES; ++i)
        {
            if (voices[i].active) mixed += oscillator_sample(&voices[i]);
        }
#if DOOM_ENABLE_MUSIC && DOOM_MUSIC_SPEAKER_MASTERING
        mixed = master_music_sample(mixed);
#endif
        if (mixed > 32767) mixed = 32767;
        else if (mixed < -32768) mixed = -32768;
        samples[s] = (int16_t)mixed;
    }
}

static void music_generator(audio_buffer_t *buffer)
{
    size_t count;
    int16_t *samples = I_PicoSoundBufferSamples(buffer, &count);
    memset(samples, 0, count * sizeof(*samples));

    if (!mutex_try_enter(&music_mutex, NULL))
    {
        return;
    }

    restart_if_safe();
    if (!music_playing || music_paused || restart_pending || song_iter == NULL)
    {
        mutex_exit(&music_mutex);
        return;
    }

    size_t rendered = 0;
    while (rendered < count && music_playing && !restart_pending)
    {
        while (samples_to_event == 0 && music_playing && !restart_pending)
        {
            midi_event_t *event;
            if (!MIDI_GetNextEvent(song_iter, &event))
            {
                music_playing = false;
                break;
            }
            process_event(event);
            if (!restart_pending && music_playing)
            {
                samples_to_event = ticks_to_samples(MIDI_GetDeltaTime(song_iter));
            }
        }

        size_t chunk = count - rendered;
        if (samples_to_event < chunk) chunk = (size_t)samples_to_event;
        if (chunk == 0) continue;
        render_voices(samples + rendered, chunk);
        rendered += chunk;
        samples_to_event -= chunk;
    }

    mutex_exit(&music_mutex);
}

// music_mutex must be held. A null generator lets the shared audio backend
// sleep between sound effects instead of continuously producing silent music
// buffers. Keeping the iterator intact makes a later volume increase resume
// the current track without allocating or reloading anything.
static void update_music_generator_locked(void)
{
    I_PicoSoundSetMusicGenerator(
        music_playing && !music_paused && music_volume > 0
            ? music_generator
            : NULL);
}

static boolean I_LiteMusic_Init(void)
{
    mutex_init(&music_mutex);
    reset_channels();
    silence_voices();
    music_initialized = I_PicoSoundIsInitialized();
    return music_initialized;
}

static void I_LiteMusic_Shutdown(void)
{
    I_PicoSoundSetMusicGenerator(NULL);
    mutex_enter_blocking(&music_mutex);
    music_playing = false;
    music_initialized = false;
    song_iter = NULL;
    song_file = NULL;
    silence_voices();
    update_music_generator_locked();
    mutex_exit(&music_mutex);
}

static void I_LiteMusic_SetVolume(int volume)
{
    if (volume < 0) volume = 0;
    if (volume > 127) volume = 127;
    mutex_enter_blocking(&music_mutex);
    music_volume = (uint8_t)volume;
    for (unsigned int i = 0; i < MUSIC_VOICES; ++i)
    {
        if (voices[i].active)
        {
#if DOOM_MUSIC_LAB_SMOOTH
            voices[i].target_gain = voice_gain(&voices[i]);
#else
            voices[i].gain = voice_gain(&voices[i]);
#endif
        }
    }
    update_music_generator_locked();
    mutex_exit(&music_mutex);
}

static void I_LiteMusic_Pause(void)
{
    mutex_enter_blocking(&music_mutex);
    music_paused = true;
    update_music_generator_locked();
    mutex_exit(&music_mutex);
}

static void I_LiteMusic_Resume(void)
{
    mutex_enter_blocking(&music_mutex);
    music_paused = false;
    update_music_generator_locked();
    mutex_exit(&music_mutex);
}

static void *I_LiteMusic_RegisterSong(should_be_const void *data, int len)
{
    if (!music_initialized || data == NULL || len < 8) return NULL;
    mutex_enter_blocking(&music_mutex);
    midi_file_t *file = MUSX_LoadRaw(data, len);
    mutex_exit(&music_mutex);
    return file;
}

static void I_LiteMusic_UnRegisterSong(void *handle)
{
    if (handle == NULL) return;
    I_PicoSoundSetMusicGenerator(NULL);
    mutex_enter_blocking(&music_mutex);
    if (song_file == handle)
    {
        music_playing = false;
        song_iter = NULL;
        song_file = NULL;
        silence_voices();
    }
    MIDI_FreeFile(handle);
    update_music_generator_locked();
    mutex_exit(&music_mutex);
}

static void I_LiteMusic_PlaySong(void *handle, boolean looping)
{
    if (!music_initialized || handle == NULL) return;
    mutex_enter_blocking(&music_mutex);
    song_file = handle;
    song_iter = MIDI_IterateTrack(song_file, 0);
    song_looping = looping;
    music_paused = false;
    music_playing = song_iter != NULL;
    restart_pending = false;
    restart_song_state &= (uint8_t)~2u;
    samples_to_event = 0;
    reset_channels();
    silence_voices();
    update_music_generator_locked();
    mutex_exit(&music_mutex);
}

static void I_LiteMusic_StopSong(void)
{
    // Detach first so future audio updates do not generate silent blocks.
    // An update already in flight will finish before the mutex below enters.
    I_PicoSoundSetMusicGenerator(NULL);
    mutex_enter_blocking(&music_mutex);
    music_playing = false;
    restart_pending = false;
    samples_to_event = 0;
    if (song_iter != NULL) MIDI_FreeIterator(song_iter);
    song_iter = NULL;
    silence_voices();
    update_music_generator_locked();
    mutex_exit(&music_mutex);
}

static boolean I_LiteMusic_IsPlaying(void)
{
    mutex_enter_blocking(&music_mutex);
    boolean result = music_playing;
    mutex_exit(&music_mutex);
    return result;
}

void I_SetOPLDriverVer(opl_driver_ver_t ver)
{
    (void)ver;
}

const music_module_t music_opl_module =
{
    music_devices,
    arrlen(music_devices),
    I_LiteMusic_Init,
    I_LiteMusic_Shutdown,
    I_LiteMusic_SetVolume,
    I_LiteMusic_Pause,
    I_LiteMusic_Resume,
    I_LiteMusic_RegisterSong,
    I_LiteMusic_UnRegisterSong,
    I_LiteMusic_PlaySong,
    I_LiteMusic_StopSong,
    I_LiteMusic_IsPlaying,
    NULL,
};

#endif
