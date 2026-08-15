/*
 * Milestone 0 for the Doom control scheme: verify we can actually tell
 * single/double/long press apart on both buttons before designing a
 * control scheme around it. BOOT is read via the flash-CS-float trick
 * (see lib/button); PWR has no direct GPIO at all, it's read by polling
 * the AXP2101 power chip over I2C for its own short/long-press IRQ
 * flags (see lib/pwr_button) - the chip decides "short" vs "long"
 * itself, we only synthesize "double" on top of that by timing.
 *
 * Touch d-pad zone layout, in landscape logical space (448x368, screen
 * rendered rotated 90 - see display_init()). Asymmetric, hugging the
 * bottom-left corner per Alexander's sketch: LEFT and DOWN sit right at
 * the physical screen edges so a resting hand barely has to move to find
 * them by feel; UP and RIGHT get deliberately bigger targets since
 * they're used more and a mistap there costs more. Not a uniform grid -
 * four independently-sized rectangles, each tuned by feel over several
 * rounds of on-hardware testing (see docs/DECISIONS.md).
 *
 * The touch controller only ever reports raw NATIVE (portrait) panel
 * coordinates - it has no idea we're rendering rotated. touch_to_logical()
 * applies the same rotation GUI_Paint uses for drawing (see
 * Paint_SetPixel's ROTATE_90 case) so zone math can work in the same
 * logical landscape space as the visual layout. Confirmed correct on
 * hardware (button-edge orientation matched expectation).
 *
 * Result is shown as text on the AMOLED screen, so this is testable
 * standalone on battery, no laptop needed - press a button or touch a
 * zone, read the screen.
 */
#include <stdio.h>
#include "pico/stdlib.h"
#include "DEV_Config.h"
#include "display.h"
#include "bootsel_button.h"
#include "pwr_button.h"
#include "FT3168.h"
#include "AMOLED_1in8.h"

typedef struct {
    const char *name;
    int x0, y0, x1, y1; // half-open [x0,x1) x [y0,y1) in logical landscape space
} zone_t;

// Tuned from Alexander's sketch (screenshot with LFT/UP/DWN/RGHT overlaid).
// LEFT and DOWN touch the screen edges; UP is tall, RIGHT is wide.
static const zone_t zones[] = {
    { "LEFT",  0,   270, 65,  345 },
    { "UP",    70,  90,  100, 300 },
    { "DOWN",  70,  300, 100, 368 },
    { "RIGHT", 105, 260, 290, 340 },
};
#define NUM_ZONES (sizeof(zones) / sizeof(zones[0]))

static void touch_to_logical(uint16_t raw_x, uint16_t raw_y, int *logical_x, int *logical_y)
{
    // Inverse of Paint_SetPixel's ROTATE_90 transform (X=W-y-1, Y=x).
    *logical_x = raw_y;
    *logical_y = AMOLED_1IN8_WIDTH - raw_x - 1;
}

// Returns a zone name if touched, "(outside zones)" if touching but not in
// any zone, or NULL if not touching at all. *lx/*ly (when touched) give the
// logical landscape coordinates, for print-debugging.
static const char *poll_touch_zone(int *lx_out, int *ly_out)
{
    uint8_t fingers = (uint8_t)FT3168_ReadState(FT3168_FINGER_NUMBER);
    if (fingers == 0) return NULL;

    FT3168_Get_Point();
    int lx, ly;
    touch_to_logical(FT3168.x_point, FT3168.y_point, &lx, &ly);
    *lx_out = lx;
    *ly_out = ly;

    for (size_t i = 0; i < NUM_ZONES; i++) {
        const zone_t *z = &zones[i];
        if (lx >= z->x0 && lx < z->x1 && ly >= z->y0 && ly < z->y1) return z->name;
    }
    return "(outside zones)";
}

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
    FT3168_Init(FT3168_Point_Mode);

    printf("\r\n--- Button/touch test: press BOOT, PWR, or touch a d-pad zone ---\r\n");
    display_show_lines("Press a button", "or touch a zone");

    const char *last_zone = NULL;

    while (true) {
        int boot_result = poll_boot_button();
        int pwr_result = poll_pwr_button();
        int lx = 0, ly = 0;
        const char *zone = poll_touch_zone(&lx, &ly);

        if (boot_result) {
            printf("BOOT: %s\r\n", result_label(boot_result));
            display_show_lines("BOOT", result_label(boot_result));
        }
        if (pwr_result) {
            printf("PWR: %s\r\n", result_label(pwr_result));
            display_show_lines("PWR", result_label(pwr_result));
        }
        if (zone != last_zone) {
            if (zone) {
                printf("TOUCH: %s (raw x=%d y=%d, logical x=%d y=%d)\r\n",
                       zone, FT3168.x_point, FT3168.y_point, lx, ly);
                display_show_lines("TOUCH", zone);
            } else {
                printf("TOUCH: released\r\n");
            }
            last_zone = zone;
        }

        sleep_ms(20);
    }
}
