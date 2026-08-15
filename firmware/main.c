/*
 * Milestone 0 for the Doom control scheme: verify we can actually tell
 * single/double/long press apart on both buttons before designing a
 * control scheme around it. BOOT is read via the flash-CS-float trick
 * (see lib/button); PWR has no direct GPIO at all, it's read by polling
 * the AXP2101 power chip over I2C for its own short/long-press IRQ
 * flags (see lib/pwr_button) - the chip decides "short" vs "long"
 * itself, we only synthesize "double" on top of that by timing.
 *
 * Result is shown as text on the AMOLED screen, so this is testable
 * standalone on battery, no laptop needed - press a button, read the
 * screen.
 */
#include <stdio.h>
#include "pico/stdlib.h"
#include "DEV_Config.h"
#include "display.h"
#include "bootsel_button.h"
#include "pwr_button.h"

#define DOUBLE_PRESS_WINDOW_US (400 * 1000)
#define BOOT_LONG_PRESS_US     (1200 * 1000)

// Returns 0=none, 1=single, 2=double, 3=long
static int poll_boot_button(void)
{
    static bool was_pressed = false;
    static absolute_time_t press_started;
    static absolute_time_t last_single_release;
    static bool awaiting_double = false;

    bool pressed = bootsel_button_pressed();

    if (pressed && !was_pressed) {
        press_started = get_absolute_time();
    }

    if (!pressed && was_pressed) {
        was_pressed = false;
        int64_t held_us = absolute_time_diff_us(press_started, get_absolute_time());

        if (held_us > BOOT_LONG_PRESS_US) {
            awaiting_double = false;
            return 3;
        }

        if (awaiting_double &&
            absolute_time_diff_us(last_single_release, get_absolute_time()) < DOUBLE_PRESS_WINDOW_US) {
            awaiting_double = false;
            return 2;
        }

        awaiting_double = true;
        last_single_release = get_absolute_time();
        return 0;
    }
    was_pressed = pressed;

    if (awaiting_double && !pressed &&
        absolute_time_diff_us(last_single_release, get_absolute_time()) > DOUBLE_PRESS_WINDOW_US) {
        awaiting_double = false;
        return 1;
    }
    return 0;
}

// Returns 0=none, 1=single, 2=double, 3=long
static int poll_pwr_button(void)
{
    static absolute_time_t last_short_press;
    static bool awaiting_double = false;

    pwr_button_event_t ev = pwr_button_poll();

    if (ev == PWR_BUTTON_LONG_PRESS) {
        awaiting_double = false;
        return 3;
    }

    if (ev == PWR_BUTTON_SHORT_PRESS) {
        if (awaiting_double &&
            absolute_time_diff_us(last_short_press, get_absolute_time()) < DOUBLE_PRESS_WINDOW_US) {
            awaiting_double = false;
            return 2;
        }
        awaiting_double = true;
        last_short_press = get_absolute_time();
        return 0;
    }

    if (awaiting_double &&
        absolute_time_diff_us(last_short_press, get_absolute_time()) > DOUBLE_PRESS_WINDOW_US) {
        awaiting_double = false;
        return 1;
    }
    return 0;
}

static const char *result_label(int result)
{
    switch (result) {
        case 1: return "SINGLE";
        case 2: return "DOUBLE";
        case 3: return "LONG";
        default: return "?";
    }
}

int main()
{
    DEV_Module_Init();
    display_init();

    printf("\r\n--- Button test: press BOOT or PWR ---\r\n");
    display_show_lines("Press BOOT", "or PWR");

    while (true) {
        int boot_result = poll_boot_button();
        int pwr_result = poll_pwr_button();

        if (boot_result) {
            printf("BOOT: %s\r\n", result_label(boot_result));
            display_show_lines("BOOT", result_label(boot_result));
        }
        if (pwr_result) {
            printf("PWR: %s\r\n", result_label(pwr_result));
            display_show_lines("PWR", result_label(pwr_result));
        }

        sleep_ms(20);
    }
}
