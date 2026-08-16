/*
 * PIO-based I2S driver for the ES8311 codec, adapted from Waveshare's
 * RP2350-Touch-AMOLED-1.8 demo (C/02-ES8311).
 */
#include <string.h>
#include "pico/stdlib.h"
#include "pico/critical_section.h"
#include "hardware/pio.h"
#include "hardware/clocks.h"
#include "hardware/dma.h"
#include "hardware/irq.h"
#include "audio_pio.h"
#include "audio_pio.pio.h"

#define AUDIO_DMA_BUFFER_COUNT 2
#define AUDIO_DMA_SAMPLES_PER_BUFFER 512

enum audio_buffer_state {
    AUDIO_BUFFER_FREE,
    AUDIO_BUFFER_FILLING,
    AUDIO_BUFFER_QUEUED,
    AUDIO_BUFFER_PLAYING,
};

static uint32_t audio_buffers[AUDIO_DMA_BUFFER_COUNT][AUDIO_DMA_SAMPLES_PER_BUFFER];
static const uint32_t silence_buffer[AUDIO_DMA_SAMPLES_PER_BUFFER];
static uint8_t buffer_state[AUDIO_DMA_BUFFER_COUNT];
static uint8_t queued_buffers[AUDIO_DMA_BUFFER_COUNT];
static uint8_t queue_head;
static uint8_t queue_count;
static int8_t playing_buffer = -1;
static int audio_dma_channel = -1;
static volatile bool audio_dma_initialized;
static critical_section_t audio_dma_lock;

void set_mclk_frequency(uint32_t mclk_freq)
{
    double system_clock_frequency = clock_get_hz(clk_sys);
    double div = (system_clock_frequency / mclk_freq) / 5;
    pio_sm_set_clkdiv(pico_audio.pio_1, pico_audio.sm_mclk, div);
}

static void start_next_dma_locked(void)
{
    const uint32_t *source = silence_buffer;

    playing_buffer = -1;
    if (queue_count) {
        uint8_t index = queued_buffers[queue_head];
        queue_head = (queue_head + 1) % AUDIO_DMA_BUFFER_COUNT;
        queue_count--;
        buffer_state[index] = AUDIO_BUFFER_PLAYING;
        playing_buffer = (int8_t)index;
        source = audio_buffers[index];
    }

    dma_channel_set_read_addr(audio_dma_channel, source, false);
    dma_channel_set_trans_count(audio_dma_channel,
                                AUDIO_DMA_SAMPLES_PER_BUFFER, true);
}

static void __isr __time_critical_func(audio_dma_irq_handler)(void)
{
    if (audio_dma_channel < 0
        || !dma_irqn_get_channel_status(1, (uint)audio_dma_channel)) {
        return;
    }

    dma_irqn_acknowledge_channel(1, (uint)audio_dma_channel);
    critical_section_enter_blocking(&audio_dma_lock);
    if (playing_buffer >= 0) {
        buffer_state[(uint8_t)playing_buffer] = AUDIO_BUFFER_FREE;
    }
    if (audio_dma_initialized) {
        start_next_dma_locked();
    }
    critical_section_exit(&audio_dma_lock);
}

bool audio_dma_init(void)
{
    if (audio_dma_initialized) return true;

    memset(buffer_state, AUDIO_BUFFER_FREE, sizeof(buffer_state));
    queue_head = 0;
    queue_count = 0;
    playing_buffer = -1;
    critical_section_init(&audio_dma_lock);

    audio_dma_channel = dma_claim_unused_channel(false);
    if (audio_dma_channel < 0) {
        critical_section_deinit(&audio_dma_lock);
        return false;
    }

    dma_channel_config config = dma_channel_get_default_config((uint)audio_dma_channel);
    channel_config_set_transfer_data_size(&config, DMA_SIZE_32);
    channel_config_set_read_increment(&config, true);
    channel_config_set_write_increment(&config, false);
    channel_config_set_dreq(&config,
                            pio_get_dreq(pico_audio.pio_2,
                                         pico_audio.sm_dout, true));
    dma_channel_configure((uint)audio_dma_channel, &config,
                          &pico_audio.pio_2->txf[pico_audio.sm_dout],
                          NULL, 0, false);

    irq_set_exclusive_handler(DMA_IRQ_1, audio_dma_irq_handler);
    dma_irqn_acknowledge_channel(1, (uint)audio_dma_channel);
    dma_irqn_set_channel_enabled(1, (uint)audio_dma_channel, true);
    audio_dma_initialized = true;
    irq_set_enabled(DMA_IRQ_1, true);

    // Keep clocks and the codec fed from the moment output starts. When no
    // mixed buffer is ready, the IRQ simply schedules another silence block.
    critical_section_enter_blocking(&audio_dma_lock);
    start_next_dma_locked();
    critical_section_exit(&audio_dma_lock);
    return true;
}

