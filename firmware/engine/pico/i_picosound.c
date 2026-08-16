//
// Copyright(C) 1993-1996 Id Software, Inc.
// Copyright(C) 2005-2014 Simon Howard
// Copyright(C) 2008 David Flater
// Copyright(C) 2021-2022 Graham Sanderson
// Copyright(C) 2026 Alexander (ES8311/PIO adaptation)
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// DESCRIPTION:
//	System interface for sound - ADPCM sound-effect mixing is unchanged
//	from upstream (it never touched pico_audio_i2s directly), only the
//	output path is swapped for mp3player's proven ES8311/PIO I2S driver.
//	Music is a stub for now (see i_oplmusic.c) - the music_generator hook
//	below is wired up but never populated, so mixing only ever produces
//	sound effects. See doom/docs/DECISIONS.md.
//

#include "config.h"

#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <doom/sounds.h>
#include <z_zone.h>

#include "deh_str.h"
#include "i_sound.h"
#include "m_misc.h"
#include "w_wad.h"

#include "doomtype.h"
#include "i_picosound.h"

#include "DEV_Config.h"
#include "audio_pio.h"
#include "es8311.h"
#include "pico/mutex.h"
#if PICO_ON_DEVICE
#include "bootlog.h"
#endif

#define ADPCM_BLOCK_SIZE 128
#define ADPCM_SAMPLES_PER_BLOCK_SIZE 249
#define MIX_MAX_VOLUME 128

// Mono samples produced per I_Pico_UpdateSound() call. Two converted buffers
// are owned by audio_pio's DMA queue; the mixer returns immediately when both
// are occupied rather than stalling either render core on the I2S FIFO.
#define MIX_BUFFER_SAMPLES 512

typedef struct channel_s channel_t;

static volatile enum {
    FS_NONE,
    FS_FADE_OUT,
    FS_FADE_IN,
    FS_SILENT,
} fade_state;
#define FADE_STEP 8 // must be power of 2
uint16_t fade_level;

struct channel_s
{
    const uint8_t *data;
    const uint8_t *data_end;
    uint32_t offset;
    uint32_t step;
    uint8_t left, right; // 0-255
    uint8_t decompressed_size;
#if SOUND_LOW_PASS
    uint8_t alpha256;
#endif
    int8_t decompressed[ADPCM_SAMPLES_PER_BLOCK_SIZE];
};

struct audio_buffer {
    int16_t samples[MIX_BUFFER_SAMPLES];
};

// ====== FROM ADPCM-LIB =====
#define CLIP(data, min, max) \
if ((data) > (max)) data = max; \
else if ((data) < (min)) data = min;

static const uint16_t step_table[89] = {
        7, 8, 9, 10, 11, 12, 13, 14,
        16, 17, 19, 21, 23, 25, 28, 31,
        34, 37, 41, 45, 50, 55, 60, 66,
        73, 80, 88, 97, 107, 118, 130, 143,
        157, 173, 190, 209, 230, 253, 279, 307,
        337, 371, 408, 449, 494, 544, 598, 658,
        724, 796, 876, 963, 1060, 1166, 1282, 1411,
        1552, 1707, 1878, 2066, 2272, 2499, 2749, 3024,
        3327, 3660, 4026, 4428, 4871, 5358, 5894, 6484,
        7132, 7845, 8630, 9493, 10442, 11487, 12635, 13899,
        15289, 16818, 18500, 20350, 22385, 24623, 27086, 29794,
        32767
};

static const int index_table[] = {
        -1, -1, -1, -1, 2, 4, 6, 8
};
// =============================

static void (*music_generator)(audio_buffer_t *buffer);
static struct audio_buffer mix_buffer;

static volatile boolean sound_initialized = false;

// pd_render.cpp's SafeUpdateSound() calls I_UpdateSound() from both cores.
// This mutex serializes mixing and every start/stop/parameter mutation of
// channels[]. The old code protected only the mixer, leaving game-core sound
// triggers racing core1's ADPCM decode even after output itself was locked.
static mutex_t update_sound_mutex;
static channel_t channels[NUM_SOUND_CHANNELS];

static boolean use_sfx_prefix;

static inline bool is_channel_playing(int channel) {
    return channels[channel].decompressed_size != 0;
}

static inline void stop_channel(int channel) {
    channels[channel].decompressed_size = 0;
}

static bool check_and_init_channel(int channel) {
    return sound_initialized && ((uint)channel) < NUM_SOUND_CHANNELS;
}

