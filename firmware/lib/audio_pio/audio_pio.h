/*
 * PIO-based I2S driver for the ES8311 codec, adapted from Waveshare's
 * RP2350-Touch-AMOLED-1.8 demo (C/02-ES8311). Trimmed to the reusable
 * driver surface — the demo playback routines (Happy_birthday_out,
 * Sine_440hz_out, etc., tied to Waveshare's bundled sample data) are
 * not carried over.
 */
#ifndef _PICO_AUDIO_PIO_H
#define _PICO_AUDIO_PIO_H

#include "pico/stdlib.h"
#include "hardware/pio.h"
#include "hardware/clocks.h"

#define AUDIO_PIO __CONCAT(pio, PICO_AUDIO_PIO)
#define GPIO_FUNC_PIOx __CONCAT(GPIO_FUNC_PIO, PICO_AUDIO_PIO)

#define PICO_MCLK_FREQ      44100 * 256
#define PICO_SAMPLE_FREQ    44100
#define PICO_AUDIO_VOLUME   73
#define PICO_AUDIO_COUNT    1
#define PICO_AUDIO_RES_IN   16
#define PICO_AUDIO_RES_OUT  16
#define PICO_AUDIO_MIC_GAIN 3
#define PICO_AUDIO_DOUT     20
#define PICO_AUDIO_DIN      21
#define PICO_AUDIO_MCLK     22
#define PICO_AUDIO_LRCLK    23
#define PICO_AUDIO_BCLK     24
#define PICO_AUDIO_SM_DOUT  0
#define PICO_AUDIO_SM_DIN   1
#define PICO_AUDIO_SM_MCLK  2

typedef struct pico_audio_struct
{
    uint32_t mclk_freq;
    uint32_t sample_freq;
    uint8_t  res_in;
    uint8_t  res_out;
    uint8_t  mic_gain;
    uint8_t  volume;
    uint8_t  channel_count;
    uint8_t  audio_dout;
    uint8_t  audio_din;
    uint8_t  audio_mclk;
    uint8_t  audio_lrclk;
    uint8_t  audio_bclk;
    PIO      pio_1;
    PIO      pio_2;
    uint8_t  sm_dout;
    uint8_t  sm_din;
    uint8_t  sm_mclk;
} pico_audio_t;

static pico_audio_t pico_audio = {
    .mclk_freq = PICO_MCLK_FREQ,
    .sample_freq = PICO_SAMPLE_FREQ,
    .channel_count = PICO_AUDIO_COUNT,
    .res_in = PICO_AUDIO_RES_IN,
    .res_out = PICO_AUDIO_RES_OUT,
    .mic_gain = PICO_AUDIO_MIC_GAIN,
    .volume = PICO_AUDIO_VOLUME,
    .audio_dout = PICO_AUDIO_DOUT,
    .audio_din = PICO_AUDIO_DIN,
    .audio_mclk = PICO_AUDIO_MCLK,
    .audio_lrclk = PICO_AUDIO_LRCLK,
    .audio_bclk = PICO_AUDIO_BCLK,
    .pio_1 = pio1,
    .pio_2 = pio2,
    .sm_dout = PICO_AUDIO_SM_DOUT,
    .sm_din = PICO_AUDIO_SM_DIN,
    .sm_mclk = PICO_AUDIO_SM_MCLK
};

void dout_pio_init(void);
void din_pio_init(void);
void mclk_pio_init(void);
void set_mclk_frequency(uint32_t frequency);

// Non-blocking I2S output. The driver owns two 512-frame DMA buffers and
// continuously feeds either a queued buffer or silence to the PIO state
// machine. Callers should only mix when audio_queue_available() is true.
bool audio_dma_init(void);
void audio_dma_shutdown(void);
bool audio_queue_available(void);
bool audio_try_queue(const int16_t *samples, uint32_t sample_count);

#endif //_PICO_AUDIO_PIO_H
