/*
 * PIO-based I2S driver for the ES8311 codec, adapted from Waveshare's
 * RP2350-Touch-AMOLED-1.8 demo (C/02-ES8311).
 */
#include <stdlib.h>
#include <string.h>
#include "pico/stdlib.h"
#include "hardware/pio.h"
#include "hardware/clocks.h"
#include "audio_pio.h"
#include "audio_pio.pio.h"

void set_mclk_frequency(uint32_t mclk_freq)
{
    double system_clock_frequency = clock_get_hz(clk_sys);
    double div = (system_clock_frequency / mclk_freq) / 5;
    pio_sm_set_clkdiv(pico_audio.pio_1, pico_audio.sm_mclk, div);
}

int32_t *data_treating(const int16_t *audio, uint32_t len)
{
    int32_t *samples = (int32_t *)calloc(len, sizeof(int32_t));
    for (uint32_t i = 0; i < len; i++)
    {
        if (pico_audio.channel_count == 1)
        {
            samples[i] = audio[i] * 65536;
        }
        else
        {
            samples[i] = audio[i] * 65536 + audio[i];
        }
    }
    return samples;
}

void audio_out(int32_t *samples, int32_t len)
{
    for (uint16_t i = 0; i < len; i++)
        pio_sm_put_blocking(pico_audio.pio_2, pico_audio.sm_dout, samples[i]);
}

void dout_pio_init(void)
{
    pio_sm_claim(pico_audio.pio_2, pico_audio.sm_dout);
    uint offset = pio_add_program(pico_audio.pio_2, &audio_pio_program);
    audio_pio_program_init(pico_audio.pio_2, pico_audio.sm_dout, offset, pico_audio.audio_dout, pico_audio.audio_lrclk);
    pio_sm_set_clkdiv(pico_audio.pio_2, pico_audio.sm_dout, 1.0f);
    pio_sm_set_enabled(pico_audio.pio_2, pico_audio.sm_dout, true);
}

void din_pio_init(void)
{
    pio_sm_claim(pico_audio.pio_1, pico_audio.sm_din);
    uint offset = pio_add_program(pico_audio.pio_1, &read_pio_program);
    read_pio_program_init(pico_audio.pio_1, pico_audio.sm_din, offset, pico_audio.audio_din, pico_audio.audio_lrclk);
    pio_sm_set_clkdiv(pico_audio.pio_1, pico_audio.sm_din, 1.0f);
    pio_sm_set_enabled(pico_audio.pio_1, pico_audio.sm_din, true);
}

void mclk_pio_init(void)
{
    pio_sm_claim(pico_audio.pio_1, pico_audio.sm_mclk);
    uint offset = pio_add_program(pico_audio.pio_1, &mclk_pio_program);
    mclk_pio_program_init(pico_audio.pio_1, pico_audio.sm_mclk, offset, pico_audio.audio_mclk);
    set_mclk_frequency(pico_audio.mclk_freq);
    pio_sm_set_enabled(pico_audio.pio_1, pico_audio.sm_mclk, true);
}