int adpcm_decode_block_s8(int8_t *outbuf, const uint8_t *inbuf, int inbufsize)
{
    int samples = 1, chunks;

    if (inbufsize < 4)
        return 0;

    int32_t pcmdata = (int16_t) (inbuf [0] | (inbuf [1] << 8));
    *outbuf++ = pcmdata>>8u;
    int index = inbuf[2];

    if (index < 0 || index > 88 || inbuf [3])     // sanitize the input a little...
        return 0;

    inbufsize -= 4;
    inbuf += 4;

    chunks = inbufsize / 4;
    samples += chunks * 8;

    while (chunks--) {
        for (int i = 0; i < 4; ++i) {
            int step = step_table[index], delta = step >> 3;

            if (*inbuf & 1) delta += (step >> 2);
            if (*inbuf & 2) delta += (step >> 1);
            if (*inbuf & 4) delta += step;
            if (*inbuf & 8) delta = -delta;

            pcmdata += delta;
            index += index_table [*inbuf & 0x7];
            CLIP(index, 0, 88);
            CLIP(pcmdata, -32768, 32767);
            outbuf [i * 2] = pcmdata>>8u;

            step = step_table[index], delta = step >> 3;

            if (*inbuf & 0x10) delta += (step >> 2);
            if (*inbuf & 0x20) delta += (step >> 1);
            if (*inbuf & 0x40) delta += step;
            if (*inbuf & 0x80) delta = -delta;

            pcmdata += delta;
            index += index_table[(*inbuf >> 4) & 0x7];
            CLIP(index, 0, 88);
            CLIP(pcmdata, -32768, 32767);
            outbuf [i * 2 + 1] = pcmdata>>8u;
            inbuf++;
        }

        outbuf += 8;
    }

    return samples;
}

static void decompress_buffer(channel_t *channel) {
    if (channel->data == channel->data_end) {
        channel->decompressed_size = 0;
    } else {
        int block_size = MIN(ADPCM_BLOCK_SIZE, channel->data_end - channel->data);
        channel->decompressed_size = adpcm_decode_block_s8(channel->decompressed, channel->data, block_size);
        assert(channel->decompressed_size && channel->decompressed_size <= sizeof(channel->decompressed));
        channel->data += block_size;
    }
}

static boolean init_channel_for_sfx(channel_t *ch, const sfxinfo_t *sfxinfo, int pitch)
{
    int lumpnum = sfx_mut(sfxinfo)->lumpnum;
    int lumplen = W_LumpLength(lumpnum);

    const uint8_t *data = W_CacheLumpNum(lumpnum, PU_STATIC);

    if (lumplen < 8 || data[0] != 0x03 || data[1] != 0x80) // 0x80 == only support compressed
    {
        return false;
    }

    int length = lumplen - 8;

    ch->data = data + 8;
    ch->data_end = ch->data + length;

    uint32_t sample_freq = (data[3] << 8) | data[2];
    if (pitch == NORM_PITCH)
        ch->step = sample_freq * 65536 / PICO_SOUND_SAMPLE_FREQ;
    else
        ch->step = (uint32_t)((sample_freq * pitch) * 65536ull
                              / (PICO_SOUND_SAMPLE_FREQ * NORM_PITCH));

    decompress_buffer(ch);
    ch->offset = 0;

#if SOUND_LOW_PASS
    ch->alpha256 = 256u * 201u * sample_freq / (201u * sample_freq + 64u * (uint)PICO_SOUND_SAMPLE_FREQ);
#endif
    return true;
}

static void GetSfxLumpName(const sfxinfo_t *sfx, char *buf, size_t buf_len)
{
    if (sfx->link != NULL)
    {
        sfx = sfx->link;
    }

    if (use_sfx_prefix)
    {
        M_snprintf(buf, buf_len, "ds%s", DEH_String(sfx->name));
    }
    else
    {
        M_StringCopy(buf, DEH_String(sfx->name), buf_len);
    }
}

static void I_Pico_PrecacheSounds(should_be_const sfxinfo_t *sounds, int num_sounds)
{
    // no-op
}

static int I_Pico_GetSfxLumpNum(should_be_const sfxinfo_t *sfx)
{
    char namebuf[9];
    GetSfxLumpName(sfx, namebuf, sizeof(namebuf));
    return W_GetNumForName(namebuf);
}