void audio_dma_shutdown(void)
{
    if (!audio_dma_initialized) return;

    critical_section_enter_blocking(&audio_dma_lock);
    audio_dma_initialized = false;
    critical_section_exit(&audio_dma_lock);

    dma_irqn_set_channel_enabled(1, (uint)audio_dma_channel, false);
    irq_set_enabled(DMA_IRQ_1, false);
    dma_channel_abort((uint)audio_dma_channel);
    dma_irqn_acknowledge_channel(1, (uint)audio_dma_channel);
    irq_remove_handler(DMA_IRQ_1, audio_dma_irq_handler);
    dma_channel_unclaim((uint)audio_dma_channel);
    audio_dma_channel = -1;
    pio_sm_set_enabled(pico_audio.pio_2, pico_audio.sm_dout, false);
    critical_section_deinit(&audio_dma_lock);
}

bool audio_queue_available(void)
{
    if (!audio_dma_initialized) return false;

    bool available = false;
    critical_section_enter_blocking(&audio_dma_lock);
    for (uint i = 0; i < AUDIO_DMA_BUFFER_COUNT; i++) {
        if (buffer_state[i] == AUDIO_BUFFER_FREE) {
            available = true;
            break;
        }
    }
    critical_section_exit(&audio_dma_lock);
    return available;
}

bool audio_try_queue(const int16_t *samples, uint32_t sample_count)
{
    if (!audio_dma_initialized || !samples) return false;
    if (sample_count > AUDIO_DMA_SAMPLES_PER_BUFFER) {
        sample_count = AUDIO_DMA_SAMPLES_PER_BUFFER;
    }

    int buffer_index = -1;
    critical_section_enter_blocking(&audio_dma_lock);
    for (uint i = 0; i < AUDIO_DMA_BUFFER_COUNT; i++) {
        if (buffer_state[i] == AUDIO_BUFFER_FREE) {
            buffer_state[i] = AUDIO_BUFFER_FILLING;
            buffer_index = (int)i;
            break;
        }
    }
    critical_section_exit(&audio_dma_lock);
    if (buffer_index < 0) return false;

    uint32_t *destination = audio_buffers[buffer_index];
    for (uint32_t i = 0; i < sample_count; i++) {
        uint32_t mono = (uint32_t)(uint16_t)samples[i];
        destination[i] = pico_audio.channel_count == 1
                       ? mono << 16
                       : (mono << 16) | mono;
    }
    for (uint32_t i = sample_count; i < AUDIO_DMA_SAMPLES_PER_BUFFER; i++) {
        destination[i] = 0;
    }

    critical_section_enter_blocking(&audio_dma_lock);
    if (!audio_dma_initialized) {
        buffer_state[buffer_index] = AUDIO_BUFFER_FREE;
        critical_section_exit(&audio_dma_lock);
        return false;
    }
    uint8_t queue_tail = (queue_head + queue_count) % AUDIO_DMA_BUFFER_COUNT;
    queued_buffers[queue_tail] = (uint8_t)buffer_index;
    queue_count++;
    buffer_state[buffer_index] = AUDIO_BUFFER_QUEUED;
    critical_section_exit(&audio_dma_lock);
    return true;
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
