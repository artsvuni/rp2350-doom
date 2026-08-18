/*
 * Isolated runtime-BOOT safety probe for RP2350-Touch-AMOLED-1.8.
 *
 * This is intentionally not Doom. It runs on core 0 only, never initializes
 * the screen, codec, I2S PIO, DMA, I2C, touch, IMU, or core 1, and keeps both
 * the speaker amplifier and AMOLED power disabled. Its only purpose is to
 * prove one bounded BOOT press/release can be sampled through the Pico SDK's
 * flash-safe execution API without recreating the earlier abnormal hardware
 * behaviour.
 *
 * Success is deliberately observable without USB/UART stdio: after two pressed and
 * two released 50 ms samples, the probe enters ROM BOOTSEL. The host should
 * then see the RP2350 bootrom with picotool. A held button never triggers the
 * reboot; release is mandatory.
 */
#include "hardware/gpio.h"
#include "pico/bootrom.h"
#include "pico/error.h"
#include "pico/stdlib.h"

#include "bootsel_button.h"

#define AMOLED_POWER_PIN 17
#define SPEAKER_AMP_PIN 19
#define AUDIO_PIN_FIRST 20
#define AUDIO_PIN_LAST 24
#define SAMPLE_PERIOD_MS 50
#define FLASH_SAFE_TIMEOUT_MS 2
#define STABLE_SAMPLES 2

static void disable_output_peripherals(void)
{
    // Disable these outputs before waiting for user input.
    gpio_init(SPEAKER_AMP_PIN);
    gpio_set_dir(SPEAKER_AMP_PIN, GPIO_OUT);
    gpio_put(SPEAKER_AMP_PIN, 0);

    gpio_init(AMOLED_POWER_PIN);
    gpio_set_dir(AMOLED_POWER_PIN, GPIO_OUT);
    gpio_put(AMOLED_POWER_PIN, 0);

    // Leave every I2S signal as a high-impedance input. The codec and its
    // clocks are never initialized, and no PIO state machine is claimed.
    for (uint pin = AUDIO_PIN_FIRST; pin <= AUDIO_PIN_LAST; ++pin) {
        gpio_init(pin);
        gpio_set_dir(pin, GPIO_IN);
        gpio_disable_pulls(pin);
    }
}

int main(void)
{
    disable_output_peripherals();

    unsigned pressed_samples = 0;
    unsigned released_samples = 0;
    bool stable_press_seen = false;

    while (true) {
        bool pressed = false;
        int result = bootsel_button_pressed_flash_safe(
            &pressed, FLASH_SAFE_TIMEOUT_MS);

        if (result != PICO_OK) {
            // A failed safety handshake is never interpreted as input.
            pressed_samples = 0;
            released_samples = 0;
            stable_press_seen = false;
            sleep_ms(100);
            continue;
        }

        if (!stable_press_seen) {
            released_samples = 0;
            if (pressed) {
                if (++pressed_samples >= STABLE_SAMPLES) {
                    stable_press_seen = true;
                }
            } else {
                pressed_samples = 0;
            }
        } else if (!pressed) {
            if (++released_samples >= STABLE_SAMPLES) {
                sleep_ms(100);
                reset_usb_boot(0, 0);
            }
        } else {
            released_samples = 0;
        }

        sleep_ms(SAMPLE_PERIOD_MS);
    }
}