static void update_sound_params_locked(int handle, int vol, int sep)
{
    int left, right;

    left = ((254 - sep) * vol) / 127;
    right = ((sep) * vol) / 127;

    if (left < 0) left = 0;
    else if ( left > 255) left = 255;
    if (right < 0) right = 0;
    else if (right > 255) right = 255;

    channels[handle].left = left;
    channels[handle].right = right;
}

static void I_Pico_UpdateSoundParams(int handle, int vol, int sep)
{
    if (!sound_initialized || handle < 0 || handle >= NUM_SOUND_CHANNELS) {
        return;
    }

    mutex_enter_blocking(&update_sound_mutex);
    if (sound_initialized) update_sound_params_locked(handle, vol, sep);
    mutex_exit(&update_sound_mutex);
}

static int I_Pico_StartSound(should_be_const sfxinfo_t *sfxinfo, int channel, int vol, int sep, int pitch)
{
    if (!check_and_init_channel(channel)) return -1;

    mutex_enter_blocking(&update_sound_mutex);
    if (!sound_initialized) {
        mutex_exit(&update_sound_mutex);
        return -1;
    }
    stop_channel(channel);
    channel_t *ch = &channels[channel];
    if (!init_channel_for_sfx(ch, sfxinfo, pitch)) {
        assert(!is_channel_playing(channel));
    }
    update_sound_params_locked(channel, vol, sep);
    mutex_exit(&update_sound_mutex);
    return channel;
}

static void I_Pico_StopSound(int channel)
{
    if (check_and_init_channel(channel)) {
        mutex_enter_blocking(&update_sound_mutex);
        stop_channel(channel);
        mutex_exit(&update_sound_mutex);
    }
}

static boolean I_Pico_SoundIsPlaying(int channel)
{
    if (!check_and_init_channel(channel)) return false;
    mutex_enter_blocking(&update_sound_mutex);
    boolean playing = is_channel_playing(channel);
    mutex_exit(&update_sound_mutex);
    return playing;
}

// Mix active sound-effect channels into one mono buffer only when the DMA
// backend has capacity. The queue owns the converted samples after submit,
// so neither core ever waits for the 44.1kHz I2S stream itself.
static void I_Pico_UpdateSound(void)
{
#if PICO_ON_DEVICE && DEBUG_NO_SOUND
    // Optional diagnostic switch: bypass both mixing and asynchronous output.
    return;
#endif
    if (!sound_initialized) return;

    // Called from both cores (see update_sound_mutex's comment above) -
    // skip this call entirely if the other core is already mid-update,
    // rather than block (blocking here could stall a render-timing-
    // critical caller on core0, or core1's own tic-servicing loop).
    // Dropping an occasional audio update is a far better failure mode
    // than the two cores racing on channels[]/mix_buffer/the PIO push.
    if (!mutex_try_enter(&update_sound_mutex, NULL)) return;

    // The DMA backend already emits silence on underflow. Do not eagerly
    // fill both producer buffers with silence: that wastes mixer time and
    // adds up to two blocks of latency before a newly-triggered SFX starts.
    bool needs_mix = music_generator != NULL
                  || (fade_state != FS_NONE && fade_state != FS_SILENT);
    for (int ch = 0; ch < NUM_SOUND_CHANNELS && !needs_mix; ch++) {
        needs_mix = is_channel_playing(ch);
    }
    if (!needs_mix) {
        mutex_exit(&update_sound_mutex);
        return;
    }
    if (!audio_queue_available()) {
        mutex_exit(&update_sound_mutex);
        return;
    }

    int16_t *samples = mix_buffer.samples;
    if (music_generator) {
        music_generator(&mix_buffer);
    } else {
        memset(samples, 0, sizeof(mix_buffer.samples));
    }

    for(int ch=0; ch < NUM_SOUND_CHANNELS; ch++) {
        if (is_channel_playing(ch)) {
            channel_t *channel = &channels[ch];
            assert(channel->decompressed_size);
            int vol_mono = (channel->left/2 + channel->right/2) / 2;
            uint offset_end = channel->decompressed_size * 65536;
            assert(channel->offset < offset_end);
#if SOUND_LOW_PASS
            int alpha256 = channel->alpha256;
            int beta256 = 256 - alpha256;
            int sample = channel->decompressed[channel->offset >> 16];
#endif
            for(int s=0;s<MIX_BUFFER_SAMPLES;s++) {
#if !SOUND_LOW_PASS
                int sample = channel->decompressed[channel->offset >> 16];
#else
                sample = (beta256 * sample + alpha256 * channel->decompressed[channel->offset >> 16]) / 256;
#endif
                int mixed = samples[s] + sample * vol_mono;
                CLIP(mixed, -32768, 32767);
                samples[s] = (int16_t)mixed;
                channel->offset += channel->step;
                if (channel->offset >= offset_end) {
                    channel->offset -= offset_end;
                    decompress_buffer(channel);
                    offset_end = channel->decompressed_size * 65536;
                    if (channel->offset >= offset_end) {
                        stop_channel(ch);
                        break;
                    }
                }
            }
        }
    }

    if (fade_state == FS_SILENT) {
        memset(samples, 0, sizeof(mix_buffer.samples));
    } else if (fade_state != FS_NONE) {
        int fade_step = fade_state == FS_FADE_IN ? FADE_STEP : -FADE_STEP;
        int i;
        for(i=0;i<MIX_BUFFER_SAMPLES && fade_level;i++) {
            samples[i] = (samples[i] * (int)fade_level) >> 16;
            fade_level += fade_step;
        }
        if (!fade_level) {
            if (fade_state == FS_FADE_OUT) {
                for(;i<MIX_BUFFER_SAMPLES;i++) {
                    samples[i] = 0;
                }
                fade_state = FS_SILENT;
            } else {
                fade_state = FS_NONE;
            }
        }
    }

    audio_try_queue(samples, MIX_BUFFER_SAMPLES);
    mutex_exit(&update_sound_mutex);
}

