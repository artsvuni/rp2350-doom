/*
 * Standalone physical listening lab for Doom's embedded music path.
 *
 * This intentionally uses the already-flashed WHX, MUSX decoder, synthesiser,
 * mixer, DMA queue and ES8311 driver from the game.  It does not initialise
 * gameplay or the renderer, so a candidate boots directly into a repeatable
 * E1M1 listening test without adding proprietary assets to the firmware.
 */

#include "config.h"

#if DOOM_MUSIC_LAB

#include "pico/stdlib.h"

#include "bootlog.h"
#include "i_sound.h"
#include "pico/i_picosound.h"
#include "pwr_button.h"
#include "w_wad.h"
#include "z_zone.h"

#ifndef DOOM_MUSIC_LAB_VARIANT
#define DOOM_MUSIC_LAB_VARIANT 0
#endif

// A deliberately quiet, unmistakable 440 Hz square tone. This bypasses the
// MUSX event/voice code but still exercises Doom's real mixer, DMA queue, PIO
// I2S output, ES8311 codec and speaker. If this is audible but E1M1 is not, the
// failure is above the hardware-output layer.
static uint32_t tone_phase;

static void lab_tone_generator(audio_buffer_t *buffer)
{
    size_t count;
    int16_t *samples = I_PicoSoundBufferSamples(buffer, &count);
    const uint32_t step = (uint32_t)(((uint64_t)440u << 32)
                                   / PICO_SOUND_SAMPLE_FREQ);

    for (size_t i = 0; i < count; ++i)
    {
        samples[i] = (tone_phase & 0x80000000u) ? 2200 : -2200;
        tone_phase += step;
    }
}

static void lab_output_check(void)
{
    tone_phase = 0;
    I_PicoSoundSetMusicGenerator(lab_tone_generator);
    absolute_time_t until = make_timeout_time_ms(400);
    while (!time_reached(until))
    {
        I_UpdateSound();
        sleep_ms(1);
    }
    I_PicoSoundSetMusicGenerator(NULL);
}

static void __attribute__((noreturn)) lab_stop(const char *message)
{
    bootlog_print(message);
    while (true)
    {
        sleep_ms(100);
    }
}

static const char *lab_label(void)
{
#if DOOM_MUSIC_LAB_VARIANT == 0
    return "MUSIC LAB 0 BASELINE";
#elif DOOM_MUSIC_LAB_VARIANT == 1
    return "MUSIC LAB 1 SMOOTH";
#elif DOOM_MUSIC_LAB_VARIANT == 2
    return "MUSIC LAB 2 22KHZ";
#elif DOOM_MUSIC_LAB_VARIANT == 3
    return "MUSIC LAB 3 CODEC";
#else
    return "MUSIC LAB INVALID";
#endif
}

void __attribute__((noreturn)) MusicLab_Run(void)
{
    // D_DoomMain has already initialized the WHX, allocator, codec, output
    // queue, display and render core before entering this bounded lab.
    lumpindex_t lump = W_CheckNumForName("D_E1M1");
    if (lump < 0)
    {
        lab_stop("MUSIC LAB: NO E1M1");
    }

    if (!I_PicoSoundIsInitialized())
    {
        lab_stop("MUSIC LAB: AUDIO FAIL");
    }
    I_SetMusicVolume(64);  // Same accepted 8/15 in-game baseline level.

    void *song = I_RegisterSong(W_CacheLumpNum(lump, PU_STATIC),
                                W_LumpLength(lump));
    if (song == NULL)
    {
        lab_stop("MUSIC LAB: SONG FAIL");
    }

    bootlog_print("TONE THEN E1M1");
    lab_output_check();
    I_PlaySong(song, true);
    if (!I_MusicIsPlaying())
    {
        lab_stop("MUSIC LAB: PLAY FAIL");
    }
    bootlog_print(lab_label());
    pwr_button_enable_edges();

    while (true)
    {
        pwr_button_event_t event = pwr_button_poll();
        if (event & PWR_BUTTON_SHORT_PRESS)
        {
            I_StopSong();
            lab_output_check();
            I_PlaySong(song, true);
            bootlog_print(lab_label());
        }
        I_UpdateSound();
        // The 1 ms yield is far below the 11.6 ms 512-sample buffer period,
        // while also giving the USB/runtime services a predictable window.
        sleep_ms(1);
    }
}

#endif