static void I_Pico_ShutdownSound(void)
{
    if (!sound_initialized)
    {
        return;
    }
    mutex_enter_blocking(&update_sound_mutex);
    sound_initialized = false;
    mutex_exit(&update_sound_mutex);
    audio_dma_shutdown();
}

static boolean I_Pico_InitSound(boolean _use_sfx_prefix)
{
    use_sfx_prefix = _use_sfx_prefix;

    pico_audio.channel_count = 1;
    pico_audio.sample_freq = PICO_SOUND_SAMPLE_FREQ;
    pico_audio.mclk_freq = PICO_SOUND_SAMPLE_FREQ * 256;

#if PICO_ON_DEVICE
    bootlog_print("s1: before es8311_init");
#endif
    es8311_init(pico_audio);
#if PICO_ON_DEVICE
    bootlog_print("s2: es8311_init OK");
#endif
    es8311_sample_frequency_config(pico_audio.mclk_freq, pico_audio.sample_freq);
#if PICO_ON_DEVICE
    bootlog_print("s3: freq_config OK");
#endif
    es8311_voice_volume_set(pico_audio.volume);
#if PICO_ON_DEVICE
    bootlog_print("s4: volume_set OK");
#endif
    mclk_pio_init();
#if PICO_ON_DEVICE
    bootlog_print("s5: mclk_pio_init OK");
#endif
    dout_pio_init();
#if PICO_ON_DEVICE
    bootlog_print("s6: dout_pio_init OK");
#endif

    if (!audio_dma_init()) {
#if PICO_ON_DEVICE
        bootlog_print("s7: audio DMA unavailable");
#endif
        return false;
    }

    mutex_init(&update_sound_mutex);
    sound_initialized = true;
    return true;
}

static snddevice_t sound_pico_devices[] =
{
    SNDDEVICE_SB,
};

sound_module_t sound_pico_module =
{
    sound_pico_devices,
    arrlen(sound_pico_devices),
    I_Pico_InitSound,
    I_Pico_ShutdownSound,
    I_Pico_GetSfxLumpNum,
    I_Pico_UpdateSound,
    I_Pico_UpdateSoundParams,
    I_Pico_StartSound,
    I_Pico_StopSound,
    I_Pico_SoundIsPlaying,
    I_Pico_PrecacheSounds,
};

bool I_PicoSoundIsInitialized(void) {
    return sound_initialized;
}

void I_PicoSoundSetMusicGenerator(void (*generator)(audio_buffer_t *buffer)) {
    music_generator = generator;
}

void I_PicoSoundFade(bool in) {
    fade_state = in ? FS_FADE_IN : FS_FADE_OUT;
    fade_level = in ? FADE_STEP : 0x10000 - FADE_STEP;
}

bool I_PicoSoundFading(void) {
    return fade_state == FS_FADE_IN || fade_state == FS_FADE_OUT;
}
